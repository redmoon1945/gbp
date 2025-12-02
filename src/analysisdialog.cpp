/*
 *  Copyright (C) 2024-2025 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
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
#include "qgraphicslayout.h"
#include "ui_analysisdialog.h"
#include "gbpcontroller.h"
#include "gbplogger.h"
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


// --- For data attached to the legend list of Relative Weight ---
// Anonymous namespace means private to this .cpp file ---
namespace {
    struct LegendItemInfo {
        quint8 rank;        // position (1 = highest percentage)
        double amount;      // always positive
        double percentage;  // e.g. 4% => 4
        QUuid id;           // csd ID
    };
}
// so that it can be used as a QVariant, which is required by setData
Q_DECLARE_METATYPE(LegendItemInfo)



AnalysisDialog::AnalysisDialog(QLocale theLocale, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AnalysisDialog)
{
    ui->setupUi(this);
    locale = theLocale;

    QFont font ;

    // *** RELATIVE WEIGHT CONTROLS ***

    seriesRelativeWeigth = new QPieSeries;
    chartRelativeWeight = new QChart;

    // Set margin according to font
    // QFontMetrics fm(chartRelativeWeight->titleFont());   // or series->labelsFont()
    // int margin = fm.height() * 2;  // two lines worth of space
    // chartRelativeWeight->setMargins(QMargins(10, margin, 10, 10));

    chartRelativeWeight->addSeries(seriesRelativeWeigth); // take ownership
    chartRelativeWeight->setAnimationOptions(QChart::AllAnimations);
    chartRelativeWeight->legend()->hide();
    chartRelativeWeight->legend()->setAlignment(Qt::AlignRight);
    chartViewRelativeWeigth = new QChartView(chartRelativeWeight, ui->chartRelativeWeigthWidget);
    chartViewRelativeWeigth->setRenderHint(QPainter::Antialiasing);
    chartRelativeWeight->layout()->setContentsMargins(1, 1, 1, 1);
    chartRelativeWeight->setBackgroundRoundness(0);


    // Must have as many colors as max no of elements + 1., that is *** 26 ***
    // These colors are optimized.
    colorsRelativeWeigth = {
        QColor(228,26,28),    // red
        QColor(55,126,184),   // blue
        QColor(77,175,74),    // green
        QColor(152,78,163),   // purple
        QColor(255,127,0),    // orange
        QColor(255,255,51),   // yellow
        QColor(247,129,191),  // pink
        QColor(0,191,255),    // deep sky blue
        QColor(46,139,87),    // sea green
        QColor(255,69,0),     // red-orange
        QColor(138,43,226),   // blue violet
        QColor(60,179,113),   // medium sea green
        QColor(210,105,30),   // chocolate
        QColor(0,128,128),    // teal
        QColor(255,20,147),   // deep pink
        QColor(70,130,180),   // steel blue
        QColor(199,21,133),   // medium violet red
        QColor(218,165,32),   // goldenrod
        QColor(0,100,0),      // dark green
        QColor(123,104,238),  // medium slate blue
        QColor(139,69,19),    // saddle brown
        QColor(127,255,212),  // aquamarine
        QColor(244,164,96),   // sandy brown
        QColor(32,178,170),   // light sea green
        QColor(186,85,211),   // orchid
        QColor(0,206,209)     // dark turquoise
    };

    if(GbpController::getInstance().useDarkModeForChart()==true){
        chartRelativeWeight->setTheme(QChart::ChartThemeDark);
    } else {
        chartRelativeWeight->setTheme(QChart::ChartThemeLight);
    }

    // set current tab : make sure Relative Weight Pie chart is shown first
    ui->tabWidget->setCurrentIndex(0);
    ui->globalExportCsvPushButton->setVisible(true);
    ui->globalExportImagePushButton->setVisible(true);

    // Set initial from/to dates
    QDate tomorrow = GbpController::getInstance().getTomorrow();
    ui->fromDateEdit->setDate(tomorrow);
    rwPreviousFromDate = tomorrow;
    ui->toDateEdit->setDate(tomorrow.addYears(1).addDays(-1));
    rwPreviousToDate = tomorrow.addYears(1).addDays(-1);

    // misc init
    ui->noElementsLabel->setText(tr("No of most significant items :"));
    ui->noElementsSpinBox->setValue(10);
    ui->chartRelativeWeigthWidget->installEventFilter(this);

    // Widen Date widget
    QFontMetrics fm = ui->fromDateEdit->fontMetrics();
    ui->fromDateEdit->setMinimumWidth(fm.averageCharWidth()*20);
    ui->toDateEdit->setMinimumWidth(fm.averageCharWidth()*20);

    // Set color of "incomes" and "expenses" radio buttons
    ui->incomesRelativeWeigthRadioButton->setStyleSheet(Util::getStyleSheetStringForColor(
        GbpController::getInstance().getIncomeColor()));
    ui->expensesRelativeWeigthRadioButton->setStyleSheet(Util::getStyleSheetStringForColor(
        GbpController::getInstance().getExpenseColor()));

    // Set monospace font for list box and smaller font
    font = ui->RW_listWidget->font();
    QFont monoF = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoF.setPointSize(font.pointSize());
    Util::changeFontSize(monoF, Util::FontResizeIntensity::AVERAGE, true);
    ui->RW_listWidget->setFont(monoF);

    // Set the amount string to be able to display max number without streching the Dialog
    // At this time, we dont have access to a currency, so use the max amount of decimal.
    int numChars = CurrencyHelper::maxCharForMaxAmountInDouble(
        CurrencyHelper::maxValueAllowedForNoOfDecimalsForCurrency());
    QFontMetrics fmAmount(ui->rwAmountLabel->font());
    int widthAmount = fm.horizontalAdvance(QString(numChars*1.5, '8'));
    ui->rwAmountLabel->setMinimumWidth(widthAmount);

    // *** MONTHLY REPORT - CHART ***

    initReportChart(ReportType::MONTHLY);
    // other settings
    fillMonthlyReportComboBoxWithMonthNames();
    ui->monthlyReportChartFromMonthComboBox->setCurrentIndex(GbpController::getInstance()
        .getTomorrow().month()-1);
    ui->monthlyReportChartFromYearSpinBox->setValue(GbpController::getInstance().getTomorrow()
        .year());
    ui->monthlyReportChartDurationSpinBox->setValue(12);
    // make smaller selected bar info font
    font = ui->monthlyReportChartSelectedLabel->font();
    Util::changeFontSize(font, Util::FontResizeIntensity::AVERAGE, true);
    ui->monthlyReportChartSelectedTextLabel->setFont(font);
    ui->monthlyReportChartSelectedLabel->setFont(font);
    // Make stats areas using smaller font
    ui->monthlyReportStatsLabel->setFont(font);

    // *** Yearly REPORT - CHART ***

    initReportChart(ReportType::YEARLY);
    // other settings
    ui->yearlyReportChartFromYearSpinBox->setValue(GbpController::getInstance().getTomorrow()
        .year());
    ui->yearlyReportChartDurationSpinBox->setValue(10);
    // make smaller selected bar info
    font = ui->yearlyReportChartSelectedLabel->font();
    Util::changeFontSize(font, Util::FontResizeIntensity::AVERAGE, true);
    ui->yearlyReportChartSelectedTextLabel->setFont(font);
    ui->yearlyReportChartSelectedLabel->setFont(font);
    // Make stats areas using smaller font
    ui->annualReportStatsLabel->setFont(font);

    // *** MONTHLY REPORT - TABLE CONTROLS ***

    ui->monthlyReportTableWidget->setColumnCount(5);
    ui->monthlyReportTableWidget->setHorizontalHeaderLabels({tr("Month"),tr("Incomes"),
        tr("Expenses"),tr("Delta"),tr("Cash balance")});
    ui->monthlyReportTableWidget->setSortingEnabled(false);
    ui->monthlyReportTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers); // no edition
    ui->monthlyReportTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->monthlyReportTableWidget->verticalHeader()->setVisible(true);

    // *** YEARLY REPORT - TABLE CONTROLS ***

    ui->yearlyReportTableWidget->setColumnCount(5);
    ui->yearlyReportTableWidget->setHorizontalHeaderLabels({tr("Year"),tr("Incomes"),
        tr("Expenses"),tr("Delta"),tr("Cash balance")});
    ui->yearlyReportTableWidget->setSortingEnabled(false);
    ui->yearlyReportTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);  // no edition
    ui->yearlyReportTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->yearlyReportTableWidget->verticalHeader()->setVisible(true);

    // *** TAGS Table ***

    ui->tagsFromDateEdit->setDate(tomorrow);
    ui->tagsToDateEdit->setDate(tomorrow.addYears(1).addDays(-1));
    tagsPreviousFromDate = tomorrow;
    tagsPreviousToDate = tomorrow.addYears(1).addDays(-1);

    // Widen Date widgets
    fm = ui->tagsFromDateEdit->fontMetrics();
    ui->tagsFromDateEdit->setMinimumWidth(fm.averageCharWidth()*20);
    ui->tagsToDateEdit->setMinimumWidth(fm.averageCharWidth()*20);
    // set some variables
    availableTags.clear();
    selectedTags.clear();
    ui->tagsTableWidget->setColumnCount(3); // tag name, amount, weight in percentage
    QStringList tagTableHeaders ={tr("Tag's name"), tr("Total amount"), tr("Weight (%1)").arg("%")};
    QStringList tagTableHeadersTooltips = {"","",
        tr("For this period of time, percentage of total amount for Csds associated with this tag "
        "relative to the total amount for all Csds regardless of tags.")};
    for (int col = 0; col < 3; ++col) {
        QTableWidgetItem *headerItem = new QTableWidgetItem(tagTableHeaders[col]);
        if(col==2){
            headerItem->setToolTip(tagTableHeadersTooltips[col]);
        }
        ui->tagsTableWidget->setHorizontalHeaderItem(col, headerItem);
    }
    ui->tagsTableWidget->setSortingEnabled(true);

    // Set color of "incomes" and "expenses" radio buttons
    ui->tagsIncomesRadioButton->setStyleSheet(Util::getStyleSheetStringForColor(
        GbpController::getInstance().getIncomeColor()));
    ui->tagsExpensesRadioButton->setStyleSheet(Util::getStyleSheetStringForColor(
        GbpController::getInstance().getExpenseColor()));

    // resize column of tag table
    //ui->tagsTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    QHeaderView *header = ui->tagsTableWidget->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch); // Column 0: stretch
    QFontMetrics fm2 = ui->tagsTableWidget->fontMetrics();
    header->setSectionResizeMode(1, QHeaderView::Fixed);
    ui->tagsTableWidget->setColumnWidth(1, fm2.averageCharWidth()*20);
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->tagsTableWidget->setColumnWidth(2, fm2.averageCharWidth()*20);

    // Force tag table to be initially sorted by weigth (descending)
    ui->tagsTableWidget->sortItems(2, Qt::DescendingOrder);

    // Set tags no of label to have smaller fonts and italic
    font = ui->tags_noTagsSelectedLabel->font();
    Util::changeFontSize(font, Util::FontResizeIntensity::WEAK, true);
    font.setItalic(true);
    ui->tags_noTagsSelectedLabel->setFont(font);

    // Init total amount to 0
    ui->tags_totalAmountLabel->setText("");

    // Set focus on "Close" button
    ui->closePushButton->setDefault(true);

    // *** Dialog children and connections ***

    // Create Dialog for selecting tags
    selectTagsDlg = new ChooseTagsDialog(this);
    selectTagsDlg->setModal(true);

    // connect emitters & receivers for Dialogs : choose Tags
    QObject::connect(this, &AnalysisDialog::signalChooseTagsPrepareContent, selectTagsDlg,
        &ChooseTagsDialog::slotPrepareContent);
    QObject::connect(selectTagsDlg, &ChooseTagsDialog::signalResult, this,
        &AnalysisDialog::slotChooseTagsResult);
    QObject::connect(selectTagsDlg, &ChooseTagsDialog::signalCompleted, this,
        &::AnalysisDialog::slotChooseTagsCompleted);

    adjustSize();  // Pack the dialog to fit its contents


    ready = true;
}


AnalysisDialog::~AnalysisDialog()
{
    delete ui;
}


void AnalysisDialog::slotAnalysisPrepareContent( QWeakPointer<CombinedFeStreams> chartRawDataRef,
    Tags availabletags, CurrencyInfo currencyInfo, double startingAmount)
{
    this->chartRawDataRef = chartRawDataRef;
    this->currInfo = currencyInfo;
    this->startingAmount = startingAmount;

    // *** RELATIVE WEIGTH ***
    updateRelativeWeightChart();

    // *** MONTHLY AND YEARLY REPORTS ***
    // calculate data : will be used by tables and charts.
    recalculate_BinsData(ReportType::MONTHLY, ui->monthlyReportTableWidget);
    recalculate_BinsData(ReportType::YEARLY, ui->yearlyReportTableWidget);
    // update report tables accordingly
    redisplay_MonthlyYearlyReportTableData(ReportType::MONTHLY, ui->monthlyReportTableWidget);
    redisplay_MonthlyYearlyReportTableData(ReportType::YEARLY, ui->yearlyReportTableWidget);
    // update monthly and yearly charts
    redisplay_ReportChart(ReportType::MONTHLY);
    redisplay_ReportChart(ReportType::YEARLY);
    //
    ui->monthlyReportChartSelectedTextLabel->setText("");

    // *** Tags ***

    this->availableTags = availabletags;

    // Try to conserve the tags already selected from previous invocation of the dialog.
    // Scenario or tags set may have changed. Otherwise, we take all the tag available
    QSet<QUuid> formerSet = selectedTags.getFilterTagIdSet();
    availableTags.cleanIdList(formerSet);
    selectedTags.setFilterTagIdSet(formerSet);
    if (formerSet.size()==0) {
        // scenario may have changed or tags modified in the current scenario. In that case,
        // we select all the tags for convenience.
        selectedTags.setFilterTagIdSet(availableTags.getTagIdSetAsQset());
    }

    // Update no of tags selected
    tags_UpdateNoTagsSelected();

    // Rebuild data and update tag table
    redisplay_TagTable();

    // Set focus on Close button
    ui->closePushButton->setFocus();

    LOG_DEBUG_INFO(QString("Analysis dialog invoked"));
}


void AnalysisDialog::slotChooseTagsResult(QSet<QUuid> chosenTags)
{
    selectedTags.setFilterTagIdSet(chosenTags);

    // Update no of tags selected
    tags_UpdateNoTagsSelected();

    // Rebuild the data and update the tag chart
    redisplay_TagTable();
}


void AnalysisDialog::slotChooseTagsCompleted(bool canceled)
{

}


void AnalysisDialog::themeChanged()
{
    if(GbpController::getInstance().useDarkModeForChart()==true){
        chartMonthlyReport->setTheme(QChart::ChartThemeDark);
        chartYearlyReport->setTheme(QChart::ChartThemeDark);
        chartRelativeWeight->setTheme(QChart::ChartThemeDark);
    } else {
        chartMonthlyReport->setTheme(QChart::ChartThemeLight);
        chartYearlyReport->setTheme(QChart::ChartThemeLight);
        chartRelativeWeight->setTheme(QChart::ChartThemeLight);
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


void AnalysisDialog::updateRelativeWeightChart()
{
    if (!ready){
        return;
    }

    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario.isNull()){
        return; // if no scenario loaded (should not happen)
    }

    // Make sure the raw data is available (which should always be the case)
    QSharedPointer<CombinedFeStreams> chartRawData = chartRawDataRef.toStrongRef();
    if(chartRawData.isNull()){
        return; // should never happen
    }

    // Some Init and data def
    int noOfElements= ui->noElementsSpinBox->value();
    QDate from = ui->fromDateEdit->date();
    QDate to = ui->toDateEdit->date();
    QDate tomorrow = GbpController::getInstance().getTomorrow();
    RelWeightGrouping grouping = getGroupingTypeSelected();

    // Clear Legends widget
    clearLegendWidgets();

    // *** step 1 : Build the relative weight bins for the pie chart by counting individual
    // contributions of each CSD if grouping=INCOMES or EXPENSES or each tag if grouping=TAGS.
    // Results : key = CSD or tag UUID, value = total amount contributed
    // in the period (never negative)
    QHash<QUuid,double> bins = {}; // hold the temp results, to be sorted later
    double grandTotal=0;
    QList<CombinedFeStreams::DailyInfo> listDi = chartRawData->getCombinedStreams();
    const qsizetype size = listDi.size();
    for (int var = 0; var < size; ++var) {

        // +++ Pre-processing +++
        CombinedFeStreams::DailyInfo di = listDi[var];
        if(di.used == false){
            continue;
        }
        QDate date = tomorrow.addDays(var);

        // +++ processing +++

        if( (date<from) || (date>to) ){
            continue;   // not in the interval
        }
        if( (grouping==RelWeightGrouping::RWGROUPING_INCOMES) ||
            (grouping==RelWeightGrouping::RWGROUPING_EXPENSES)){
            // set a generic pointer of the list of CSD (either incomes or expenses)
            QList<Fe> *prtCsdList = ((grouping==RelWeightGrouping::RWGROUPING_INCOMES)
                ?(&(di.incomesList)):(&(di.expensesList)));
            // Iterate through the list of CSD of that occurrence day
            for( int i=0; i<prtCsdList->count(); ++i ){
                Fe fed = prtCsdList->at(i);
                QSharedPointer<Csd> csdRef = fed.csdPtr.toStrongRef();
                if (csdRef.isNull()) {
                    continue; // should never happen
                }
                grandTotal += fabs(fed.amount);
                double binValue = bins.value(csdRef->getId(),-1);
                if(binValue >= 0){// existing entry
                    binValue += fabs(fed.amount);
                } else { // new entry
                    binValue = fabs(fed.amount);
                }
                bins.insert(csdRef->getId(), binValue); // replace current value or create new one
            }
        } else{
            // Illegal grouping : should never happen
            throw std::logic_error("Invalid grouping value");
        }
    }

    // *** step 2 : sort the bins by amount (biggest to smallest).
    QList<LegendItemInfo> tempList; // result of the sort
    foreach(QUuid id, bins.keys()){
        double d = bins.value(id);
        // rank will be set later after the sort
        LegendItemInfo p = {.rank=0, .amount = d, .percentage= (100*d/grandTotal), .id = id};
        tempList.append(p);
    }
    std::sort(tempList.begin(), tempList.end(), [](const LegendItemInfo& p1, const LegendItemInfo& p2) {
        return (p1.amount > p2.amount);
    });
    for(int counter=0; counter<tempList.size(); counter++){
        tempList[counter].rank = counter+1;
    }

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
        QUuid nullId = QUuid::fromString(QStringView()); // will be null QUuid because not valid
        LegendItemInfo othersPair = {.amount = cumulAmountRejected,
            .percentage = cumulPercentageRejected, .id = nullId };
        tempList.append(othersPair);

        // re-sort new TempList because "others" may have been created ****
        std::sort(tempList.begin(), tempList.end(), [](const LegendItemInfo& p1,
            const LegendItemInfo& p2) {
            return (p1.amount > p2.amount);
        });
        for(int counter=0; counter<tempList.size(); counter++){
            tempList[counter].rank = counter+1;
        }
    }

    // *** step 4 : transform into pie data ***

    // recreate the pie
    chartRelativeWeight->removeAllSeries();
    seriesRelativeWeigth = new QPieSeries;

    // Adjust "margin so that we can see out labels even when big fonts. This is tricky...
    seriesRelativeWeigth->setPieSize(0.7); // smaller pie = more room for labels

    // Set rotation for Pie
    seriesRelativeWeigth->setPieStartAngle(ui->RW_horizontalSlider->value());
    seriesRelativeWeigth->setPieEndAngle(360+ui->RW_horizontalSlider->value());

    bool found;
    QList<QString> originalSliceNames;
    for(int i=0;i<tempList.size();i++){
        LegendItemInfo p = tempList.at(i);
        if (p.id.isNull()==true){
            // this is the "others" element
            seriesRelativeWeigth->append(tr("Others"), fabs(p.amount));
            originalSliceNames.append(tr("Others"));
        } else{
            // name is CSD name
            QString name;
            if ( (grouping==RelWeightGrouping::RWGROUPING_INCOMES) ||
                (grouping==RelWeightGrouping::RWGROUPING_EXPENSES) ) {
                QColor color;
                scenario->getCsdNameAndColorFromId(p.id, name, color,found); // always found
            } else{
                throw std::logic_error("Invalid grouping value"); // should never happen
            }
            originalSliceNames.append(name);
            seriesRelativeWeigth->append(name, p.amount);
        }

    }

    // *** step 5 : labels for slices ***
    seriesRelativeWeigth->setLabelsVisible(true);
    int slSize = seriesRelativeWeigth->slices().size();
    for (int i = 0; i < slSize; ++i) {
        QPieSlice *slice = seriesRelativeWeigth->slices().at(i);
        // change font
        QFont f = slice->labelFont();
        Util::changeFontSize(f, Util::FontResizeIntensity::AVERAGE,true);
        slice->setLabelFont(f);
        if (ui->RW_ShowLabelsCheckBox->isChecked()==true) {
            slice->setLabelVisible(true);
        } else {
            slice->setLabelVisible(false);
        }
        // set label
        //slice->setLabel(QString("%1").arg(i+1));
        //slice->setLabelArmLengthFactor(0.5); // default is 15. Clipping problem (bug ?)
        // set color
        slice->setBrush(colorsRelativeWeigth[i]);
    }
    chartRelativeWeight->addSeries(seriesRelativeWeigth);

    // *** step 6 : Update legend (items the right listbox) ***
    ui->RW_listWidget->clear();
    QFontMetrics fm(ui->RW_listWidget->font());
    int sizeFontList = 0.9*fm.height();
    for (int i = 0; i < slSize; ++i) {
        QPieSlice *slice = seriesRelativeWeigth->slices().at(i);
        QListWidgetItem *item = new QListWidgetItem();

        //  add color marker
        QPixmap pix(sizeFontList, sizeFontList);
        pix.fill(slice->brush().color());
        item->setIcon(QIcon(pix));

        LegendItemInfo p = tempList.at(i);
        QString s = QString("%1").arg(originalSliceNames.at(i));
        item->setText(s);

        // We attach a LegendItemInfo varaible to the item so that we can used the data later
        item->setData(Qt::UserRole,QVariant::fromValue(p));

        ui->RW_listWidget->addItem(item);
    }

}


void AnalysisDialog::clearLegendWidgets()
{
    ui->rwRankLabel->setText("");
    ui->rwAmountLabel->setText("");
    ui->rwPercentageLabel->setText("");
    ui->rwPercentageSignLabel->setVisible(false);
}


void AnalysisDialog::updateLegendWidgets(int rank, double amount, double percentage)
{
    if (rank==-1) {
        ui->rwRankLabel->setText("");
    } else {
        ui->rwRankLabel->setText(QString::number(rank));
    }

    QString amountString = CurrencyHelper::formatAmount(amount, currInfo, locale, false);
    ui->rwAmountLabel->setText(amountString);
    ui->rwPercentageLabel->setText(QString::number(percentage, 'f', 2));
    ui->rwPercentageSignLabel->setVisible(true);
}


void AnalysisDialog::recalculate_BinsData(ReportType rTypr,
    QTableWidget* tableWidget)
{
    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario.isNull()){
        return; // if no scenario loaded (should not happen)
    }

    // Make sure the raw data is available (which should always be the case)
    QSharedPointer<CombinedFeStreams> chartRawData = chartRawDataRef.toStrongRef();
    if(chartRawData.isNull()){
        return; // should never happen
    }

    // Build a generic ptr to bin set according to rTypr (less code to duplicate)
    QMap<QDate,Bin>* binsPtr;
    if (rTypr==ReportType::MONTHLY) {
        binsPtr = &binsMonthly;
    } else {
        binsPtr = &binsYearly;
    }

    // Reset bin set to empty
    (*binsPtr).clear();

    // Init and resize bins data for maximum range , that is from "tomorrow" to
    // "max scenario limit", irrespective of the span of rawdata we have. The "key" dates
    // corresponds to the beginning of a bin and act as an identifier for this bin.
    QDate tomorrow = GbpController::getInstance().getTomorrow();
    qint64 tomorrowJulianDay = tomorrow.toJulianDay();
    QDate to = tomorrow.addYears(scenario->getFeGenerationDuration()).addDays(-1);
    QDate toMonthYear = QDate(to.year(), (rTypr==ReportType::MONTHLY)?(to.month()):(1), 1);
    QDate date = QDate(tomorrow.year(), (rTypr==ReportType::MONTHLY)?(tomorrow.month()):(1), 1);
    while( date <= toMonthYear ){
        (*binsPtr).insert(date,{.income=0, .expense=0, .delta=0, .cashBalance=0});
        if(rTypr==ReportType::MONTHLY){
            date = date.addMonths(1);
        } else {
            date = date.addYears(1);
        }
    }

    double eopCashBalance = startingAmount; // cumulative cash balance at the end of a period

    // For each and every bin, add the contribution of all raw data elements fitting this bin.
    // Order of iteration must be asc. date, to correctly calculate cumulative cash balance
    QList<QDate> binsKeys = (*binsPtr).keys(); // keys are placed in ascending order
    QList<CombinedFeStreams::DailyInfo> listDi = chartRawData->getCombinedStreams();
    foreach(QDate binDate, binsKeys){
        Bin binData = {.income=0, .expense=0,
            .delta=0, .cashBalance=eopCashBalance};
        // Get the list of all dates included in this bin
        QList<QDate> dateList = getListOfDatesCoveredbyBin(rTypr, binDate);

        // Check if raw data exist for each date. Order of iteration must be asc. date,
        // to correctly calculate cumulative cash balance If yes, add to contribution
        // and update eopCashBalance
        foreach(QDate d, dateList){
            if (d<tomorrow) {
                continue; // we dont have data before tomorrow
            }
            // Convert date to index in CombinedFeStreams
            int index = d.toJulianDay() - tomorrowJulianDay;
            // Get the DailyInfo concerned
            if (index < listDi.size()) {
                const CombinedFeStreams::DailyInfo di = listDi[index];
                // Proceeed with calculation
                binData.income += di.totalIncomes;
                binData.expense += fabs(di.totalExpenses);
                binData.delta += (di.totalIncomes + di.totalExpenses);
                eopCashBalance += (di.totalIncomes + di.totalExpenses);
                binData.cashBalance = eopCashBalance;
                // write back the final value of the bin data
                (*binsPtr).insert(binDate, binData);
            }

        }
    }

 }


// use already calculated bins to update table content
void AnalysisDialog::redisplay_MonthlyYearlyReportTableData(ReportType rTypr,
    QTableWidget* tableWidget)
{
    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario.isNull()){
        return; // if no scenario loaded (should not happen)
    }
    // choose the right bins
    QMap<QDate,Bin>* binsPtr;
    if (rTypr==ReportType::MONTHLY) {
        binsPtr = &binsMonthly;
    } else {
        binsPtr = &binsYearly;
    }

    // determine how many rows in the table. We display all month/years between
    // tomorrow and max date as established by the scenario
    QDate tomorrow = GbpController::getInstance().getTomorrow();
    QDate maxDate = tomorrow.addYears(scenario->getFeGenerationDuration()).addDays(-1);
    int noRows;
    int noOfMonths = 1 + (12*maxDate.year()+maxDate.month()) -
        (12*tomorrow.year()+tomorrow.month());
    int noOfYears = 1 + (maxDate.year()) - (tomorrow.year());
    if (rTypr==ReportType::MONTHLY) {
        noRows = noOfMonths;
    } else{
        noRows = noOfYears;
    }

    // Generate the list of dates
    QList<QDate> dateList;
    QDate date = tomorrow;
    if (rTypr==ReportType::MONTHLY) {
        while(date <= maxDate){
            dateList.append(QDate(date.year(),date.month(),1));
            date = date.addMonths(1);
        }
    } else{
        while(date <= maxDate){
            dateList.append(QDate(date.year(),1,1));
            date = date.addYears(1);
        }
    }

    // fill table
    tableWidget->clearContents();
    tableWidget->setRowCount(noRows); // must be done BEFORE inserting item...
    int row = 0;
    QString s1,s2,s3,s4,s5; // one for each column
    QFont defaultFont = tableWidget->font();
    QFont monoTableFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoTableFont.setPointSize(defaultFont.pointSize());

    QBrush negAmountColor = QBrush(GbpController::getInstance().getExpenseColor());
    QBrush posAmountColor = QBrush(GbpController::getInstance().getIncomeColor());
    foreach(QDate date, dateList){
        Bin mr;
        if ( true == binsPtr->contains(date)){
            mr = binsPtr->value(date);
        } else {
            mr = {.income=0, .expense=0, .delta=0, .cashBalance=0};
        }

        // build column items
        if (rTypr==ReportType::MONTHLY) {
            s1 = locale.toString(date,"yyyy MMMM");
        } else {
            s1 = locale.toString(date,"yyyy");
        }
        s2 = CurrencyHelper::formatAmount(mr.income,currInfo,locale,false);
        s3 = CurrencyHelper::formatAmount(mr.expense,currInfo,locale,false);
        s4 = CurrencyHelper::formatAmount(mr.delta,currInfo,locale,false);
        s5 = CurrencyHelper::formatAmount(mr.cashBalance,currInfo,locale,false);
        QTableWidgetItem* wi1 = new QTableWidgetItem(s1);
        QTableWidgetItem* wi2 = new QTableWidgetItem(s2);
        QTableWidgetItem* wi3 = new QTableWidgetItem(s3);
        QTableWidgetItem* wi4 = new QTableWidgetItem(s4);
        QTableWidgetItem* wi5 = new QTableWidgetItem(s5);
        wi1->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        wi2->setFont(monoTableFont);
        wi2->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        wi3->setFont(monoTableFont);
        wi3->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        wi4->setFont(monoTableFont);
        wi4->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        wi5->setFont(monoTableFont);
        wi5->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if(mr.delta<0){
            wi4->setForeground(negAmountColor);
        } else if (mr.delta>0) {
            wi4->setForeground(posAmountColor);
        }
        if(mr.cashBalance<0){
            wi5->setForeground(negAmountColor);
        } else if (mr.cashBalance>0) {
            wi5->setForeground(posAmountColor);
        }
        // add to intrinsic table model
        tableWidget->setItem(row,0,wi1);
        tableWidget->setItem(row,1,wi2);
        tableWidget->setItem(row,2,wi3);
        tableWidget->setItem(row,3,wi4);
        tableWidget->setItem(row,4,wi5);
        row++;
    }

}


// Using already calculated data, completely rebuild the Report Chart (eigher Monthly or Yearly)
void AnalysisDialog::redisplay_ReportChart(ReportType type)
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
    if (type == ReportType::MONTHLY) {
        chartPtr = &chartMonthlyReport;
        chartViewPtr = &chartViewMonthlyReport;
        xAxisPtr = &chartMonthlyReportAxisX;
        yAxisPtr = &chartMonthlyReportAxisY;
        statLabelPtr = ui->monthlyReportStatsLabel;
    } else {
        chartPtr = &chartYearlyReport;
        chartViewPtr = &chartViewYearlyReport;
        xAxisPtr = &chartYearlyReportAxisX;
        yAxisPtr = &chartYearlyReportAxisY;
        statLabelPtr = ui->annualReportStatsLabel;
    }

    // If no data to display
    if (chartRawData->getNoOfElementsUsed()==0){
        (*chartPtr)->removeAllSeries();
        QStringList empty = QStringList();
        empty.append("No data");
        (*xAxisPtr)->setCategories(empty);
        (*yAxisPtr)->setRange(0,1);
        statLabelPtr->setText("");
        return;  // nothing to do after
    }

    // determine which data set is to be used
    GraphDataSourceType dsType = findWhichSetsIsToBeUsedReportChart(type);

    // choose the right bins
    QMap<QDate,Bin>* binsPtr;
    if (type==ReportType::MONTHLY) {
        binsPtr = &binsMonthly;
    } else {
         binsPtr = &binsYearly;
    }

    // Set subset of bin's data to use for display
    QDate startDate, endDate ;
    getStartEndDateReportChart(type, startDate, endDate);

    // Calculate stats
    QDate date = startDate;
    double mean=0;
    double stdDeviation=0;
    double sum=0;
    QList<double> statData;
    while(date<=endDate){
        Bin mr = {.income=0, .expense=0, .delta=0};
        if (true == binsPtr->contains(date) ){
            mr = binsPtr->value(date);
        }
        // record the right data
        if (dsType==GraphDataSourceType::DST_INCOME) {
            statData.append(mr.income);
        } else if (dsType==GraphDataSourceType::DST_EXPENSE){
            statData.append(-mr.expense);
        } else if(dsType==GraphDataSourceType::DST_DELTA){
            statData.append(mr.delta);
        }
        // go to next date
        if (type==ReportType::MONTHLY) {
            date = date.addMonths(1);
        } else {
            date = date.addYears(1);
        }
    }
    calculateStatsForGraph(statData, mean, stdDeviation, sum);

    // build new set of chart data (3 sets of QBarSet) and categories
    QColor red = GbpController::getInstance().getExpenseColor();
    QColor green = GbpController::getInstance().getIncomeColor();
    double maxY = 1;
    double minY = 0;
    QStringList categories;
    QBarSet *setIncomes = new QBarSet(tr("Incomes"));
    setIncomes->setColor(green);
    QBarSet *setExpenses = new QBarSet(tr("Expenses"));
    setExpenses->setColor(red);
    QBarSet *setDeltasPositive = new QBarSet(tr("Deltas - Surplus"));
    setDeltasPositive->setColor(green);
    QBarSet *setDeltasNegative = new QBarSet(tr("Deltas - Deficit"));
    setDeltasNegative->setColor(red);

    date= startDate;
    while(date<=endDate){
        Bin mr = {.income=0, .expense=0, .delta=0};
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
    getMinMaxReportChart(type, setIncomes, setExpenses, setDeltasPositive, setDeltasNegative,
        minY, maxY);

    // deallocate all the stuff currently in use
    (*chartPtr)->removeAllSeries(); // deallocate all the current stuff, EXCLUDING both axis

    // completely rebuild the series, discard set not to be used
    QBarSeries* series = new QBarSeries();
    if( (dsType==GraphDataSourceType::DST_INCOME) ||
        (dsType==GraphDataSourceType::DST_INCOME_EXPENSE) ){
        series->append(setIncomes); // take ownership
    } else{
        delete setIncomes;
    }
    if( (dsType==GraphDataSourceType::DST_EXPENSE) ||
        (dsType==GraphDataSourceType::DST_INCOME_EXPENSE) ){
        series->append(setExpenses); // take ownership
    } else{
        delete setExpenses;
    }
    if( dsType==GraphDataSourceType::DST_DELTA ){
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
            QString finaleCatString = QString("%1 %2").arg(locale.monthName(month,
                QLocale::FormatType::ShortFormat)).arg(catStringList.at(1));
            QString text = QString("%1 / %2 / %3").arg(set->label()).arg(finaleCatString)
                .arg(CurrencyHelper::formatAmount(set->at(index),currInfo, locale, true));
            ui->monthlyReportChartSelectedTextLabel->setText(text);
        });
    } else {
        QObject::connect(series, &QAbstractBarSeries::clicked, series, [=](int index, QBarSet *set){
            QStringList cats = chartYearlyReportAxisX->categories();
            set->deselectAllBars();
            bool ok;
            QString cat = cats.at(index);
            QString text = QString("%1 / %2 / %3").arg(set->label()).arg(cat)
                .arg(CurrencyHelper::formatAmount(set->at(index),currInfo, locale, true));
            ui->yearlyReportChartSelectedTextLabel->setText(text);
        });
    }

    // Set Chart Title
    setMonthlyYearlyChartTitle((*chartPtr), dsType);

    // Erase Selected Bar Value
   if (type==ReportType::MONTHLY) {
    ui->monthlyReportChartSelectedTextLabel->setText("");
   } else {
    ui->yearlyReportChartSelectedTextLabel->setText("");
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
    if (dsType != GraphDataSourceType::DST_INCOME_EXPENSE) {
        QString statText;
        if (dsType==GraphDataSourceType::DST_EXPENSE) {
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


void AnalysisDialog::on_closePushButton_clicked()
{
    hide();
    // clear some now useless data, since they will be rebuild when the form is shown again.
    binsMonthly = {};
    binsYearly = {};
    availableTags.clear();
    tagsTableData.clear();

    LOG_DEBUG_INFO(QString("Analysis dialog closed"));
}


void AnalysisDialog::on_AnalysisDialog_rejected()
{
    on_closePushButton_clicked();
}



void AnalysisDialog::on_noElementsSpinBox_valueChanged(int arg1)
{
    updateRelativeWeightChart();
}


void AnalysisDialog::exportTextMonthlyYearlyReport(ReportType rType) {

    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario.isNull()){
        return; // if no scenario loaded (should not happen)
    }

    LOG_INFO(QString("Initiating Analysis %1 Report export in CSV format with amount %2")
        .arg((rType==ReportType::MONTHLY)?("Monthly"):("Yearly"))
        .arg((GbpController::getInstance().getExportTextAmountLocalized()==true)?
            ("localized"):("not localized")));

    QString defaultExtensionUsed = "CSV files (*.csv *.CSV)";
    QString filter = tr("CSV files (*.csv *.CSV);;Text files (*.txt *.TXT);;All files (*)");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Select a file"),
        GbpController::getInstance().getLastDirExport(), filter, &defaultExtensionUsed);
    if (fileName == ""){
        // User has canceled
        LOG_INFO("Export has been canceled");
        return;
    }

    // fix the filename to add the proper suffix
    QFileInfo fi(fileName);
    if(fi.suffix()==""){    // user has not specified an extension
        fileName.append(".csv");
    }
    GbpController::getInstance().setLastDirExport(fi.absolutePath());

    QFile file(fileName);
    if (false == file.open(QFile::WriteOnly | QFile::Truncate)){
        QMessageBox::critical(nullptr,tr("Error"),tr("Export process failed. Cannot open the "
            "file for saving"));
        GbpLogger::getInstance().logError(
            QString("Export failed : Cannot open file %1").arg(REDACT(fileName)));
        return;
    }

    QMap<QDate,Bin>* binsPtr;
    QString dateFormat;
    if (rType == ReportType::MONTHLY) {
        dateFormat = "yyyy-MM";
        binsPtr = &binsMonthly;
    } else {
        dateFormat = "yyyy";
        binsPtr = &binsYearly;
    }

    QString inc ;
    QString exp ;
    QString total ;
    QString cashBalance;
    QString s;

    // determine how many elements there are. We display all month/years between
    // tomorrow and max date as established by the scenario
    QDate tomorrow = GbpController::getInstance().getTomorrow();
    QDate maxDate = tomorrow.addYears(scenario->getFeGenerationDuration()).addDays(-1);
    int noRows;
    int noOfMonths = 1 + (12*maxDate.year()+maxDate.month()) -
        (12*tomorrow.year()+tomorrow.month());
    int noOfYears = 1 + (maxDate.year()) - (tomorrow.year());
    if (rType==ReportType::MONTHLY) {
        noRows = noOfMonths;
    } else{
        noRows = noOfYears;
    }

    // Generate the list of dates
    QList<QDate> dateList;
    QDate date = tomorrow;
    if (rType==ReportType::MONTHLY) {
        while(date <= maxDate){
            dateList.append(QDate(date.year(),date.month(),1));
            date = date.addMonths(1);
        }
    } else{
        while(date <= maxDate){
            dateList.append(QDate(date.year(),1,1));
            date = date.addYears(1);
        }
    }

    // write header
    s = QString("%1\t%2\t%3\t%4\t%5\n").arg(tr("Period"),tr("Total incomes"),tr("Total expenses"),
        tr("Delta"),tr("Cash balance"));
    file.write(s.toUtf8());

    // write data
    double lastCashBalance = startingAmount;
    foreach(QDate date, dateList){
        Bin item;
        if ( true == binsPtr->contains(date)){
            item = binsPtr->value(date);
            lastCashBalance = item.cashBalance;
        } else {
            item = {.income=0, .expense=0, .delta=0, .cashBalance=lastCashBalance};
        }

        QString dateString = locale.toString(date,dateFormat);
        if (GbpController::getInstance().getExportTextAmountLocalized()) {
            // Localized
            inc = CurrencyHelper::formatAmount(item.income, currInfo, locale, false);
            exp = CurrencyHelper::formatAmount(item.expense, currInfo, locale, false);
            total = CurrencyHelper::formatAmount(item.delta, currInfo, locale, false);
            cashBalance = CurrencyHelper::formatAmount(item.cashBalance, currInfo, locale, false);
        } else {
            // not localized
            inc = QString::number(item.income,'f', currInfo.noOfDecimal);
            exp = QString::number(item.expense,'f', currInfo.noOfDecimal);
            total = QString::number(item.delta,'f', currInfo.noOfDecimal);
            cashBalance = QString::number(item.cashBalance,'f', currInfo.noOfDecimal);
        }

        s = QString("%1\t%2\t%3\t%4\t%5\n").arg(dateString,inc,exp,total,cashBalance);
        file.write(s.toUtf8());
    }
    file.close();
    LOG_INFO("Export succeeded");

}


void AnalysisDialog::exportChartAsImage(QWidget* chartWidget, QString desc)
{
    LOG_INFO("Initiating chart export in PNG format : "+desc);

    // get file name
    QString defaultExtensionUsed = "PNG files (*.png *.PNG)";
    QString filter = tr("PNG files (*.png *.PNG);;All files (*)");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Select an image file"),
        GbpController::getInstance().getLastDirExport(), filter, &defaultExtensionUsed);
    if (fileName != ""){
        // fix the filename to add the proper suffix
        QFileInfo fi(fileName);
        if(fi.suffix()==""){    // user has not specified an extension
            fileName.append(".png");
        }
        GbpController::getInstance().setLastDirExport(fi.absolutePath());

        bool successful;
        successful = chartWidget->grab().save(fileName,"PNG", 100) ;  // max quality
        if(successful == false){
            QMessageBox::critical(nullptr,tr("Error"),
                tr("Export failed. The creation of the image file did not succeed"));
            GbpLogger::getInstance().logError(QString("Export failed. The creation of the image "
                "file %1 did not succeed").arg(REDACT(fileName)));
            return;
        }
        LOG_INFO(QString("Export to file %1 has succeeded")
            .arg(REDACT(fileName)));
    } else{
        LOG_INFO("Export canceled by user");
    }
}


void AnalysisDialog::exportRelativeWeightLegendAsCsvFile()
{
    LOG_INFO(
        QString("Initiating Relative Weight legend export in CSV format with amount %1")
         .arg((GbpController::getInstance().getExportTextAmountLocalized()==true)?
        ("localized"):("not localized")));

    QString defaultExtensionUsed = "CSV files (*.csv *.CSV)";
    QString filter = tr("CSV files (*.csv *.CSV);;Text files (*.txt *.TXT);;All files (*)");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Select a file"),
        GbpController::getInstance().getLastDirExport(), filter, &defaultExtensionUsed);
    if (fileName == ""){
        // User has canceled
        LOG_INFO("Export has been canceled");
        return;
    }

    // fix the filename to add the proper suffix
    QFileInfo fi(fileName);
    if(fi.suffix()==""){    // user has not specified an extension
        fileName.append(".csv");
    }
    GbpController::getInstance().setLastDirExport(fi.absolutePath());

    QFile file(fileName);
    if (false == file.open(QFile::WriteOnly | QFile::Truncate)){
        QMessageBox::critical(nullptr,tr("Error"),tr("Export process failed. Cannot open the "
            "file for saving"));
        GbpLogger::getInstance().logError(QString("Export failed : Cannot open file %1")
            .arg(REDACT(fileName)));
        return;
    }

    QString rank ;
    QString percentage ;
    QString name ;
    QString amount;
    QString s;
    double d;
    int i;
    bool b;

    // determine how many elements there are. We display all month/years between
    // tomorrow and max date as established by the scenario
    int noRows = ui->RW_listWidget->count();

    // write header
    s = QString("%1\t%2\t%3\t%4\n").arg(tr("Rank"),tr("Percentage"),tr("Csd name"),
        tr("Amount"));
    file.write(s.toUtf8());

    // write data
    for (int var = 0; var < noRows; ++var) {
        // Get line
        QListWidgetItem* item = ui->RW_listWidget->item(var);
        QString name = item->text();

        LegendItemInfo info = item->data(Qt::UserRole).value<LegendItemInfo>();

        // Rank
        if (GbpController::getInstance().getExportTextAmountLocalized()) {
            rank = CurrencyHelper::formatAmount(info.rank, currInfo, locale, false);
        } else {
            rank =QString::number(static_cast<int>(info.rank));
        }

        // Percentage
        if (GbpController::getInstance().getExportTextAmountLocalized()) {
            percentage = CurrencyHelper::formatAmount(info.percentage, currInfo, locale, false);
        } else {
            percentage = QString::number(info.percentage,'f', currInfo.noOfDecimal);
        }

        // Amount
        if (GbpController::getInstance().getExportTextAmountLocalized()) {
            amount = CurrencyHelper::formatAmount(info.amount, currInfo, locale, false);
        } else {
            amount = QString::number(info.amount,'f', currInfo.noOfDecimal);
        }

        // Write the result
        s = QString("%1\t%2\t%3\t%4\n").arg(rank, percentage, name, amount);
        file.write(s.toUtf8());
    }

    file.close();
    LOG_INFO(QString("Export to %1 is successful").arg(REDACT(fileName)));
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
        chartTitle = tr("Monthly incomes and expenses");
        ui->monthlyReportStatsLabel->setText("");
    } else {
        chartPtr = &chartYearlyReport;
        chartViewPtr = &chartViewYearlyReport;
        xAxisPtr = &chartYearlyReportAxisX;
        yAxisPtr = &chartYearlyReportAxisY;
        widgetPtr = &(ui->yearlyReportChartWidget);
        chartTitle = tr("Yearly incomes and expenses");
        ui->annualReportStatsLabel->setText("");
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
    //(*chartPtr)->setTitle(chartTitle);
    (*chartPtr)->setAnimationOptions(QChart::SeriesAnimations);
    (*chartViewPtr) = new QChartView((*chartPtr), (*widgetPtr)); // take ownership
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
    if (type == ReportType::MONTHLY) {
        Util::changeFontSize(fontX, Util::FontResizeIntensity::AVERAGE, true);
    } else {
        Util::changeFontSize(fontX, Util::FontResizeIntensity::AVERAGE, true);
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
    if (type == ReportType::MONTHLY) {
        Util::changeFontSize(fontY, Util::FontResizeIntensity::AVERAGE, true);
    } else {
        Util::changeFontSize(fontY, Util::FontResizeIntensity::AVERAGE, true);
    }
    (*yAxisPtr)->setLabelsFont(fontY);
    //
    (*yAxisPtr)->setRange(0,1);
    (*yAxisPtr)->setTickCount(11); // 10 bins
    (*yAxisPtr)->setMinorTickCount(4); // 5 bins
    (*chartPtr)->addAxis((*yAxisPtr), Qt::AlignLeft); // the CHART (not the series) takes ownership
    series->attachAxis((*yAxisPtr));
    if(GbpController::getInstance().useDarkModeForChart()==true){
        (*chartPtr)->setTheme(QChart::ChartThemeDark);
    } else {
        (*chartPtr)->setTheme(QChart::ChartThemeLight);
    }
    (*widgetPtr)->installEventFilter(this);
}


// Return full QDate for start and end period for the displayed bar chart, from the widgets content
void AnalysisDialog::getStartEndDateReportChart(ReportType type, QDate &startDate,
    QDate &endDate) const
{
    if (type==ReportType::YEARLY) {
        startDate = QDate(ui->yearlyReportChartFromYearSpinBox->value(),1,1);
        endDate = startDate.addYears(ui->yearlyReportChartDurationSpinBox->value()-1);
    } else {
        startDate = QDate(ui->monthlyReportChartFromYearSpinBox->value(),
            ui->monthlyReportChartFromMonthComboBox->currentIndex()+1,1);
        endDate = startDate.addMonths(ui->monthlyReportChartDurationSpinBox->value()-1);
    }
}


void AnalysisDialog::getMinMaxReportChart(ReportType type, QBarSet* incomes, QBarSet* expenses,
    QBarSet* deltasPositive, QBarSet* deltasNegative, double& minY, double& maxY)
{
    GraphDataSourceType dsType = findWhichSetsIsToBeUsedReportChart(type);

    // default values : it works because we must have at least one element to search for in the sets
    minY = 0;
    maxY = 1;

    if( (dsType==GraphDataSourceType::DST_INCOME) ||
        (dsType==GraphDataSourceType::DST_INCOME_EXPENSE) ){
        // process incomes
        for(int i=0;i<incomes->count();i++){
            if ( incomes->at(i) > maxY ){
                maxY = incomes->at(i);
            }
        }
    }
    if( (dsType==GraphDataSourceType::DST_EXPENSE) ||
        (dsType==GraphDataSourceType::DST_INCOME_EXPENSE) ){
        // process expenses
        for(int i=0;i<expenses->count();i++){
            if ( expenses->at(i) > maxY ){
                maxY = expenses->at(i);
            }
        }
    }
    if( dsType==GraphDataSourceType::DST_DELTA ){
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


// Determine what data source the annual or monthly chart will use
AnalysisDialog::GraphDataSourceType AnalysisDialog::findWhichSetsIsToBeUsedReportChart(
    ReportType type)
{
    if (type==ReportType::MONTHLY) {
        if( ui->monthlyReportChartIncomesRadioButton->isChecked() ){
            return GraphDataSourceType::DST_INCOME;
        } else if (ui->monthlyReportChartExpensesRadioButton->isChecked()){
            return GraphDataSourceType::DST_EXPENSE;
        } else if (ui->monthlyReportChartIncomesExpensesRadioButton->isChecked()){
            return GraphDataSourceType::DST_INCOME_EXPENSE;
        } else if (ui->monthlyReportChartDeltasRadioButton->isChecked()){
            return GraphDataSourceType::DST_DELTA;
        } else {
            // should never happen
            throw std::logic_error("No control checked for Graph Monthly Report");
        }
    } else if (type==ReportType::YEARLY) {
        if( ui->yearlyReportChartIncomesRadioButton->isChecked() ){
            return GraphDataSourceType::DST_INCOME;
        } else if (ui->yearlyReportChartExpensesRadioButton->isChecked()){
            return GraphDataSourceType::DST_EXPENSE;
        } else if (ui->yearlyReportChartIncomesExpensesRadioButton->isChecked()){
            return GraphDataSourceType::DST_INCOME_EXPENSE;
        } else if (ui->yearlyReportChartDeltasRadioButton->isChecked()){
            return GraphDataSourceType::DST_DELTA;
        } else {
            // should never happen
            throw std::logic_error("No control checked for Graph Yearly Report");
        }
    } else {
        // should never happen
        throw std::invalid_argument("Report Type value is invalid");
    }
}


void AnalysisDialog::setMonthlyYearlyChartTitle(QChart* chartPtr, GraphDataSourceType dsType)
{
    // if ( dsType==GraphDataSourceType::DST_INCOME_EXPENSE ) {
    //     chartPtr->setTitle(tr("Incomes and expenses"));
    // } else if (dsType==GraphDataSourceType::DST_INCOME){
    //     chartPtr->setTitle(tr("Incomes"));
    // } else if(dsType==GraphDataSourceType::DST_EXPENSE){
    //     chartPtr->setTitle(tr("Expenses"));
    // } else if(dsType==GraphDataSourceType::DST_DELTA){
    //     chartPtr->setTitle(tr("Deltas"));
    // } else {
    //     // should never happen
    //     chartPtr->setTitle("");
    // }
}


// Variance and standard deviation are both measures of variability in a dataset, but they differ
// in their calculation and interpretation. Variance is the average of the squared differences
// from the mean, while standard deviation is the square root of the variance. Standard deviation
// is often preferred because it is expressed in the same units as the original data, making it
// easier to interpret in the context of the data set
void AnalysisDialog::calculateStatsForGraph(const QList<double> data, double &mean,
    double &stdDeviation, double &sum)
{
    // init
    mean = 0;
    stdDeviation = 0;
    sum = 0;

    if (data.size()==0) {
        return;
    }

    // Calculate the mean
    for (double value : data) {
        sum += value;
    }
    mean = sum / data.size();

    // Calculate the variance
    double varianceSum = 0.0;
    for (double value : data) {
        varianceSum += (value - mean) * (value - mean);
    }
    double variance = varianceSum / data.size(); // For population standard deviation

    // Calculate the standard deviation
    stdDeviation = std::sqrt(variance);
}


// Get the list of all dates included in a specific bin
QList<QDate> AnalysisDialog::getListOfDatesCoveredbyBin(ReportType type, QDate binDate)
{
    QList<QDate> answer;
    if (type == ReportType::MONTHLY) {
        // Month
        QDate fDate = QDate(binDate.year(), binDate.month(), 1);
        QDate tDate = QDate(binDate.year(), binDate.month(), binDate.daysInMonth());
        QDate date = fDate;
        while(date <= tDate){
            // add the element to the list
            answer.append(date);
            // increase date
            date = date.addDays(1);
        }
    } else {
        // Year
        QDate fDate = QDate(binDate.year(),1,1);
        QDate tDate = QDate(binDate.year(),12,31);
        QDate date = fDate;
        while(date <= tDate){
            // add the element to the list
            answer.append(date);
            // increase date
            date = date.addDays(1);
        }
    }
    return answer;
}


void AnalysisDialog::tags_UpdateNoTagsSelected()
{
    ui->tags_noTagsSelectedLabel->setText(tr("%1 selected").arg(selectedTags.size()));
}


void AnalysisDialog::exportTagsAnalysisAsCsv()
{
    // *** get a file name ***
    QString defaultExtensionUsed = "CSV files (*.csv *.CSV)";
    QString filter = tr("CSV files (*.csv *.CSV);;Text files (*.txt *.TXT);;All files (*)");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Select a file"),
        GbpController::getInstance().getLastDirExport(), filter, &defaultExtensionUsed);
    if (fileName == ""){
        return;
    }
    // *** fix the filename to add the proper suffix ***
    QFileInfo fi(fileName);
    if(fi.suffix()==""){    // user has not specified an extension
        fileName.append(".csv");
    }
    GbpController::getInstance().setLastDirExport(fi.absolutePath());
    LOG_INFO(QString("Attempting to export Tags Analysis to CSV file \"%1\" ...")
        .arg(REDACT(fileName)));

    QFile file(fileName);
    if (false == file.open(QFile::WriteOnly | QFile::Truncate)){
        QMessageBox::critical(nullptr,tr("Error"),tr("Cannot open the file for writing"));
        GbpLogger::getInstance().logError(
            QString("Export failed : Cannot open the file for saving"));
        return;
    }

    // *** export to the file ***
    QString s;

    // write header
    s = QString("%1\t%2\t%3\n").arg(tr("Tag name"),tr("Total Amount"), tr("Weight (%)"));
    file.write(s.toUtf8());

    // write data : no sorting by weight...
    QString amountString;
    for (auto it = tagsTableData.constBegin(); it != tagsTableData.constEnd(); ++it) {
        const QUuid& key = it.key();
        const TagContribution& data = it.value();

        if (GbpController::getInstance().getExportTextAmountLocalized()) {
            // Localized
            amountString = CurrencyHelper::formatAmount(data.amount, currInfo, locale, false);
        } else {
            // not localized
            amountString = QString::number(data.amount,'f', currInfo.noOfDecimal);
        }

        s = QString("%1\t%2\t%3\n").arg(data.name).arg(amountString).arg(100*data.weight);
        file.write(s.toUtf8());
    }

    // Close file and exit
    file.close();
    LOG_INFO("Export succeeded");
}


void AnalysisDialog::redisplay_TagTable()
{
    if (ready==false) {
        return;
    }

    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario.isNull()){
        return; // if no scenario loaded (should not happen)
    }

    // *** Get/validate To and From dates ***
    QDate from = ui->tagsFromDateEdit->date();
    QDate to = ui->tagsToDateEdit->date();
    QDate tomorrow = GbpController::getInstance().getTomorrow();
    if(!from.isValid()){
        QMessageBox::critical(nullptr,tr("Error"),tr("\"From\" date is invalid"));
        return;
    } else if (!to.isValid()) {
        QMessageBox::critical(nullptr,tr("Error"),tr("\"To\" date is invalid"));
        return;
    } else if (to<from){
        QString fromString = from.toString(Qt::ISODate);
        QString toString = to.toString(Qt::ISODate);
        QString s = QString(tr("\"To\" date %1 cannot occur before \"From\" date %2"))
            .arg(toString,fromString);
        QMessageBox::critical(nullptr,tr("Error"),s.toLocal8Bit().data());
        return;
    } else if (from<tomorrow){
        QString fromString = from.toString(Qt::ISODate);
        QString tomorrowString = tomorrow.toString(Qt::ISODate);
        QString s = QString(tr("\"From\" date %1 cannot be smaller than \"tomorrow\" %2"))
            .arg(fromString,tomorrowString);
        QMessageBox::critical(nullptr,tr("Error"),s.toLocal8Bit().data());
        return;
    }

    // *** Rebuild tags data ***
    tagsTableData.clear();
    double totalWithoutTags;
    tags_rebuildData(from, to, tagsTableData, totalWithoutTags); // key is Tag ID

    // Disable sorting when filling the table
    ui->tagsTableWidget->setSortingEnabled(false);

    // *** Update the table with the content ***
    ui->tagsTableWidget->clearContents();
    ui->tagsTableWidget->setRowCount(tagsTableData.size());
    int row = 0;

    for (auto it = tagsTableData.constBegin(); it != tagsTableData.constEnd(); ++it) {
        const QUuid& key = it.key();
        const TagContribution& data = it.value();
        // Use key and value (read-only)
        QString s1 = data.name;
        QString s2 = CurrencyHelper::formatAmount(abs(data.amount),currInfo,locale,false);
        QString s3 = QString::number(data.weight*100, 'f', 3);
        QTableWidgetItem* wi1 = new QTableWidgetItem(s1);
        QTableWidgetItem* wi2 = new NumericTableWidgetItem(s2);
        wi2->setData(Qt::UserRole, data.amount);
        QTableWidgetItem* wi3 = new NumericTableWidgetItem(s3);
        wi3->setData(Qt::UserRole, data.weight);
        wi1->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        wi2->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        wi3->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tagsTableWidget->setItem(row,0,wi1);
        ui->tagsTableWidget->setItem(row,1,wi2);
        ui->tagsTableWidget->setItem(row,2,wi3);
        row++;
    }

    // update total amount and set the color
    ui->tags_totalAmountLabel->setText(CurrencyHelper::formatAmount(abs(totalWithoutTags),currInfo,
        locale,true));
    if (ui->tagsIncomesRadioButton->isChecked()==true) {
        ui->tags_totalAmountLabel->setStyleSheet(Util::getStyleSheetStringForColor(
            GbpController::getInstance().getIncomeColor()));
    } else {
        ui->tags_totalAmountLabel->setStyleSheet(Util::getStyleSheetStringForColor(
            GbpController::getInstance().getExpenseColor()));
    }

    // re-enable sorting
    ui->tagsTableWidget->setSortingEnabled(true);
}


void AnalysisDialog::tags_rebuildData( QDate from, QDate to, QHash<QUuid,TagContribution>& data,
    double& totalWithoutTags)
{
    // Reset output variables
    totalWithoutTags = 0;
    data.clear();

    // Init the structure for each selected tag
    bool found;
    QSet<QUuid> selectedTagsSet = selectedTags.getFilterTagIdSet();
    for (const QUuid& tagId : selectedTagsSet) {
        // Get tags's name
        Tag t = availableTags.getTag(tagId,found);
        if (found==false) {
            continue; // should never happen
        }
        // Add to "data"
        data.insert(tagId, {.name=t.getName(), .amount=0, .weight=0});
    }


    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario.isNull()){
        return ; // if no scenario loaded (should not happen)
    }

    // Make sure the raw data is available (which should always be the case)
    QSharedPointer<CombinedFeStreams> chartRawData = chartRawDataRef.toStrongRef();
    if(chartRawData.isNull()){
        return ; // should never happen
    }

    // take a reference to all the relationships defined between Csds and Tags
    TagCsdRelationships r = scenario->getTagCsdRelationships() ;

    // are we processing income or expense related stuff ?
    bool incomeProcessing = (ui->tagsIncomesRadioButton->isChecked()? (true):(false));

    QList<CombinedFeStreams::DailyInfo> listDi = chartRawData->getCombinedStreams();
    const qsizetype diSize = listDi.size();
    QDate tomorrow = GbpController::getInstance().getTomorrow();

    /*
     * Strategy : for each existing DailyInfo, look in the incomeList/expenseList and for each
     * Csd found, get the list of all linked tags. For each tag found,
     * add the contribution to "data".
     */

    for (int var = 0; var < diSize; ++var) {

        // +++ Pre-processing +++

        CombinedFeStreams::DailyInfo di = listDi[var];
        if(di.used == false){
            continue;
        }
        QDate diDate = tomorrow.addDays(var);


        // +++ Processing +++

        // Make sure this date is inside the user-selected period
        if ( (diDate<from) || (diDate>to) ) {
            continue;
        }

        // Add total income and expense
        if (incomeProcessing==true) {
            totalWithoutTags += di.totalIncomes;
        } else {
            totalWithoutTags += di.totalExpenses;
        }

        // Proceed by iterating through all the elements for that day and distribute the
        // contribution of each tag
        QList<Fe>* feListPtr = (incomeProcessing==true)?(&(di.incomesList)):(&(di.expensesList));
        foreach (const Fe fe, *feListPtr) {
            // Get a pointer to the CSD
            QSharedPointer<Csd> csdPtr = fe.csdPtr.toStrongRef();
            if(csdPtr.isNull()){
                continue;   // should never happen
            }
            // Get all the tags associated to this CSD and for each add the contribution
            const QSet<QUuid> idSet = r.getRelationshipsForCsd(csdPtr->getId());
            for (const QUuid& tagId : idSet) {
                // Add the contribution of this tag if it is in the list of selected tags
                if (selectedTags.containsTagId(tagId)==true) {
                    TagContribution v = data.value(tagId, {}); // may already exist or not
                    v.amount += fe.amount; // can be negative
                    data.insert(tagId,v);
                }
            }
        }
    }

    // Calculate weight. totalWithoutTags an be 0 !
    for (auto it = data.begin(); it != data.end(); ++it) {
        const QUuid& key = it.key(); // Key is read-only
        TagContribution& value = it.value(); // Value is modifiable

        // If totalWithoutTags==0, we set weight to 0 since it is irrelevant
        if (totalWithoutTags==0) {
            value.weight = 0;
        } else{
            value.weight = value.amount/totalWithoutTags;
        }
    }


    return ;
}



