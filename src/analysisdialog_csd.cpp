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
#include "customqchartview.h"
#include "ui_analysisdialog.h"
#include "gbpcontroller.h"
#include "gbplogger.h"
#include "util.h"
#include <QDateTimeAxis>
#include <QLegendMarker>
#include <QLineSeries>
#include <QScatterSeries>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPainter>
#include <algorithm>
#include <limits>


void AnalysisDialog::csdInitChart()
{
    csdChart = new QChart();
    csdChart->legend()->setVisible(false);
    csdChart->setLocale(locale);
    csdChart->setLocalizeNumbers(true);

    // X axis
    csdAxisX = new QDateTimeAxis;
    csdAxisX->setTickCount(6);
    csdAxisX->setFormat(locale.dateFormat(QLocale::ShortFormat));
    csdAxisX->setRange(QDateTime(QDate(2000, 1, 1), QTime(0, 0, 0)),
        QDateTime(QDate(2001, 1, 1), QTime(0, 0, 0)));
    csdChart->addAxis(csdAxisX, Qt::AlignBottom);

    // Y axis
    csdAxisY = new QValueAxis;
    csdAxisY->setTickCount(6);
    csdAxisY->setRange(0, 1);
    csdChart->addAxis(csdAxisY, Qt::AlignLeft);

    // Reduce axis font sizes
    QFont xFont = csdAxisX->labelsFont();
    Util::changeFontSize(xFont, Util::FontResizeIntensity::AVERAGE, true,
        "CSD tab - X axis");
    csdXAxisFontSize = static_cast<uint>(xFont.pointSize());
    csdAxisX->setLabelsFont(xFont);

    QFont yFont = csdAxisY->labelsFont();
    Util::changeFontSize(yFont, Util::FontResizeIntensity::AVERAGE, true,
        "CSD tab - Y axis");
    csdYAxisFontSize = static_cast<uint>(yFont.pointSize());
    csdAxisY->setLabelsFont(yFont);

    // Chart view embedded in the native widget
    csdChartView = new CustomQChartView(csdChart,
        GbpController::getInstance().getWheelRotatedAwayZoomIn(), ui->csdChartWidget);
    csdChartView->setRenderHint(QPainter::Antialiasing, true);

    csdThemeChanged();
}


void AnalysisDialog::csdThemeChanged()
{
    if (!csdChart) return;
    if (GbpController::getInstance().useDarkModeForChart()) {
        csdChart->setTheme(QChart::ChartThemeDark);
        csdChart->setBackgroundBrush(QBrush(QColor("black")));
    } else {
        csdChart->setTheme(QChart::ChartThemeLight);
        csdChart->setBackgroundBrush(QBrush(QColor("white")));
    }
    // Changing theme may reset font sizes — restore them
    QFont xFont = csdAxisX->labelsFont();
    xFont.setPointSize(static_cast<int>(csdXAxisFontSize));
    csdAxisX->setLabelsFont(xFont);
    QFont yFont = csdAxisY->labelsFont();
    yFont.setPointSize(static_cast<int>(csdYAxisFontSize));
    csdAxisY->setLabelsFont(yFont);

    // Theme change resets series styling — restore selected-point color on scatter series
    for (QAbstractSeries* absS : csdChart->series()) {
        QScatterSeries* scatter = qobject_cast<QScatterSeries*>(absS);
        if (!scatter) continue;
        if (GbpController::getInstance().useDarkModeForChart())
            scatter->setSelectedColor(GbpController::getInstance().getDarkModeSelectedPointColor());
        else
            scatter->setSelectedColor(
                GbpController::getInstance().getLightModeSelectedPointColor());
    }
}


