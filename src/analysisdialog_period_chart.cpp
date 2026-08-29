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
#include "gbpcontroller.h"
#include "util.h"
#include <QChart>
#include <QCursor>
#include <QFileDialog>
#include <QLegendMarker>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QToolTip>
#include <QValueAxis>
#include <qgraphicslayout.h>


/**
 * @brief Rebuild and repaint the bar chart for the given period granularity.
 *
 * Reads the pre-computed bins (binsMonthly or binsYearly), selects the date
 * window defined by periodChartOffset and the bar-count spin-box, then
 * replaces all chart series with a fresh QBarSeries whose content matches
 * the radio-button selection (income / expenses / both / delta). Statistics
 * (mean, std-dev, sum) are shown in the stats label for single-set modes.
 *
 * @note Colors are applied **after** QChart::addSeries() to prevent the chart
 *       theme from overriding them: Qt Charts increments an internal palette
 *       index on every addSeries() call and ignores any setColor() made before.
 *
 * @param type MONTHLY or YEARLY — selects which chart object (and bins) to use.
 */
void AnalysisDialog::periodChartRedisplayChart(PeriodType type)
{
    // Make sure the raw data is available (which should always be the case)
    QSharedPointer<CombinedFeStreams> chartRawData = chartRawDataRef.toStrongRef();
    if(chartRawData.isNull()){
        return; // should never happen
    }
    QList<CombinedFeStreams::DailyInfo> listDi = chartRawData->getCombinedStreams();
    const qsizetype sizeDi = listDi.size();

    // Set pointers to appropriate entities depending on the value of "type"
    QChart** chartPtr;
    QChartView** chartViewPtr = nullptr;
    QBarCategoryAxis** xAxisPtr;
    QValueAxis** yAxisPtr;
    QLabel *statLabelPtr;
    if (type == PeriodType::MONTHLY) {
        chartPtr = &chartMonthlyReport;
        chartViewPtr = &chartViewMonthlyReport;
        xAxisPtr = &chartMonthlyReportAxisX;
        yAxisPtr = &chartMonthlyReportAxisY;
        statLabelPtr = ui->periodChartStatsLabel;
    } else {
        chartPtr = &chartYearlyReport;
        chartViewPtr = &chartViewYearlyReport;
        xAxisPtr = &chartYearlyReportAxisX;
        yAxisPtr = &chartYearlyReportAxisY;
        statLabelPtr = ui->periodChartStatsLabel;
    }

    // If no data to display
    if (chartRawData->getNoOfElementsUsed()==0){
        (*chartPtr)->removeAllSeries();
        QStringList empty = QStringList();
        empty.append("No data");
        (*xAxisPtr)->setCategories(empty);
        (*yAxisPtr)->setRange(0,1);
        statLabelPtr->setText("");
        periodChartUpdateNavButtons(type);
        return;  // nothing to do after
    }

    // determine which data set is to be used
    PeriodChartSourceType dsType = periodChartFindWhichSetIsToBeUsed(type);

    // choose the right bins
    QMap<QDate,Bin>* binsPtr;
    if (type==PeriodType::MONTHLY) {
        binsPtr = &binsMonthly;
    } else {
         binsPtr = &binsYearly;
    }

    // Set subset of bin's data to use for display
    QDate startDate, endDate ;
    periodChartGetStartEndDates(type, startDate, endDate);

    // Calculate stats
    QDate date = startDate;
    double mean=0;
    double median=0;
    double stdDeviation=0;
    double sum=0;
    QList<double> statData;
    while(date<=endDate){
        Bin mr = Bin();
        if (true == binsPtr->contains(date) ){
            mr = binsPtr->value(date);
        }
        // record the right data
        if (dsType==PeriodChartSourceType::DST_INCOME) {
            statData.append(mr.income);
        } else if (dsType==PeriodChartSourceType::DST_EXPENSE){
            statData.append(-mr.expense);
        } else if(dsType==PeriodChartSourceType::DST_DELTA){
            statData.append(mr.delta);
        }
        // go to next date
        if (type==PeriodType::MONTHLY) {
            date = date.addMonths(1);
        } else {
            date = date.addYears(1);
        }
    }
    Util::calculateStats(statData, mean, stdDeviation, median, sum);

    // build new set of chart data (3 sets of QBarSet) and categories
    QColor red = GbpController::getInstance().getExpenseColor();
    QColor green = GbpController::getInstance().getIncomeColor();
    double maxY = 1;
    double minY = 0;
    QStringList categories;
    QBarSet *setIncomes = new QBarSet(tr("Income"));
    QBarSet *setExpenses = new QBarSet(tr("Expenses"));
    QBarSet *setDeltasPositive = new QBarSet(tr("Deltas - Surplus"));
    QBarSet *setDeltasNegative = new QBarSet(tr("Deltas - Deficit"));

    date= startDate;
    while(date<=endDate){
        Bin mr = Bin();
        if (true == binsPtr->contains(date) ){
            mr = binsPtr->value(date);
        }
        setIncomes->append(mr.income);
        setExpenses->append(mr.expense);
        if (mr.delta<0) {
            setDeltasNegative->append(mr.delta);
            setDeltasPositive->append(0);
        } else {
            setDeltasPositive->append(mr.delta);
            setDeltasNegative->append(0);
        }
        categories.append(periodChartBuildCategoryName(type,date));
        // go to next date
        if (type==PeriodType::MONTHLY) {
            date = date.addMonths(1);
        } else {
            date = date.addYears(1);
        }
    }

    // find min-max in all the set
    periodChartGetMinMax(type, setIncomes, setExpenses, setDeltasPositive, setDeltasNegative,
        minY, maxY);

    // deallocate all the stuff currently in use
    (*chartPtr)->removeAllSeries(); // deallocate all the current stuff, EXCLUDING both axis

    // completely rebuild the series, discard set not to be used
    QBarSeries* series = new QBarSeries();
    if( (dsType==PeriodChartSourceType::DST_INCOME) ||
        (dsType==PeriodChartSourceType::DST_INCOME_EXPENSE) ){
        series->append(setIncomes); // take ownership
    } else{
        delete setIncomes;
    }
    if( (dsType==PeriodChartSourceType::DST_EXPENSE) ||
        (dsType==PeriodChartSourceType::DST_INCOME_EXPENSE) ){
        series->append(setExpenses); // take ownership
    } else{
        delete setExpenses;
    }
    if( dsType==PeriodChartSourceType::DST_DELTA ){
        series->append(setDeltasPositive); // take ownership
        series->append(setDeltasNegative); // take ownership
    } else{
        delete setDeltasPositive;
        delete setDeltasNegative;
    }
    (*chartPtr)->addSeries(series); // take ownership

    // Apply custom colors AFTER addSeries so they override whatever the chart theme assigned.
    // Qt Charts re-applies the theme palette to every bar set on addSeries(), which increments
    // an internal color index and ignores any setColor() called before addSeries().
    if( (dsType==PeriodChartSourceType::DST_INCOME) ||
        (dsType==PeriodChartSourceType::DST_INCOME_EXPENSE) ){
        setIncomes->setColor(green);
    }
    if( (dsType==PeriodChartSourceType::DST_EXPENSE) ||
        (dsType==PeriodChartSourceType::DST_INCOME_EXPENSE) ){
        setExpenses->setColor(red);
    }
    if( dsType==PeriodChartSourceType::DST_DELTA ){
        setDeltasPositive->setColor(green);
        setDeltasNegative->setColor(red);
    }

    // connect hover tooltip
    if (type==PeriodType::MONTHLY) {
        QObject::connect(series, &QAbstractBarSeries::hovered, series,
            [=](bool status, int index, QBarSet *set) {
                if (!status) { QToolTip::hideText(); return; }
                // Recompute the period date directly from the bar index instead of parsing
                // the axis category text back, since that text is now purely locale-formatted
                // (see periodChartBuildCategoryName()) and no longer a parseable "M-yy" key.
                QDate periodDate = startDate.addMonths(index);
                QString period = QString("%1 %2")
                    .arg(locale.monthName(periodDate.month(), QLocale::FormatType::ShortFormat))
                    .arg(locale.toString(periodDate.year()%100));
                QString text = QString("%1\n%2\n%3").arg(set->label()).arg(period)
                    .arg(CurrencyHelper::formatAmount(set->at(index), currInfo, locale, true));
                QToolTip::showText(QCursor::pos(), text);
            });
    } else {
        QObject::connect(series, &QAbstractBarSeries::hovered, series,
            [=](bool status, int index, QBarSet *set) {
                if (!status) { QToolTip::hideText(); return; }
                int year = startDate.addYears(index).year();
                // A year is an identifier, not a quantity - no thousands separator.
                QLocale noGroupLocale = locale;
                noGroupLocale.setNumberOptions(QLocale::OmitGroupSeparator);
                QString text = QString("%1\n%2\n%3").arg(set->label())
                    .arg(noGroupLocale.toString(year))
                    .arg(CurrencyHelper::formatAmount(set->at(index), currInfo, locale, true));
                QToolTip::showText(QCursor::pos(), text);
            });
    }


    // place legend to the right
    QLegend *legend = (*chartPtr)->legend();
    legend->setAlignment(Qt::AlignRight);

    // rebuild the 2 axes and attach
    (*xAxisPtr)->setCategories(categories);
    series->attachAxis((*xAxisPtr));
    (*yAxisPtr)->setRange(minY,maxY);
    series->attachAxis((*yAxisPtr));

    // Refresh the info in stats area
    if (dsType != PeriodChartSourceType::DST_INCOME_EXPENSE) {
        QString statText;
        if (dsType==PeriodChartSourceType::DST_EXPENSE) {
            statText = tr("Mean:%1  StdDeviation:%2  Sum:%3")
                .arg(CurrencyHelper::formatAmount(-mean,currInfo, locale, false))
                .arg(CurrencyHelper::formatAmount(stdDeviation,currInfo, locale, false))
                .arg(CurrencyHelper::formatAmount(-sum,currInfo, locale, false));
        } else {
            statText = tr("Mean:%1  StdDeviation:%2  Sum:%3")
                .arg(CurrencyHelper::formatAmount(mean,currInfo, locale, false))
                .arg(CurrencyHelper::formatAmount(stdDeviation,currInfo, locale, false))
                .arg(CurrencyHelper::formatAmount(sum,currInfo, locale, false));
        }
        statLabelPtr->setText(statText);
    } else {
        statLabelPtr->setText("");
    }

    periodChartUpdateNavButtons(type);
}




