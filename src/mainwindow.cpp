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
#include "currencyhelper.h"
#include "editscenariodialog.h"
#include "util.h"
#include <QDateTime>
#include <QDesktopServices>
#include <cfloat>
#include <qforeach.h>



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
    scenarioPropertiesDlg = new ScenarioPropertiesDialog(locale,this);
    scenarioPropertiesDlg->setModal(true);

    // rebuild "recent files" submenu (settings must have been loaded before)
    recentFilesMenuInit();
    recentFilesMenuUpdate();

    // resize some QLabel to be sure we have enough space to display stuff
    QFontMetrics fm(ui->ciDateLabel->font());
    ui->ciAmountLabel->setMinimumWidth(fm.averageCharWidth()*15);
    ui->ciDeltaLabel->setMinimumWidth(fm.averageCharWidth()*10);

    // set minimum/maximum value of baseline and erase currency label
    // no currency has more than 3 decimal digits
    ui->baselineDoubleSpinBox->setMaximum(CurrencyHelper::maxValueAllowedForAmountInDouble(3));
    ui->baselineDoubleSpinBox->setMinimum(-CurrencyHelper::maxValueAllowedForAmountInDouble(3));
    ui->baselineCurrencyLabel->setText("");

    // display todays's date in bottom startAmountLabel
    ui->startAmountLabel->setText(tr("Start Amount for Today %1 :").arg(
        locale.toString(GbpController::getInstance().getToday(),"yyyy-MMM-dd")));

    // set scaling factor from settings
    chartScalingFactor = 1 + (GbpController::getInstance().getPercentageMainChartScaling())/100.0;

    // use smaller font for Info list and enable custom sorting
    QFont listFont = ui->ciDetailsListWidget->font();
    uint oldFontSize = listFont.pointSize();
    uint newFontSize = Util::changeFontSize(1,true, oldFontSize);
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        QString("Main Window - FE List - Font size from %1 to %2").arg(oldFontSize).arg(
        newFontSize));
    listFont.setPointSize(newFontSize);
    ui->ciDetailsListWidget->setFont(listFont);
    ui->ciDetailsListWidget->setSortingEnabled(true);

    // use smaller font for "resize" toolbar buttons
    QFont resizeToolbarFont = ui->toolButton_1M->font();
    oldFontSize = resizeToolbarFont.pointSize();
    newFontSize = Util::changeFontSize(1,true, oldFontSize);
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        QString("Main Window - Toolbar - Font size from %1 to %2").arg(oldFontSize).arg(
        newFontSize));
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

    // update general info section
    emptyGeneralInfoSection();
    emptyDailyInfoSection();

    // Build the QChart
    initChart();

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
    // connect MainWindow and Scenario Properties dialog
    QObject::connect(this, &MainWindow::signalScenarioPropertiesPrepareContent, scenarioPropertiesDlg, &ScenarioPropertiesDialog::slotPrepareContent);
    QObject::connect(scenarioPropertiesDlg, &ScenarioPropertiesDialog::signalScenarioPropertiesCompleted, this, &MainWindow::slotDateIntervalCompleted );
    // connect MainWindow and About Dialog
    QObject::connect(this, &MainWindow::signalAboutDialogPrepareContent, aboutDlg, &AboutDialog::slotAboutDialogPrepareContent);
}


MainWindow::~MainWindow()
{
    delete ui;
}


bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Resize && object == ui->curveWidget){
        chartView->resize(ui->curveWidget->size());
    }
    return QObject::eventFilter(object, event);
}


// Used ONLY by the toolbuttons above. A scenario myst be loaded.
void MainWindow::rescaleXaxis(uint noOfMonths)
{
    if (!(GbpController::getInstance().isScenarioLoaded())){
        return;
    }
    QDateTime newTo = fullFromDateX.addMonths(noOfMonths).addDays(-1);
    // this is to rescale Yaxis
    rescaleChart({.mode=X_RESCALE::X_RESCALE_CUSTOM, .from=fullFromDateX, .to=newTo}, true);
}


void MainWindow::shiftGraph(bool toTheRight)
{
    // Get current Xaxis limits
    QDateTime xMin = axisX->min();
    QDateTime xMax = axisX->max();

    // shift the range
    int delta = xMin.daysTo(xMax);
    if (toTheRight==true) {
        xMin = xMin.addDays(delta+1);
        xMax = xMax.addDays(delta+1);
    } else {
        xMin = xMin.addDays(-(delta+1));
        xMax = xMax.addDays(-(delta+1));
    }
    xAxisRescale mode = {.mode=X_RESCALE::X_RESCALE_CUSTOM, .from=xMin, .to=xMax};
    rescaleChart(mode, false);
}


void MainWindow::themeChanged()
{
    if(GbpController::getInstance().getIsDarkModeSet()==true){
        chart->setTheme(QChart::ChartThemeDark);
        chart->setBackgroundBrush(QBrush(QColor("black")));
    } else {
        chart->setTheme(QChart::ChartThemeLight);
        chart->setBackgroundBrush(QBrush(QColor("white")));
    }
    setSeriesCharacteristics();
    // Changing theme "sometimes" change font size (???). Set them again to be sure
    // it stays constant
    setXaxisFontSize(xAxisFontSize);
    setYaxisFontSize(yAxisFontSize);
}


void MainWindow::setSeriesCharacteristics(){
    if(GbpController::getInstance().getIsDarkModeSet()==true){
        // point color
        scatterSeries->setBrush(GbpController::getInstance().getDarkModePointColor());
        // selected point color
        scatterSeries->setSelectedColor(GbpController::getInstance().
            getDarkModeSelectedPointColor());
        // curve color (shadow series)
        QPen pen = QPen(GbpController::getInstance().getDarkModeCurveColor());
        shadowSeries->setPen(pen);
    } else {
        // point color
        scatterSeries->setBrush(GbpController::getInstance().getLightModePointColor());
        // selected point color
        scatterSeries->setSelectedColor(GbpController::getInstance().
            getLightModeSelectedPointColor());
        // curve color (shadow series)
        QPen pen = QPen(GbpController::getInstance().getLightModeCurveColor());
        shadowSeries->setPen(pen);
    }
    scatterSeries->setBorderColor(Qt::transparent);    // no border on points
    scatterSeries->setMarkerSize(GbpController::getInstance().getChartPointSize());
    shadowSeries->setPointsVisible(false);

}