void AnalysisDialog::csdRedisplayList()
{
    if (!ready) return;

    QSharedPointer<CombinedFeStreams> chartRawData = chartRawDataRef.toStrongRef();
    if (chartRawData.isNull()) {
        const QSignalBlocker blocker(ui->csdListWidget);
        ui->csdListWidget->clear();
        if (csdChart) csdChart->removeAllSeries();
        return;
    }

    bool incomeMode = ui->csdIncomesRadioButton->isChecked();

    // Collect unique CSDs from the entire raw data (UUID -> name, decoration color)
    QMap<QUuid, QString> csdIdToName;
    QMap<QUuid, QColor>  csdIdToColor;
    const QList<CombinedFeStreams::DailyInfo>& listDi = chartRawData->getCombinedStreams();
    for (const CombinedFeStreams::DailyInfo& di : listDi) {
        if (!di.used) continue;
        const QList<Fe>& feList = incomeMode ? di.incomesList : di.expensesList;
        for (const Fe& fe : feList) {
            QSharedPointer<Csd> csd = fe.csdPtr.toStrongRef();
            if (csd.isNull() || !csd->getActive()) continue;
            QUuid id = csd->getId();
            if (!csdIdToName.contains(id)) {
                csdIdToName.insert(id, csd->getName());
                csdIdToColor.insert(id, csd->getDecorationColor());
            }
        }
    }

    // Sort by name (case-insensitive) for display
    QList<QPair<QString, QUuid>> sortedCsds;
    sortedCsds.reserve(csdIdToName.size());
    for (auto it = csdIdToName.constBegin(); it != csdIdToName.constEnd(); ++it) {
        sortedCsds.append({it.value(), it.key()});
    }
    std::sort(sortedCsds.begin(), sortedCsds.end(),
        [](const QPair<QString, QUuid>& a, const QPair<QString, QUuid>& b) {
            return a.first.toLower() < b.first.toLower();
        });

    // Use the persistent selected set for the current mode as source of truth
    const QSet<QUuid>& checkedIds = incomeMode ? csdCheckedIncomeIds : csdCheckedExpenseIds;

    // Rebuild the list, blocking selection signals to avoid redundant chart rebuilds
    const bool colorizeNames = GbpController::getInstance().getAllowDecorationColor();
    {
        const QSignalBlocker blocker(ui->csdListWidget);
        ui->csdListWidget->clear();
        for (const auto& pair : sortedCsds) {
            QListWidgetItem* item = new QListWidgetItem(pair.first);
            item->setData(Qt::UserRole, pair.second);
            if (colorizeNames) {
                QColor decoColor = csdIdToColor.value(pair.second);
                if (decoColor.isValid())
                    item->setForeground(QBrush(decoColor));
            }
            ui->csdListWidget->addItem(item);
            item->setSelected(checkedIds.contains(pair.second));
        }
    }

    csdRedisplayChart();
}