AnalysisDialog::RelWeightGrouping AnalysisDialog::getGroupingTypeSelected()
{
    if (ui->incomesRelativeWeigthRadioButton->isChecked()==true) {
        return RelWeightGrouping::RWGROUPING_INCOMES;
    } else if (ui->expensesRelativeWeigthRadioButton->isChecked()==true){
        return RelWeightGrouping::RWGROUPING_EXPENSES;
    } else {
        // should never happen
        throw std::logic_error("Analysis Dialog : no group selected");
    }
}


void AnalysisDialog::on_monthlyReportChartDurationSpinBox_valueChanged(int arg1)
{
    redisplay_ReportChart(ReportType::MONTHLY);
}


void AnalysisDialog::on_monthlyReportChartRightToolButton_clicked()
{
    QDate start = QDate(ui->monthlyReportChartFromYearSpinBox->value(),
        ui->monthlyReportChartFromMonthComboBox->currentIndex()+1,1);
    start = start.addMonths(ui->monthlyReportChartDurationSpinBox->value());
    ui->monthlyReportChartFromYearSpinBox->setValue(start.year());
    ui->monthlyReportChartFromMonthComboBox->setCurrentIndex(start.month()-1);
    redisplay_ReportChart(ReportType::MONTHLY);
}


void AnalysisDialog::on_monthlyReportChartLeftToolButton_clicked()
{
    QDate start = QDate(ui->monthlyReportChartFromYearSpinBox->value(),
        ui->monthlyReportChartFromMonthComboBox->currentIndex()+1,1);
    start = start.addMonths(-ui->monthlyReportChartDurationSpinBox->value());
    ui->monthlyReportChartFromYearSpinBox->setValue(start.year());
    ui->monthlyReportChartFromMonthComboBox->setCurrentIndex(start.month()-1);
    redisplay_ReportChart(ReportType::MONTHLY);
}