void MainWindow::reduceAxisFontSize()
{
    // X axis
    QFont xAxisFont = axisX->labelsFont();
    xAxisFontSize = Util::changeFontSize(2, true, xAxisFont.pointSize()); // set for ever
    setXaxisFontSize(xAxisFontSize);

    //  Y axis
    QFont yAxisFont = axisY->labelsFont();
    yAxisFontSize = Util::changeFontSize(2, true, yAxisFont.pointSize()); // set for ever
    setYaxisFontSize(yAxisFontSize);
}


void MainWindow::setXaxisFontSize(uint fontSize){
    QFont xAxisFont = axisX->labelsFont();
    xAxisFont.setPointSize(fontSize);
    axisX->setLabelsFont(xAxisFont);
}


void MainWindow::setYaxisFontSize(uint fontSize){
    QFont yAxisFont = axisY->labelsFont();
    yAxisFont.setPointSize(fontSize);
    axisY->setLabelsFont(yAxisFont);
}


void MainWindow::fillDailyInfoSection(const QDate& date, double amount,
    const CombinedFeStreams::DailyInfo& di)
{
    ui->ciDateLabel->setText(locale.toString(date, locale.dateFormat(QLocale::ShortFormat)));
    QColor red = QColor(210,0,0);
    QColor green = QColor(0,190,0);

    QString countryCode = GbpController::getInstance().getScenario()->getCountryCode();
    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromCountryCode(locale, countryCode,
        found);
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
    // we need this in order to sort the list by StreamDef name
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
            if( (GbpController::getInstance().getAllowDecorationColor()==true) &&
                (color.isValid())){
                item->setForeground(color);
            }
            ui->ciDetailsListWidget->addItem(item) ;  // list widget will take ownership of the item
        }
    }
    foreach(FeDisplay fed, di.expensesList){
        sc->getStreamDefNameAndColorFromId(fed.id, sName, color, streamFound);
        if(found){// should always be found
            item = new CustomListItem(fed.toString(sName, currInfo, locale), sName);
            if( (GbpController::getInstance().getAllowDecorationColor()==true) && (color.isValid())){
                item->setForeground(color);
            }
            ui->ciDetailsListWidget->addItem(item) ;  // list widget will take ownership of the item
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


// A scenario must have been loaded
void MainWindow::fillGeneralInfoSection()
{
   if (!(GbpController::getInstance().isScenarioLoaded())){
    return;
   }
   QString countryCode = GbpController::getInstance().getScenario()->getCountryCode();
   bool found;
   CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromCountryCode(locale, countryCode,
                                                                          found);
   if(!found){
       // should never happen
       return;
   }
   ui->giNoDaysLabel->setText(locale.toString(chartRawData.count()));

   // Total no of events
   ui->giNoEventsLabel->setText(locale.toString(calculateTotalNoOfEvents()));
}


void MainWindow::emptyGeneralInfoSection()
{
    ui->giNoDaysLabel->setText("");
    ui->giNoEventsLabel->setText("");
}


// current Scenario has changed, reset the baseline widgets
void MainWindow::resetBaselineWidgets()
{
    if (!(GbpController::getInstance().isScenarioLoaded())){
        return;
    }
    QString countryCode = GbpController::getInstance().getScenario()->getCountryCode();
    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromCountryCode(locale, countryCode,
        found);
    if(!found){
        // should never happen
        return;
    }

    ui->baselineDoubleSpinBox->setMaximum(CurrencyHelper::maxValueAllowedForAmountInDouble(currInfo.noOfDecimal));
    ui->baselineDoubleSpinBox->setMinimum(-CurrencyHelper::maxValueAllowedForAmountInDouble(currInfo.noOfDecimal));
    ui->baselineDoubleSpinBox->setDecimals(currInfo.noOfDecimal);
    ui->baselineCurrencyLabel->setText(currInfo.isoCode);
    ui->baselineDoubleSpinBox->setValue(0);
}


// Compared current scenario in memory with its counterpart on disk. If an error (i.e. mismatch)
// occurred, true is returned in "match". If no scenario loaded, match = true.
// If the file on disk is an older version, oldVersion is set to true and a mismatch is returned, even
// if the content is the same.
void MainWindow::checkIfCurrentScenarioMatchesDiskVersion(bool &match, bool& oldVersion) const {
    match = true;
    oldVersion = false;
    // anything loaded ?
    if ( GbpController::getInstance().isScenarioLoaded() == false ){
        match = true;
        return; // no scenario load matches with "empty" disk file
    }
    // current scenario is a new scenario (not saved yet), so it is a mismatched by default
    if ( GbpController::getInstance().getFullFileName()== "" ){
        match = false;
        return;
    }
    // load scenario on disk and compare to what we have in memory
    Scenario::FileResult result = Scenario::loadFromFile(GbpController::getInstance().getFullFileName());
    if(result.version1found){
        // the file version on disk is older than the current one. This is automatically a mismatch.
        match = false;
        oldVersion = true;
        return;
    } else if (result.code != Scenario::FileResultCode::SUCCESS){
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


// To be called before current scenario must be closed following an action (e.g. new scenario,
// open a scenario, close app). If scenario file is of another version, it means that we had
// problems updating the file when loading it : as a consequence, do nothing and discard the
// scenario object.
// Return true if user wants to proceed, false otherwise (Cancel or ESC pressed)
bool MainWindow::checkIfCurrentScenarioNeedsToBeSavedBeforeProceeding()
{
    bool match;
    bool oldVersion;

    // check if content of scenario in memory matches what is on file
    checkIfCurrentScenarioMatchesDiskVersion(match, oldVersion);

    if((oldVersion==true) || (match==true) ){
        return true;
    }

    QMessageBox::StandardButtons answer;
    answer = QMessageBox::question(this,tr("Modifications not saved"),
        tr("Current scenario has been modified, but it is not saved yet on disk. Do you want to SAVE THE CHANGES before going forward ?"),
        QMessageBox::StandardButtons(QMessageBox::StandardButton::Yes|
        QMessageBox::StandardButton::No|QMessageBox::StandardButton::Cancel),
        QMessageBox::StandardButton::Cancel);
    if (answer == QMessageBox::StandardButton::Yes ){
        // save current scenario and proceed. If new scenario, user has to choose a file name
        on_actionSave_triggered();
    } else if (answer == QMessageBox::StandardButton::Cancel){
        return false; // Cancel or ESC pressed
    }

    return true;
}


void MainWindow::initChart()
{
    // intercept resize event of curveWidget and resize ChartView
    ui->curveWidget->installEventFilter(this);

    // initial values for axes limits
    // we are interested only from TOMORROW to infinity
    fullFromDateX = QDateTime(GbpController::getInstance().getTomorrow(),QTime(0,0,0));
    fullToDateX = fullFromDateX.addYears(Scenario::DEFAULT_DURATION_FE_GENERATION).addDays(-1);

    // Step 1 : Create the chart
    chart = new QChart();
    chart->legend()->hide();
    chart->setTitle(tr("No Scenario Loaded"));
    chart->setLocale(locale);
    chart->setLocalizeNumbers(true);

    // Step 2 : create X axis
    axisX = new QDateTimeAxis;
    axisX->setTickCount(11);
    axisX->setFormat(locale.dateFormat(QLocale::ShortFormat));
    axisX->setRange(fullFromDateX, fullToDateX);
    chart->addAxis(axisX, Qt::AlignBottom);

    // Step 3 : create Y axis
    axisY = new QValueAxis;
    axisY->setTickCount(11);
    axisY->setRange(0,1);
    chart->addAxis(axisY, Qt::AlignLeft);

    // Step 4 : create empty series and attach them to both axis and chart
    QList<QPointF> timeData = {};        // raw data for scatterSeries (real data)
    QList<QPointF> shadowTimeData = {};
    replaceChartSeries(timeData, shadowTimeData);

    // reduce font size for axis
    reduceAxisFontSize();

    chartView = new CustomQChartView(chart,
        GbpController::getInstance().getWheelRotatedAwayZoomIn(), ui->curveWidget);
    chartView->setRenderHint(QPainter::Antialiasing, true);

    connect(axisX,&QDateTimeAxis::rangeChanged, this, &MainWindow::handleXaxisRangeChange);
    connect(axisY,&QValueAxis::rangeChanged, this, &MainWindow::handleYaxisRangeChange);

    // configure dark or light mode for chart. reduceAxisFontSize() must have been called once
    // before
    themeChanged();
}


// this generate a "close-event"
void MainWindow::on_actionQuit_triggered()
{
    QApplication::quit();
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    // check first if the current scenario needs to be saved
    if(false == checkIfCurrentScenarioNeedsToBeSavedBeforeProceeding()){
        event->ignore();
        return;
    }

    QMessageBox::StandardButton answer;
    answer = QMessageBox::question(this,tr("About to quit"),
                tr("Do you really want to quit the application ?"),
                QMessageBox::StandardButtons(QMessageBox::StandardButton::Yes|QMessageBox::StandardButton::No),
                QMessageBox::StandardButton::No);
    if ( answer==QMessageBox::StandardButton::No ) {
        event->ignore();
        return;
    }

    // proceed with quitting the application
    GbpController::getInstance().saveSettings();
    event->accept();
}


void MainWindow::on_actionAbout_triggered()
{
    emit signalAboutDialogPrepareContent(locale);
    aboutDlg->show();
}


void MainWindow::on_actionAbout_Qt_triggered()
{
    QApplication::aboutQt();
}


void MainWindow::on_actionOpen_triggered()
{

    // check first if the current scenario needs to be saved
    if(false == checkIfCurrentScenarioNeedsToBeSavedBeforeProceeding()){
        return;
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


void MainWindow::on_actionOpen_Example_triggered()
{
    // check first if the current scenario needs to be saved
    if(false == checkIfCurrentScenarioNeedsToBeSavedBeforeProceeding()){
        return;
    }

    // first, copy the json file included in the resource to a tmp directory
    // user may tamper with it, so lets add a unique ID.
    QString baseFileName = QString("/gbp_Budget_Example-%1.pdf").arg(QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces));
    QString tempFile = QDir::tempPath().append(baseFileName);
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Opening Budget example : Ready to copy in tmp directory : %1").arg(tempFile));
    QFile scenarioFile(":/Samples/resources/budget-example.json");
    if (scenarioFile.exists() == false ){
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Error, QString("Opening Budget example : Cannot find the budget example file in the internal resource : %1").arg(scenarioFile.fileName()));
        return;
    }
    bool success = scenarioFile.copy(tempFile);
    if (success==true) {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Opening Budget example : Copy succeeded"));
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Error, QString("Opening Budget example : Copy failed"));
        return;
    }

    // then, just open the file
    bool result = loadScenarioFile(tempFile);
    if (result==true){
        // update recent file opened list
        GbpController::getInstance().recentFilenamesAdd(tempFile, maxRecentFiles);
        recentFilesMenuUpdate();
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
        Scenario::FileResult result = saveScenario(fileName);
        if (result.code == Scenario::FileResultCode::SUCCESS){
            // switch scenario
            GbpController::getInstance().setFullFileName(fileName);
            // update status bar
            msgStatusbar(tr("Scenario saved successfully"));
            // add it to the recent files list
            GbpController::getInstance().recentFilenamesAdd(fileName, maxRecentFiles);
            recentFilesMenuUpdate();
        } else {
            QMessageBox::critical(nullptr,"Saving Scenario Failed",result.errorStringUI);
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
    Scenario::FileResult result = saveScenario(fileName);
    if(result.code == Scenario::FileResultCode::SUCCESS){
        // update status bar
        msgStatusbar(tr("Scenario saved successfully"));
    } else {
        QMessageBox::critical(nullptr,"Saving Scenario Failed",result.errorStringUI);
    }
}

// Load a scenario file into a Scenario object.
// If the scenario file is of an older file format version, always attempt
// to update it transparently (could fail if user does not have the write permission on the file).
// Return true if successful, false otherwise
bool MainWindow::loadScenarioFile(QString fileName)
{
    if (GbpController::getInstance().getLogLevel()==GbpController::Debug){
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info,
            QString("Attempting to load scenario from file \"%1\" ...").arg(fileName));
    } else if (GbpController::getInstance().getLogLevel()==GbpController::Minimal){
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
            QString("Attempting to load scenario ..."));
    }

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
                errorStringUI = QString(tr("Error found in the file content.\n\nDetails : %1")).arg(
                    fr.errorStringUI);
                errorStringLog = QString("Error found in the file content.\n\nDetails : %1").arg(
                    fr.errorStringLog);
                break;
            default:
                errorStringUI = fr.errorStringUI;
                errorStringLog = fr.errorStringLog;
                break;
            }
            QMessageBox::critical(nullptr,tr("Loading Scenario Failed"), errorStringUI);
            GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Error,
                QString("Loading scenario failed :  error code = %1 , error message = %2").arg(
                fr.code).arg(fr.errorStringLog));
            return false;
        }
    } catch(...){
        std::exception_ptr p = std::current_exception();
        errorStringUI = QString(tr("An unexpected error has occured.\n\nDetails : %1")).arg(
            (p ? p.__cxa_exception_type()->name() : "null"));
        QMessageBox::critical(nullptr,tr("Loading Scenario Failed"), errorStringUI);
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Error,
            QString("Loading scenario failed : unexpected exception occured : %1").arg(
            (p ? p.__cxa_exception_type()->name() : "null")));
        return false;
    }
    // switch scenario
    GbpController::getInstance().setScenario(fr.scenarioPtr);
    GbpController::getInstance().setFullFileName(fileName);
    chart->setTitle(fr.scenarioPtr->getName());

    // update the "last directory" used in settings
    QFileInfo fi(fileName);
    GbpController::getInstance().setLastDir(fi.path());

    // update the scenario data : recalculate flow data and refresh chart
    //
    QList<QPointF> timeData;
    QList<QPointF> shadowTimeData;
    regenerateRawData(timeData, shadowTimeData);
    replaceChartSeries(timeData, shadowTimeData);
    rescaleChart({.mode=X_RESCALE::X_RESCALE_DATA_MAX}, true);

    // house keeping
    emptyDailyInfoSection();
    fillGeneralInfoSection();
    resetBaselineWidgets();
    changeYaxisLabelFormat();

    // Check if the scenario file is of an older file format version (in any case, the loaded
    // scenario object has already been converted to the latest format).  If yes, attempt to
    // transparently save back the scenario object into the file, in order to update to the latest
    // file format. This could fail if user does not have the write permission on the file.
    if (fr.version1found==true){
        Scenario::FileResult result = saveScenario(GbpController::getInstance().getFullFileName());
        if (result.code != Scenario::FileResultCode::SUCCESS) {
            QMessageBox::warning(nullptr,tr("Error updating file on disk"), tr(
                "Upgrading the file format of this scenario file from v1 to v2 failed. This could indicates a write permission issue with the file."));
        }
    }

    // update status bar
    if (fr.version1found==false) {
        msgStatusbar(tr("Scenario opened successfully"));
    } else {
        msgStatusbar(tr("Scenario opened successfully (converted from version 1 to 2)"));
    }

    // Some logging
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        QString("Scenario loaded successfully"));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info,
        QString("    File name : %1").arg(GbpController::getInstance().getFullFileName()));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info,
        QString("    Name = %1").arg(fr.scenarioPtr->getName()));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info,
        QString("    Country ISO code = %1").arg(fr.scenarioPtr->getCountryCode()));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info,
        QString("    Version = %1").arg(fr.scenarioPtr->getVersion()));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info,
        QString("    No of periodic incomes = %1").arg(fr.scenarioPtr->getIncomesDefPeriodic().
        size()));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info,
        QString("    No of irregular incomes = %1").arg(fr.scenarioPtr->getIncomesDefIrregular().
        size()));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info,
        QString("    No of periodic expenses = %1").arg(fr.scenarioPtr->getExpensesDefPeriodic().
         size()));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info,
        QString("    No of irregular expenses = %1").arg(fr.scenarioPtr->getExpensesDefIrregular().
        size()));

    // Get currency info for the current scenario about to be edited
    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromCountryCode(locale,
        fr.scenarioPtr->getCountryCode(), found);
    if(!found){
        return false; // should never happen
    }

    // display Edit Scenario Dialog
    emit signalEditScenarioPrepareContent(false, fr.scenarioPtr->getCountryCode(),currInfo);

    return true;// full success
}

