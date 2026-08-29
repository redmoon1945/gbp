/*
 *  Copyright (C) 2024-2026 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
 *  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/#AGPL/>.
 */

#include "analysisdialog.h"
#include "ui_analysisdialog.h"
#include "currencyhelper.h"
#include "gbpcontroller.h"
#include "util.h"
#include <QApplication>
#include <QColorDialog>
#include <QFrame>
#include <QHeaderView>
#include <algorithm>
#include <limits>


void AnalysisDialog::periodHeatmapRedisplayTable()
{
    // --- Reset table ---
    QTableWidget* tw = ui->periodHeatmapTableWidget;
    tw->clearContents();
    tw->setRowCount(0);
    tw->horizontalHeader()->setVisible(true);
    tw->verticalHeader()->setVisible(true);

    // A year is an identifier, not a quantity - it must never get a thousands separator
    // (locale.toString(2026) would otherwise render e.g. "2,026" or, in Bengali, "২,০২৬").
    // Used for every year displayed in this view, and to measure how wide a year can get.
    QLocale yearLocale = locale;
    yearLocale.setNumberOptions(QLocale::OmitGroupSeparator);

    bool yearlyMode = ui->periodHeatmapYearlyRadioButton->isChecked();
    const QMap<QDate,Bin>* const binsPtr = yearlyMode ? &binsYearly : &binsMonthly;
    const QMap<QDate,Bin>& bins = *binsPtr;

    bool excludeCurrent = ui->periodHeatmapExcludeCurrentCheckBox->isChecked();
    QDate today = GbpController::getInstance().getTomorrow().addDays(-1);
    QDate excludeKey = yearlyMode ? QDate(today.year(), 1, 1)
        : QDate(today.year(), today.month(), 1);
    bool excludeLast = ui->periodHeatmapExcludeLastCheckBox->isChecked();

    // Hide all legend widgets when there is no data to display.
    if (bins.isEmpty()) {
        ui->periodHeatmapLegendMinFrame->setVisible(false);
        ui->periodHeatmapLegendMinLabel->setVisible(false);
        ui->periodHeatmapLegendMaxFrame->setVisible(false);
        ui->periodHeatmapLegendMaxLabel->setVisible(false);
        ui->periodHeatmapLegendZeroFrame->setVisible(false);
        ui->periodHeatmapLegendZeroLabel->setVisible(false);
        ui->periodHeatmapLegendNoEventsSwatch->setVisible(false);
        ui->periodHeatmapLegendNoEventsLabel->setVisible(false);
        ui->periodHeatmapLegendOutsideSwatch->setVisible(false);
        ui->periodHeatmapLegendOutsideLabel->setVisible(false);
        return;
    }

    // Last period in the projection (the final key in the map). Excluding it removes it from
    // the colour scale so an extreme outlier at the far end of the projection doesn't skew all
    // other cells, and marks it as out-of-period in the table.
    QDate excludeLastKey = bins.lastKey();

    // --- Determine active display mode ---
    bool byIncome  = ui->periodHeatmapIncomesRadioButton->isChecked();
    bool byExpense = ui->periodHeatmapExpensesRadioButton->isChecked();
    bool byDelta   = !byIncome && !byExpense;

    // --- Build table structure ---
    int firstYear = bins.firstKey().year();
    int lastYear  = bins.lastKey().year();

    if (yearlyMode) {
        // Fixed 10-column grid; headers hidden (year is written inside each cell).
        // When the current year is excluded it is removed from the grid entirely.
        int nYears = lastYear - firstYear + 1;
        int excludedCount = 0;
        if (excludeCurrent && excludeKey.year() >= firstYear && excludeKey.year() <= lastYear)
            ++excludedCount;
        if (excludeLast && excludeLastKey.year() >= firstYear && excludeLastKey.year() <= lastYear)
            ++excludedCount;
        int nDisplay = nYears - excludedCount;
        int nRows  = std::max(1, (nDisplay + 9) / 10);
        tw->setRowCount(nRows);
        tw->setColumnCount(10);
        tw->horizontalHeader()->setVisible(false);
        tw->verticalHeader()->setVisible(false);
    } else {
        // One row per year, 12 columns (Jan-Dec).
        int noRows = lastYear - firstYear + 1;
        tw->setRowCount(noRows);
        tw->setColumnCount(12);
        for (int m = 1; m <= 12; ++m) {
            auto* h = new QTableWidgetItem(locale.monthName(m, QLocale::ShortFormat));
            h->setTextAlignment(Qt::AlignCenter);
            tw->setHorizontalHeaderItem(m - 1, h);
        }
        for (int r = 0; r < noRows; ++r) {
            auto* h = new QTableWidgetItem(yearLocale.toString(firstYear + r));
            h->setTextAlignment(Qt::AlignCenter);
            tw->setVerticalHeaderItem(r, h);
        }
    }

    // --- Normalization pass ---
    // Computes the maximum value(s) used to map each bin to a [0,1] blend factor,
    // and tracks the actual min/max displayed values for the legend labels.
    // Income/expense: single maxVal (all values positive, one-sided scale).
    // Delta: separate maxPosDelta and maxNegDelta so each side independently reaches
    // full color saturation at its own extreme, regardless of their relative magnitudes.
    // Bins with no income AND no expense are skipped (shown as "no events", not colored).
    double maxVal = 0.0;
    double maxPosDelta = 0.0;
    double maxNegDelta = 0.0;
    double legendMin = std::numeric_limits<double>::max();
    double legendMax = std::numeric_limits<double>::lowest();
    bool legendReady = false;

    for (auto it = bins.constBegin(); it != bins.constEnd(); ++it) {
        if ((excludeCurrent && it.key() == excludeKey) ||
            (excludeLast && it.key() == excludeLastKey)) {
            continue;
        }
        const Bin& b = it.value();
        if (b.income == 0.0 && b.expense == 0.0){
            continue;
        }

        if (byIncome){
            maxVal = std::max(maxVal, b.income);
        } else if (byExpense) {
            maxVal = std::max(maxVal, b.expense);
        } else {
            if (b.delta > 0.0) {
                maxPosDelta = std::max(maxPosDelta, b.delta);
            } else if (b.delta < 0.0) {
                maxNegDelta = std::max(maxNegDelta, -b.delta);
            }
        }

        double displayVal;
        bool valid;
        if (byIncome) {
            displayVal = b.income;
            valid = (b.income  > 0.0);
        } else if (byExpense) {
            displayVal = b.expense;
            valid = (b.expense > 0.0);
        } else {
            displayVal = b.delta;
            valid = true;
        }

        if (valid) {
            legendMin = std::min(legendMin, displayVal);
            legendMax = std::max(legendMax, displayVal);
            legendReady = true;
        }
    }

    // --- Color setup ---
    // Max (saturated) color for the active mode.
    // modeFromColor is the negative-side extreme, only meaningful in delta mode.
    QColor modeToColor;
    if (byIncome == true) {
        modeToColor = periodHeatmapIncomeToColor;
    } else if (byExpense == true) {
        modeToColor = periodHeatmapExpenseToColor;
    } else {
        modeToColor = periodHeatmapDeltaToColor;
    }
    QColor modeFromColor = periodHeatmapDeltaFromColor;

    bool dark = GbpController::getInstance().useDarkModeForChart();

    // --- Special cell colours ---
    // Out-of-period: light neutral gray, no text.
    QColor oorBg = dark ? QColor(52, 52, 56)   : QColor(210, 210, 215);

    // No events: medium gray + centred bullet (•). Year is omitted in yearly mode.
    QColor noeBg = dark ? QColor(90, 90, 95)   : QColor(155, 155, 160);
    QColor noeFg = dark ? QColor(195, 195, 200) : QColor(60, 60, 65);

    // --- Color helpers ---

    // Linear RGB blend between two colors at position t in [0, 1].
    auto blend = [](QColor from, QColor to, double t) -> QColor {
        return QColor(
            qRound(from.red() + t * (to.red() - from.red())),
            qRound(from.green() + t * (to.green() - from.green())),
            qRound(from.blue() + t * (to.blue() - from.blue()))
        );
    };

    // Income/expense minimum color: same hue as the max color but near-background brightness,
    // so the smallest non-zero cell looks like a barely visible tint rather than a strong color.
    // Saturation kept high (230) to preserve hue identity across the full range.
    // Dark theme: low brightness (45) blends into the dark background.
    // Light theme: high brightness (220) blends into the light background.
    int hue = modeToColor.hsvHue();
    if (hue == -1) {
        hue = 0; // achromatic max color fallback
    }
    QColor modeMinColor = QColor::fromHsv(hue, 230, dark ? 45 : 220);

    // Delta neutral color at delta == 0 (breakeven, income == expense).
    // Also used as the blend start for both positive and negative sides, keeping the
    // diverging scale symmetric and theme-consistent.
    // Pure black: blends cleanly into dark backgrounds and anchors zero unambiguously on light
    // ones.
    QColor deltaNeutral = QColor(0, 0, 0);

    // Maps a bin to its cell background color based on the active mode.
    auto cellColor = [&](const Bin& bin) -> QColor {
        if (byIncome || byExpense) {
            // Sequential scale: faint tint (t=0) → full saturation (t=1).
            double v = byIncome ? bin.income : bin.expense;
            double t = (maxVal > 0.0) ? std::min(1.0, v / maxVal) : 1.0;
            return blend(modeMinColor, modeToColor, t);
        }
        if (bin.delta > 0.0) {
            // Positive side: neutral → positive-extreme color.
            double t = (maxPosDelta > 0.0) ? std::min(1.0, bin.delta / maxPosDelta) : 1.0;
            return blend(deltaNeutral, modeToColor, t);
        }
        if (bin.delta < 0.0) {
            // Negative side: neutral → negative-extreme color.
            double t = (maxNegDelta > 0.0) ? std::min(1.0, -bin.delta / maxNegDelta) : 1.0;
            return blend(deltaNeutral, modeFromColor, t);
        }
        return deltaNeutral; // exact breakeven: income == expense
    };

    // --- Populate cells ---
    if (yearlyMode) {
        // Each cell = one year. Year label written in white; no headers.
        // Years fill left-to-right, top-to-bottom in a fixed 10-column grid.
        // The excluded year (if any) is omitted entirely — it takes no cell.
        QVector<int> displayYears;
        for (int y = firstYear; y <= lastYear; ++y) {
            if ((excludeCurrent && QDate(y, 1, 1) == excludeKey) ||
                (excludeLast && QDate(y, 1, 1) == excludeLastKey)) {
                continue;
            }
            displayYears.append(y);
        }
        int nDisplay = displayYears.size();
        int nRows    = std::max(1, (nDisplay + 9) / 10);
        int nCells   = nRows * 10;
        for (int i = 0; i < nCells; ++i) {
            int row = i / 10;
            int col = i % 10;
            auto* item = new QTableWidgetItem();
            item->setFlags(Qt::ItemIsEnabled);
            item->setTextAlignment(Qt::AlignCenter);

            if (i >= nDisplay) {
                // Trailing cells beyond the last year: out-of-period, no text.
                item->setBackground(oorBg);
            } else {
                int year = displayYears[i];
                item->setText(yearLocale.toString(year));
                item->setForeground(QColor(Qt::white));
                QDate key(year, 1, 1);
                if (!bins.contains(key)) {
                    item->setBackground(oorBg);
                    item->setText(QString());
                    item->setForeground(QColor());
                } else {
                    const Bin& bin = bins.value(key);
                    if (bin.income == 0.0 && bin.expense == 0.0) {
                        item->setBackground(noeBg);
                        item->setText(QString(QChar(0x2022))); // • — overrides year
                        item->setForeground(noeFg);
                        item->setToolTip(QString("%1\n%2").arg(yearLocale.toString(year))
                            .arg(tr("No events")));
                    } else {
                        double rawVal = byIncome  ? bin.income
                            : byExpense ? bin.expense : std::abs(bin.delta);
                        if (rawVal == 0.0 && !byDelta) {
                            item->setBackground(noeBg);
                            item->setText(QString(QChar(0x2022))); // • — overrides year
                            item->setForeground(noeFg);
                        } else {
                            item->setBackground(cellColor(bin));
                        }
                        QString tip = yearLocale.toString(year) +
                            "\n" + tr("Income:   ") +
                            CurrencyHelper::formatAmount(bin.income, currInfo, locale, false) +
                            "\n" + tr("Expenses: ") +
                            CurrencyHelper::formatAmount(bin.expense, currInfo, locale, false) +
                            "\n" + tr("Delta:    ") +
                            CurrencyHelper::formatAmount(bin.delta, currInfo, locale, false) +
                            "\n" + tr("Balance:  ") +
                            CurrencyHelper::formatAmount(bin.cashBalance, currInfo, locale, false);
                        item->setToolTip(tip);
                    }
                }
            }
            tw->setItem(row, col, item);
        }
    } else {
        // Each cell falls into exactly one of three states:
        //   1. Outside period : the month predates or follows the scenario range entirely.
        //                       Shown with a neutral gray background, no text.
        //   2. No events      : the month is within the scenario but has no income and no expense
        //                       (e.g. the scenario has not started recording yet, or a gap).
        //                       Shown with a black background and a centred bullet (•).
        //   3. Data           : the month has at least one financial event. Background color is
        //                       computed by cellColor() from the active mode and normalization.
        //                       Exception: in income or expense mode, a month that has events but
        //                       zero activity for the active type (e.g. a pure-expense month while
        //                       in income mode) is treated as no-events rather than colored black,
        //                       because its zero is uninformative rather than a meaningful
        //                       breakeven.
        //                       In delta mode, zero IS meaningful (income == expense), so it is
        //                       colored with deltaNeutral (pure black) instead.
        // All cells carry a tooltip showing the full income/expense/delta/balance breakdown.
        int noRows = lastYear - firstYear + 1;
        for (int r = 0; r < noRows; ++r) {
            int year = firstYear + r;
            for (int col = 0; col < 12; ++col) {
                int month = col + 1;
                QDate key(year, month, 1);
                auto* item = new QTableWidgetItem();
                item->setFlags(Qt::ItemIsEnabled);
                item->setTextAlignment(Qt::AlignCenter);

                if ((excludeCurrent && key == excludeKey) ||
                    (excludeLast && key == excludeLastKey) || !bins.contains(key)) {
                    // Outside scenario period, or excluded period.
                    item->setBackground(oorBg);
                } else {
                    const Bin& bin = bins.value(key);

                    if (bin.income == 0.0 && bin.expense == 0.0) {
                        // State 2 : within period but no financial events recorded.
                        item->setBackground(noeBg);
                        item->setText(QString(QChar(0x2022))); // •
                        item->setForeground(noeFg);
                        item->setToolTip(
                            QString("%1 %2\n%3")
                                .arg(locale.monthName(month, QLocale::LongFormat))
                                .arg(yearLocale.toString(year))
                                .arg(tr("No events")));
                    } else {
                        // State 3 : month has data — pick color based on active mode.
                        double rawVal = byIncome  ? bin.income
                            : byExpense ? bin.expense : std::abs(bin.delta);
                        if (rawVal == 0.0 && !byDelta) {
                            // Zero in income/expense mode means no activity for that type this
                            // month. Treat as no-events (not a meaningful zero).
                            item->setBackground(noeBg);
                            item->setText(QString(QChar(0x2022))); // •
                            item->setForeground(noeFg);
                        } else {
                            // Normal colored cell; delta==0 lands here too and returns
                            // deltaNeutral.
                            item->setBackground(cellColor(bin));
                        }

                        // Tooltip always shows the full breakdown regardless of the active mode.
                        QString tip = locale.monthName(month, QLocale::LongFormat) +
                            QString(" %1\n").arg(yearLocale.toString(year)) + tr("Income:   ") +
                            CurrencyHelper::formatAmount(bin.income, currInfo, locale, false) +
                            "\n" + tr("Expenses: ") +
                            CurrencyHelper::formatAmount(bin.expense, currInfo, locale, false) +
                            "\n" + tr("Delta:    ") +
                            CurrencyHelper::formatAmount(bin.delta, currInfo, locale, false) +
                            "\n" + tr("Balance:  ") +
                            CurrencyHelper::formatAmount(bin.cashBalance, currInfo, locale, false);
                        item->setToolTip(tip);
                    }
                }
                tw->setItem(r, col, item);
            }
        }
    }

    // --- Column / row sizing ---
    // Scale the table font by the user-selected cell size factor before measuring anything,
    // so all font-metrics-derived sizes (column width, row height, legend swatches) scale together.
    static const double cellSizeFactors[] = {1.0, 1.5, 2.0};
    QFont scaledFont = QApplication::font();
    scaledFont.setPointSizeF(scaledFont.pointSizeF() * cellSizeFactors[periodHeatmapCellSizeIndex]);
    tw->setFont(scaledFont);

    if (yearlyMode) {
        // Square cells sized to the year text width; headers are hidden.
        QFontMetrics fm(scaledFont);
        int cellSide = fm.horizontalAdvance(yearLocale.toString(9999)) + 20;
        tw->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        tw->horizontalHeader()->setDefaultSectionSize(cellSide);
        tw->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        tw->verticalHeader()->setDefaultSectionSize(cellSide);
    } else {
        tw->horizontalHeader()->setFont(scaledFont);
        tw->horizontalHeader()->setMinimumHeight(QFontMetrics(scaledFont).height() + 10);
        // Columns are sized to the widest abbreviated month name plus a small margin.
        // Rows are sized to the font line height plus a small margin.
        QFontMetrics fm(tw->horizontalHeader()->font());
        int maxW = 0;
        for (int m = 1; m <= 12; ++m) {
            maxW = std::max(maxW, fm.horizontalAdvance(locale.monthName(m, QLocale::ShortFormat)));
        }
        tw->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        tw->horizontalHeader()->setDefaultSectionSize(maxW + 12);
        tw->verticalHeader()->setFont(scaledFont);
        tw->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        tw->verticalHeader()->setDefaultSectionSize(fm.height() + 6);
        tw->verticalHeader()->setFixedWidth(fm.horizontalAdvance(yearLocale.toString(9999)) + 12);
    }

    // --- Legend ---
    // Swatch size is half the font line height, kept proportional to the current font.
    int swatchSize = QFontMetrics(scaledFont).height() / 2;
    ui->periodHeatmapLegendMinFrame->setFixedSize(swatchSize, swatchSize);
    ui->periodHeatmapLegendMaxFrame->setFixedSize(swatchSize, swatchSize);
    ui->periodHeatmapLegendZeroFrame->setFixedSize(swatchSize, swatchSize);
    ui->periodHeatmapLegendNoEventsSwatch->setFixedSize(swatchSize, swatchSize);
    ui->periodHeatmapLegendOutsideSwatch->setFixedSize(swatchSize, swatchSize);

    // QFrame ignores palette-based background changes (setAutoFillBackground + QPalette has no
    // visible effect on QFrame). The only reliable way to set a solid fill is via a stylesheet.
    // This lambda encapsulates that workaround so call sites remain readable.
    auto setFrameColor = [](QFrame* f, QColor c) {
        f->setStyleSheet(QString("QFrame { background-color: rgb(%1,%2,%3); }")
            .arg(c.red()).arg(c.green()).arg(c.blue()));
    };

    auto renderSwatch = [](QLabel* lbl, const QColor& bgColor,
                           const QString& symbol = QString(),
                           const QColor& fgColor = QColor()) {
        QString style = QString("background-color: %1; border: 1px solid white;").arg(bgColor.name());
        if (fgColor.isValid())
            style += QString(" color: %1;").arg(fgColor.name());
        lbl->setStyleSheet(style);
        lbl->setText(symbol);
        lbl->setAlignment(Qt::AlignCenter);
    };

    setFrameColor(ui->periodHeatmapLegendZeroFrame, deltaNeutral);
    ui->periodHeatmapLegendZeroFrame->setVisible(byDelta);
    ui->periodHeatmapLegendZeroLabel->setVisible(byDelta);

    renderSwatch(ui->periodHeatmapLegendNoEventsSwatch, noeBg, QString(QChar(0x2022)), noeFg);
    renderSwatch(ui->periodHeatmapLegendOutsideSwatch,  oorBg);
    ui->periodHeatmapLegendNoEventsSwatch->setVisible(true);
    ui->periodHeatmapLegendNoEventsLabel->setVisible(true);
    ui->periodHeatmapLegendOutsideSwatch->setVisible(true);
    ui->periodHeatmapLegendOutsideLabel->setVisible(true);

    // Min/max color swatches and labels are only shown when there is at least one colored cell.
    if (legendReady) {
        // Construct synthetic bins carrying just the legend boundary values so that cellColor()
        // can compute the correct color for those boundary values without duplicating logic.
        Bin minBin, maxBin;
        if (byIncome) {
            minBin.income = legendMin;
            maxBin.income = legendMax;
        } else if (byExpense) {
            minBin.expense = legendMin;
            maxBin.expense = legendMax;
        } else {
            minBin.delta = legendMin;
            maxBin.delta = legendMax;
        }
        setFrameColor(ui->periodHeatmapLegendMinFrame, cellColor(minBin));
        setFrameColor(ui->periodHeatmapLegendMaxFrame, cellColor(maxBin));

        ui->periodHeatmapLegendMinLabel->setText(
            tr("Min: ") + CurrencyHelper::formatAmount(legendMin, currInfo, locale, false));
        ui->periodHeatmapLegendMaxLabel->setText(
            tr("Max: ") + CurrencyHelper::formatAmount(legendMax, currInfo, locale, false));

        ui->periodHeatmapLegendMinFrame->setVisible(true);
        ui->periodHeatmapLegendMinLabel->setVisible(true);
        ui->periodHeatmapLegendMaxFrame->setVisible(true);
        ui->periodHeatmapLegendMaxLabel->setVisible(true);
    } else {
        ui->periodHeatmapLegendMinFrame->setVisible(false);
        ui->periodHeatmapLegendMinLabel->setVisible(false);
        ui->periodHeatmapLegendMaxFrame->setVisible(false);
        ui->periodHeatmapLegendMaxLabel->setVisible(false);
    }
}