/**
 * @brief Format a period start date as a short x-axis category label.
 *
 * Monthly: "M-YY"  (e.g. "3-25" for March 2025).
 * Yearly:  "YYYY"  (e.g. "2025").
 *
 * @param type MONTHLY or YEARLY.
 * @param date First day of the target period (first of month, or first of year).
 * @return Formatted label string.
 */
QString AnalysisDialog::periodChartBuildCategoryName(PeriodType type, QDate date) const
{
    // Month/year numbers here are identifiers (calendar positions), not quantities - no
    // thousands separator (irrelevant at 1-2 digits, but kept consistent with the rest of
    // the app). This string is only ever displayed on the axis - callers that need the
    // underlying date back (e.g. hover tooltips) recompute it directly instead of parsing
    // this text, so it is safe for it to be purely locale-formatted here.
    QLocale noGroupLocale = locale;
    noGroupLocale.setNumberOptions(QLocale::OmitGroupSeparator);
    if (type == PeriodType::MONTHLY) {
        int smallYear = date.year()%100;
        QString s = QString("%1-%2").arg(noGroupLocale.toString(date.month()))
            .arg(noGroupLocale.toString(smallYear));
        return s;
    } else {
        return noGroupLocale.toString(date.year());
    }
}


/**
 * @brief Create and wire up the QChart, QChartView, and both axes for one period type.
 *
 * Called once per period type during dialog initialisation. A placeholder
 * QBarSeries with empty bar sets is installed so that the widget renders
 * correctly before any scenario is loaded. The axes are added to the chart
 * (giving the chart ownership) and attached to the placeholder series; they
 * remain alive across subsequent removeAllSeries() / addSeries() cycles in
 * periodChartRedisplayChart().
 *
 * @param type MONTHLY or YEARLY — determines which member pointers are populated
 *             and whether the chart view is initially visible.
 */