// Save current scenario under the provided filename. No error message is displayed, but
// loggin is performed.
// Return info about the operation result in Scenario::FileResult.
Scenario::FileResult MainWindow::saveScenario(QString fileName){
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario();

    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info,
        QString("Attempting to save scenario \"%1\" under file name \"%2\"").arg(
        scenario->getName()).arg(fileName));

    Scenario::FileResult fr = scenario->saveToFile(fileName);
    if(fr.code != Scenario::FileResultCode::SUCCESS){
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Warning,
            QString("Saving scenario failed : error code=%1  error message=%2").arg(fr.code).arg(
            fr.errorStringLog));
        return fr;
    }

    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString(
        "Scenario saved successully"));
    // update the "last directory" used in settings
    QFileInfo fi(fileName);
    GbpController::getInstance().setLastDir(fi.path());

    return fr;
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
    if(false == checkIfCurrentScenarioNeedsToBeSavedBeforeProceeding()){
        return;
    }

    // First, select a currency : see slot "slotCountryHasBeenSelected" for the follow up
    emit signalSelectCountryPrepareContent();
    selectCountryDialog->show();
}


// Rescale both axis of the chart. Y axis is always auto-scaled. Chart's data is NOT changed.
// A scenario must be loaded.
// Input paramters:
//   xAxisRescaleMode : how the xAxis will be rescaled
//   addMarginAroundXaxis : Normally true. It means X axis limit are extended by the "rescaling
//                          factor" set in Options, in order to prevent border point to fall
//                          directly on Y axis. For "shitfing" however, we want this turned Off
//                          (false)
void MainWindow::rescaleChart(xAxisRescale xAxisRescaleMode, bool addMarginAroundXaxis)
{
    if (!(GbpController::getInstance().isScenarioLoaded())){
        return;
    }

    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario();

    // recalculate X axis scenario max limits ( "Options->Scenario years" may have changed)
    fullToDateX = fullFromDateX.addYears(scenario->getFeGenerationDuration()).addDays(-1);

    // Get chart raw data
    QList<QPointF> timeData = scatterSeries->points();
    QList<QPointF> shadowTimeData = shadowSeries->points();

    // Get current currency
    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromCountryCode(locale,
        scenario->getCountryCode(), found);
    if(!found){
        return; // should never happen
    }

    // remember current X range
    QDateTime oldXmin = axisX->min();
    QDateTime oldXmax = axisX->max();

    // *** Rescale X axis if requested. Data can be empty. ***

    QDateTime xFrom,xTo;    // required also for Y axis re-scaling
    if ( xAxisRescaleMode.mode==X_RESCALE::X_RESCALE_NONE ){
        // keep the current one
        xFrom = oldXmin;
        xTo = oldXmax;
    } else if ( xAxisRescaleMode.mode==X_RESCALE::X_RESCALE_CUSTOM ){
        // set according to maximum allowed by scenario
        xFrom = xAxisRescaleMode.from;
        xTo = xAxisRescaleMode.to;
    } else if ( xAxisRescaleMode.mode==X_RESCALE::X_RESCALE_SCENARIO_MAX ){
        // set according to maximum allowed by scenario
        xFrom = fullFromDateX;
        xTo = fullToDateX;
    } else if ( xAxisRescaleMode.mode==X_RESCALE::X_RESCALE_DATA_MAX ){
        // DATA MAX : set according to data content
        if (timeData.size()==0) {
            // no data : it is better to show the max extend of X axis
            // so that it is obvious that there is no data
            xFrom = fullFromDateX;
            xTo = fullToDateX;
        } else if (timeData.size()==1){
            xFrom = QDateTime::fromMSecsSinceEpoch(timeData.first().x()).addDays(-1);
            xTo = QDateTime::fromMSecsSinceEpoch(timeData.last().x()).addDays(1);
        } else {
            xFrom = QDateTime::fromMSecsSinceEpoch(timeData.first().x());
            xTo = QDateTime::fromMSecsSinceEpoch(timeData.last().x());
        }
    } else {
        // unkown...
        throw std::invalid_argument("Invalid xAxisRescaleMode");
    }
    // Add margin around xMin/xMax if requested (in most most cases it is)
    QDateTime displayXfrom = xFrom;
    QDateTime displayXto = xTo;
    if(addMarginAroundXaxis==true){
        Util::calculateZoomXaxis(displayXfrom, displayXto,
           GbpController::getInstance().getPercentageMainChartScaling()/100.0);
    }
    // Set Min/Max with rescaling factor
    if(xAxisRescaleMode.mode != X_RESCALE::X_RESCALE_NONE){
        axisX->setRange(displayXfrom, displayXto);
    }

    // *** Always rescale Y axis. Find min/max of range [xMin,xMax] ***
    // Find min/max for Y axis. If just 1 point, or all Y values are the same,
    // spread the scale to 0.95 min to 1.05 max init Y min/max
    double yFrom ;
    double yTo ;
    if (timeData.size()==0){
        // no data
        yFrom = 0;
        yTo = 1;
    } else {
        bool result = Util::findMinMaxInYvalues(timeData, xFrom.toMSecsSinceEpoch(),
                xTo.toMSecsSinceEpoch(), yFrom, yTo);
        // if no data is in the interval [xFrom-xTo], set arbitrary limits
        if(result==false){
            yFrom = 0;
            yTo = 1;
        }
    }
    // Add margin around yMin/yMax
    double displayYfrom = yFrom;
    double displayYto = yTo;
    Util::calculateZoomYaxis(displayYfrom, displayYto, GbpController::getInstance().getPercentageMainChartScaling()/100.0);
    axisY->setRange(displayYfrom, displayYto);



}