void AnalysisDialog::on_monthlyReportChartFromMonthComboBox_currentIndexChanged(int index)
{
    redisplay_ReportChart(ReportType::MONTHLY);
}


void AnalysisDialog::on_monthlyReportChartFromYearSpinBox_valueChanged(int arg1)
{
    redisplay_ReportChart(ReportType::MONTHLY);
}



void AnalysisDialog::on_monthlyReportChartIncomesRadioButton_clicked()
{
    redisplay_ReportChart(ReportType::MONTHLY);
}


void AnalysisDialog::on_monthlyReportChartExpensesRadioButton_clicked()
{
    redisplay_ReportChart(ReportType::MONTHLY);
}


void AnalysisDialog::on_monthlyReportChartIncomesExpensesRadioButton_clicked()
{
    redisplay_ReportChart(ReportType::MONTHLY);
}


void AnalysisDialog::on_monthlyReportChartDeltasRadioButton_clicked()
{
    redisplay_ReportChart(ReportType::MONTHLY);
}


void AnalysisDialog::on_yearlyReportChartFromYearSpinBox_valueChanged(int arg1)
{
    redisplay_ReportChart(ReportType::YEARLY);
}


void AnalysisDialog::on_yearlyReportChartDurationSpinBox_valueChanged(int arg1)
{
    redisplay_ReportChart(ReportType::YEARLY);
}