void AnalysisDialog::periodChartInitChart(PeriodType type)
{
    QChart** chartPtr;
    QChartView** chartViewPtr = nullptr;
    QBarCategoryAxis** xAxisPtr;
    QValueAxis** yAxisPtr;
    QWidget** widgetPtr;
    QString chartTitle;
    if (type == PeriodType::MONTHLY) {
        chartPtr = &chartMonthlyReport;
        chartViewPtr = &chartViewMonthlyReport;
        xAxisPtr = &chartMonthlyReportAxisX;
        yAxisPtr = &chartMonthlyReportAxisY;
        widgetPtr = &(ui->periodChartWidget);
        chartTitle = tr("Monthly income and expenses");
        ui->periodChartStatsLabel->setText("");
    } else {
        chartPtr = &chartYearlyReport;
        chartViewPtr = &chartViewYearlyReport;
        xAxisPtr = &chartYearlyReportAxisX;
        yAxisPtr = &chartYearlyReportAxisY;
        widgetPtr = &(ui->periodChartWidget);
        chartTitle = tr("Yearly income and expenses");
        ui->periodChartStatsLabel->setText("");
    }

    // Data (sets) will be discarded and rebuild whe the dialog is re-shown (Prepare slot)
    QBarSet *set0 = new QBarSet(tr("Income"));
    QBarSet *set1 = new QBarSet(tr("Expenses"));
    QBarSeries* series = new QBarSeries();
    series->append(set0);  // take ownership
    series->append(set1);  // take ownership

    (*chartPtr) = new QChart();
    (*chartPtr)->setLocalizeNumbers(true);
    (*chartPtr)->addSeries(series); // take ownership
    //(*chartPtr)->setTitle(chartTitle);
    (*chartPtr)->setAnimationOptions(QChart::SeriesAnimations);
    (*chartViewPtr) = new QChartView((*chartPtr), (*widgetPtr)); // take ownership
    (*chartViewPtr)->setVisible(type == PeriodType::MONTHLY);
    (*chartViewPtr)->setRenderHint(QPainter::Antialiasing);
    (*chartPtr)->legend()->show();
    (*chartPtr)->legend()->setAlignment(Qt::AlignBottom);
    // 1 pixel wide border
    (*chartPtr)->layout()->setContentsMargins(1, 1, 1, 1);
    (*chartPtr)->setBackgroundRoundness(0);

    // x axis
    QStringList categories={};
    (*xAxisPtr)  = new QBarCategoryAxis();
    //
    QFont fontX = (*xAxisPtr)->labelsFont();
    if (type == PeriodType::MONTHLY) {
        Util::changeFontSize(fontX, Util::FontResizeIntensity::AVERAGE, true,
            "AnalysisDialog::init monthly ReportChart - X axis");
    } else {
        Util::changeFontSize(fontX, Util::FontResizeIntensity::AVERAGE, true,
            "AnalysisDialog::init yearly ReportChart - X axis");
    }
    (*xAxisPtr)->setLabelsFont(fontX);
    //
    (*xAxisPtr)->append(categories);
    (*chartPtr)->addAxis((*xAxisPtr), Qt::AlignBottom); //the CHART (not the series) takes ownership
    series->attachAxis((*xAxisPtr));
    // y axis
    (*yAxisPtr) = new QValueAxis();
    //
    QFont fontY = (*yAxisPtr)->labelsFont();
    if (type == PeriodType::MONTHLY) {
        Util::changeFontSize(fontY, Util::FontResizeIntensity::AVERAGE, true,
            "AnalysisDialog::init monthly ReportChart - Y axis");
    } else {
        Util::changeFontSize(fontY, Util::FontResizeIntensity::AVERAGE, true,
            "AnalysisDialog::init yearly ReportChart - Y axis");
    }
    (*yAxisPtr)->setLabelsFont(fontY);
    //
    (*yAxisPtr)->setRange(0,1);
    (*yAxisPtr)->setTickCount(6);      // 5 divisions
    (*yAxisPtr)->setMinorTickCount(0); // no minor divisions
    (*chartPtr)->addAxis((*yAxisPtr), Qt::AlignLeft); // the CHART (not the series) takes ownership
    series->attachAxis((*yAxisPtr));
    if(GbpController::getInstance().useDarkModeForChart()==true){
        (*chartPtr)->setTheme(QChart::ChartThemeDark);
    } else {
        (*chartPtr)->setTheme(QChart::ChartThemeLight);
    }
    (*widgetPtr)->installEventFilter(this);
}