// Regenerate the Scenario Flow Data (that is the chartRawData). Do not touch the chart,
// series or axis.
// Output Parameters:
//   timeData : list of final amount per day.
//   shadowTimeData : list of final amount per day, plus fake points to simulate steps in line curve
void MainWindow::regenerateRawData(QList<QPointF>& timeData, QList<QPointF>& shadowTimeData){

    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario();

    // regenerate the flow data from the Scenario (can be long). Always reenerate for maximum
    // duration
    uint saturationNo;
    QDate toLimit = GbpController::getInstance().getTomorrow().addYears(
        scenario->getFeGenerationDuration()).addDays(-1);
    chartRawData.clear();
    chartRawData = scenario->generateFinancialEvents(GbpController::getInstance().getToday(),
        locale, DateRange(GbpController::getInstance().getTomorrow(), toLimit),
        (GbpController::getInstance().getUsePresentValue()==true)?
        (GbpController::getInstance().getPvDiscountRate()):(0),
        GbpController::getInstance().getTomorrow(), saturationNo);

    // Update chart raw data that will later be used to display the curves
    QList<QDate> keys = chartRawData.keys();
    timeData.clear();
    timeData.reserve(keys.size());
    shadowTimeData.clear();
    shadowTimeData.reserve(2*keys.size()); // max possible
    double cumulAmount = ui->baselineDoubleSpinBox->value(); // important !
    QDateTime dt;
    int i=0;
    double dtMsec ;
    QPointF pt;
    foreach(QDate date, keys){
        CombinedFeStreams::DailyInfo item = chartRawData.value(date);
        cumulAmount += item.totalDelta;
        dtMsec = QDateTime(date, QTime(0,0,0)).toMSecsSinceEpoch();
        // real data
        pt = {dtMsec,cumulAmount};
        timeData.append(pt);
        // shadow data
        if(i!=0){
            // insert fake point. Since we cant insert 2 Y values
            // for the same X value, use the trick to insert the fake one just 1 msec before
            pt = {dtMsec-1,timeData.at(i-1).y()};
            shadowTimeData.append(pt);
        }
        pt = {dtMsec,cumulAmount};
        shadowTimeData.append(pt);
        // next point
        i++;
    }
}