void AnalysisDialog::on_periodHeatmapMonthlyRadioButton_clicked()
{
    ui->periodHeatmapExcludeCurrentCheckBox->setText(tr("Exclude current month"));
    ui->periodHeatmapExcludeLastCheckBox->setText(tr("Exclude last month"));
    periodHeatmapRedisplayTable();
}

void AnalysisDialog::on_periodHeatmapYearlyRadioButton_clicked()
{
    ui->periodHeatmapExcludeCurrentCheckBox->setText(tr("Exclude current year"));
    ui->periodHeatmapExcludeLastCheckBox->setText(tr("Exclude last year"));
    periodHeatmapRedisplayTable();
}

void AnalysisDialog::on_periodHeatmapExcludeCurrentCheckBox_checkStateChanged(Qt::CheckState)
{
    periodHeatmapRedisplayTable();
}

void AnalysisDialog::on_periodHeatmapExcludeLastCheckBox_checkStateChanged(Qt::CheckState)
{
    periodHeatmapRedisplayTable();
}

void AnalysisDialog::on_periodHeatmapIncomesRadioButton_clicked()
{
    periodHeatmapUpdateColorButtons();
    periodHeatmapRedisplayTable();
}

void AnalysisDialog::on_periodHeatmapExpensesRadioButton_clicked()
{
    periodHeatmapUpdateColorButtons();
    periodHeatmapRedisplayTable();
}