/**
 * @brief Compute the first and last period dates visible in the current chart window.
 *
 * Applies periodChartOffset (number of periods scrolled forward from the
 * scenario start) and the bar-count spin-box value to derive the window.
 * Both dates are clamped to the scenario's date range so the window never
 * extends beyond available data.
 *
 * @param type        MONTHLY or YEARLY.
 * @param[out] startDate First-of-period for the leftmost visible bar.
 * @param[out] endDate   First-of-period for the rightmost visible bar.
 */
void AnalysisDialog::periodChartGetStartEndDates(PeriodType type, QDate &startDate,
    QDate &endDate) const
{
    QDate from = ui->globalFromDateEdit->date();
    QDate to   = ui->globalToDateEdit->date();
    QDate fullStart, fullEnd;
    if (type == PeriodType::YEARLY) {
        fullStart = QDate(from.year(), 1, 1);
        fullEnd   = QDate(to.year(), 1, 1);
    } else {
        fullStart = QDate(from.year(), from.month(), 1);
        fullEnd   = QDate(to.year(), to.month(), 1);
    }

    int barCount = ui->periodChartBarCountSpinBox->value();

    if (type == PeriodType::YEARLY) {
        startDate = fullStart.addYears(periodChartOffset);
    } else {
        startDate = fullStart.addMonths(periodChartOffset);
    }
    if (startDate > fullEnd) startDate = fullEnd;

    if (type == PeriodType::YEARLY) {
        endDate = startDate.addYears(barCount - 1);
    } else {
        endDate = startDate.addMonths(barCount - 1);
    }
    if (endDate > fullEnd) endDate = fullEnd;
}


