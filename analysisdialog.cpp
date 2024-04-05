/*
 *  Copyright (C) 2024 Claude Dumas <claudedumas63@protonmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "analysisdialog.h"
#include "ui_analysisdialog.h"
#include "gbpcontroller.h"
#include "util.h"
#include <QChart>
#include <QPieSeries>
#include <QFileDialog>
#include <QMessageBox>
#include <QLegendMarker>
#include <QFontDatabase>
#include <QStringView>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <limits.h>



AnalysisDialog::AnalysisDialog(QLocale theLocale, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AnalysisDialog)
{
    ui->setupUi(this);
    locale = theLocale;

    // *** RELATIVE WEIGHT CONTROLS ***
    seriesRelativeWeigth = new QPieSeries;
    chartRelativeWeigth = new QChart;
    chartRelativeWeigth->addSeries(seriesRelativeWeigth); // take ownership
    chartRelativeWeigth->setAnimationOptions(QChart::AllAnimations);
    chartRelativeWeigth->legend()->show();
    chartRelativeWeigth->legend()->setAlignment(Qt::AlignRight);
    chartViewRelativeWeigth = new QChartView(chartRelativeWeigth, ui->chartRelativeWeigthWidget);
    chartViewRelativeWeigth->setRenderHint(QPainter::Antialiasing);
    // Must have as many colors as max no of elements + 1. See https://www.w3.org/TR/SVG11/types.html#ColorKeywords
    colorsRelativeWeigth = {QColor("cyan"), QColor("magenta"), QColor("red"), QColor("lightpink"), QColor("darkRed"),
              QColor("darkCyan"), QColor("darkMagenta"), QColor("green"), QColor("darkGreen"), QColor("yellow"),
              QColor("azure"), QColor("blueviolet"), QColor("chocolate"), QColor("lightgrey"), QColor("gold"),
              QColor("lightcoral"), QColor("firebrick"), QColor("dimgray"), QColor("darksalmon"), QColor("darkturquoise"),
              QColor("darkolivegreen"), QColor("crimson"), QColor("blueviolet"), QColor("bisque"), QColor("orchid"),
              QColor("palegreen")};
    if(GbpController::getInstance().getChartDarkMode()==true){
        chartRelativeWeigth->setTheme(QChart::ChartThemeDark);
    } else {
        chartRelativeWeigth->setTheme(QChart::ChartThemeLight);
    }
    ui->tabWidget->setCurrentIndex(0);  // make sure Relative Weight Pie chart is shown first
    // other settings
    ui->titleLabel->setText(tr("Relative Weight of Incomes For That Period"));
    ui->noElementsLabel->setText(tr("No of most significant incomes to use :"));
    ui->fromDateEdit->setDate(GbpController::getInstance().getTomorrow());
    ui->toDateEdit->setDate(GbpController::getInstance().getTomorrow().addYears(1).addDays(-1));
    ui->noElementsSpinBox->setValue(5);
    ui->chartRelativeWeigthWidget->installEventFilter(this);
    // widen Date widget
    QFontMetrics fm = ui->fromDateEdit->fontMetrics();
    ui->fromDateEdit->setMinimumWidth(fm.averageCharWidth()*20);
    ui->toDateEdit->setMinimumWidth(fm.averageCharWidth()*20);

    // *** MONTHLY REPORT - CHART ***
    initReportChart(ReportType::MONTHLY);
    // other settings
    fillMonthlyReportComboBoxWithMonthNames();
    ui->monthlyReportChartFromMonthComboBox->setCurrentIndex(GbpController::getInstance().getTomorrow().month()-1);
    ui->monthlyReportChartFromYearSpinBox->setValue(GbpController::getInstance().getTomorrow().year());
    ui->monthlyReportChartDurationSpinBox->setValue(12);
    // make smaller selected bar info
    QFont font = ui->monthlyReportChartSelectedLabel->font();
    font.setPointSize(Util::changeFontSize(false, true, font.pointSize()));
    ui->monthlyReportChartSelectedTextLabel->setFont(font);
    ui->monthlyReportChartSelectedLabel->setFont(font);

    // *** Yearly REPORT - CHART ***
    initReportChart(ReportType::YEARLY);
    // other settings
    ui->yearlyReportChartFromYearSpinBox->setValue(GbpController::getInstance().getTomorrow().year());
    ui->yearlyReportChartDurationSpinBox->setValue(10);
    // make smaller selected bar info
    ui->yearlyReportChartSelectedTextLabel->setFont(font);
    ui->yearlyReportChartSelectedLabel->setFont(font);

    // *** MONTHLY REPORT - TABLE CONTROLS ***
    ui->monthlyReportTableWidget->setColumnCount(4);
    ui->monthlyReportTableWidget->setHorizontalHeaderLabels({tr("Month"),tr("Incomes"),tr("Expenses"),tr("Delta")});
    ui->monthlyReportTableWidget->setSortingEnabled(false);
    ui->monthlyReportTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);   // no edition
    ui->monthlyReportTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);// force equal with of columns
    ui->monthlyReportTableWidget->verticalHeader()->setVisible(true);
    ui->monthlyPresentValuesCheckBox->setChecked(false);
    ui->monthlyDiscountRateDoubleSpinBox->setValue(5);
    ui->monthlyDiscountRateDoubleSpinBox->setEnabled(false);

    // *** YEARLY REPORT - TABLE CONTROLS ***
    ui->yearlyReportTableWidget->setColumnCount(4);
    ui->yearlyReportTableWidget->setHorizontalHeaderLabels({tr("Year"),tr("Incomes"),tr("Expenses"),tr("Delta")});
    ui->yearlyReportTableWidget->setSortingEnabled(false);
    ui->yearlyReportTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);   // no edition
    ui->yearlyReportTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);// force equal with of columns
    ui->yearlyReportTableWidget->verticalHeader()->setVisible(true);
    ui->yearlyPresentValuesCheckBox->setChecked(false);
    ui->yearlyDiscountRateDoubleSpinBox->setValue(5);
    ui->yearlyDiscountRateDoubleSpinBox->setEnabled(false);;

    // Child Dialogs
    exportChartDlg = new ExportChartDialog(this); //  auto destroyed by Qt
    exportChartDlg->setModal(true);

    // connections
    QObject::connect(this, &AnalysisDialog::signalExportChartPrepareContent, exportChartDlg, &ExportChartDialog::slotPrepareContent);
    QObject::connect(exportChartDlg, &ExportChartDialog::signalExportChartResult, this, &AnalysisDialog::slotExportChartResult );
    QObject::connect(exportChartDlg, &ExportChartDialog::signalExportChartCompleted, this, &AnalysisDialog::slotExportChartCompleted );

    ready = true;
}


AnalysisDialog::~AnalysisDialog()
{
    delete ui;
}


void AnalysisDialog::slotAnalysisPrepareContent(QMap<QDate,CombinedFeStreams::DailyInfo> chartRawData, CurrencyInfo currencyInfo)
{
    this->chartRawData = chartRawData;
    this->currInfo = currencyInfo;

    // *** RELATIVE WEIGTH ***
    updateRelativeWeightChart();

    // *** MONTHLY AND YEARLY REPORTS ***
    // calculate data
    recalculate_MonthlyYearlyReportData(MONTHLY, ui->monthlyDiscountRateDoubleSpinBox->value(), ui->monthlyReportTableWidget);
    recalculate_MonthlyYearlyReportData(YEARLY, ui->yearlyDiscountRateDoubleSpinBox->value(), ui->yearlyReportTableWidget);
    // update report tables accordingly
    redisplay_MonthlyYearlyReportTableData(MONTHLY, ui->monthlyPresentValuesCheckBox->isChecked(), ui->monthlyReportTableWidget);
    redisplay_MonthlyYearlyReportTableData(YEARLY, ui->yearlyPresentValuesCheckBox->isChecked(), ui->yearlyReportTableWidget);
    // update monthly and yearly charts
    redisplay_ReportChart(ReportType::MONTHLY, false);
    redisplay_ReportChart(ReportType::YEARLY, false);

    // misc suff
    ui->monthlyReportChartSelectedTextLabel->setText("");

    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Analysis Dialog invoked"));
}



void AnalysisDialog::slotExportChartResult(bool success)
{

}


void AnalysisDialog::slotExportChartCompleted()
{

}


void AnalysisDialog::themeChanged()
{
    if(GbpController::getInstance().getChartDarkMode()==true){
        chartMonthlyReport->setTheme(QChart::ChartThemeDark);
        chartYearlyReport->setTheme(QChart::ChartThemeDark);
        chartRelativeWeigth->setTheme(QChart::ChartThemeDark);
    } else {
        chartMonthlyReport->setTheme(QChart::ChartThemeLight);
        chartYearlyReport->setTheme(QChart::ChartThemeLight);
        chartRelativeWeigth->setTheme(QChart::ChartThemeLight);
    }
}


bool AnalysisDialog::eventFilter(QObject *object, QEvent *event)
{
    if ( (event->type() == QEvent::Resize) && (object == ui->chartRelativeWeigthWidget) ){
        chartViewRelativeWeigth->resize(ui->chartRelativeWeigthWidget->size());
    }
    if ( (event->type() == QEvent::Resize) && (object == ui->monthlyReportChartWidget) ){
        chartViewMonthlyReport->resize(ui->monthlyReportChartWidget->size());
    }
    if ( (event->type() == QEvent::Resize) && (object == ui->yearlyReportChartWidget)){
        chartViewYearlyReport->resize(ui->yearlyReportChartWidget->size());
    }
    return QObject::eventFilter(object, event);
}


// completely recalculate and redisplay Pie (relative weight)
void AnalysisDialog::updateRelativeWeightChart()
{
    if (!ready){
        return;
    }

    // build pie data
    int noOfElements= ui->noElementsSpinBox->value();
    bool isIncome = ui->incomesRelativeWeigthRadioButton->isChecked();
    QDate from = ui->fromDateEdit->date();
    QDate to = ui->toDateEdit->date();
    QDate tomorrow = GbpController::getInstance().getTomorrow();

    if(!from.isValid()){
        QMessageBox::critical(nullptr,tr("Invalid Dates"),tr("From Date is invalid"));
        return;
    } else if (!to.isValid()) {
        QMessageBox::critical(nullptr,tr("Invalid Dates"),tr("To Date is invalid"));
        return;
    } else if (to<from){
        QString fromString = from.toString(Qt::ISODate);
        QString toString = to.toString(Qt::ISODate);
        QString s = QString(tr("\"To\" Date %1 cannot occur before \"From\" Date %2")).arg(toString).arg(fromString);
        QMessageBox::critical(nullptr,"Invalid Dates",s.toLocal8Bit().data());
        return;
    } else if (from<tomorrow){
        QString fromString = from.toString(Qt::ISODate);
        QString tomorrowString = tomorrow.toString(Qt::ISODate);
        QString s = QString(tr("\"From\" Date %1 cannot be smaller than \"tomorrow\" %2")).arg(fromString).arg(tomorrowString);
        QMessageBox::critical(nullptr,tr("Invalid Dates"),s.toLocal8Bit().data());
        return;
    }

    // *** step 1 : count individual contribution of each StreamDef ***
    QHash<QUuid,double> bins = {}; // no need to sort keys. key = Stream Def UUID, value = total amount contributed by the StreamDef in the period. Positive for Income, negative for expense
    double d;
    double minDouble = std::numeric_limits<double>::min();
    double maxDouble = std::numeric_limits<double>::max();
    double grandTotal=0;
    foreach(QDate date, chartRawData.keys()){
        if( (date<from) || (date>to) ){
            continue;   // not in the interval
        }
        CombinedFeStreams::DailyInfo di = chartRawData.value(date);
        if(isIncome){
            for( int i=0; i<di.incomesList.count(); ++i ){
                FeDisplay fed = di.incomesList.at(i);
                grandTotal += fed.amount;
                d = bins.value(fed.id,minDouble);
                if(d != minDouble){
                    d += fed.amount; // existing entry
                } else {
                    d = fed.amount;
                }
                bins.insert(fed.id, d); // replace current value

            }
        } else {
            for( int i=0; i<di.expensesList.count(); ++i ){
                FeDisplay fed = di.expensesList.at(i);
                grandTotal += fed.amount;
                d = bins.value(fed.id,maxDouble);  // d is negative number
                if(d != maxDouble){
                    d += fed.amount;  // existing entry
                } else {
                    d = fed.amount;
                }
                bins.insert(fed.id, d);
            }
        }
    }

    // *** step 2 : sort by amount (biggest to smallest). Make positive the neg ative no of Expenses ***
    QList<Pair> tempList;
    foreach(QUuid id, bins.keys()){
        double d = fabs(bins.value(id));
        Pair p = {.amount = d, .percentage= (100*d/grandTotal), .id = id};
        tempList.append(p);
    }
    std::sort(tempList.begin(), tempList.end(), [](const Pair& p1, const Pair& p2) {
        return (p1.amount > p2.amount);
    });

    // *** step 3 : shrink to "n" elements + 1 "others" when required ***
    int noElements = ui->noElementsSpinBox->value(); // 1 is minimum
    if( tempList.size() > noElements ){
        // rejected elements are regrouped in a new single element "others", with null QUuid
        int noRejected = tempList.size() - noElements;
        double cumulPercentageRejected = 0;
        double cumulAmountRejected = 0;
        for(int i=0; i<noRejected;i++){
            cumulPercentageRejected += tempList.at(noElements+i).percentage;
            cumulAmountRejected += tempList.at(noElements+i).amount;
        }
        tempList.remove(noElements,noRejected);
        QUuid nullId = QUuid::fromString(QStringView()); // this will be a null QUuid because it is not valid
        Pair othersPair = {.amount = cumulAmountRejected, .percentage = cumulPercentageRejected,.id = nullId };
        tempList.append(othersPair);

        // re-sort new TempList because "others" may have been created ****
        std::sort(tempList.begin(), tempList.end(), [](const Pair& p1, const Pair& p2) {
            return (p1.amount > p2.amount);
        });
    }

    // *** step 4 : transform into pie data ***
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario();
    chartRelativeWeigth->removeAllSeries();
    seriesRelativeWeigth = new QPieSeries;
    bool found;
    QList<QString> originalSliceNames;
    for(int i=0;i<tempList.size();i++){
        Pair p = tempList.at(i);
        if (p.id.isNull()==true){
            // this is the "others" element
            seriesRelativeWeigth->append(tr("Others"), fabs(p.amount));
            originalSliceNames.append(tr("Others"));
        } else{
            QString name = scenario->getStreamDefNameFromId(p.id, found); // will always be found
            originalSliceNames.append(name);
            seriesRelativeWeigth->append(name, fabs(p.amount));
        }

    }

    // *** step 5 : labels for slices ***
    seriesRelativeWeigth->setLabelsVisible(true);
    int c=0;
    for(auto slice : seriesRelativeWeigth->slices()){
        slice->setLabel(QString("#%1: %2%").arg(c+1).arg(100*slice->percentage(), 0, 'f', 1));
        slice->setLabelArmLengthFactor(0.25);
        slice->setBrush(colorsRelativeWeigth[c]);
        c++;
    }
    chartRelativeWeigth->addSeries(seriesRelativeWeigth);

    // *** step 6 : overwrite legend items ***
    c = 0;
    QList<QLegendMarker *> mList = chartRelativeWeigth->legend()->markers();
    for(auto legendMarker : mList){
        QString amountString = CurrencyHelper::formatAmount(tempList.at(c).amount,currInfo, locale, true);
        QString s = QString("#%1: %2 (%3)").arg(c+1).arg(Util::elideText(originalSliceNames.at(c),30,true)).arg(amountString);
        legendMarker->setLabel(s);
        c++;
    }

}


// annualDiscountRate is in percentage
void AnalysisDialog::recalculate_MonthlyYearlyReportData(ReportType rTypr, double annualDiscountRate, QTableWidget* tableWidget)
{
    if(rTypr==MONTHLY){
        binsMonthly = {};
        binsMonthlyPresentValues = {};
    } else {
        binsYearly = {};
        binsYearlyPresentValues = {};
    }

    if (chartRawData.size()==0){
        return;  // nothing to do after
    }

    QDate from = chartRawData.firstKey();
    QDate to = chartRawData.lastKey();
    QDate toMonthYear = QDate(to.year(), (rTypr==MONTHLY)?(to.month()):(1), 1);
    QDate date = QDate(from.year(), (rTypr==MONTHLY)?(from.month()):(1), 1);
    while( date <= toMonthYear ){
        if(rTypr==MONTHLY){
            binsMonthly.insert(date,{.income=0, .expense=0, .delta=0});
            date = date.addMonths(1);
        } else {
            binsYearly.insert(date,{.income=0, .expense=0, .delta=0});
            date = date.addYears(1);
        }
    }

    // fill FUTURE values bins
    MonthlyYearlyReport binData;
    QDate binDate;
    if (rTypr==MONTHLY) {
        foreach(QDate date, chartRawData.keys()){
            CombinedFeStreams::DailyInfo di =  chartRawData.value(date);
            binDate = QDate(date.year(), date.month(), 1);
            if ( true==binsMonthly.contains(binDate) ){
                MonthlyYearlyReport binData = binsMonthly.value(binDate);
                binData.income += di.totalIncomes;
                binData.expense += fabs(di.totalExpenses);
                binData.delta += di.totalDelta;
                binsMonthly.insert(binDate, binData);
            } else {
                MonthlyYearlyReport binData;
                binData.income = di.totalIncomes;
                binData.expense = fabs(di.totalExpenses);
                binData.delta = di.totalDelta;
                binsMonthly.insert(binDate, binData);
            }
        }
    } else {
        foreach(QDate date, chartRawData.keys()){
            CombinedFeStreams::DailyInfo di =  chartRawData.value(date);
            binDate = QDate(date.year(), 1, 1);
            if ( true==binsYearly.contains(binDate) ){
                MonthlyYearlyReport binData = binsYearly.value(binDate);
                binData.income += di.totalIncomes;
                binData.expense += fabs(di.totalExpenses);
                binData.delta += di.totalDelta;
                binsYearly.insert(binDate, binData);
            } else {
                MonthlyYearlyReport binData;
                binData.income = di.totalIncomes;
                binData.expense = fabs(di.totalExpenses);
                binData.delta = di.totalDelta;
                binsYearly.insert(binDate, binData);
            }
        }
    }

    // calculate PRESENT value bins
    QDate tomorrow = GbpController::getInstance().getTomorrow();
    if (rTypr==MONTHLY) {
        tomorrow = QDate(tomorrow.year(), tomorrow.month(), 1); // fist bin
        double monthlyDiscountRate = Util::annualToMonthlyGrowth(annualDiscountRate);
        foreach(QDate date, binsMonthly.keys()){
            MonthlyYearlyReport mr = binsMonthly.value(date);
            uint period = noOfMonthDifference(tomorrow, date);
            mr.income = Util::presentValue(mr.income, monthlyDiscountRate, period);
            mr.expense = Util::presentValue(mr.expense, monthlyDiscountRate, period);
            mr.delta = Util::presentValue(mr.delta, monthlyDiscountRate, period);
            binsMonthlyPresentValues.insert(date, mr);
        }
    } else {
        tomorrow = QDate(tomorrow.year(), 1, 1); // fist bin
        foreach(QDate date, binsYearly.keys()){
            MonthlyYearlyReport mr = binsYearly.value(date);
            uint period = noOfYearDifference(tomorrow, date);
            mr.income = Util::presentValue(mr.income, annualDiscountRate, period);
            mr.expense = Util::presentValue(mr.expense, annualDiscountRate, period);
            mr.delta = Util::presentValue(mr.delta, annualDiscountRate, period);
            binsYearlyPresentValues.insert(date, mr);
        }
    }

}


// use already calculated bins to update table content
void AnalysisDialog::redisplay_MonthlyYearlyReportTableData(ReportType rTypr, bool usePresentValues, QTableWidget* tableWidget)
{
    if (chartRawData.size()==0){
        tableWidget->clearContents();
        tableWidget->setRowCount(0);
        return;  // nothing to do after
    }

    // choose the right bins amongst 4 possibilities
    QMap<QDate,MonthlyYearlyReport>* binsPtr;
    if (rTypr==MONTHLY) {
        if (usePresentValues) {
            binsPtr = &binsMonthlyPresentValues;
        } else {
            binsPtr = &binsMonthly;
        }
    } else {
        if (usePresentValues) {
            binsPtr = &binsYearlyPresentValues;
        } else {
            binsPtr = &binsYearly;
        }
    }

    // fill table
    tableWidget->clearContents();
    tableWidget->setRowCount(binsPtr->size()); // must be done BEFORE inserting item...
    int row = 0;
    QString s1,s2,s3,s4;
    QFont defaultFont = tableWidget->font();
    QFont monoTableFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoTableFont.setPointSize(defaultFont.pointSize());

    QBrush red = QBrush(QColor(210,0,0));
    QBrush green = QBrush(QColor(0,200,0));
    foreach(QDate date, binsPtr->keys()){
        MonthlyYearlyReport mr = binsPtr->value(date);
        // build items
        if (rTypr==MONTHLY) {
            s1 = locale.toString(date,"yyyy MMMM");
        } else {
            s1 = locale.toString(date,"yyyy");
        }
        s2 = CurrencyHelper::formatAmount(mr.income,currInfo,locale,false);
        s3 = CurrencyHelper::formatAmount(mr.expense,currInfo,locale,false);
        s4 = CurrencyHelper::formatAmount(mr.delta,currInfo,locale,false);
        QTableWidgetItem* wi1 = new QTableWidgetItem(s1);
        QTableWidgetItem* wi2 = new QTableWidgetItem(s2);
        QTableWidgetItem* wi3 = new QTableWidgetItem(s3);
        QTableWidgetItem* wi4 = new QTableWidgetItem(s4);
        wi1->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        wi2->setFont(monoTableFont);
        wi2->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        wi3->setFont(monoTableFont);
        wi3->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        wi4->setFont(monoTableFont);
        wi4->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if(mr.delta<0){
            wi4->setForeground(red);
        } else {
            wi4->setForeground(green);
        }
        // add to intrinsic table model
        tableWidget->setItem(row,0,wi1);
        tableWidget->setItem(row,1,wi2);
        tableWidget->setItem(row,2,wi3);
        tableWidget->setItem(row,3,wi4);
        row++;
    }

}


// Using already calculated data, completely rebuid the Report Chart (eigher Monthly or Yearly)
void AnalysisDialog::redisplay_ReportChart(ReportType type, bool usePresentValues)
{
    // Set pointers to appropriate entities depending on the value of "type"
    QChart** chartPtr;
    QChartView** chartViewPtr = nullptr;
    QBarCategoryAxis** xAxisPtr;
    QValueAxis** yAxisPtr;
    if (type == ReportType::MONTHLY) {
        chartPtr = &chartMonthlyReport;
        chartViewPtr = &chartViewMonthlyReport;
        xAxisPtr = &chartMonthlyReportAxisX;
        yAxisPtr = &chartMonthlyReportAxisY;
    } else {
        chartPtr = &chartYearlyReport;
        chartViewPtr = &chartViewYearlyReport;
        xAxisPtr = &chartYearlyReportAxisX;
        yAxisPtr = &chartYearlyReportAxisY;
    }

    if (chartRawData.size()==0){
        (*chartPtr)->removeAllSeries();
        QStringList empty = QStringList();
        empty.append("No data");
        (*xAxisPtr)->setCategories(empty);
        (*yAxisPtr)->setRange(0,1);
        return;  // nothing to do after
    }

    // determine which set is to be used
    bool useIncomes;
    bool useExpenses;
    bool useDeltas;
    findWhichSetsIsToBeUsedReportChart(type, useIncomes, useExpenses, useDeltas);

    // choose the right bins amongst 4 possibilities
    QMap<QDate,MonthlyYearlyReport>* binsPtr;
    if (type==ReportType::MONTHLY) {
        if (usePresentValues) {
            binsPtr = &binsMonthlyPresentValues;
        } else {
            binsPtr = &binsMonthly;
        }
    } else {
        if (usePresentValues) {
            binsPtr = &binsYearlyPresentValues;
        } else {
            binsPtr = &binsYearly;
        }
    }

    // build new set of chart data (3 sets of QBarSet) and categories
    double maxY = 1;
    double minY = 0;
    QStringList categories;
    QBarSet *setIncomes = new QBarSet(tr("Incomes"));
    setIncomes->setColor(QColor::fromRgb(0,200,0));
    QBarSet *setExpenses = new QBarSet(tr("Expenses"));
    setExpenses->setColor(QColor::fromRgb(200,0,0));
    QBarSet *setDeltasPositive = new QBarSet(tr("Deltas - Surplus"));
    setDeltasPositive->setColor(QColor::fromRgb(0,170,0));
    QBarSet *setDeltasNegative = new QBarSet(tr("Deltas - Deficit"));
    setDeltasNegative->setColor(QColor::fromRgb(170,0,0));
    QDate startDate, endDate ;
    getStartEndDateReportChart(type, startDate, endDate);
    QDate date= startDate;
    while(date<=endDate){
        MonthlyYearlyReport mr = {.income=0, .expense=0, .delta=0};
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
        categories.append(buildBarChartCategoryName(type,date));
        // go to next date
        if (type==ReportType::MONTHLY) {
            date = date.addMonths(1);
        } else {
            date = date.addYears(1);
        }
    }

    // find min-max in all the set
    getMinMaxReportChart(type, setIncomes, setExpenses, setDeltasPositive, setDeltasNegative, minY, maxY);

    // deallocate all the stuff currently in use
    (*chartPtr)->removeAllSeries(); // deallocate all the current stuff, EXCLUDING both axis

    // completely rebuild the series, discard set not to be used
    QBarSeries* series = new QBarSeries();
    if( useIncomes ){
        series->append(setIncomes); // take ownership
    } else{
        delete setIncomes;
    }
    if( useExpenses ){
        series->append(setExpenses); // take ownership
    } else{
        delete setExpenses;
    }
    if( useDeltas ){
        series->append(setDeltasPositive); // take ownership
        series->append(setDeltasNegative); // take ownership
    } else{
        delete setDeltasPositive;
        delete setDeltasNegative;
    }
    (*chartPtr)->addSeries(series); // take ownership
    // connect a "lambda" to detect and process bar selection
    if (type==ReportType::MONTHLY) {
        QObject::connect(series, &QAbstractBarSeries::clicked, series, [=](int index, QBarSet *set) {
            QStringList cats = chartMonthlyReportAxisX->categories();
            set->deselectAllBars();
            bool ok;
            QStringList catStringList = cats.at(index).split("-");
            int month = catStringList.at(0).toInt(&ok);
            if (!ok) {
                return ; // should never happen
            }
            QString finaleCatString = QString("%1 %2").arg(locale.monthName(month, QLocale::FormatType::ShortFormat)).arg(catStringList.at(1));
            QString text = QString("%1 / %2 / %3").arg(set->label()).arg(finaleCatString).arg(CurrencyHelper::formatAmount(set->at(index),currInfo, locale, true));
            ui->monthlyReportChartSelectedTextLabel->setText(text);
        });
    } else {
        QObject::connect(series, &QAbstractBarSeries::clicked, series, [=](int index, QBarSet *set) {
            QStringList cats = chartYearlyReportAxisX->categories();
            set->deselectAllBars();
            bool ok;
            QString cat = cats.at(index);
            QString text = QString("%1 / %2 / %3").arg(set->label()).arg(cat).arg(CurrencyHelper::formatAmount(set->at(index),currInfo, locale, true));
            ui->yearlyReportChartSelectedTextLabel->setText(text);
        });
    }


    // rebuild the 2 axes and attach
    (*xAxisPtr)->setCategories(categories);
    series->attachAxis((*xAxisPtr));
    (*yAxisPtr)->setRange(minY,maxY);
    series->attachAxis((*yAxisPtr));
}


// Calculate the no of months covered by [from,to].
// 2 dates inside the same month produce a result of 0.
uint AnalysisDialog::noOfMonthDifference(const QDate &from, const QDate &to) const
{
    if (from.isValid()==false){
        throw std::invalid_argument("from is an invalid date");
    }
    if (to.isValid()==false){
        throw std::invalid_argument("to is an invalid date");
    }
    if(to<from){
        throw std::invalid_argument("to is before from");
    }
    return ( (12*to.year())+to.month()) - ( (12*from.year())+from.month()) ;
}


uint AnalysisDialog::noOfYearDifference(const QDate &from, const QDate &to) const
{
    if (from.isValid()==false){
        throw std::invalid_argument("from is an invalid date");
    }
    if (to.isValid()==false){
        throw std::invalid_argument("to is an invalid date");
    }
    if(to<from){
        throw std::invalid_argument("to is before from");
    }
    return ( to.year() - from.year() ) ;
}



void AnalysisDialog::on_incomesRelativeWeigthRadioButton_toggled(bool checked)
{
    if(ui->incomesRelativeWeigthRadioButton->isChecked()==true){
        ui->titleLabel->setText(tr("Relative Weight of Incomes For That Period"));
        ui->noElementsLabel->setText(tr("No of most significant incomes to use :"));
    } else {
        ui->titleLabel->setText(tr("Relative Weight of Expenses For That Period"));
        ui->noElementsLabel->setText(tr("No of most significant expenses to use :"));
    }
    updateRelativeWeightChart();
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("Income radiobutton changed to %1").arg(checked));
}




void AnalysisDialog::on_closePushButton_clicked()
{
    hide();
    chartRawData = {};  // it was just a reference
    binsMonthly = {};   // now useless
    binsYearly = {};    // now useless
    binsMonthlyPresentValues = {};   // now useless
    binsYearlyPresentValues = {};    // now useless

    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Analysis Dialog closed"));
}


void AnalysisDialog::on_AnalysisDialog_rejected()
{
    on_closePushButton_clicked();
}



void AnalysisDialog::on_noElementsSpinBox_valueChanged(int arg1)
{
    updateRelativeWeightChart();
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("No of elements changed to %1").arg(arg1));

}


void AnalysisDialog::on_fromDateEdit_userDateChanged(const QDate &date)
{
    updateRelativeWeightChart();
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("From date changed to %1").arg(date.toString()));
}


void AnalysisDialog::on_toDateEdit_userDateChanged(const QDate &date)
{
    updateRelativeWeightChart();
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("To date changed to %1").arg(date.toString()));
}


void AnalysisDialog::on_exportImageRelativeWeigthPushButton_clicked()
{
    emit signalExportChartPrepareContent(ui->chartRelativeWeigthWidget);
    exportChartDlg->show();
}


void AnalysisDialog::on_exportMonthlyReportToTextPushButton_clicked()
{
     exportTextMonthlyYearlyReport(ReportType::MONTHLY);
}


void AnalysisDialog::on_exportYearlyReportToTextPushButton_clicked()
{
    exportTextMonthlyYearlyReport(ReportType::YEARLY);

}


void AnalysisDialog::exportTextMonthlyYearlyReport(ReportType rType) {
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Initiating Report type \"%1\" export").arg(rType));

    QString defaultExtension = ".csv";
    QString defaultExtensionUsed = ".csv";
    QString filter = tr("Text Files (*.txt *.TXT *.csv *.CSV)");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Select a File"), GbpController::getInstance().getLastDir(), filter, &defaultExtensionUsed);
    if (fileName != ""){
        // fix the filename to add the proper suffix
        QFileInfo fi(fileName);
        if(fi.suffix()==""){    // user has not specified an extension
            fileName.append(defaultExtension);
        }

        QFile file(fileName);
        if (false == file.open(QFile::WriteOnly | QFile::Truncate)){
            QMessageBox::critical(nullptr,tr("Export Failed"),tr("Cannot open the file for saving"));
            GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("Yearly Report export failed : Cannot open file %1").arg(fileName));
            return;
        }

        QMap<QDate,MonthlyYearlyReport>* binsPtr;
        QString dateFormat;
        if (rType == MONTHLY) {
            dateFormat = "yyyy-MM";
            if (ui->monthlyPresentValuesCheckBox->isChecked()==true) {
                binsPtr = &binsMonthlyPresentValues;
            } else {
                binsPtr = &binsMonthly;
            }
        } else {
            dateFormat = "yyyy";
            if (ui->yearlyPresentValuesCheckBox->isChecked()==true) {
                binsPtr = &binsYearlyPresentValues;
            } else {
                binsPtr = &binsYearly;
            }
        }

        QString inc ;
        QString exp ;
        QString total ;
        QString s;

        // write header
        s = QString("%1\t%2\t%3\t%4\n").arg(tr("Period"),tr("Total Incomes"),tr("Total Expenses"),tr("Delta"));
        file.write(s.toUtf8());

        // write data
        foreach(QDate date, binsPtr->keys()){
            MonthlyYearlyReport item = binsPtr->value(date);
            QString dateString = locale.toString(date,dateFormat);
            if (GbpController::getInstance().getExportTextAmountLocalized()) {
                // Localized
                inc = CurrencyHelper::formatAmount(item.income, currInfo, locale, false);
                exp = CurrencyHelper::formatAmount(item.expense, currInfo, locale, false);
                total = CurrencyHelper::formatAmount(item.delta, currInfo, locale, false);
            } else {
                // not localized
                inc = QString::number(item.income,'f', currInfo.noOfDecimal);
                exp = QString::number(item.expense,'f', currInfo.noOfDecimal);
                total = QString::number(item.delta,'f', currInfo.noOfDecimal);
            }

            s = QString("%1\t%2\t%3\t%4\n").arg(dateString,inc,exp,total);
            file.write(s.toUtf8());
        }
        file.close();
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "Success of Yearly Report export");
    } else{
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "Yearly Report export canceled");
    }
}


void AnalysisDialog::fillMonthlyReportComboBoxWithMonthNames() const
{
    for(int i=1;i<=12;i++){
        ui->monthlyReportChartFromMonthComboBox->addItem(locale.monthName(i),i);
    }
}


// Must be as small as possible
QString AnalysisDialog::buildBarChartCategoryName(ReportType type, QDate date) const
{
    if (type == ReportType::MONTHLY) {
        int smallYear = date.year()%100;
        QString s = QString("%1-%2").arg(date.month()).arg(smallYear);
        return s;
    } else {
        QString s = QString("%1").arg(date.year());
        return s;
    }
}


// Init Chart widget for Monthly or Yearly report chart
void AnalysisDialog::initReportChart(ReportType type)
{
    QChart** chartPtr;
    QChartView** chartViewPtr = nullptr;
    QBarCategoryAxis** xAxisPtr;
    QValueAxis** yAxisPtr;
    QWidget** widgetPtr;
    QString chartTitle;
    if (type == ReportType::MONTHLY) {
        chartPtr = &chartMonthlyReport;
        chartViewPtr = &chartViewMonthlyReport;
        xAxisPtr = &chartMonthlyReportAxisX;
        yAxisPtr = &chartMonthlyReportAxisY;
        widgetPtr = &(ui->monthlyReportChartWidget);
        chartTitle = tr("Monthly Incomes and Expenses");
    } else {
        chartPtr = &chartYearlyReport;
        chartViewPtr = &chartViewYearlyReport;
        xAxisPtr = &chartYearlyReportAxisX;
        yAxisPtr = &chartYearlyReportAxisY;
        widgetPtr = &(ui->yearlyReportChartWidget);
        chartTitle = tr("Yearly Incomes and Expenses");
    }

    // Data (sets) will be discarded and rebuild whe the dialog is re-shown (Prepare slot)
    QBarSet *set0 = new QBarSet(tr("Incomes"));
    QBarSet *set1 = new QBarSet(tr("Expenses"));
    QBarSeries* series = new QBarSeries();
    series->append(set0);  // take ownership
    series->append(set1);  // take ownership

    (*chartPtr) = new QChart();
    (*chartPtr)->setLocalizeNumbers(true);
    (*chartPtr)->addSeries(series); // take ownership
    (*chartPtr)->setTitle(chartTitle);
    (*chartPtr)->setAnimationOptions(QChart::SeriesAnimations);
    (*chartViewPtr) = new QChartView((*chartPtr), (*widgetPtr)); // take ownership
    (*chartViewPtr)->setRenderHint(QPainter::Antialiasing);
    (*chartPtr)->legend()->show();
    (*chartPtr)->legend()->setAlignment(Qt::AlignBottom);
    QStringList categories={};
    (*xAxisPtr)  = new QBarCategoryAxis();
    (*xAxisPtr)->append(categories);
    (*chartPtr)->addAxis((*xAxisPtr), Qt::AlignBottom); // the CHART (not the series) takes ownership
    series->attachAxis((*xAxisPtr));
    (*yAxisPtr) = new QValueAxis();
    (*yAxisPtr)->setRange(0,1);
    (*yAxisPtr)->setTickCount(11); // 10 bins
    (*yAxisPtr)->setMinorTickCount(4); // 5 bins
    (*chartPtr)->addAxis((*yAxisPtr), Qt::AlignLeft); // the CHART (not the series) takes ownership
    series->attachAxis((*yAxisPtr));
    if(GbpController::getInstance().getChartDarkMode()==true){
        (*chartPtr)->setTheme(QChart::ChartThemeDark);
    } else {
        (*chartPtr)->setTheme(QChart::ChartThemeLight);
    }
    (*widgetPtr)->installEventFilter(this);
}


// Return full QDate for start end end period for the displayed bar chart, from the widgets content
void AnalysisDialog::getStartEndDateReportChart(ReportType type, QDate &startDate, QDate &endDate) const
{
    if (type==ReportType::YEARLY) {
        startDate = QDate(ui->yearlyReportChartFromYearSpinBox->value(),1,1);
        endDate = startDate.addYears(ui->yearlyReportChartDurationSpinBox->value()-1);
    } else {
        startDate = QDate(ui->monthlyReportChartFromYearSpinBox->value(),ui->monthlyReportChartFromMonthComboBox->currentIndex()+1,1);
        endDate = startDate.addMonths(ui->monthlyReportChartDurationSpinBox->value()-1);
    }
}


void AnalysisDialog::getMinMaxReportChart(ReportType type, QBarSet* incomes, QBarSet* expenses, QBarSet* deltasPositive, QBarSet* deltasNegative, double& minY, double& maxY)
{
    bool useIncomes;
    bool useExpenses;
    bool useDeltas;
    findWhichSetsIsToBeUsedReportChart(type, useIncomes, useExpenses, useDeltas);

    // default values : it works because we must have at least one element to search for in the sets
    minY = 0;
    maxY = 1;

    if( useIncomes ){
        // process incomes
        for(int i=0;i<incomes->count();i++){
            if ( incomes->at(i) > maxY ){
                maxY = incomes->at(i);
            }
        }
    }
    if( useExpenses ){
        // process expenses
        for(int i=0;i<expenses->count();i++){
            if ( expenses->at(i) > maxY ){
                maxY = expenses->at(i);
            }
        }
    }
    if( useDeltas ){
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


void AnalysisDialog::findWhichSetsIsToBeUsedReportChart(ReportType type, bool &incomesSet, bool &expensesSet, bool &deltasSet)
{
    if (type==ReportType::MONTHLY) {
        if( ui->monthlyReportChartIncomesRadioButton->isChecked() || ui->monthlyReportChartIncomesExpensesRadioButton->isChecked() ){
            incomesSet = true;;
        } else{
            incomesSet = false;
        }
        if( ui->monthlyReportChartExpensesRadioButton->isChecked() || ui->monthlyReportChartIncomesExpensesRadioButton->isChecked() ){
            expensesSet = true;
        } else{
            expensesSet = false;
        }
        if( ui->monthlyReportChartDeltasRadioButton->isChecked() ){
            deltasSet = true;
        } else{
            deltasSet = false;
        }
    } else {
        if( ui->yearlyReportChartIncomesRadioButton->isChecked() || ui->yearlyReportChartIncomesExpensesRadioButton->isChecked() ){
            incomesSet = true;;
        } else{
            incomesSet = false;
        }
        if( ui->yearlyReportChartExpensesRadioButton->isChecked() || ui->yearlyReportChartIncomesExpensesRadioButton->isChecked() ){
            expensesSet = true;
        } else{
            expensesSet = false;
        }
        if( ui->yearlyReportChartDeltasRadioButton->isChecked() ){
            deltasSet = true;
        } else{
            deltasSet = false;
        }
    }
}


void AnalysisDialog::on_yearlyPresentValuesCheckBox_clicked()
{
    if (ui->yearlyPresentValuesCheckBox->isChecked()==true) {
        ui->yearlyDiscountRateDoubleSpinBox->setEnabled(true);
    } else {
        ui->yearlyDiscountRateDoubleSpinBox->setEnabled(false);
    }

    // update data and redisplay
    recalculate_MonthlyYearlyReportData(ReportType::YEARLY, ui->yearlyDiscountRateDoubleSpinBox->value(), ui->yearlyReportTableWidget);
    redisplay_MonthlyYearlyReportTableData(ReportType::YEARLY, ui->yearlyPresentValuesCheckBox->isChecked(), ui->yearlyReportTableWidget);
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("Yearly Discount rate values checkbox changed to %1").arg(ui->yearlyPresentValuesCheckBox->isChecked()));
}



void AnalysisDialog::on_monthlyPresentValuesCheckBox_clicked()
{
    if (ui->monthlyPresentValuesCheckBox->isChecked()==true) {
        ui->monthlyDiscountRateDoubleSpinBox->setEnabled(true);
    } else {
        ui->monthlyDiscountRateDoubleSpinBox->setEnabled(false);
    }
    // update data and redisplay
    recalculate_MonthlyYearlyReportData(ReportType::MONTHLY, ui->monthlyDiscountRateDoubleSpinBox->value(), ui->monthlyReportTableWidget);
    redisplay_MonthlyYearlyReportTableData(ReportType::MONTHLY, ui->monthlyPresentValuesCheckBox->isChecked(), ui->monthlyReportTableWidget);
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("Monthly Discount rate values checkbox changed to %1").arg(ui->monthlyPresentValuesCheckBox->isChecked()));
}


void AnalysisDialog::on_yearlyDiscountRateDoubleSpinBox_valueChanged(double arg1)
{
    // update data and redisplay
    recalculate_MonthlyYearlyReportData(ReportType::YEARLY, ui->yearlyDiscountRateDoubleSpinBox->value(), ui->yearlyReportTableWidget);
    redisplay_MonthlyYearlyReportTableData(ReportType::YEARLY, ui->yearlyPresentValuesCheckBox->isChecked(), ui->yearlyReportTableWidget);
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("Yearly DiscountRate changed to %1").arg(arg1));
}


void AnalysisDialog::on_monthlyDiscountRateDoubleSpinBox_valueChanged(double arg1)
{
    // update data and redisplay
    recalculate_MonthlyYearlyReportData(ReportType::MONTHLY, ui->monthlyDiscountRateDoubleSpinBox->value(), ui->monthlyReportTableWidget);
    redisplay_MonthlyYearlyReportTableData(ReportType::MONTHLY, ui->monthlyPresentValuesCheckBox->isChecked(), ui->monthlyReportTableWidget);
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("Monthly DiscountRate changed to %1").arg(arg1));
}


void AnalysisDialog::on_monthlyReportChartDurationSpinBox_valueChanged(int arg1)
{
    redisplay_ReportChart(ReportType::MONTHLY, false);
}


void AnalysisDialog::on_monthlyReportChartRightToolButton_clicked()
{
    QDate start = QDate(ui->monthlyReportChartFromYearSpinBox->value(),ui->monthlyReportChartFromMonthComboBox->currentIndex()+1,1);
    start = start.addMonths(ui->monthlyReportChartDurationSpinBox->value());
    ui->monthlyReportChartFromYearSpinBox->setValue(start.year());
    ui->monthlyReportChartFromMonthComboBox->setCurrentIndex(start.month()-1);
    redisplay_ReportChart(ReportType::MONTHLY, false);
}


void AnalysisDialog::on_monthlyReportChartLeftToolButton_clicked()
{
    QDate start = QDate(ui->monthlyReportChartFromYearSpinBox->value(),ui->monthlyReportChartFromMonthComboBox->currentIndex()+1,1);
    start = start.addMonths(-ui->monthlyReportChartDurationSpinBox->value());
    ui->monthlyReportChartFromYearSpinBox->setValue(start.year());
    ui->monthlyReportChartFromMonthComboBox->setCurrentIndex(start.month()-1);
    redisplay_ReportChart(ReportType::MONTHLY, false);
}


void AnalysisDialog::on_monthlyReportChartFromMonthComboBox_currentIndexChanged(int index)
{
    redisplay_ReportChart(ReportType::MONTHLY, false);
}


void AnalysisDialog::on_monthlyReportChartFromYearSpinBox_valueChanged(int arg1)
{
    redisplay_ReportChart(ReportType::MONTHLY, false);
}



void AnalysisDialog::on_monthlyReportChartIncomesRadioButton_clicked()
{
    redisplay_ReportChart(ReportType::MONTHLY, false);
}


void AnalysisDialog::on_monthlyReportChartExpensesRadioButton_clicked()
{
    redisplay_ReportChart(ReportType::MONTHLY, false);
}


void AnalysisDialog::on_monthlyReportChartIncomesExpensesRadioButton_clicked()
{
    redisplay_ReportChart(ReportType::MONTHLY, false);
}


void AnalysisDialog::on_monthlyReportChartDeltasRadioButton_clicked()
{
    redisplay_ReportChart(ReportType::MONTHLY, false);
}


void AnalysisDialog::on_exportImageMonthlyReportChartPushButton_clicked()
{
    emit signalExportChartPrepareContent(ui->monthlyReportChartWidget);
    exportChartDlg->show();
}


void AnalysisDialog::on_yearlyReportChartFromYearSpinBox_valueChanged(int arg1)
{
    redisplay_ReportChart(ReportType::YEARLY, false);
}


void AnalysisDialog::on_yearlyReportChartDurationSpinBox_valueChanged(int arg1)
{
    redisplay_ReportChart(ReportType::YEARLY, false);
}


void AnalysisDialog::on_yearlyReportChartLeftToolButton_clicked()
{
    QDate start = QDate(ui->yearlyReportChartFromYearSpinBox->value(),1,1);
    start = start.addYears(-ui->yearlyReportChartDurationSpinBox->value());
    ui->yearlyReportChartFromYearSpinBox->setValue(start.year());
    redisplay_ReportChart(ReportType::YEARLY, false);
}


void AnalysisDialog::on_yearlyReportChartRightToolButton_clicked()
{
    QDate start = QDate(ui->yearlyReportChartFromYearSpinBox->value(),1,1);
    start = start.addYears(ui->yearlyReportChartDurationSpinBox->value());
    ui->yearlyReportChartFromYearSpinBox->setValue(start.year());
    redisplay_ReportChart(ReportType::YEARLY, false);
}


void AnalysisDialog::on_yearlyReportChartIncomesRadioButton_clicked()
{
    redisplay_ReportChart(ReportType::YEARLY, false);
}


void AnalysisDialog::on_yearlyReportChartExpensesRadioButton_clicked()
{
    redisplay_ReportChart(ReportType::YEARLY, false);
}


void AnalysisDialog::on_yearlyReportChartIncomesExpensesRadioButton_clicked()
{
    redisplay_ReportChart(ReportType::YEARLY, false);
}


void AnalysisDialog::on_yearlyReportChartDeltasRadioButton_clicked()
{
    redisplay_ReportChart(ReportType::YEARLY, false);
}


void AnalysisDialog::on_exportImageYearlyReportChartPushButton_clicked()
{
    emit signalExportChartPrepareContent(ui->yearlyReportChartWidget);
    exportChartDlg->show();
}