void AnalysisDialog::csdRedisplayChart()
{
    if (!ready || !csdChart) return;

    csdClearSelectedPoint();

    auto clearListIcons = [this]() {
        const QSignalBlocker blocker(ui->csdListWidget);
        for (int i = 0; i < ui->csdListWidget->count(); ++i)
            ui->csdListWidget->item(i)->setIcon(QIcon());
    };

    QSharedPointer<CombinedFeStreams> chartRawData = chartRawDataRef.toStrongRef();
    if (chartRawData.isNull()) {
        csdChart->removeAllSeries();
        clearListIcons();
        return;
    }

    QDate from     = ui->globalFromDateEdit->date();
    QDate to       = ui->globalToDateEdit->date();
    QDate tomorrow = GbpController::getInstance().getTomorrow();
    if (!from.isValid() || !to.isValid() || to < from || from < tomorrow) {
        csdChart->removeAllSeries();
        clearListIcons();
        return;
    }

    bool incomeMode = ui->csdIncomesRadioButton->isChecked();
    const QSet<QUuid>& selectedIds =
        incomeMode ? csdCheckedIncomeIds : csdCheckedExpenseIds;

    // Collect selected CSDs in list order (order determines color assignment)
    QList<QUuid> checkedOrder;
    QMap<QUuid, QString> checkedNames;
    QMap<QUuid, QList<QPointF>> csdRawPoints;

    for (int i = 0; i < ui->csdListWidget->count(); ++i) {
        QListWidgetItem* item = ui->csdListWidget->item(i);
        QUuid id = item->data(Qt::UserRole).toUuid();
        if (selectedIds.contains(id)) {
            checkedOrder.append(id);
            checkedNames.insert(id, item->text());
            csdRawPoints.insert(id, {});
        }
    }

    if (checkedOrder.isEmpty()) {
        csdChart->removeAllSeries();
        clearListIcons();
        return;
    }

    // Maintain persistent color-slot assignment so each CSD keeps its assigned color
    // as long as it remains selected, regardless of what other CSDs are added/removed.
    const int modeIdx = incomeMode ? 0 : 1;
    QMap<QUuid, int>& colorAssign = csdColorAssign[modeIdx];
    for (auto it = colorAssign.begin(); it != colorAssign.end(); ) {
        it = selectedIds.contains(it.key()) ? ++it : colorAssign.erase(it);
    }
    for (const QUuid& id : checkedOrder) {
        if (!colorAssign.contains(id)) {
            QSet<int> used;
            for (int s : colorAssign) used.insert(s);
            for (int slot = 0; slot < 10; ++slot) {
                if (!used.contains(slot)) { colorAssign.insert(id, slot); break; }
            }
        }
    }

    // Single pass through raw data — listDi is already in ascending date order
    const QList<CombinedFeStreams::DailyInfo>& listDi = chartRawData->getCombinedStreams();
    const qsizetype diSize = listDi.size();
    for (qsizetype var = 0; var < diSize; ++var) {
        const CombinedFeStreams::DailyInfo& di = listDi[var];
        if (!di.used) continue;
        QDate diDate = tomorrow.addDays(static_cast<int>(var));
        if (diDate < from || diDate > to) continue;

        qreal xVal = static_cast<qreal>(
            QDateTime(diDate, QTime(0, 0, 0)).toMSecsSinceEpoch());
        const QList<Fe>& feList = incomeMode ? di.incomesList : di.expensesList;

        for (const Fe& fe : feList) {
            QSharedPointer<Csd> csd = fe.csdPtr.toStrongRef();
            if (csd.isNull()) continue;
            QUuid id = csd->getId();
            if (!csdRawPoints.contains(id)) continue;
            csdRawPoints[id].append({xVal, qAbs(fe.amount)});
        }
    }

    // Create series and add to chart
    csdChart->removeAllSeries();
    QList<QLineSeries*> createdSeries;

    for (const QUuid& id : checkedOrder) {
        const QList<QPointF>& rawPts = csdRawPoints.value(id);

        // Build step data using the MainWindow shadow approach:
        // insert a fake point 1 ms before each event at the previous Y value so the
        // line holds its current level until the exact moment the next event occurs.
        QList<QPointF> stepData;
        stepData.reserve(rawPts.size() > 0 ? rawPts.size() * 2 - 1 : 0);
        for (qsizetype i = 0; i < rawPts.size(); ++i) {
            if (i > 0) {
                stepData.append({rawPts[i].x() - 1.0, rawPts[i - 1].y()});
            }
            stepData.append(rawPts[i]);
        }

        QLineSeries* series = new QLineSeries();
        series->setName(checkedNames.value(id));
        series->append(stepData);
        csdChart->addSeries(series);
        series->attachAxis(csdAxisX);
        series->attachAxis(csdAxisY);
        createdSeries.append(series);
    }

    // Okabe-Ito palette: verified ≥3:1 contrast against both black and white backgrounds.
    static const QColor csdColors[10] = {
        QColor(  0, 114, 178),  // blue
        QColor(213,  94,   0),  // vermillion
        QColor(  0, 158, 115),  // teal green
        QColor(204, 121, 167),  // mauve
        QColor(230, 159,   0),  // amber
        QColor( 86, 180, 233),  // sky blue
        QColor(220,  50,  47),  // red
        QColor(148, 103, 189),  // purple
        QColor(140,  86,  75),  // brown
        QColor(127, 127, 127),  // gray
    };

    // Separate scatter series for point markers: the step line contains fake "hold"
    // points 1 ms before each real point, so setPointsVisible on the line would render
    // two overlapping dots per event. Use a dedicated QScatterSeries with raw points only.
    bool showPoints = ui->csdShowPointsCheckBox->isChecked();
    qreal pointSize = GbpController::getInstance().getChartPointSize();
    QList<QScatterSeries*> createdMarkers;
    for (int i = 0; i < checkedOrder.size(); ++i) {
        const QUuid& id = checkedOrder[i];
        QScatterSeries* markers = new QScatterSeries();
        markers->setName(QString());
        markers->append(csdRawPoints.value(id));
        markers->setMarkerSize(pointSize);
        markers->setVisible(showPoints);
        csdChart->addSeries(markers);
        markers->attachAxis(csdAxisX);
        markers->attachAxis(csdAxisY);
        if (GbpController::getInstance().useDarkModeForChart())
            markers->setSelectedColor(GbpController::getInstance().getDarkModeSelectedPointColor());
        else
            markers->setSelectedColor(GbpController::getInstance().getLightModeSelectedPointColor());
        const auto legendMarkers = csdChart->legend()->markers(markers);
        for (QLegendMarker* lm : legendMarkers)
            lm->setVisible(false);
        connect(markers, &QScatterSeries::clicked, this,
            [this, markers](const QPointF& pt) { csdPointClicked(markers, pt); });
        createdMarkers.append(markers);
    }

    // Apply all colors after every addSeries() is done: Qt Charts re-applies the theme
    // to all existing series on each addSeries() call, so colors must be set last.
    for (int i = 0; i < createdSeries.size(); ++i) {
        QPen pen(csdColors[colorAssign.value(checkedOrder[i])]);
        pen.setWidth(2);
        createdSeries[i]->setPen(pen);
    }
    for (int i = 0; i < createdMarkers.size(); ++i) {
        const QColor& col = csdColors[colorAssign.value(checkedOrder[i])];
        createdMarkers[i]->setColor(col);
        createdMarkers[i]->setBorderColor(col);
    }

    // Update list item icons: colored square for checked items, transparent for unchecked.
    // QSignalBlocker prevents setIcon from re-triggering itemChanged.
    {
        const QSignalBlocker blocker(ui->csdListWidget);
        const int sz = ui->csdListWidget->iconSize().width();
        const QColor borderColor =
            GbpController::getInstance().useDarkModeForChart() ? Qt::white : Qt::black;
        for (int i = 0; i < ui->csdListWidget->count(); ++i) {
            QListWidgetItem* lwItem = ui->csdListWidget->item(i);
            QUuid id = lwItem->data(Qt::UserRole).toUuid();
            QPixmap px(sz, sz);
            if (colorAssign.contains(id)) {
                px.fill(borderColor);
                QPainter p(&px);
                p.fillRect(1, 1, sz - 2, sz - 2, csdColors[colorAssign.value(id)]);
            } else {
                px.fill(Qt::transparent);
            }
            lwItem->setIcon(QIcon(px));
        }
    }

    csdRescaleChart();

    // Y axis format
    QString yValFormat = QString("\%.%1f").arg(currInfo.noOfDecimal);
    csdAxisY->setLabelFormat(yValFormat);

    // Restore font sizes (theme change via addSeries may have reset them)
    QFont xFont = csdAxisX->labelsFont();
    xFont.setPointSize(static_cast<int>(csdXAxisFontSize));
    csdAxisX->setLabelsFont(xFont);
    QFont yFont = csdAxisY->labelsFont();
    yFont.setPointSize(static_cast<int>(csdYAxisFontSize));
    csdAxisY->setLabelsFont(yFont);
}