/**
 * @brief Enable or disable the Prev / Next navigation buttons.
 *
 * Prev is enabled when periodChartOffset > 0 (not yet at the start of the
 * scenario). Next is enabled when the current window does not reach the last
 * period of the scenario.
 *
 * @param type MONTHLY or YEARLY — used to compute the total number of periods.
 */
void AnalysisDialog::periodChartUpdateNavButtons(PeriodType type)
{
    QDate from = ui->globalFromDateEdit->date();
    QDate to   = ui->globalToDateEdit->date();
    int totalSteps;
    if (type == PeriodType::YEARLY) {
        totalSteps = to.year() - from.year() + 1;
    } else {
        totalSteps = (to.year() - from.year()) * 12 + (to.month() - from.month()) + 1;
    }
    int barCount = ui->periodChartBarCountSpinBox->value();
    ui->periodChartPrevPushButton->setEnabled(periodChartOffset > 0);
    ui->periodChartNextPushButton->setEnabled(periodChartOffset + barCount < totalSteps);
}




/**
 * @brief Compute the Y-axis range [minY, maxY] for the currently visible bars.
 *
 * Reads the active display type from the radio buttons, then scans only the
 * bar sets that are relevant to that type. For all modes except DST_DELTA,
 * minY is 0 (all values are non-negative). For DST_DELTA, minY can be
 * negative (deficit bars extend below the x-axis).
 *
 * @param type           MONTHLY or YEARLY — forwarded to periodChartFindWhichSetIsToBeUsed().
 * @param incomes        Income bar set (all values >= 0).
 * @param expenses       Expense bar set (all values >= 0, stored as absolute amounts).
 * @param deltasPositive Surplus delta bar set (values >= 0).
 * @param deltasNegative Deficit delta bar set (values <= 0).
 * @param[out] minY      Minimum visible Y value (0 except in delta mode).
 * @param[out] maxY      Maximum visible Y value (at least 1).
 */
void AnalysisDialog::periodChartGetMinMax(PeriodType type, QBarSet* incomes, QBarSet* expenses,
    QBarSet* deltasPositive, QBarSet* deltasNegative, double& minY, double& maxY)
{
    PeriodChartSourceType dsType = periodChartFindWhichSetIsToBeUsed(type);

    // default values : it works because we must have at least one element to search for in the sets
    minY = 0;
    maxY = 1;

    if( (dsType==PeriodChartSourceType::DST_INCOME) ||
        (dsType==PeriodChartSourceType::DST_INCOME_EXPENSE) ){
        // process incomes
        for(int i=0;i<incomes->count();i++){
            if ( incomes->at(i) > maxY ){
                maxY = incomes->at(i);
            }
        }
    }
    if( (dsType==PeriodChartSourceType::DST_EXPENSE) ||
        (dsType==PeriodChartSourceType::DST_INCOME_EXPENSE) ){
        // process expenses
        for(int i=0;i<expenses->count();i++){
            if ( expenses->at(i) > maxY ){
                maxY = expenses->at(i);
            }
        }
    }
    if( dsType==PeriodChartSourceType::DST_DELTA ){
        // process deltas - max
        for(int i=0;i<deltasPositive->count();i++){
            if ( deltasPositive->at(i) > maxY ){
                maxY = deltasPositive->at(i);
            }
        }
        // process deltas - min
        for(int i=0;i<deltasNegative->count();i++){
            if ( deltasNegative->at(i) < minY ){
                minY = deltasNegative->at(i);
            }
        }
    }
}