// Rebuild chart's series and set characteristics (like Colors).
// data and shadowData are coming from chartRawData.
// Does not update the Chart (rescaling)
// Input Parameters:
//   timeData : list of final amount per day.
//   shadowTimeData : list of final amount per day, plus fake points to simulate steps in line curve
void MainWindow::replaceChartSeries(QList<QPointF> data, QList<QPointF> shadowData)
{
    // first destroy the current series and all the data they have
    chart->removeAllSeries();

    // rebuild
    scatterSeries = new QScatterSeries(); // only true data, for markers only, superimposed
    shadowSeries = new QLineSeries(); // to simulate step curve

    // Set scatter chart characteristics
    if (ui->showPointsCheckBox->isChecked()==true) {
        scatterSeries->show();
    } else {
        scatterSeries->hide();
    }

    // set colors and characteristics for the series
    setSeriesCharacteristics();

    // intercept point selection
    connect(scatterSeries, SIGNAL(clicked(QPointF)), this, SLOT(mypoint_clicked(QPointF)));

    // fill series with data
    scatterSeries->append(data); // take ownership
    shadowSeries->append(shadowData); // take ownership

    // attach to chart
    chart->addSeries(shadowSeries);  // chart takes ownership
    chart->addSeries(scatterSeries); // chart takes ownership

    // re-attach axis
    shadowSeries->attachAxis(axisX);
    scatterSeries->attachAxis(axisX);
    shadowSeries->attachAxis(axisY);
    scatterSeries->attachAxis(axisY);

    // no point have been selected yet
    indexLastPointSelected = -1;
}