void AnalysisDialog::on_periodHeatmapDeltaRadioButton_clicked()
{
    periodHeatmapUpdateColorButtons();
    periodHeatmapRedisplayTable();
}


void AnalysisDialog::periodHeatmapSetColorButtonStyle(QPushButton* btn, QColor color)
{
    btn->setStyleSheet(
        QString("QPushButton { background-color: %1; border: none; }").arg(color.name()));
}

void AnalysisDialog::periodHeatmapUpdateColorButtons()
{
    bool byIncome  = ui->periodHeatmapIncomesRadioButton->isChecked();
    bool byExpense = ui->periodHeatmapExpensesRadioButton->isChecked();
    bool byDelta   = !byIncome && !byExpense;

    // Square reset buttons sized to the line height of the app font after AVERAGE shrinkage
    QFont btnFont = QApplication::font();
    Util::changeFontSize(btnFont, Util::FontResizeIntensity::AVERAGE, true,
        "PeriodHeatmap reset buttons");
    int s = QFontMetrics(btnFont).height();
    ui->periodHeatmapFromResetPushButton->setFixedSize(s, s);
    ui->periodHeatmapToResetPushButton->setFixedSize(s, s);

    // Min-color controls are only meaningful in delta mode
    ui->periodHeatmapFromColorLabel->setVisible(byDelta);
    ui->periodHeatmapFromColorPushButton->setVisible(byDelta);
    ui->periodHeatmapFromResetPushButton->setVisible(byDelta);

    if (byDelta) {
        periodHeatmapSetColorButtonStyle(ui->periodHeatmapFromColorPushButton,
                                         periodHeatmapDeltaFromColor);
    }

    QColor to;
    if (byIncome == true) {
        to = periodHeatmapIncomeToColor;
    } else if (byExpense == true) {
        to = periodHeatmapExpenseToColor;
    } else {
        to = periodHeatmapDeltaToColor;
    }
    periodHeatmapSetColorButtonStyle(ui->periodHeatmapToColorPushButton, to);
}