/**
 * @brief Return the PeriodChartSourceType corresponding to the checked radio button.
 *
 * @param type Unused; present for API symmetry with other periodChart* helpers.
 * @return Active data-source type (DST_INCOME, DST_EXPENSE, DST_INCOME_EXPENSE,
 *         or DST_DELTA).
 * @throws std::logic_error if no radio button is checked (should never happen).
 */
AnalysisDialog::PeriodChartSourceType AnalysisDialog::periodChartFindWhichSetIsToBeUsed(
    PeriodType type)
{
    if (ui->periodChartTypeIncomeRadioButton->isChecked()) {
        return PeriodChartSourceType::DST_INCOME;
    } else if (ui->periodChartTypeExpensesRadioButton->isChecked()) {
        return PeriodChartSourceType::DST_EXPENSE;
    } else if (ui->periodChartTypeIncomeAndExpensesRadioButton->isChecked()) {
        return PeriodChartSourceType::DST_INCOME_EXPENSE;
    } else if (ui->periodChartTypeDeltasRadioButton->isChecked()) {
        return PeriodChartSourceType::DST_DELTA;
    } else {
        // should never happen
        throw std::logic_error(QString("%1: No control checked for Graph Report")
            .arg(Q_FUNC_INFO).toStdString());
    }
}


void AnalysisDialog::on_periodChartTypeIncomeRadioButton_clicked()
{
    periodChartRedisplayChart(
        ui->periodChartMonthlyRadioButton->isChecked() ? PeriodType::MONTHLY : PeriodType::YEARLY);
}




void AnalysisDialog::on_periodChartTypeExpensesRadioButton_clicked()
{
    periodChartRedisplayChart(
        ui->periodChartMonthlyRadioButton->isChecked() ? PeriodType::MONTHLY : PeriodType::YEARLY);
}




void AnalysisDialog::on_periodChartTypeIncomeAndExpensesRadioButton_clicked()
{
    periodChartRedisplayChart(
        ui->periodChartMonthlyRadioButton->isChecked() ? PeriodType::MONTHLY : PeriodType::YEARLY);
}




void AnalysisDialog::on_periodChartTypeDeltasRadioButton_clicked()
{
    periodChartRedisplayChart(
        ui->periodChartMonthlyRadioButton->isChecked() ? PeriodType::MONTHLY : PeriodType::YEARLY);
}




void AnalysisDialog::on_periodChartMonthlyRadioButton_clicked()
{
    periodChartOffset = 0;
    ui->periodChartDurationUnitLabel->setText(tr(" months"));
    if (chartViewYearlyReport)  chartViewYearlyReport->setVisible(false);
    if (chartViewMonthlyReport) chartViewMonthlyReport->setVisible(true);
    periodChartRedisplayChart(PeriodType::MONTHLY);
}




void AnalysisDialog::on_periodChartAnnuallyRadioButton_clicked()
{
    periodChartOffset = 0;
    ui->periodChartDurationUnitLabel->setText(tr(" years"));
    if (chartViewMonthlyReport) chartViewMonthlyReport->setVisible(false);
    if (chartViewYearlyReport)  chartViewYearlyReport->setVisible(true);
    periodChartRedisplayChart(PeriodType::YEARLY);
}


void AnalysisDialog::on_periodChartPrevPushButton_clicked()
{
    int barCount = ui->periodChartBarCountSpinBox->value();
    periodChartOffset = qMax(0, periodChartOffset - barCount);
    PeriodType type = ui->periodChartMonthlyRadioButton->isChecked() ?
        PeriodType::MONTHLY : PeriodType::YEARLY;
    periodChartRedisplayChart(type);
}


void AnalysisDialog::on_periodChartNextPushButton_clicked()
{
    int barCount = ui->periodChartBarCountSpinBox->value();
    periodChartOffset += barCount;
    PeriodType type = ui->periodChartMonthlyRadioButton->isChecked() ?
        PeriodType::MONTHLY : PeriodType::YEARLY;
    periodChartRedisplayChart(type);
}


void AnalysisDialog::on_periodChartBarCountSpinBox_valueChanged(int /*value*/)
{
    periodChartOffset = 0;
    PeriodType type = ui->periodChartMonthlyRadioButton->isChecked() ?
        PeriodType::MONTHLY : PeriodType::YEARLY;
    periodChartRedisplayChart(type);
}