void MainWindow::slotSelectCountryCompleted()
{
    // do nothing
}


// regenerateData at true means all FE list must be recalculated from scratch.
void MainWindow::slotEditScenarioResult(bool currentlyEditingNewScenario, bool regenerateData)
{
    if (currentlyEditingNewScenario){
        // this is a brand new scenario, not saved yet on disk
        GbpController::getInstance().setFullFileName(""); // indicate scenario file is not saved
        // All data have to be generated
        QList<QPointF> timeData;        // raw data for scatterSeries (real data)
        QList<QPointF> shadowTimeData;
        regenerateRawData(timeData, shadowTimeData);
        replaceChartSeries(timeData, shadowTimeData);
        rescaleChart({.mode=X_RESCALE::X_RESCALE_DATA_MAX}, true);
        emptyDailyInfoSection();
        resetBaselineWidgets();
        changeYaxisLabelFormat();
        // update general info section
        fillGeneralInfoSection();
        // notify user
        msgStatusbar(tr("A new scenario has been created"));
    } else{
        // this is an existing scenario, that has been modified (potentially).
        // Do not touch X axis if no regeneration has to be performed
        if (regenerateData==true) {
            QList<QPointF> timeData;        // raw data for scatterSeries (real data)
            QList<QPointF> shadowTimeData;
            regenerateRawData(timeData, shadowTimeData);
            replaceChartSeries(timeData, shadowTimeData);
            rescaleChart({.mode=X_RESCALE::X_RESCALE_DATA_MAX}, true);
            emptyDailyInfoSection();
            // update general info section
            fillGeneralInfoSection();
        }
        // notify user
        msgStatusbar(tr("Current scenario has been modified"));
    }
    // Reset the chart title, because we cannot know if it has changed
    chart->setTitle(GbpController::getInstance().getScenario()->getName());

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

    // Act on impact for main Cash Balance chart
    if(impact.data == OptionsDialog::OPTIONS_IMPACT_DATA::DATA_RECALCULATE){
        // the raw data must be fully recalculated
        QList<QPointF> timeData;
        QList<QPointF> shadowTimeData;
        regenerateRawData(timeData, shadowTimeData);
        replaceChartSeries(timeData, shadowTimeData);
        rescaleChart({.mode=X_RESCALE::X_RESCALE_NONE}, true);
        emptyDailyInfoSection();
    } else if(impact.chart_scaling ==
        OptionsDialog::OPTIONS_IMPACT_CHART_SCALING::CHART_SCALING_RESCALE){
        // Because it is difficult to add the overscaling factor in all circumstances, we cut
        // short and fully redisplay the chart with "fit scaling"
         rescaleChart({.mode=X_RESCALE::X_RESCALE_DATA_MAX}, true);
    }

    // Act for chart theme changes
    if (impact.charts_theme==OptionsDialog::OPTIONS_IMPACT_CHARTS_THEME::CHARTS_THEME_REFRESH) {
        // re-theme all charts
        themeChanged();
        analysisDlg->themeChanged();
    }

    // Act for decoration color changes
    if (impact.decorationColorStreamDef ==
        OptionsDialog::OPTIONS_IMPACT_DECORATION_COLOR::DECO_REFRESH){
        if ( GbpController::getInstance().isScenarioLoaded()==true){
            // update scenario StreamDef list now
            editScenarioDlg->allowDecorationColor(
                GbpController::getInstance().getAllowDecorationColor());
            // redisplay Daily Info info panel to update the name colors. To do so, simulate
            // a click on the already selected point.
            QList<int> selPoints = scatterSeries->selectedPoints();
            if (selPoints.size()==1){
                QList<QPointF> thePoints = scatterSeries->points();
                QPointF pt = thePoints.at(selPoints.at(0));
                mypoint_clicked(pt);
            }
        }
    }

    // Act for mouse wheel zooming behavior change
    if (impact.mouseWheelZoom ==
        OptionsDialog::OPTIONS_IMPACT_WHEEL_ZOOM::WHEEL_ZOOM_REFRESH){
        chartView->setWheelRotatedAwayZoomIn(
            GbpController::getInstance().getWheelRotatedAwayZoomIn());
    }

    GbpController::getInstance().saveSettings();
    msgStatusbar(tr("Options changes have been successfully saved"));
}