void AnalysisDialog::on_periodHeatmapFromColorPushButton_clicked()
{
    // Button is visible only in delta mode
    QColor color = QColorDialog::getColor(periodHeatmapDeltaFromColor, this, tr("Color chooser"));
    if (color.isValid()) {
        periodHeatmapDeltaFromColor = color;
        periodHeatmapSetColorButtonStyle(ui->periodHeatmapFromColorPushButton,
                                         periodHeatmapDeltaFromColor);
        periodHeatmapRedisplayTable();
    }
}

void AnalysisDialog::on_periodHeatmapToColorPushButton_clicked()
{
    bool byIncome  = ui->periodHeatmapIncomesRadioButton->isChecked();
    bool byExpense = ui->periodHeatmapExpensesRadioButton->isChecked();
    QColor* current;
    if (byIncome == true) {
        current = &periodHeatmapIncomeToColor;
    } else if (byExpense == true) {
        current = &periodHeatmapExpenseToColor;
    } else {
        current = &periodHeatmapDeltaToColor;
    }
    QColor color = QColorDialog::getColor(*current, this, tr("Color chooser"));
    if (color.isValid()) {
        *current = color;
        periodHeatmapSetColorButtonStyle(ui->periodHeatmapToColorPushButton, *current);
        periodHeatmapRedisplayTable();
    }
}