void AnalysisDialog::on_yearlyReportChartLeftToolButton_clicked()
{
    QDate start = QDate(ui->yearlyReportChartFromYearSpinBox->value(),1,1);
    start = start.addYears(-ui->yearlyReportChartDurationSpinBox->value());
    ui->yearlyReportChartFromYearSpinBox->setValue(start.year());
    redisplay_ReportChart(ReportType::YEARLY);
}


void AnalysisDialog::on_yearlyReportChartRightToolButton_clicked()
{
    QDate start = QDate(ui->yearlyReportChartFromYearSpinBox->value(),1,1);
    start = start.addYears(ui->yearlyReportChartDurationSpinBox->value());
    ui->yearlyReportChartFromYearSpinBox->setValue(start.year());
    redisplay_ReportChart(ReportType::YEARLY);
}


void AnalysisDialog::on_yearlyReportChartIncomesRadioButton_clicked()
{
    redisplay_ReportChart(ReportType::YEARLY);
}


void AnalysisDialog::on_yearlyReportChartExpensesRadioButton_clicked()
{
    redisplay_ReportChart(ReportType::YEARLY);
}


void AnalysisDialog::on_yearlyReportChartIncomesExpensesRadioButton_clicked()
{
    redisplay_ReportChart(ReportType::YEARLY);
}