void MainWindow::slotOptionsCompleted()
{
}


void MainWindow::slotDateIntervalResult(QDate from, QDate to)
{
    QDateTime fromDateTime= QDateTime(from, QTime(0,0,0));
    QDateTime toDateTime= QDateTime(to, QTime(0,0,0));
    // rescale Yaxis
    rescaleChart({.mode=X_RESCALE::X_RESCALE_CUSTOM, .from=fromDateTime, .to=toDateTime}, true);
}


void MainWindow::slotDateIntervalCompleted()
{

}


void MainWindow::slotScenarioPropertiesCompleted()
{

}


// A point has been selected or unselected. This is SLOW... Optimisation required.
void MainWindow::mypoint_clicked(QPointF pt)
{
    // find the index of the point in the series
    QList<QPointF> ptList = scatterSeries->points();
    int index = ptList.indexOf(pt);
    if(index==-1){
        // should not happen
        return;
    }

    QDateTime dt = QDateTime::fromMSecsSinceEpoch(pt.x());
    if (dt.isValid()==false){
        return;
    }

    // set selected points to normal color, then unselect
    //
    //if (scatterSeries->isPointSelected(index)==true) {
    if (indexLastPointSelected==index) {
        // unselect the point
        scatterSeries->setPointSelected(indexLastPointSelected,false);
        indexLastPointSelected = -1;
        emptyDailyInfoSection();
    } else {
        // those 2 are very slow with high number of points. Need optimization...
        // Tried to block signal : it does the job but then points do change color when selected...
        //scatterSeries->deselectAllPoints();
        if(indexLastPointSelected != -1){
            scatterSeries->setPointSelected(indexLastPointSelected,false);
        }
        scatterSeries->setPointSelected(index,true);
        // convert to date and get the corresponding DI
        QDate date = dt.date();
        // should always be there, but if not, for any reason, do nothing
        CombinedFeStreams::DailyInfo defaultDi;
        defaultDi.totalDelta = DBL_MAX; // this is an illegal value
        CombinedFeStreams::DailyInfo di = chartRawData.value(date, defaultDi);
        if(di.totalDelta==DBL_MAX){
            return; // point not found, this is not normal
        }
        fillDailyInfoSection(date, pt.y(), di );
        indexLastPointSelected = index;
    }
}


void MainWindow::handleXaxisRangeChange(QDateTime dtFrom, QDateTime dtTo)
{
    QDate from = dtFrom.date();
    QDate to = dtTo.date();
    Util::DateDifference delta = Util::dateDifference(from, to);

    QString s = locale.toString(from, locale.dateFormat(QLocale::ShortFormat));
    ui->dateRangeFromLabel->setText(s);
    s = locale.toString(to, locale.dateFormat(QLocale::ShortFormat));
    ui->dateRangeToLabel->setText(s);

    QString deltaStringYear = tr("y",this->metaObject()->className());
    QString deltaStringMonth= tr("m",this->metaObject()->className());
    QString deltaStringDay= tr("d",this->metaObject()->className());
    QString deltaString = QString("%1%2 %3%4 %5%6").arg(delta.years).arg(deltaStringYear).arg(delta.months).arg(deltaStringMonth).arg(delta.days).arg(deltaStringDay);
    ui->deltaRangeXLabel->setText(deltaString);
}


void MainWindow::handleYaxisRangeChange(qreal min, qreal max)
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
        if(false == checkIfCurrentScenarioNeedsToBeSavedBeforeProceeding()){
            return;
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
    rescaleChart({ .mode=X_RESCALE::X_RESCALE_DATA_MAX}, true);
}


void MainWindow::on_toolButton_Max_clicked()
{
    rescaleChart({.mode=X_RESCALE::X_RESCALE_SCENARIO_MAX}, true);}


void MainWindow::on_toolButton_1M_clicked()
{
    rescaleXaxis(1);
}


void MainWindow::on_toolButton_3M_clicked()
{
    rescaleXaxis(3);
}


void MainWindow::on_toolButton_6M_clicked()
{
    rescaleXaxis(6);
}


void MainWindow::on_toolButton_1Y_clicked()
{
    rescaleXaxis(12);
}


void MainWindow::on_toolButton_2Y_clicked()
{
    rescaleXaxis(2*12);
}


void MainWindow::on_toolButton_3Y_clicked()
{
    rescaleXaxis(3*12);
}


void MainWindow::on_toolButton_5Y_clicked()
{
    rescaleXaxis(5*12);
}


void MainWindow::on_toolButton_10Y_clicked()
{
    rescaleXaxis(10*12);
}


void MainWindow::on_toolButton_15Y_clicked()
{
    rescaleXaxis(15*12);
}


void MainWindow::on_toolButton_20Y_clicked()
{
    rescaleXaxis(20*12);
}


void MainWindow::on_toolButton_25Y_clicked()
{
    rescaleXaxis(25*12);
}


void MainWindow::on_toolButton_50Y_clicked()
{
    rescaleXaxis(50*12);
}


void MainWindow::on_showPointsCheckBox_stateChanged(int arg1)
{
    if (ui->showPointsCheckBox->isChecked()==true) {
        scatterSeries->show();
    } else {
        scatterSeries->hide();
    }
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
    QList<QPointF> timeData;
    QList<QPointF> shadowTimeData;
    regenerateRawData(timeData, shadowTimeData);
    replaceChartSeries(timeData, shadowTimeData);
    rescaleChart({.mode=X_RESCALE::X_RESCALE_NONE}, true);
    ui->baselineDoubleSpinBox->clearFocus();
}


void MainWindow::on_customToolButton_clicked()
{
    if (!(GbpController::getInstance().isScenarioLoaded())){
        return;
    }
    // overscaling will be apply again...Too complicated to remove it (0,1,2,N points)
    QDate from = axisX->min().date();
    QDate to = axisX->max().date();

    emit signalDateIntervalPrepareContent(from,to);
    dateIntervalDlg->show();
}


