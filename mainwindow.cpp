/*
 *  Copyright (C) 2024 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QObject>
#include <QMessageBox>
#include <QLineSeries>
#include <QPalette>
#include <QPen>
#include <QColor>
#include "gbpcontroller.h"
#include "qevent.h"
#include "currencyhelper.h"
#include "editscenariodialog.h"
#include "util.h"
#include "qcustomplot.h"





MainWindow::MainWindow(QLocale systemLocale, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow), locale(systemLocale)
{
    // build UI
    ui->setupUi(this);

    // create child dialogs
    editScenarioDlg = new EditScenarioDialog(locale,this); //  auto destroyed by Qt
    editScenarioDlg->setModal(false);
    selectCountryDialog = new SelectCountryDialog(locale, this); // auto destroyed by Qt
    selectCountryDialog->setModal(true);
    optionsDlg = new OptionsDialog(this); //  auto destroyed by Qt
    optionsDlg->setModal(false);
    aboutDlg = new AboutDialog(this);
    aboutDlg->setModal(true);
    analysisDlg = new AnalysisDialog(locale, this);
    analysisDlg->setModal(true);
    dateIntervalDlg = new DateIntervalDialog(this);
    dateIntervalDlg->setModal(true);

    // rebuild "recent files" submenu (settings must have been loaded before)
    recentFilesMenuInit();
    recentFilesMenuUpdate();

    // resize some QLabel to be sure we have enough space to display stuff
    QFontMetrics fm(ui->ciDateLabel->font());
    // ui->ciDateLabel->setMinimumWidth(fm.averageCharWidth()*10);
    ui->ciAmountLabel->setMinimumWidth(fm.averageCharWidth()*15);
    ui->ciDeltaLabel->setMinimumWidth(fm.averageCharWidth()*10);
    // ui->baselineDoubleSpinBox->setMinimumWidth(fm.averageCharWidth()*6);

    // set minimum/maximum value of baseline and erase currency label
    ui->baselineDoubleSpinBox->setMaximum(CurrencyHelper::maxValueAllowedForAmountInDouble(3)); // no currency has more than 3 decinal digits
    ui->baselineDoubleSpinBox->setMinimum(-CurrencyHelper::maxValueAllowedForAmountInDouble(3)); // no currency has more than 3 decinal digits
    ui->baselineCurrencyLabel->setText("");

    // display todays's date in bottom startAmountLabel
    ui->startAmountLabel->setText(tr("Start Amount for Today %1 :").arg(
        locale.toString(GbpController::getInstance().getToday(),"yyyy-MMM-dd")));

    // set scaling factor from settings
    chartScalingFactor = 1 + (GbpController::getInstance().getPercentageMainChartScaling())/100.0;

    // use smaller font for Info list and enable custom sorting
    QFont listFont = ui->ciDetailsListWidget->font();
    uint oldFontSize = listFont.pointSize();
    uint newFontSize = Util::changeFontSize(false,true, oldFontSize);
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Main Window - FE List - Font size from %1 to %2").arg(oldFontSize).arg(newFontSize));
    listFont.setPointSize(newFontSize);
    ui->ciDetailsListWidget->setFont(listFont);
    ui->ciDetailsListWidget->setSortingEnabled(true);

    // use smaller font for "resize" toolbar buttons
    QFont resizeToolbarFont = ui->toolButton_1M->font();
    oldFontSize = resizeToolbarFont.pointSize();
    newFontSize = Util::changeFontSize(false,true, oldFontSize);
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Main Window - Toolbar - Font size from %1 to %2").arg(oldFontSize).arg(newFontSize));
    resizeToolbarFont.setPointSize(newFontSize);
    ui->toolButton_1M->setFont(resizeToolbarFont);
    ui->toolButton_3M->setFont(resizeToolbarFont);
    ui->toolButton_6M->setFont(resizeToolbarFont);
    ui->toolButton_1Y->setFont(resizeToolbarFont);
    ui->toolButton_2Y->setFont(resizeToolbarFont);
    ui->toolButton_3Y->setFont(resizeToolbarFont);
    ui->toolButton_5Y->setFont(resizeToolbarFont);
    ui->toolButton_10Y->setFont(resizeToolbarFont);
    ui->toolButton_15Y->setFont(resizeToolbarFont);
    ui->toolButton_20Y->setFont(resizeToolbarFont);
    ui->toolButton_25Y->setFont(resizeToolbarFont);
    ui->toolButton_50Y->setFont(resizeToolbarFont);
    ui->toolButton_Fit->setFont(resizeToolbarFont);
    ui->toolButton_Left->setFont(resizeToolbarFont);
    ui->toolButton_Right->setFont(resizeToolbarFont);
    ui->toolButton_Max->setFont(resizeToolbarFont);
    ui->customToolButton->setFont(resizeToolbarFont);

    // configure splitter
    ui->splitter->setCollapsible(0,false);
    ui->splitter->setCollapsible(1,false);
    ui->splitter->setStretchFactor(0,1);
    ui->splitter->setStretchFactor(1,0);

    // ***********************************************
    // *** CHART SETTINGS ***
    // ***********************************************

    // build dummy data representing "no scenario" situation
    QVector<QCPGraphData> timeData(0);// dummy data for "no scenario"
    // intitial values for axes limits
    fullFromDateX = GbpController::getInstance().getTomorrow(); // we are interested only from TOMORROW to infinity
    fullToDateX = fullFromDateX.addYears(GbpController::getInstance().getScenarioMaxYearsSpan()).addDays(-1);
    fullFromDoubleX = Util::dateToDateTimeLocal(fullFromDateX,QTimeZone::systemTimeZone()).toSecsSinceEpoch();
    fullToDoubleX = Util::dateToDateTimeLocal(fullToDateX,QTimeZone::systemTimeZone()).toSecsSinceEpoch();
    fitFromDoubleX = fullFromDoubleX;
    fitToDoubleX = fullToDoubleX;
    fullMinY = 0;
    fullMaxY = 1;
    // create graph and assign data to it:
    ui->curveWidget->installEventFilter(this);  // intercept resize event
    chart = new QCustomPlot(ui->curveWidget);
    chart->addGraph();
    chart->graph()->setLineStyle(QCPGraph::lsLine);
    chart->graph()->data()->set(timeData, true);
    chart->xAxis2->setVisible(false);
    chart->yAxis2->setVisible(false);
    // configure bottom axis to show date instead of number:
    QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
    dateTicker->setDateTimeFormat("d MMM\nyyyy");
    dateTicker->setTickStepStrategy(QCPAxisTicker::TickStepStrategy::tssMeetTickCount);// doesnt seem to work very well..
    dateTicker->setTickCount(10);
    chart->xAxis->setTicker(dateTicker);
    // keep default left axis, configure some stuff
    chart->yAxis->ticker()->setTickCount(10);
    chart->yAxis->setNumberFormat("f");
    chart->yAxis->setNumberPrecision(2);
    // set a more compact font size for bottom and left axis tick labels:
    QFont f = chart->xAxis->tickLabelFont();
    chart->xAxis->setTickLabelFont(QFont(f.family(), (8.0/12.0)*f.pointSize()));
    chart->yAxis->setTickLabelFont(QFont(f.family(), (8.0/12.0)*f.pointSize()));
    // set axis ranges to show all data:
    chart->xAxis->setRange(fullFromDoubleX, fullToDoubleX);
    chart->yAxis->setRange(fullMinY, fullMaxY);
    chart->yAxis->setVisible(false);
    chart->xAxis->setVisible(false);
    chart->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    // set title
    setChartTitle(tr("No Scenario Loaded"));
    // set locale so Y axis is displaying separator and decimal point correctly
    chart->setLocale(locale);
    // set data point shape
    chart->graph()->setLineStyle(QCPGraph::LineStyle::lsStepLeft);
    chart->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 10));
    // configure point selection
    chart->graph()->setSelectable(QCP::stSingleData);
    QObject::connect(chart, &QCustomPlot::selectionChangedByUser, this, &MainWindow::selectionChangedByUser);
    // register event listener for axis scale change
    QObject::connect(chart->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(rangeChangedX(QCPRange)));
    QObject::connect(chart->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(rangeChangedY(QCPRange)));

    // configure dark or light mode for chart
    setChartColors();
    // build
    chart->replot();
    // ***********************************************
    // ***********************************************

    emptyDailyInfoSection();
    setVisibilityZoomButtons();

    //
    // connect MainWindow and edit scenario dialog
    QObject::connect(this, &MainWindow::signalEditScenarioPrepareContent, editScenarioDlg, &EditScenarioDialog::slotPrepareContent);
    QObject::connect(editScenarioDlg, &EditScenarioDialog::signalEditScenarioResult, this, &MainWindow::slotEditScenarioResult );
    QObject::connect(editScenarioDlg, &EditScenarioDialog::signalEditScenarioCompleted, this, &MainWindow::slotEditScenarioCompleted );
    // connect Mainwindow and select country dialog
    QObject::connect(this, &MainWindow::signalSelectCountryPrepareContent, selectCountryDialog, &SelectCountryDialog::slotPrepareContent);
    QObject::connect(selectCountryDialog, &SelectCountryDialog::signalSelectCountryResult, this, &MainWindow::slotSelectCountryResult );
    QObject::connect(selectCountryDialog, &SelectCountryDialog::signalSelectCountryCompleted, this, &MainWindow::slotSelectCountryCompleted );
    // connect MainWindow and options dialog
    QObject::connect(this, &MainWindow::signalOptionsPrepareContent, optionsDlg, &OptionsDialog::slotPrepareContent);
    QObject::connect(optionsDlg, &OptionsDialog::signalOptionsResult, this, &MainWindow::slotOptionsResult );
    QObject::connect(optionsDlg, &OptionsDialog::signalOptionsCompleted, this, &MainWindow::slotOptionsCompleted );
    // connect MainWindow and Analysis dialog
    QObject::connect(this, &MainWindow::signalAnalysisPrepareContent, analysisDlg, &AnalysisDialog::slotAnalysisPrepareContent);
    // connect MainWindow and DateInterval dialog
    QObject::connect(this, &MainWindow::signalDateIntervalPrepareContent, dateIntervalDlg, &DateIntervalDialog::slotPrepareContent);
    QObject::connect(dateIntervalDlg, &DateIntervalDialog::signalDateIntervalResult, this, &MainWindow::slotDateIntervalResult );
    QObject::connect(dateIntervalDlg, &DateIntervalDialog::signalDateIntervalCompleted, this, &MainWindow::slotDateIntervalCompleted );
}


MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Resize && object == ui->curveWidget){
        chart->resize(ui->curveWidget->size());
    }
    return QObject::eventFilter(object, event);
}




void MainWindow::setChartTitle(QString theTitle)
{
    if (chartTitle==nullptr) {
        // first time setting
        chartTitle = new QCPTextElement(chart);
        QFont f = chartTitle->font();   // default
        chartTitle->setText(theTitle);
        chartTitle->setFont(QFont(f.family(), f.pointSize(), QFont::Bold));
        // then we add it to the main plot layout:
        chart->plotLayout()->insertRow(0); // insert an empty row above the axis rect
        chart->plotLayout()->addElement(0, 0, chartTitle); // place the title in the empty cell we've just created
    } else {
        chartTitle->setText(theTitle);
    }
}


void MainWindow::setVisibilityZoomButtons()
{
    qint16 years = GbpController::getInstance().getScenarioMaxYearsSpan();
    if (years>=50) {
        ui->toolButton_50Y->setVisible(true);
    }else{
        ui->toolButton_50Y->setVisible(false);
    }
    if (years>=25) {
        ui->toolButton_25Y->setVisible(true);
    }else{
        ui->toolButton_25Y->setVisible(false);
    }
    if (years>=20) {
        ui->toolButton_20Y->setVisible(true);
    }else{
        ui->toolButton_20Y->setVisible(false);
    }
    if (years>=15) {
        ui->toolButton_15Y->setVisible(true);
    }else{
        ui->toolButton_15Y->setVisible(false);
    }
    if (years>=10) {
        ui->toolButton_10Y->setVisible(true);
    }else{
        ui->toolButton_10Y->setVisible(false);
    }
    if (years>=5) {
        ui->toolButton_5Y->setVisible(true);
    }else{
        ui->toolButton_5Y->setVisible(false);
    }
    if (years>=3) {
        ui->toolButton_3Y->setVisible(true);
    }else{
        ui->toolButton_3Y->setVisible(false);
    }
    if (years>=2) {
        ui->toolButton_2Y->setVisible(true);
    }else{
        ui->toolButton_2Y->setVisible(false);
    }
    if (years>=1) {
        ui->toolButton_1Y->setVisible(true);
    }else{
        ui->toolButton_1Y->setVisible(false);
    }

}


void MainWindow::rescaleYaxis(uint noOfMonths)
{
    QDate newTo = fullFromDateX.addMonths(noOfMonths).addDays(-1);
    QDateTime newToDateTime= Util::dateToDateTimeLocal(newTo, QTimeZone::systemTimeZone());
    double newToDouble = newToDateTime.toSecsSinceEpoch();
    chart->xAxis->setRange(fullFromDoubleX, newToDouble);
    chart->graph(0)->rescaleValueAxis(false,true);
    // give some space around min/max
    chart->xAxis->scaleRange(chartScalingFactor);
    chart->yAxis->scaleRange(chartScalingFactor);

    chart->replot();
}


void MainWindow::shiftGraph(bool toTheRight)
{
    QCPRange range = chart->xAxis->range();
    double xFromOrig = range.lower;
    double xToOrig = range.upper;
    double xFromNew;
    double xToNew;
    double deltaRange = fabs(xToOrig-xFromOrig);
    if (toTheRight){
        xFromNew = xFromOrig + deltaRange;
        xToNew = xToOrig + deltaRange;
    } else{
        xFromNew = xFromOrig - deltaRange;
        xToNew = xToOrig - deltaRange;
    }
    //
    chart->xAxis->setRange(xFromNew, xToNew);
    chart->graph(0)->rescaleValueAxis(false,true);
    // give some space around min/max
    chart->yAxis->scaleRange(chartScalingFactor);

    chart->replot();
}


void MainWindow::setChartColors()
{

    if(GbpController::getInstance().getChartDarkMode()){
        chartTitle->setTextColor(QColor(255, 255, 255));
        chart->graph()->setPen(QPen(GbpController::getInstance().getCurveDarkModeColor()));
        chart->setBackground(Qt::black);
        chart->xAxis->setBasePen(QPen(QColor(255, 255, 255)));
        chart->xAxis->setTickPen(QPen(QColor(255, 255, 255)));
        chart->xAxis->setSubTickPen(QPen(QColor(255, 255, 255)));
        chart->yAxis->setBasePen(QPen(QColor(255, 255, 255)));
        chart->yAxis->setTickPen(QPen(QColor(255, 255, 255)));
        chart->yAxis->setSubTickPen(QPen(QColor(255, 255, 255)));
        chart->xAxis->setTickLabelColor(QColor(255, 255, 255));
        chart->yAxis->setTickLabelColor(QColor(255, 255, 255));
        chart->xAxis->grid()->setPen(QColor(35, 35, 35));
        chart->yAxis->grid()->setPen(QColor(35, 35, 35));
    } else {
        chartTitle->setTextColor(QColor(0, 0, 0));
        chart->graph()->setPen(QPen(GbpController::getInstance().getCurveLightModeColor()));
        chart->setBackground(Qt::white);
        chart->xAxis->setBasePen(QPen(QColor(0, 0, 0)));
        chart->xAxis->setTickPen(QPen(QColor(0, 0, 0)));
        chart->xAxis->setSubTickPen(QPen(QColor(0, 0, 0)));
        chart->yAxis->setBasePen(QPen(QColor(0, 0, 0)));
        chart->yAxis->setTickPen(QPen(QColor(0, 0, 0)));
        chart->yAxis->setSubTickPen(QPen(QColor(0, 0, 0)));
        chart->xAxis->setLabelColor(QColor(0, 0, 0));
        chart->yAxis->setLabelColor(QColor(0, 0, 0));
        chart->xAxis->setTickLabelColor(QColor(0, 0, 0));
        chart->yAxis->setTickLabelColor(QColor(0, 0, 0));
        chart->xAxis->grid()->setPen(QColor(230, 230, 230));
        chart->yAxis->grid()->setPen(QColor(230, 230, 230));
    }
}


void MainWindow::fillDailyInfoSection(const QDate& date, double amount, const CombinedFeStreams::DailyInfo& di)
{
    ui->ciDateLabel->setText(locale.toString(date,"yyyy-MMM-dd"));
    QColor red = QColor(210,0,0);
    QColor green = QColor(0,190,0);

    QString countryCode = GbpController::getInstance().getScenario()->getCountryCode();
    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromCountryCode(locale, countryCode, found);
    if(!found){
        // should never happen
        return;
    }
    QString totalDelta = locale.toString(di.totalDelta,'f', currInfo.noOfDecimal);
    ui->ciDeltaLabel->setText(totalDelta);
    QPalette palette = ui->ciDeltaLabel->palette();
    if (di.totalDelta > 0) {
        palette.setColor(QPalette::WindowText, green);
        ui->ciDeltaLabel->setPalette(palette);
    } else {
        palette.setColor(QPalette::WindowText, red);
        ui->ciDeltaLabel->setPalette(palette);
    }
    QString amountString = locale.toString(amount,'f', currInfo.noOfDecimal);
    ui->ciAmountLabel->setText(amountString);
    palette = ui->ciAmountLabel->palette();
    if (amount > 0) {
        palette.setColor(QPalette::WindowText, green);
        ui->ciAmountLabel->setPalette(palette);
    } else {
        palette.setColor(QPalette::WindowText, red);
        ui->ciAmountLabel->setPalette(palette);
    }

    bool streamFound;
    QSharedPointer<Scenario> sc = GbpController::getInstance().getScenario();

    // list box
    // we need this in ordert o sort the list by StreamDef name
    class CustomListItem : public QListWidgetItem {
    public:
        CustomListItem(const QString& text, QString theName) : QListWidgetItem(text) {
            this->setData(Qt::UserRole, theName);
        }
        bool operator<(const QListWidgetItem& other) const {
            QString theName = this->data(Qt::UserRole).toString();
            QString otherName = other.data(Qt::UserRole).toString();
            if ( QString::localeAwareCompare(theName,otherName) < 0 ){
                return true;
            } else {
                return false;
            }
        }
    };
    ui->ciDetailsListWidget->clear();
    QString sName;
    QColor color;
    QListWidgetItem *item;
    ui->ciDetailsListWidget->clear();
    foreach(FeDisplay fed, di.incomesList){
        sc->getStreamDefNameAndColorFromId(fed.id, sName, color, streamFound);
        if(found){// should always be found
            item = new CustomListItem(fed.toString(sName, currInfo, locale),sName);
            if( (GbpController::getInstance().getAllowDecorationColor()==true) && (color.isValid())){
                item->setForeground(color);
            }
            ui->ciDetailsListWidget->addItem(item) ;    // list widget will take ownership of the item
        }
    }
    foreach(FeDisplay fed, di.expensesList){
        sc->getStreamDefNameAndColorFromId(fed.id, sName, color, streamFound);
        if(found){// should always be found
            item = new CustomListItem(fed.toString(sName, currInfo, locale), sName);
            if( (GbpController::getInstance().getAllowDecorationColor()==true) && (color.isValid())){
                item->setForeground(color);
            }
            ui->ciDetailsListWidget->addItem(item) ;    // list widget will take ownership of the item
        }
     }

}


void MainWindow::emptyDailyInfoSection()
{
    ui->ciDateLabel->setText("");
    ui->ciDeltaLabel->setText("");
    ui->ciAmountLabel->setText("");
    ui->ciDetailsListWidget->clear();
}


// current Scenario has changed, update the baseline widgets
void MainWindow::updateBaselineWidgets(CurrencyInfo currInfo)
{
    ui->baselineDoubleSpinBox->setMaximum(CurrencyHelper::maxValueAllowedForAmountInDouble(currInfo.noOfDecimal));
    ui->baselineDoubleSpinBox->setMinimum(-CurrencyHelper::maxValueAllowedForAmountInDouble(currInfo.noOfDecimal));
    ui->baselineDoubleSpinBox->setDecimals(currInfo.noOfDecimal);
    ui->baselineCurrencyLabel->setText(currInfo.isoCode);
    ui->baselineDoubleSpinBox->setValue(0);
}


// Compared current scenario in memory with its counterpart on disk. If an error (i.e. mismatch)
// occured, true is returned in "match".
// If no scenario loaded, match = true
void MainWindow::checkIfCurrentScenarioMatchesDiskVersion(bool &match) const {
    match = true;
    // anything loaded ?
    if ( GbpController::getInstance().isScenarioLoaded() == false ){
        match = true;
        return; // no scenario load matches with "empty" disk file
    }
    // current scenario is a new scenario, so it is a mismatched by default
    if ( GbpController::getInstance().getFullFileName()== "" ){
        match = false;
        return;
    }
    // load scenario on disk and compare to what we have in memory
    Scenario::FileResult result = Scenario::loadFromFile(GbpController::getInstance().getFullFileName());
    if (result.code != Scenario::FileResultCode::SUCCESS){
        // unexpected error (should not happen normally at this point), declare that it matches
        match = true;
        return;
    }
    if ( (*result.scenarioPtr)== (*GbpController::getInstance().getScenario()) ){
        match = true;
        return;
    } else {
        match = false;
        return;
    }
}


// this generate a "close-event"
void MainWindow::on_actionQuit_triggered()
{
    QApplication::quit();
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    // check first if the current scenario needs to be saved
    bool match;
    checkIfCurrentScenarioMatchesDiskVersion(match);
    if (match==false){
        if ( QMessageBox::StandardButton::Yes == QMessageBox::question(this,tr("Modifications not saved"),
                tr("Current scenario has been modified, but not saved on disk. Do you want to SAVE THE CHANGES before going forward ?")) ){
            on_actionSave_triggered();  // save current scenario
        }
    } else {
        if ( QMessageBox::StandardButton::Yes != QMessageBox::question(this,tr("About to quit"),
                tr("Do you really want to quit the application ?")) ){
            event->ignore();
            return;
        }
    }

    // proceed with quitting the application
    GbpController::getInstance().saveSettings();
    event->accept();
}


void MainWindow::on_actionAbout_triggered()
{
    aboutDlg->show();
}


void MainWindow::on_actionAbout_Qt_triggered()
{
    QApplication::aboutQt();
}


void MainWindow::on_actionOpen_triggered()
{
    // check first if the current scenario needs to be saved
    bool match;
    checkIfCurrentScenarioMatchesDiskVersion(match);
    if (match==false){
        if ( QMessageBox::StandardButton::Yes == QMessageBox::question(this,tr("Modifications not saved"),
                tr("Current scenario has been modified, but not saved on disk. Do you want to SAVE THE CHANGES before going forward ?")) ){
            on_actionSave_triggered();  // save current scenario
        }
    }

    QSharedPointer<Scenario> scenario;
    QString dir = GbpController::getInstance().getLastDir();
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open an Existing Scenario"),dir);
    if (fileName != ""){
        // load the scenario file
        bool success = loadScenarioFile(fileName);
        // if successfull, add it to the recent files list
        if (success) {
            GbpController::getInstance().recentFilenamesAdd(fileName, maxRecentFiles);
            recentFilesMenuUpdate();
        }
    }
}


// a scenario must be loaded for that function to work
void MainWindow::on_actionSave_As_triggered()
{
    if ( !(GbpController::getInstance().isScenarioLoaded()) ){
        QMessageBox::critical(nullptr,tr("Saving Scenario Failed"),tr("No scenario loaded yet"));
        return;
    }

    QString dir = GbpController::getInstance().getLastDir();
    QString defaultExtension = ".json";
    QString defaultExtensionUsed = ".json";
    QString filter = "GBP Files (*.json);;All Files (*.*)";
    QString fileName = QFileDialog::getSaveFileName(this,tr("Choose Scenario Filename"),dir,filter,&defaultExtensionUsed);
    if(fileName!=""){
        // fix the filename to add the proper suffix
        QFileInfo fi(fileName);
        if(fi.suffix()==""){    // user has not specified an extension
            fileName.append(defaultExtension);
        }
        // remember last dir
        QFileInfo fileInfo(fileName);
        GbpController::getInstance().setLastDir(fileInfo.path());
        // save the current scenario file
        bool success = saveScenario(fileName);
        if (success){
            // switch scenario
            GbpController::getInstance().setFullFileName(fileName);
            // update the filename label
            //changeFilenameLabel(fileName);
            // update status bar
            msgStatusbar(tr("Scenario saved successfully"));
            // add it to the recent files list
            GbpController::getInstance().recentFilenamesAdd(fileName, maxRecentFiles);
            recentFilesMenuUpdate();
        }
    }
}





// a scenario must be loaded for that function to work
// and a filename must have already been assigned
void MainWindow::on_actionSave_triggered()
{
    if ( !(GbpController::getInstance().isScenarioLoaded())){
        // no scenario yet, must specify the file name
        QMessageBox::critical(nullptr,tr("Saving Scenario Failed"),tr("No scenario loaded yet : nothing to save"));
        return;
    }
    QString fileName = GbpController::getInstance().getFullFileName();
    if ( fileName == "" ){
        // no file name assigned : use SaveAs instead
        MainWindow::on_actionSave_As_triggered();
        return;
    }
    if ( true == saveScenario(fileName) ){
        // update status bar
        msgStatusbar(tr("Scenario saved successfully"));
    }
}


// return true if successful, false otherwise
bool MainWindow::loadScenarioFile(QString fileName)
{
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("Attempting to load scenario from file \"%1\" ...").arg(fileName));

    QString errorStringUI;
    QString errorStringLog;
    Scenario::FileResult fr ;
    try {
        fr = Scenario::loadFromFile(fileName);
        if (fr.code != Scenario::SUCCESS){
            switch (fr.code) {
            case Scenario::LOAD_CANNOT_OPEN_FILE:
                errorStringUI = fr.errorStringUI;
                errorStringLog = fr.errorStringLog;
                break;
            case Scenario::LOAD_FILE_DOES_NOT_EXIST:
                errorStringUI = fr.errorStringUI;
                errorStringLog = fr.errorStringLog;
                break;
            case Scenario::LOAD_JSON_PARSING_ERROR:
                errorStringUI = fr.errorStringUI;
                errorStringLog = fr.errorStringLog;
                break;
            case Scenario::LOAD_JSON_SEMANTIC_ERROR:
                errorStringUI = QString(tr("Error found in the file content.\n\nDetails : %1")).arg(fr.errorStringUI);
                errorStringLog = QString("Error found in the file content.\n\nDetails : %1").arg(fr.errorStringLog);
                break;
            default:
                errorStringUI = fr.errorStringUI;
                errorStringLog = fr.errorStringLog;
                break;
            }
            QMessageBox::critical(nullptr,tr("Loading Scenario Failed"), errorStringUI);
            GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Error, QString("Loading scenario failed :  error code = %1 , error message = %2").arg(fr.code).arg(fr.errorStringLog));
            return false;
        }
    } catch(...){
        std::exception_ptr p = std::current_exception();
        errorStringUI = QString(tr("An unexpected error has occured.\n\nDetails : %1")).arg((p ? p.__cxa_exception_type()->name() : "null"));
        QMessageBox::critical(nullptr,"Loading Scenario Failed", errorStringUI);
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Error, QString("Loading scenario failed : unexpected exception occured : %1").arg((p ? p.__cxa_exception_type()->name() : "null")));
        return false;
    }
    // switch scenario
    GbpController::getInstance().setScenario(fr.scenarioPtr);
    GbpController::getInstance().setFullFileName(fileName);

    // update the "last directory" used in settings
    QFileInfo fi(fileName);
    GbpController::getInstance().setLastDir(fi.path());

    // update the scenario data
    updateScenarioDataDisplayed(true,true);

    // update status bar
    msgStatusbar(tr("Scenario opened successfully"));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Scenario loaded successfully"));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    Name = %1").arg(fr.scenarioPtr->getName()));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    Country ISO code = %1").arg(fr.scenarioPtr->getCountryCode()));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    Version = %1").arg(fr.scenarioPtr->getVersion()));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    No of periodic incomes = %1").arg(fr.scenarioPtr->getIncomesDefPeriodic().size()));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    No of irregular incomes = %1").arg(fr.scenarioPtr->getIncomesDefIrregular().size()));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    No of periodic expenses = %1").arg(fr.scenarioPtr->getExpensesDefPeriodic().size()));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    No of irregular expenses = %1").arg(fr.scenarioPtr->getExpensesDefIrregular().size()));

    // Get currency info for the current scenario about to be edited
    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromCountryCode(locale, fr.scenarioPtr->getCountryCode(), found);
    if(!found){
        return false; // should never happen
    }

    // display Edit Scenario Dialog
    emit signalEditScenarioPrepareContent(false, fr.scenarioPtr->getCountryCode(),currInfo);

    return true;// full success
}


// Save current scenario under the provided filename.
// return true if successful, false otherwise
bool MainWindow::saveScenario(QString fileName){
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario();
    QString errorStringUI;
    QString errorStringLog;

    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("Attempting to save scenario \"%1\" under file name \"%2\"").arg(scenario->getName()).arg(fileName));

    try {
        Scenario::FileResult fr;
        fr = scenario->saveToFile(fileName);
        if (fr.code != Scenario::SUCCESS){
            switch (fr.code) {
            case Scenario::SAVE_ERROR_CREATING_FILE_FOR_WRITING:
                errorStringUI = fr.errorStringUI;
                errorStringLog = fr.errorStringLog;
                break;
            case Scenario::SAVE_ERROR_OPENING_FILE_FOR_WRITING:
                errorStringUI = fr.errorStringUI;
                errorStringLog = fr.errorStringLog;
                break;
            case Scenario::SAVE_ERROR_WRITING_TO_FILE:
                errorStringUI = fr.errorStringUI;
                errorStringLog = fr.errorStringLog;
                break;
            case Scenario::SAVE_ERROR_INTERNAL_JSON_CREATION:
                errorStringUI = QString(tr("Error no %1 has occured.\n\nDetails : %2")).arg(fr.code).arg(fr.errorStringUI);
                errorStringLog = QString("Error no %1 has occured.\n\nDetails : %2").arg(fr.code).arg(fr.errorStringLog);
                break;
            default:
                errorStringUI = QString(tr("Error no %1 has occured.\n\nDetails : %2")).arg(fr.code).arg(fr.errorStringUI);
                errorStringLog = QString("Error no %1 has occured.\n\nDetails : %2").arg(fr.code).arg(fr.errorStringLog);
                break;
            }
            QMessageBox::critical(nullptr,tr("Saving Scenario Failed"), errorStringUI);
            GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Warning, QString("Saving scenario failed : error code=%1  error message=%2").arg(fr.code).arg(fr.errorStringLog));
            return false;
        }
    } catch(...){
        std::exception_ptr p = std::current_exception();
        errorStringUI = QString(tr("An unexpected error has occured.\n\nDetails : %1")).arg((p ? p.__cxa_exception_type()->name() : "null"));
        QMessageBox::critical(nullptr,"Saving Scenario Failed",errorStringUI);
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Warning, QString("Saving scenario failed, unexpected exception occured : %1").arg((p ? p.__cxa_exception_type()->name() : "null")));
        return false;
    }

    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Scenario saved successully"));

    // update the "last directory" used in settings
    QFileInfo fi(fileName);
    GbpController::getInstance().setLastDir(fi.path());

    return true;
}


void MainWindow::on_actionEdit_triggered()
{
    if ( !(GbpController::getInstance().isScenarioLoaded()) ){
        QMessageBox::critical(nullptr,tr("Edit Scenario Failed"),tr("No scenario loaded"));
        return;
    }
    // Get currency info for the curent scenario about to be edited
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario();
    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromCountryCode(locale, scenario->getCountryCode(), found);
    if(!found){
        return; // should never happen
    }
    // display Edit Scenario Dialog
    emit signalEditScenarioPrepareContent(false, scenario->getCountryCode(), currInfo);
    editScenarioDlg->show();
    editScenarioDlg->activateWindow();
}


void MainWindow::on_actionNew_triggered()
{
    // check first if the current scenario needs to be saved
    bool match;
    checkIfCurrentScenarioMatchesDiskVersion(match);
    if (match==false){
        if ( QMessageBox::StandardButton::Yes == QMessageBox::question(this,tr("Modifications not saved"),
                tr("Current scenario has been modified, but not saved on disk. Do you want to SAVE THE CHANGES before going forward ?")) ){
            on_actionSave_triggered();  // save current scenario
        }
    }

    // First, select a currency : see slot "slotCountryHasBeenSelected" for the follow up
    emit signalSelectCountryPrepareContent();
    selectCountryDialog->show();
}


// All data need to be fully recalculated. Chart has to be rescaled and replotted
void MainWindow::updateScenarioDataDisplayed(bool rescaleXaxis, bool resetBaselineValue)
{

    // recalculate X axis limits ( "Options->Scenario years" may have changed)
    QTimeZone tz = QTimeZone::systemTimeZone();
    fullFromDateX = GbpController::getInstance().getTomorrow(); // Always TOMORROW
    fullToDateX = fullFromDateX.addYears(GbpController::getInstance().getScenarioMaxYearsSpan()).addDays(-1);
    fullFromDoubleX = Util::dateToDateTimeLocal(fullFromDateX,tz).toSecsSinceEpoch();
    fullToDoubleX = Util::dateToDateTimeLocal(fullToDateX,tz).toSecsSinceEpoch();
    QDate fitFromDateX = fullFromDateX;
    QDate fitToDateX = fullToDateX;
    fitFromDoubleX = fullFromDoubleX;
    fitToDoubleX = fullToDoubleX;

    if (!(GbpController::getInstance().isScenarioLoaded())){
        // *** if no scenario loaded ***  => may happen if Options->Scenario years changes without a first scenario loaded
        chart->xAxis->setRange(fullFromDoubleX, fullToDoubleX);
        chart->replot();
        return;
    } else{
        // *** a scenario is loaded ***

        // empty DailyInfo section
        emptyDailyInfoSection();

        // unselect any point selected
        chart->graph()->setSelection(QCPDataSelection());        // make sure axis are visible

        if ( chart->yAxis->visible() == false ){
            chart->yAxis->setVisible(true);
        }
        if ( chart->xAxis->visible() == false ){
            chart->xAxis->setVisible(true);
        }

        QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario();
        bool found;
        CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromCountryCode(locale, scenario->getCountryCode(), found);
        if(!found){
            return; // should never happen
        }

        // re-configure baseline edit widget if a new scenario has been loaded or created (currency may have changed)
        if( resetBaselineValue==true ){
            updateBaselineWidgets(currInfo);
        }

        // regenerate all the data from the Scenario (can be expensive)
        uint saturationNo;
        chartRawData = scenario->generateFinancialEvents(locale, DateRange(fullFromDateX, fullToDateX),saturationNo);

        // init min/max for later stage
        fullMinY = std::numeric_limits<double>::max();
        fullMaxY = std::numeric_limits<double>::min();
        if (chartRawData.size()==0){
            // no data
            fullMinY = 0;
            fullMaxY = 1;
        }

        // remember current X range (for baseline value change)
        QCPRange oldXrange = chart->xAxis->range();

        // *** Completely rebuild chart data and find Y min/max at the same time ***
        QList<QDate> keys = chartRawData.keys();
        QVector<QCPGraphData> timeData(keys.size());
        double cumulAmount = ui->baselineDoubleSpinBox->value(); // important !
        QDateTime dt;
        int i=0;
        foreach(QDate date, keys){
            CombinedFeStreams::DailyInfo item = chartRawData.value(date);
            cumulAmount += item.totalDelta;
            dt = Util::dateToDateTimeLocal(date, tz );
            timeData[i].key = dt.toSecsSinceEpoch();
            timeData[i].value = cumulAmount;
            // min / max
            if (cumulAmount > fullMaxY) {
                fullMaxY = cumulAmount;
            }
            if (cumulAmount < fullMinY) {
                fullMinY = cumulAmount;
            }
            // next point
            i++;
        }

        // if min == max, spread the scale to 0.95 min to 1.05 max
        if (fullMinY==fullMaxY) {
            double temp = fullMinY;
            fullMinY = 0.95*temp;
            fullMaxY = 1.05*temp;
        }

        // set "fit" limits :
        if (chartRawData.size()==0){
            // nothing to fit, we keep the default

        } else if(chartRawData.size()==1){
            // we set X axis limit to +/- 1 month around the point
            QDate date = chartRawData.keys().first();
            fitFromDateX = date.addMonths(-1);
            fitToDateX = date.addMonths(1);
            fitFromDoubleX = Util::dateToDateTimeLocal(fitFromDateX, QTimeZone::systemTimeZone()).toSecsSinceEpoch();
            fitToDoubleX = Util::dateToDateTimeLocal(fitToDateX, QTimeZone::systemTimeZone()).toSecsSinceEpoch();;
        } else{
            fitFromDateX = chartRawData.keys().first();
            fitToDateX = chartRawData.keys().last();
            fitFromDoubleX = Util::dateToDateTimeLocal(fitFromDateX, QTimeZone::systemTimeZone()).toSecsSinceEpoch();
            fitToDoubleX = Util::dateToDateTimeLocal(fitToDateX, QTimeZone::systemTimeZone()).toSecsSinceEpoch();;
        }

        // update chart
        setChartTitle(scenario->getName());
        chart->yAxis->setNumberPrecision(currInfo.noOfDecimal); // currency may have changed
        chart->graph()->data()->set(timeData, true);

        if ( rescaleXaxis==true ){
            chart->xAxis->setRange(fullFromDoubleX, fullToDoubleX); // do not touche the X axis if only the baseline value changed
            chart->yAxis->setRange(fullMinY, fullMaxY);
            chart->xAxis->scaleRange(chartScalingFactor);
        } else {
            chart->xAxis->setRange(oldXrange);
            chart->graph(0)->rescaleValueAxis(false,true);
        }

        chart->yAxis->scaleRange(chartScalingFactor);

        chart->replot();
    }

}


void MainWindow::slotSelectCountryCompleted()
{
    // do nothing
}


void MainWindow::slotEditScenarioResult(bool currentlyEditingNewScenario)
{
    if (currentlyEditingNewScenario){
        // this is a brand new scenario, not saved yet on disk
        GbpController::getInstance().setFullFileName(""); // indicate scenario file is not saved

        // finally , update the chart
        updateScenarioDataDisplayed(true,true);
    } else{
        // this is an existing scenario , that has been modified (potentially)
        updateScenarioDataDisplayed(false,false);
    }
}


// Follow-up to "New Scenario" menu selection (2nd step)
void MainWindow::slotSelectCountryResult(QString countryCode, CurrencyInfo currInfo)
{
    // Then, prepare and display Edit Scenario Dialog
    emit signalEditScenarioPrepareContent(true, countryCode, currInfo);
    editScenarioDlg->show();
    editScenarioDlg->activateWindow();
}


void MainWindow::slotEditScenarioCompleted()
{
    // do nothing
}


void MainWindow::slotOptionsResult(OptionsDialog::OptionsChangesImpact impact)
{
    // update main chart scaling factor in case it has been changed
    chartScalingFactor = 1 + (GbpController::getInstance().getPercentageMainChartScaling())/100.0;
    // mandatory actions
    setVisibilityZoomButtons();
    setChartColors();
    analysisDlg->themeChanged();
    // Act on impact for main chart
    switch(impact.chart){
    case OptionsDialog::OPTIONS_IMPACT_CHART::CHART_FULL_RECALCULATION_REQUIRED:
            updateScenarioDataDisplayed(true, false);
            break;
        case OptionsDialog::OPTIONS_IMPACT_CHART::CHART_RESCALE_AND_REPLOT:
            // order is important !
            chart->graph(0)->rescaleAxes(false);
            chart->xAxis->scaleRange(chartScalingFactor);
            chart->yAxis->scaleRange(chartScalingFactor);
            chart->replot();
            break;
        case OptionsDialog::OPTIONS_IMPACT_CHART::CHART_REPLOT:
            chart->replot();
            break;
        default:
            break;
    }
    // Act for decoration color changes
    if (impact.decorationColorStreamDef==OptionsDialog::OPTIONS_IMPACT_DECORATION_COLOR::DECO_REFRESH){
        if ( GbpController::getInstance().isScenarioLoaded()==true){
            editScenarioDlg->allowDecorationColor(GbpController::getInstance().getAllowDecorationColor());  // update scenation StreamDef list now
            selectionChangedByUser();   // update the daily info now
        }
    }
}


void MainWindow::slotOptionsCompleted()
{

}


void MainWindow::slotDateIntervalResult(QDate from, QDate to)
{
    QDateTime fromDateTime= Util::dateToDateTimeLocal(from, QTimeZone::systemTimeZone());
    QDateTime toDateTime= Util::dateToDateTimeLocal(to, QTimeZone::systemTimeZone());
    double fromDouble = fromDateTime.toSecsSinceEpoch();
    double toDouble = toDateTime.toSecsSinceEpoch();
    chart->xAxis->setRange(fromDouble, toDouble);
    chart->graph(0)->rescaleValueAxis(false,true);
    // give some space around min/max
    chart->xAxis->scaleRange(chartScalingFactor);
    chart->yAxis->scaleRange(chartScalingFactor);

    chart->replot();
}


void MainWindow::slotDateIntervalCompleted()
{

}


// a point has been selected or unselected
void MainWindow::selectionChangedByUser()
{
    QCPDataSelection selection = chart->graph()->selection();
    if (selection.dataPointCount()==1){
        QCPDataRange range = selection.dataRange(0);
        if(range.size()==1){
            QCPGraphDataContainer::const_iterator begin = chart->graph()->data()->at(range.begin());
            double d = begin->mainKey();
            double v = begin->mainValue();
            // convert to date and get the corresponding DI
            QDateTime dt = QDateTime::fromSecsSinceEpoch(d);
            QDate date = dt.date();
            if (chartRawData.contains(date)) {
                CombinedFeStreams::DailyInfo di = chartRawData.value(date); // should always be there
                fillDailyInfoSection(date, v, di );
            } else {
                emptyDailyInfoSection();
            }
        } else {
            emptyDailyInfoSection();
        }
    } else{
        emptyDailyInfoSection();
    }
}


// X axis scale has been changed
void MainWindow::rangeChangedX(const QCPRange &newRange)
{
    QDateTime dtFrom = QDateTime::fromSecsSinceEpoch(newRange.lower);
    QDateTime dtTo = QDateTime::fromSecsSinceEpoch(newRange.upper);
    QDate from = dtFrom.date();
    QDate to = dtTo.date();
    Util::DateDifference delta =  Util::dateDifference(from, to);

    QString s = locale.toString(from,"yyyy-MMM-dd");
    ui->dateRangeFromLabel->setText(s);
    s = locale.toString(to,"yyyy-MMM-dd");
    ui->dateRangeToLabel->setText(s);

    QString deltaStringYear = tr("y",this->metaObject()->className());
    QString deltaStringMonth= tr("m",this->metaObject()->className());
    QString deltaStringDay= tr("d",this->metaObject()->className());
    QString deltaString = QString("%1%2 %3%4 %5%6").arg(delta.years).arg(deltaStringYear).arg(delta.months).arg(deltaStringMonth).arg(delta.days).arg(deltaStringDay);
    ui->deltaRangeXLabel->setText(deltaString);

}


// Y axis scale has been changed
void MainWindow::rangeChangedY(const QCPRange &newRange)
{

}


void MainWindow::msgStatusbar(QString msg){
    ui->statusbar->showMessage(Util::elideText(msg,100,true),5000);
}


// rebuild the menu from scratch, empty it (no recent files)
void MainWindow::recentFilesMenuInit()
{
    ui->menuOpen_Recent->clear();
    QAction* recentFileAction = 0;
    // The trick to to pre-build all the possible menu items but to hide them by default
    // When a recentFile will be added the corresponding menu will be unhidden
    for(auto i = 0; i < maxRecentFiles; ++i){
        recentFileAction = new QAction(this);
        recentFileAction->setVisible(false);
        ui->menuOpen_Recent->addAction(recentFileAction);
        QObject::connect(recentFileAction, &QAction::triggered, this, &MainWindow::on_actionRecentFile_triggered);
        recentFileActionList.append(recentFileAction);
    }
    // add "clear List", always visible and located at the end of the menu
    QAction* clearRecentFilesAction = new QAction(this);
    clearRecentFilesAction->setText(tr("Clear List"));
    clearRecentFilesAction->setVisible(true);
    ui->menuOpen_Recent->addSeparator();
    ui->menuOpen_Recent->addAction(clearRecentFilesAction);
    QObject::connect(clearRecentFilesAction, &QAction::triggered, this, &MainWindow::on_actionClear_List_triggered);
}


void MainWindow::recentFilesMenuUpdate(){
    QStringList rfList = GbpController::getInstance().getRecentFilenames();

    // to be sure, do not use more than maxRecentFiles entries in the file list
    auto itEnd = 0u;
    if(rfList.size() <= maxRecentFiles)
        itEnd = rfList.size();
    else
        itEnd = maxRecentFiles;

    for (auto i = 0u; i < itEnd; ++i) {
        QString strippedName = Util::elideText(QFileInfo(rfList.at(i)).fileName()+" ["+QFileInfo(rfList.at(i)).filePath()+"]",100,true);
        recentFileActionList.at(i)->setText(strippedName);  // what is displyed in the menu item
        recentFileActionList.at(i)->setData(rfList.at(i));  // the actual full length path + filename
        recentFileActionList.at(i)->setVisible(true);
    }
    // keep hidden the unused menu items
    for (auto i = itEnd; i < maxRecentFiles; ++i){
        recentFileActionList.at(i)->setVisible(false);
    }
}


// erase the list of open files
void MainWindow::on_actionClear_List_triggered()
{
    GbpController::getInstance().recentFilenamesClear();
    recentFilesMenuUpdate();
}


void MainWindow::on_actionRecentFile_triggered(){
    QAction *action = qobject_cast<QAction *>(sender());
    if (action){

        // check first if the current scenario needs to be saved
        bool match;
        checkIfCurrentScenarioMatchesDiskVersion(match);
        if (match==false){
            if ( QMessageBox::StandardButton::Yes == QMessageBox::question(this,tr("Modifications not saved"),
                    tr("Current scenario has been modified, but not saved on disk. Do you want to SAVE THE CHANGES before going forward ?")) ){
                on_actionSave_triggered();  // save current scenario
            }
        }

        // switch scenario
        bool result = loadScenarioFile(action->data().toString());
        if (result==true){
            // update recent file opened list
            GbpController::getInstance().recentFilenamesAdd(action->data().toString(), maxRecentFiles);
            recentFilesMenuUpdate();
        }

    }
}


void MainWindow::on_actionOptions_triggered()
{
    emit signalOptionsPrepareContent();
    optionsDlg->show();
}






void MainWindow::on_toolButton_Fit_clicked()
{
    chart->xAxis->setRange(fitFromDoubleX, fitToDoubleX);
    chart->yAxis->setRange(fullMinY, fullMaxY);

    // give some space around min/max
    chart->xAxis->scaleRange(chartScalingFactor);
    chart->yAxis->scaleRange(chartScalingFactor);

    chart->replot();
}


void MainWindow::on_toolButton_Max_clicked()
{
    chart->xAxis->setRange(fullFromDoubleX, fullToDoubleX);
    chart->yAxis->setRange(fullMinY, fullMaxY);
    // give some space around min/max
    chart->xAxis->scaleRange(chartScalingFactor);
    chart->yAxis->scaleRange(chartScalingFactor);

    chart->replot();
}


void MainWindow::on_toolButton_1M_clicked()
{
    rescaleYaxis(1);
}


void MainWindow::on_toolButton_3M_clicked()
{
    rescaleYaxis(3);
}


void MainWindow::on_toolButton_6M_clicked()
{
    rescaleYaxis(6);
}


void MainWindow::on_toolButton_1Y_clicked()
{
    rescaleYaxis(12);
}


void MainWindow::on_toolButton_2Y_clicked()
{
    rescaleYaxis(2*12);
}


void MainWindow::on_toolButton_3Y_clicked()
{
    rescaleYaxis(3*12);
}


void MainWindow::on_toolButton_5Y_clicked()
{
    rescaleYaxis(5*12);
}


void MainWindow::on_toolButton_10Y_clicked()
{
    rescaleYaxis(10*12);
}


void MainWindow::on_toolButton_15Y_clicked()
{
    rescaleYaxis(15*12);
}


void MainWindow::on_toolButton_20Y_clicked()
{
    rescaleYaxis(20*12);
}


void MainWindow::on_toolButton_25Y_clicked()
{
    rescaleYaxis(25*12);
}


void MainWindow::on_toolButton_50Y_clicked()
{
    rescaleYaxis(50*12);
}


void MainWindow::on_showPointsCheckBox_stateChanged(int arg1)
{
    if (ui->showPointsCheckBox->isChecked()==true) {
        chart->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 10));
    } else {
        chart->graph()->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone, 10));
    }
    chart->replot();
}


void MainWindow::on_actionAnalysis_triggered()
{
    bool found;

    // get currency, if a scenario has been loaded, otherwise create a dummy one (CAD)
    CurrencyInfo currInfo;
    if ( true == GbpController::getInstance().isScenarioLoaded() ){
        currInfo = CurrencyHelper::getCurrencyInfoFromCountryCode(locale, GbpController::getInstance().getScenario()->getCountryCode(), found); // it will be found
    } else {
        currInfo = CurrencyHelper::getCurrencyInfoFromCountryCode(locale, "CAN", found); // it will be found
    }

    emit signalAnalysisPrepareContent(chartRawData, currInfo);
    analysisDlg->show();
}


void MainWindow::on_toolButton_Right_clicked()
{
    shiftGraph(true);
}


void MainWindow::on_toolButton_Left_clicked()
{
    shiftGraph(false);
}


void MainWindow::on_exportTextFilePushButton_clicked()
{
    if ( !(GbpController::getInstance().isScenarioLoaded())){
        // no scenario yet, must specify the file name
        QMessageBox::critical(nullptr,tr("Export Failed"),tr("No scenario loaded yet : nothing to save"));
        return;
    }


    // *** get a file name ***
    QString defaultExtension = ".csv";
    QString defaultExtensionUsed = ".csv";
    QString filter = tr("Text Files (*.txt *.TXT *.csv *.CSV)");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Select a File"), GbpController::getInstance().getLastDir(), filter, &defaultExtensionUsed);
    if (fileName == ""){
        return;
    }
    // *** fix the filename to add the proper suffix ***
    QFileInfo fi(fileName);
    if(fi.suffix()==""){    // user has not specified an extension
        fileName.append(defaultExtension);
    }
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("Attempting to export result to text file \"%1\" ...").arg(fileName));


    QFile file(fileName);
    if (false == file.open(QFile::WriteOnly | QFile::Truncate)){
        QMessageBox::critical(nullptr,tr("Export Failed"),tr("Cannot open the file for writing"));
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("    Export failed : Cannot open the file for saving"));
        return;
    }

    // *** get currency info for this scenario ***
    QString countryCode = GbpController::getInstance().getScenario()->getCountryCode();
    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromCountryCode(locale, countryCode, found);
    if(!found){
        // should never happen
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("    Export failed : Cannot find the currency"));
        return;
    }

    // *** export to the file ***

    QString dateFormat = "yyyy-MM-dd";
    double cumulAmount = ui->baselineDoubleSpinBox->value();  // start amount;
    QList<QDate> keys = chartRawData.keys();
    QString totalIncomes;
    QString totalExpenses;
    QString totalDelta;
    QString cumulSum ;
    QString s;

    // write header
    s = QString("%1\t%2\t%3\t%4\t%5\n").arg(tr("Date"),tr("Total Daily Incomes"),tr("Total Daily Expenses"),tr("Total Delta"),tr("Cumulative Total"));
    file.write(s.toUtf8());

    // write data
    foreach(QDate date, keys){
        CombinedFeStreams::DailyInfo item = chartRawData.value(date);
        cumulAmount += item.totalDelta;

        QString dateString = locale.toString(date,dateFormat);
        if (GbpController::getInstance().getExportTextAmountLocalized()) {
            // Localized
            totalIncomes = CurrencyHelper::formatAmount(item.totalIncomes, currInfo, locale, false);
            totalExpenses = CurrencyHelper::formatAmount(item.totalExpenses, currInfo, locale, false);
            totalDelta = CurrencyHelper::formatAmount(item.totalDelta, currInfo, locale, false);
            cumulSum = CurrencyHelper::formatAmount(cumulAmount, currInfo, locale, false);
        } else {
            // not localized
            totalIncomes = QString::number(item.totalIncomes,'f', currInfo.noOfDecimal);
            totalExpenses = QString::number(item.totalExpenses,'f', currInfo.noOfDecimal);
            totalDelta = QString::number(item.totalDelta,'f', currInfo.noOfDecimal);
            cumulSum = QString::number(cumulAmount,'f', currInfo.noOfDecimal);
        }

        s = QString("%1\t%2\t%3\t%4\t%5\n").arg(dateString,totalIncomes,totalExpenses,totalDelta,cumulSum);
        file.write(s.toUtf8());
    }
    file.close();
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("    Export succeeded"));
}


void MainWindow::on_baselineDoubleSpinBox_editingFinished()
{
    updateScenarioDataDisplayed(false,false);
    ui->baselineDoubleSpinBox->clearFocus();
}


void MainWindow::on_customToolButton_clicked()
{
    emit signalDateIntervalPrepareContent();
    dateIntervalDlg->show();
}