void AnalysisDialog::on_yearlyReportChartDeltasRadioButton_clicked()
{
    redisplay_ReportChart(ReportType::YEARLY);
}



bool AnalysisDialog::Bin::operator==(const Bin &o) const
{
    return (this->income==o.income) && (this->expense==o.expense) &&
           (this->cashBalance==o.cashBalance) && (this->delta==o.delta);
}


bool AnalysisDialog::Bin::operator!=(const Bin &o) const
{
    return !(*this==o);
}


void AnalysisDialog::on_incomesRelativeWeigthRadioButton_clicked()
{
    updateRelativeWeightChart();
    LOG_DEBUG_INFO("Analysis Dialog : Income radiobutton clicked");
}


void AnalysisDialog::on_expensesRelativeWeigthRadioButton_clicked()
{
    updateRelativeWeightChart();
    LOG_DEBUG_INFO("Analysis Dialog : Expense radiobutton clicked");
}


void AnalysisDialog::on_tagsSelectTagsPushButton_clicked()
{
    // send for display
    emit signalChooseTagsPrepareContent(availableTags, selectedTags.getFilterTagIdSet());
    selectTagsDlg->show();
}


// void AnalysisDialog::on_tagsFromDateEdit_userDateChanged(const QDate &date)
// {
//     redisplay_TagTable();
//     LOG_DEBUG_INFO(QString("Tags From date changed to %1").arg(date.toString()));
// }