void MainWindow::on_actionUser_Manual_triggered()
{
    // first, copy the user manual included in the resource to a tmp directory
    // Name of the file in temp dir is dependant on the version !
    QString baseFileName = QString("/gbp_User_Manual-%1.pdf").arg(QCoreApplication::applicationVersion());
    QString tempFileFullName = QDir::tempPath().append(baseFileName);
    QFile tempFile(tempFileFullName);

    // build resource name and check if it exists (it should)
    QFile userManualFile(QString(":/Doc/resources/Graphical Budget Planner - User Manual.pdf"));
    if(userManualFile.exists()==false){
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Error, QString("Viewing User Manual : User Manual %1 does not exist in the resource file").arg(userManualFile.fileName()));
        return;
    }

    //  check if the temp file exist. Copy only if non existent
    bool success;
    if (tempFile.exists()==true) {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "Viewing User Manual : File already exists in temp directory, not copied");
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Viewing User Manual : Ready to copy User Manual in tmp directory : %1").arg(tempFileFullName));
        success = userManualFile.copy(tempFileFullName);
        if (success==true) {
            GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Viewing User Manual : Copy succeeded"));

        } else {
            GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Error, QString("Viewing User Manual : Copy failed"));
            return;
        }
    }

    // then, use the system defaut application to read the file
    success = QDesktopServices::openUrl(QUrl::fromLocalFile(tempFileFullName));
    if (success==true) {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Viewing User Manual : PDF Viewer Launch succeeded"));
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Error, QString("Viewing User Manual : PDF Viewer Launch failed"));
    }

}


void MainWindow::on_actionQuick_Tutorial_triggered()
{
    // first, copy the quick tutorial included in the resource to a tmp directory
    // Name of the file in temp dir is dependant on the version !
    QString baseFileName = QString("/gbp_Quick-Tutorial-%1.pdf").arg(QCoreApplication::applicationVersion());
    QString tempFileFullName = QDir::tempPath().append(baseFileName);
    QFile tempFile(tempFileFullName);

    // build resource name and check if it exists (it should)
    QFile quickTutorialFile(QString(":/Doc/resources/Graphical Budget Planner - Quick Tutorial.pdf"));
    if(quickTutorialFile.exists()==false){
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Error, QString("Viewing Quick Tutorial : Quick Tutorial %1 does not exist in the resource file").arg(quickTutorialFile.fileName()));
        return;
    }

    //  check if the temp file exist. Copy only if non existent
    bool success;
    if (tempFile.exists()==true) {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "Viewing Quick Tutorial : File already exists in temp directory, not copied");
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Viewing Quick Tutorial : Ready to copy Quick Tutorial in tmp directory : %1").arg(tempFileFullName));
        success = quickTutorialFile.copy(tempFileFullName);
        if (success==true) {
            GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Viewing Quick Tutorial : Copy succeeded"));

        } else {
            GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Error, QString("Viewing Quick Tutorial : Copy failed"));
            return;
        }
    }

    // then, use the system defaut application to read the file
    success = QDesktopServices::openUrl(QUrl::fromLocalFile(tempFileFullName));
    if (success==true) {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Viewing Quick Tutorial : PDF Viewer Launch succeeded"));
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Error, QString("Viewing Quick Tutorial : PDF Viewer Launch failed"));
    }
}



void MainWindow::on_actionProperties_triggered()
{
    if ( !(GbpController::getInstance().isScenarioLoaded()) ){
        QMessageBox::critical(nullptr,tr("Scenario Properties Failed"),tr("No scenario loaded"));
        return;
    }

    // from this point , scenario can be also new but unsaved...
    emit signalScenarioPropertiesPrepareContent();
    scenarioPropertiesDlg->show();
    scenarioPropertiesDlg->activateWindow();
}





void MainWindow::on_actionChange_Log_triggered()
{
    // first, copy the changelog included in the resource to a tmp directory
    // Name of the file in temp dir is dependant on the version !
    QString baseFileName = QString("/gbp_CHANGELOG-%1.pdf").arg(QCoreApplication::applicationVersion());
    QString tempFileFullName = QDir::tempPath().append(baseFileName);
    QFile tempFile(tempFileFullName);

    // build resource name and check if it exists (it should)
    QFile changelogFile(QString(":/Doc/resources/Graphical Budget Planner - CHANGELOG.pdf"));
    if(changelogFile.exists()==false){
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Error, QString("Viewing Change Log : %1 does not exist in the resource file").arg(changelogFile.fileName()));
        return;
    }

    //  check if the temp file exist. Copy only if non existent
    bool success;
    if (tempFile.exists()==true) {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "Viewing Change Log : File already exists in temp directory, not copied");
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Viewing Change Log : Ready to copy Change Log in tmp directory : %1").arg(tempFileFullName));
        success = changelogFile.copy(tempFileFullName);
        if (success==true) {
            GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Viewing Change Log : Copy succeeded"));

        } else {
            GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Error, QString("Viewing Change Log : Copy failed"));
            return;
        }
    }

    // then, use the system defaut application to read the file
    success = QDesktopServices::openUrl(QUrl::fromLocalFile(tempFileFullName));
    if (success==true) {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Viewing Change Log : PDF Viewer Launch succeeded"));
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Error, QString("Viewing Change Log : PDF Viewer Launch failed"));
    }

}

void MainWindow::changeYaxisLabelFormat(){
    if (!(GbpController::getInstance().isScenarioLoaded())){
        return;
    }
    QString countryCode = GbpController::getInstance().getScenario()->getCountryCode();
    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromCountryCode(locale, countryCode,
                                                                           found);
    if(!found){
        // should never happen
        return;
    }
    QString yValFormat = QString("\%.%1f").arg(currInfo.noOfDecimal);
    axisY->setLabelFormat(yValFormat);
}


uint MainWindow::calculateTotalNoOfEvents()
{
    uint noEvents = 0;
    foreach(CombinedFeStreams::DailyInfo di, chartRawData){
        noEvents += di.incomesList.size();
        noEvents += di.expensesList.size();
    }
    return noEvents;
}