void AnalysisDialog::on_periodHeatmapFromResetPushButton_clicked()
{
    // Button is visible only in delta mode
    periodHeatmapDeltaFromColor = Qt::red;
    periodHeatmapSetColorButtonStyle(ui->periodHeatmapFromColorPushButton,
                                     periodHeatmapDeltaFromColor);
    periodHeatmapRedisplayTable();
}

void AnalysisDialog::on_periodHeatmapToResetPushButton_clicked()
{
    bool byIncome = ui->periodHeatmapIncomesRadioButton->isChecked();
    bool byExpense = ui->periodHeatmapExpensesRadioButton->isChecked();
    QColor* current;
    if (byIncome == true) {
        current = &periodHeatmapIncomeToColor;
    } else if (byExpense == true) {
        current = &periodHeatmapExpenseToColor;
    } else {
        current = &periodHeatmapDeltaToColor;
    }
    if (byExpense) {
        *current = Qt::red;
    } else {
        *current = Qt::green;
    }
    periodHeatmapSetColorButtonStyle(ui->periodHeatmapToColorPushButton, *current);
    periodHeatmapRedisplayTable();
}

void AnalysisDialog::on_periodHeatmapCellSizeComboBox_currentIndexChanged(int index)
{
    periodHeatmapCellSizeIndex = index;
    periodHeatmapRedisplayTable();
}