// void AnalysisDialog::on_tagsToDateEdit_userDateChanged(const QDate &date)
// {
//     redisplay_TagTable();
//     LOG_DEBUG_INFO(QString("Tags To date changed to %1").arg(date.toString()));
// }


void AnalysisDialog::on_tagsIncomesRadioButton_clicked()
{
    redisplay_TagTable();
    LOG_DEBUG_INFO(QString("Analysis Dialog - Tags : Income radiobutton clicked"));
}


void AnalysisDialog::on_tagsExpensesRadioButton_clicked()
{
    redisplay_TagTable();
    LOG_DEBUG_INFO(QString("Analysis Dialog - Tags : Expense radiobutton clicked"));
}


bool AnalysisDialog::TagContribution::operator==(const TagContribution &o) const
{
    return ( (this->amount==o.amount) && (this->name==o.name) && (this->weight==o.weight) );
}

bool AnalysisDialog::TagContribution::operator!=(const TagContribution &o) const
{
    return !(*this==o);
}


bool AnalysisDialog::NumericTableWidgetItem::operator<(const QTableWidgetItem &other) const
{
    bool ok1, ok2;
    double v1 = data(Qt::UserRole).toDouble(&ok1);
    double v2 = other.data(Qt::UserRole).toDouble(&ok2);

    if (ok1 && ok2)
        return v1 < v2;
    // fallback to text compare
    return text() < other.text();
}