void AnalysisDialog::csdRescaleChart()
{
    if (!csdChart) return;

    // X axis: always use the global date range so it updates immediately when
    // dates change, regardless of where the actual data points fall.
    QDate fromDate = ui->globalFromDateEdit->date();
    QDate toDate   = ui->globalToDateEdit->date();
    QDateTime xFrom, xTo;
    if (fromDate.isValid() && toDate.isValid() && toDate >= fromDate) {
        xFrom = QDateTime(fromDate, QTime(0, 0, 0));
        xTo   = QDateTime(toDate,   QTime(23, 59, 59));
    } else {
        xFrom = QDateTime(GbpController::getInstance().getTomorrow(), QTime(0, 0, 0));
        xTo   = xFrom.addYears(1).addDays(-1);
    }
    csdAxisX->setRange(xFrom, xTo);

    // Y axis: auto-scale to the visible data within the X range.
    QList<QPointF> allPoints;
    for (QAbstractSeries* absS : csdChart->series()) {
        QLineSeries* s = qobject_cast<QLineSeries*>(absS);
        if (s) allPoints.append(s->points());
    }
    double yFrom = 0.0;
    double yTo   = 1.0;
    if (!allPoints.isEmpty()) {
        bool result = Util::findMinMaxInYvalues(allPoints,
            xFrom.toMSecsSinceEpoch(), xTo.toMSecsSinceEpoch(), yFrom, yTo);
        if (!result) { yFrom = 0.0; yTo = 1.0; }
    }
    double displayYfrom = yFrom;
    double displayYto   = yTo;
    Util::calculateZoomYaxis(displayYfrom, displayYto,
        GbpController::getInstance().getPercentageMainChartScaling() / 100.0);
    csdAxisY->setRange(displayYfrom, displayYto);
}