void AnalysisDialog::on_RW_ClearSelectionPushButton_clicked()
{
    ui->RW_listWidget->clearSelection();
    ui->RW_listWidget->setCurrentItem(nullptr);
    clearLegendWidgets();
}


void AnalysisDialog::on_globalExportCsvPushButton_clicked()
{
    int index = ui->tabWidget->currentIndex();

    switch (index) {
        case 0:
            // Legend of Relative Weight
            exportRelativeWeightLegendAsCsvFile();
            break;
        case 3:
            // Monthly table report
            exportTextMonthlyYearlyReport(ReportType::MONTHLY);
            break;
        case 4:
            // Annual table report
            exportTextMonthlyYearlyReport(ReportType::YEARLY);
            break;
        case 5:
            // Tags
            exportTagsAnalysisAsCsv();
            break;
        default:
            return;
            break;
    }
}


void AnalysisDialog::on_globalExportImagePushButton_clicked()
{
    int index = ui->tabWidget->currentIndex();

    switch (index) {
        case 0:
            // Relative Weight
            exportChartAsImage(ui->chartRelativeWeigthWidget, tr("Relative weight"));
            break;
        case 1:
            // Monthly report chart
            exportChartAsImage(ui->monthlyReportChartWidget, tr("Monthly report chart"));
            break;
        case 2:
            // Annual report chart
            exportChartAsImage(ui->yearlyReportChartWidget, tr("Annual report chart"));
            break;
        default:
            return;
            break;
    }
}