// --- Slots ---

void AnalysisDialog::on_csdIncomesRadioButton_clicked()
{
    csdRedisplayList();
    LOG_DEBUG_INFO(QString("Analysis Dialog - CSD : Income radiobutton clicked"));
}


void AnalysisDialog::on_csdExpensesRadioButton_clicked()
{
    csdRedisplayList();
    LOG_DEBUG_INFO(QString("Analysis Dialog - CSD : Expense radiobutton clicked"));
}




void AnalysisDialog::on_csdListWidget_itemSelectionChanged()
{
    const QSignalBlocker blocker(ui->csdListWidget);
    QList<QListWidgetItem*> selected = ui->csdListWidget->selectedItems();

    if (selected.size() > 10) {
        for (int i = 10; i < selected.size(); ++i)
            selected[i]->setSelected(false);
        selected.resize(10);
    }

    bool incomeMode = ui->csdIncomesRadioButton->isChecked();
    QSet<QUuid>& checkedIds = incomeMode ? csdCheckedIncomeIds : csdCheckedExpenseIds;
    checkedIds.clear();
    for (QListWidgetItem* it : selected)
        checkedIds.insert(it->data(Qt::UserRole).toUuid());

    csdRedisplayChart();
}


void AnalysisDialog::on_csdFitPushButton_clicked()
{
    csdRescaleChart();
}


void AnalysisDialog::on_csdUnselectAllPushButton_clicked()
{
    bool incomeMode = ui->csdIncomesRadioButton->isChecked();
    QSet<QUuid>& checkedIds = incomeMode ? csdCheckedIncomeIds : csdCheckedExpenseIds;
    const QSignalBlocker blocker(ui->csdListWidget);
    ui->csdListWidget->clearSelection();
    checkedIds.clear();
    csdColorAssign[incomeMode ? 0 : 1].clear();
    csdRedisplayChart();
}

void AnalysisDialog::on_csdShowPointsCheckBox_stateChanged(int /*arg1*/)
{
    bool showPoints = ui->csdShowPointsCheckBox->isChecked();
    for (QAbstractSeries* absS : csdChart->series()) {
        if (qobject_cast<QScatterSeries*>(absS))
            absS->setVisible(showPoints);
    }
}

void AnalysisDialog::csdClearSelectedPoint()
{
    if (csdLastSelectedSeries && csdLastSelectedIndex >= 0)
        csdLastSelectedSeries->setPointSelected(csdLastSelectedIndex, false);
    csdLastSelectedSeries = nullptr;
    csdLastSelectedIndex  = -1;
    ui->csdSelectedPointDateLabel->clear();
    ui->csdSelectedPointValueLabel->clear();
}

void AnalysisDialog::csdPointClicked(QScatterSeries* series, const QPointF& pt)
{
    const QList<QPointF> pts = series->points();
    if (pts.isEmpty()) return;

    // QDateTimeAxis converts pixel → datetime → ms, which can introduce small
    // floating-point rounding in pt.x(), causing an exact indexOf to fail.
    // Use nearest-point search so the correct stored point is always found.
    int index = 0;
    qreal minDist = std::numeric_limits<qreal>::max();
    for (int i = 0; i < pts.size(); ++i) {
        const qreal dx = pts[i].x() - pt.x();
        const qreal dy = pts[i].y() - pt.y();
        const qreal d  = dx * dx + dy * dy;
        if (d < minDist) { minDist = d; index = i; }
    }

    if (series == csdLastSelectedSeries && index == csdLastSelectedIndex) {
        csdClearSelectedPoint();
        return;
    }

    csdClearSelectedPoint();

    series->setPointSelected(index, true);
    csdLastSelectedSeries = series;
    csdLastSelectedIndex  = index;

    const QPointF storedPt = pts[index];
    const QDate date = QDateTime::fromMSecsSinceEpoch(
        static_cast<qint64>(storedPt.x())).date();
    ui->csdSelectedPointDateLabel->setText(
        locale.toString(date, "yyyy-MMM-dd"));
    ui->csdSelectedPointValueLabel->setText(
        locale.toString(storedPt.y(), 'f', currInfo.noOfDecimal));
}