void AnalysisDialog::on_tabWidget_currentChanged(int index)
{
    switch (index) {
        case 0:
            // Relative Weight
            ui->globalExportCsvPushButton->setText(tr("Export legend..."));
            ui->globalExportCsvPushButton->setVisible(true);
            ui->globalExportImagePushButton->setVisible(true);
            break;
        case 1:
            // Monthly report chart
            ui->globalExportCsvPushButton->setVisible(false);
            ui->globalExportImagePushButton->setVisible(true);
            break;
        case 2:
            // Annual report chart
            ui->globalExportCsvPushButton->setVisible(false);
            ui->globalExportImagePushButton->setVisible(true);
            break;
        case 3:
            // Monthly table report
            ui->globalExportCsvPushButton->setText(tr("Export table..."));
            ui->globalExportCsvPushButton->setVisible(true);
            ui->globalExportImagePushButton->setVisible(false);
            break;
        case 4:
            // Annual table report
            ui->globalExportCsvPushButton->setText(tr("Export table..."));
            ui->globalExportCsvPushButton->setVisible(true);
            ui->globalExportImagePushButton->setVisible(false);
            break;
        case 5:
            // Tags
            ui->globalExportCsvPushButton->setText(tr("Export table..."));
            ui->globalExportCsvPushButton->setVisible(true);
            ui->globalExportImagePushButton->setVisible(false);
            break;
        default:
            return;
            break;
    }
}


void AnalysisDialog::on_RW_horizontalSlider_valueChanged(int value)
{
    int step = 30;
    int snapped = ((value + step/2) / step) * step;
    if (snapped != value){
        ui->RW_horizontalSlider->setValue(snapped);
    }
    // redraw the pie
    seriesRelativeWeigth->setPieStartAngle(snapped);
    seriesRelativeWeigth->setPieEndAngle(360+snapped);
}


void AnalysisDialog::on_RW_ShowLabelsCheckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    for (QPieSlice *s : seriesRelativeWeigth->slices()) {
        s->setLabelVisible(ui->RW_ShowLabelsCheckBox->isChecked());
    }
}


void AnalysisDialog::on_RW_listWidget_itemSelectionChanged()
{
    // remove stand out for all
    QList<QPieSlice *> list = seriesRelativeWeigth->slices();
    for (auto slice : seriesRelativeWeigth->slices()) {
        slice->setExploded(false);
    }

    // Clear Legend widgets
    clearLegendWidgets();

    // Get selection : row index is equivalent to rank
    QItemSelectionModel* selectionModel = ui->RW_listWidget->selectionModel();
    QModelIndexList selectedRows = selectionModel->selectedRows();

    // If there is element selected, just return
    if (selectedRows.size()==0) {
        return;
    }

    // make the slice stand out
    double cummulPercentage=0;
    double cummulAmount=0;
    uint lastRank=-1;
    double max = CurrencyHelper::maxValueAllowedForAmountInDouble(currInfo.noOfDecimal);
    foreach (const QModelIndex &index, selectedRows) {
        // set slice appearence
        QPieSlice *slice = seriesRelativeWeigth->slices().at(index.row());
        slice->setExploded(true);
        slice->setExplodeDistanceFactor(0.10); // 10% of radius
        // calculate cumulative amount and percentage
        LegendItemInfo info = index.data(Qt::UserRole).value<LegendItemInfo>();
        cummulAmount += info.amount;
        if(cummulAmount > max){
            cummulAmount = max;
        }
        cummulPercentage += info.percentage;
        lastRank = info.rank;
    }

    // fill the Legend widgets
    if (selectedRows.size()==1) {
        updateLegendWidgets(lastRank, cummulAmount, cummulPercentage);
    } else {
        // rank is irrelevant if more than 1 row selected
        updateLegendWidgets(-1, cummulAmount, cummulPercentage);
    }

}


void AnalysisDialog::on_RW_ApplyDatesPushButton_clicked()
{
    // Validate dates

    QDate from = ui->fromDateEdit->date();
    QDate to = ui->toDateEdit->date();
    QDate tomorrow = GbpController::getInstance().getTomorrow();

    if(from.isValid()==false){
        QString fromString = from.toString(Qt::ISODate);
        QString toString = to.toString(Qt::ISODate);
        QString s = QString(tr("\"From\" date %1 is invalid"))
            .arg(fromString);
        QMessageBox::critical(nullptr,tr("Error"),s.toLocal8Bit().data());

        // return to previous values
        ui->fromDateEdit->setDate(rwPreviousFromDate);
        ui->toDateEdit->setDate(rwPreviousToDate);

        return;
    } else if (to.isValid()==false){
        QString fromString = from.toString(Qt::ISODate);
        QString toString = to.toString(Qt::ISODate);
        QString s = QString(tr("\"To\" date %1 is invalid"))
            .arg(toString);
        QMessageBox::critical(nullptr,tr("Error"),s.toLocal8Bit().data());

        // return to previous values
        ui->fromDateEdit->setDate(rwPreviousFromDate);
        ui->toDateEdit->setDate(rwPreviousToDate);

        return;
    } else if (to < from){
        QString fromString = from.toString(Qt::ISODate);
        QString toString = to.toString(Qt::ISODate);
        QString s = QString(tr("\"To\" date %1 cannot occur before \"From\" date %2"))
            .arg(toString).arg(fromString);
        QMessageBox::critical(nullptr,tr("Error"),s.toLocal8Bit().data());

        // return to previous values
        ui->fromDateEdit->setDate(rwPreviousFromDate);
        ui->toDateEdit->setDate(rwPreviousToDate);

        return;
    } else if (from < tomorrow){
        QString fromString = from.toString(Qt::ISODate);
        QString tomorrowString = tomorrow.toString(Qt::ISODate);
        QString s = QString(tr("\"From\" date %1 cannot occur before \"tomorrow\" %2"))
            .arg(fromString).arg(tomorrowString);
        QMessageBox::critical(nullptr,tr("Error"),s.toLocal8Bit().data());

        // return to previous value
        ui->fromDateEdit->setDate(rwPreviousFromDate);
        ui->toDateEdit->setDate(rwPreviousToDate);

        return;
    } else if (to < tomorrow){
        QString toString = to.toString(Qt::ISODate);
        QString tomorrowString = tomorrow.toString(Qt::ISODate);
        QString s = QString(tr("\"To\" date %1 cannot occur before \"tomorrow\" %2"))
            .arg(toString).arg(tomorrowString);
        QMessageBox::critical(nullptr,tr("Error"),s.toLocal8Bit().data());

        // return to previous value
        ui->fromDateEdit->setDate(rwPreviousFromDate);
        ui->toDateEdit->setDate(rwPreviousToDate);

        return;
    }

    // Remember these new dates
    rwPreviousFromDate = from;
    rwPreviousToDate = to;

    // Make the required updates to the charts and legends
    updateRelativeWeightChart();
    LOG_DEBUG_INFO(QString("RW - From-To date changed to %1 %2")
        .arg(from.toString()).arg(to.toString()));

}


void AnalysisDialog::on_tags_ApplyDatesPushButton_clicked()
{

    // Validate dates

    QDate from = ui->tagsFromDateEdit->date();
    QDate to = ui->tagsToDateEdit->date();
    QDate tomorrow = GbpController::getInstance().getTomorrow();

    if(from.isValid()==false){
        QString fromString = from.toString(Qt::ISODate);
        QString toString = to.toString(Qt::ISODate);
        QString s = QString(tr("\"From\" date %1 is invalid"))
            .arg(fromString);
        QMessageBox::critical(nullptr,tr("Error"),s.toLocal8Bit().data());

        // return to previous values
        ui->tagsFromDateEdit->setDate(tagsPreviousFromDate);
        ui->tagsToDateEdit->setDate(tagsPreviousToDate);

        return;
    } else if (to.isValid()==false){
        QString fromString = from.toString(Qt::ISODate);
        QString toString = to.toString(Qt::ISODate);
        QString s = QString(tr("\"To\" date %1 is invalid"))
            .arg(toString);
        QMessageBox::critical(nullptr,tr("Error"),s.toLocal8Bit().data());
        // return to previous values
        ui->tagsFromDateEdit->setDate(tagsPreviousFromDate);
        ui->tagsToDateEdit->setDate(tagsPreviousToDate);

        return;
    } else if (to < from){
        QString fromString = from.toString(Qt::ISODate);
        QString toString = to.toString(Qt::ISODate);
        QString s = QString(tr("\"To\" date %1 cannot occur before \"From\" date %2"))
            .arg(toString).arg(fromString);
        QMessageBox::critical(nullptr,tr("Error"),s.toLocal8Bit().data());
        // return to previous values
        ui->tagsFromDateEdit->setDate(tagsPreviousFromDate);
        ui->tagsToDateEdit->setDate(tagsPreviousToDate);

        return;
    } else if (from < tomorrow){
        QString fromString = from.toString(Qt::ISODate);
        QString tomorrowString = tomorrow.toString(Qt::ISODate);
        QString s = QString(tr("\"From\" date %1 cannot occur before \"tomorrow\" %2"))
            .arg(fromString).arg(tomorrowString);
        QMessageBox::critical(nullptr,tr("Error"),s.toLocal8Bit().data());
        // return to previous values
        ui->tagsFromDateEdit->setDate(tagsPreviousFromDate);
        ui->tagsToDateEdit->setDate(tagsPreviousToDate);

        return;
    } else if (to < tomorrow){
        QString toString = to.toString(Qt::ISODate);
        QString tomorrowString = tomorrow.toString(Qt::ISODate);
        QString s = QString(tr("\"To\" date %1 cannot occur before \"tomorrow\" %2"))
            .arg(toString).arg(tomorrowString);
        QMessageBox::critical(nullptr,tr("Error"),s.toLocal8Bit().data());
        // return to previous values
        ui->tagsFromDateEdit->setDate(tagsPreviousFromDate);
        ui->tagsToDateEdit->setDate(tagsPreviousToDate);

        return;
    }

    // Remember these new dates
    tagsPreviousFromDate = from;
    tagsPreviousToDate = to;

    // Make the required updates to the tables
    redisplay_TagTable();
    LOG_DEBUG_INFO(QString("Tags - From-To date changed to %1 %2")
        .arg(from.toString()).arg(to.toString()));

}

