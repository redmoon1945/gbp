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

#include "mainwindow.h"
#include "csvexporter.h"
#include "ui_mainwindow.h"
#include <QObject>
#include <QMessageBox>
#include <QLineSeries>
#include <QPalette>
#include <QPen>
#include <QColor>
#include <QDir>
#include "gbpcontroller.h"
#include "gbplogger.h"
#include "currencyhelper.h"
#include "editscenariodialog.h"
#include "util.h"
#include "uiutil.h"
#include <QDateTime>
#include <QTimer>
#include <QDesktopServices>
#include <qforeach.h>
#include <QSizePolicy>
#include "gbpqmessage.h"
#include "irregularcsd.h"
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QStandardPaths>


MainWindow::MainWindow(QLocale systemLocale, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow), locale(systemLocale)
{
    // build UI
    ui->setupUi(this);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);

    // create child dialogs
    editScenarioDlg = new EditScenarioDialog(locale); //  NOT auto destroyed by Qt
    editScenarioDlg->setModal(false);
    pvCalculatorDlg = new PresentValueCalculatorDialog(locale); //  NOT auto destroyed by Qt
    pvCalculatorDlg->setModal(false);
    selectCurrencyDialog = new SelectCurrencyDialog(locale, this); // auto destroyed by Qt
    selectCurrencyDialog->setModal(true);
    optionsDlg = new OptionsDialog(this); //  auto destroyed by Qt
    optionsDlg->setModal(true);
    aboutDlg = new AboutDialog(this); //  auto destroyed by Qt
    aboutDlg->setModal(true);
    analysisDlg = new AnalysisDialog(locale, this); //  auto destroyed by Qt
    analysisDlg->setModal(true);
    dateIntervalDlg = new DateIntervalDialog(this); //  auto destroyed by Qt
    dateIntervalDlg->setModal(true);
    scenarioPropertiesDlg = new ScenarioPropertiesDialog(locale,this); //  auto destroyed by Qt
    scenarioPropertiesDlg->setModal(true);
    anonymizeDlg = new AnonymizeDialog(this);//  auto destroyed by Qt
    anonymizeDlg->setModal(true);

    // rebuild "recent files" submenu (settings must have been loaded before)
    recentFilesMenuInit();
    recentFilesMenuUpdate();

    // resize some QLabel to be sure we have enough space to display stuff (e.g. max amount)
    QFontMetrics fm(ui->ciDateLabel->font());
    int w = fm.horizontalAdvance(QString(17, QLatin1Char('8')));
    ui->ciAmountLabel->setMinimumWidth(w);
    w = fm.horizontalAdvance(QString(17, QLatin1Char('8')));
    ui->ciDeltaLabel->setMinimumWidth(w);

    // set minimum/maximum value of baseline and erase currency label
    // no currency has more than 3 decimal digits
    ui->baselineDoubleSpinBox->setMaximum(CurrencyHelper::maxValueAllowedForAmountInDouble(3));
    ui->baselineDoubleSpinBox->setMinimum(-CurrencyHelper::maxValueAllowedForAmountInDouble(3));

    // display todays's date in bottom startAmountLabel
    ui->startAmountLabel->setText(tr("Start amount as of today %1 :").arg(
        locale.toString(GbpController::getInstance().getToday(),"yyyy-MMM-dd")));

    // set scaling factor from settings
    chartScalingFactor = 1 + (GbpController::getInstance().getPercentageMainChartScaling())/100.0;

    // Use QApplication::font() as the base for all font size calculations and
    // explicitly override every widget group, including those that KDE overrides
    // with separate Toolbar/Menu font roles (QToolButton, QMenuBar, QMenu).
    // This ensures consistent behaviour across all platforms and desktop environments.
    QFont appFont = QApplication::font();

    // use smaller font for ciWidgets + enable custom sorting for the list
    QFont listFont = appFont;
    Util::changeFontSize(listFont, Util::FontResizeIntensity::WEAK, true,
        "Main window - ci info list");
    ui->ciDetailsListWidget->setFont(listFont);
    ui->ciDetailsListWidget->setSortingEnabled(true);
    QFont ciFont = appFont;
    Util::changeFontSize(ciFont, Util::FontResizeIntensity::WEAK, true,
        "Main window - ci widgets");
    ui->ciDate->setFont(ciFont);
    ui->ciDateLabel->setFont(ciFont);
    ui->ciAmount->setFont(ciFont);
    ui->ciAmountLabel->setFont(ciFont);
    ui->ciDelta->setFont(ciFont);
    ui->ciDeltaLabel->setFont(ciFont);

    // use smaller font for statistics area
    QFont giFont = appFont;
    Util::changeFontSize(giFont, Util::FontResizeIntensity::WEAK, true,
        "Main window - gi widgets");
    ui->giNoDays->setFont(giFont);
    ui->giNoDaysLabel->setFont(giFont);
    ui->giNoEvents->setFont(giFont);
    ui->giNoEventsLabel->setFont(giFont);

    // use smaller font for date range area
    QFont drFont = appFont;
    Util::changeFontSize(drFont, Util::FontResizeIntensity::WEAK, true,
        "Main window - date range widgets");
    ui->dateRangeFrom->setFont(drFont);
    ui->dateRangeFromLabel->setFont(drFont);
    ui->dateRangeTo->setFont(drFont);
    ui->dateRangeToLabel->setFont(drFont);
    ui->deltaRange->setFont(drFont);
    ui->deltaRangeXLabel->setFont(drFont);

    // use smaller font for "resize" toolbar buttons
    QFont resizeToolbarFont = appFont;
    Util::changeFontSize(resizeToolbarFont, Util::FontResizeIntensity::WEAK, true,
        "Main window - resize toolbar buttons");
    ui->toolButton_1M->setFont(resizeToolbarFont);
    ui->toolButton_3M->setFont(resizeToolbarFont);
    ui->toolButton_6M->setFont(resizeToolbarFont);
    ui->toolButton_1Y->setFont(resizeToolbarFont);
    ui->toolButton_2Y->setFont(resizeToolbarFont);
    ui->toolButton_3Y->setFont(resizeToolbarFont);
    ui->toolButton_4Y->setFont(resizeToolbarFont);
    ui->toolButton_5Y->setFont(resizeToolbarFont);
    ui->toolButton_10Y->setFont(resizeToolbarFont);
    ui->toolButton_15Y->setFont(resizeToolbarFont);
    ui->toolButton_20Y->setFont(resizeToolbarFont);
    ui->toolButton_25Y->setFont(resizeToolbarFont);
    ui->toolButton_Fit->setFont(resizeToolbarFont);
    ui->toolButton_EOY->setFont(resizeToolbarFont);
    ui->toolButton_Left->setFont(resizeToolbarFont);
    ui->toolButton_Right->setFont(resizeToolbarFont);
    ui->toolButton_Max->setFont(resizeToolbarFont);
    ui->customToolButton->setFont(resizeToolbarFont);

    // Explicitly set menu fonts to override KDE's separate Menu font role
    ui->menubar->setFont(appFont);
    ui->menuFile->setFont(appFont);
    ui->menuOpen_Recent->setFont(appFont);
    ui->menuTools->setFont(appFont);
    ui->menuHelp->setFont(appFont);

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

    // Widen the menu items (Not clear why Qt is not doing that automatically)
    adjustMenuItemLength();

    //
    // connect MainWindow and edit scenario dialog
    QObject::connect(this, &MainWindow::signalEditScenarioPrepareContent, editScenarioDlg,
        &EditScenarioDialog::slotPrepareContent);
    QObject::connect(editScenarioDlg, &EditScenarioDialog::signalEditScenarioResult, this,
        &MainWindow::slotEditScenarioResult );
    QObject::connect(editScenarioDlg, &EditScenarioDialog::signalEditScenarioCompleted, this,
        &MainWindow::slotEditScenarioCompleted );
    // connect Mainwindow and select country dialog
    QObject::connect(this, &MainWindow::signalSelectCountryPrepareContent, selectCurrencyDialog,
        &SelectCurrencyDialog::slotPrepareContent);
    QObject::connect(selectCurrencyDialog, &SelectCurrencyDialog::signalSelectCountryResult, this,
        &MainWindow::slotSelectCountryResult );
    QObject::connect(selectCurrencyDialog,
        &SelectCurrencyDialog::signalSelectCountryCompleted, this,
        &MainWindow::slotSelectCountryCompleted );
    // connect MainWindow and options dialog
    QObject::connect(this, &MainWindow::signalOptionsPrepareContent, optionsDlg,
        &OptionsDialog::slotPrepareContent);
    QObject::connect(optionsDlg, &OptionsDialog::signalOptionsResult, this,
        &MainWindow::slotOptionsResult );
    QObject::connect(optionsDlg, &OptionsDialog::signalOptionsCompleted, this,
        &MainWindow::slotOptionsCompleted );
    // connect MainWindow and Analysis dialog
    QObject::connect(this, &MainWindow::signalAnalysisPrepareContent, analysisDlg,
        &AnalysisDialog::slotAnalysisPrepareContent);
    // connect MainWindow and DateInterval dialog
    QObject::connect(this, &MainWindow::signalDateIntervalPrepareContent, dateIntervalDlg,
        &DateIntervalDialog::slotPrepareContent);
    QObject::connect(dateIntervalDlg, &DateIntervalDialog::signalDateIntervalResult, this,
        &MainWindow::slotDateIntervalResult );
    QObject::connect(dateIntervalDlg, &DateIntervalDialog::signalDateIntervalCompleted, this,
        &MainWindow::slotDateIntervalCompleted );
    // connect MainWindow and Scenario Properties dialog
    QObject::connect(this, &MainWindow::signalScenarioPropertiesPrepareContent,
        scenarioPropertiesDlg, &ScenarioPropertiesDialog::slotPrepareContent);
    QObject::connect(scenarioPropertiesDlg,
        &ScenarioPropertiesDialog::signalScenarioPropertiesCompleted, this,
        &MainWindow::slotDateIntervalCompleted );
    // connect MainWindow and About Dialog
    QObject::connect(this, &MainWindow::signalAboutDialogPrepareContent, aboutDlg,
        &AboutDialog::slotAboutDialogPrepareContent);
    // connect MainWindow and PV Calculator
    QObject::connect(this, &MainWindow::signalPvDialogPrepareContent, pvCalculatorDlg,
        &PresentValueCalculatorDialog::slotPrepareContent);
    // connect MainWindow and Anonymize dialog
    QObject::connect(this, &MainWindow::signalAnonymizePrepareContent, anonymizeDlg,
        &AnonymizeDialog::slotPrepareContent);
    QObject::connect(anonymizeDlg, &AnonymizeDialog::signalResult, this,
        &MainWindow::slotAnonymizeResult );
    QObject::connect(anonymizeDlg, &AnonymizeDialog::signalCompleted, this,
        &MainWindow::slotAnonymizeCompleted );
}


MainWindow::~MainWindow()
{
    delete editScenarioDlg; // we have not set parent
    delete pvCalculatorDlg; // we have not set parent
    delete ui;
}


bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::Resize && object == ui->curveWidget){
        chartView->resize(ui->curveWidget->size());
    }
    return QObject::eventFilter(object, event);
}


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
    if(GbpController::getInstance().useDarkModeForChart()==true){
        chart->setTheme(QChart::ChartThemeDark);
        chart->setBackgroundBrush(QBrush(QColor("black")));
        QPen gridPen(GbpController::getInstance().getGridlinesDarkModeColor());
        axisX->setGridLinePen(gridPen);
        axisY->setGridLinePen(gridPen);
    } else {
        chart->setTheme(QChart::ChartThemeLight);
        chart->setBackgroundBrush(QBrush(QColor("white")));
        QPen gridPen(GbpController::getInstance().getGridlinesLightModeColor());
        axisX->setGridLinePen(gridPen);
        axisY->setGridLinePen(gridPen);
    }
    setSeriesCharacteristics();
    // Changing theme "sometimes" change font size (???). Set them again to be sure
    // it stays constant
    setXaxisFontSize(xAxisFontSize);
    setYaxisFontSize(yAxisFontSize);
}


void MainWindow::setSeriesCharacteristics(){
    if(GbpController::getInstance().useDarkModeForChart()==true){
        // point color
        scatterSeries->setBrush(GbpController::getInstance().getDarkModePointColor());
        // selected point color
        scatterSeries->setSelectedColor(GbpController::getInstance().
            getDarkModeSelectedPointColor());
        // curve color (shadow series)
        QPen pen = QPen(GbpController::getInstance().getDarkModeCurveColor());
        shadowSeries->setPen(pen);
        // Y=0 gridline
        QPen pen3 = QPen(GbpController::getInstance().getYZeroLineDarkModeColor());
        pen3.setStyle(Qt::CustomDashLine);
        pen3.setDashPattern({10, 6});
        zeroYvalueLineSeries->setPen(pen3);
    } else {
        // point color
        scatterSeries->setBrush(GbpController::getInstance().getLightModePointColor());
        // selected point color
        scatterSeries->setSelectedColor(GbpController::getInstance().
            getLightModeSelectedPointColor());
        // curve color (shadow series)
        QPen pen = QPen(GbpController::getInstance().getLightModeCurveColor());
        shadowSeries->setPen(pen);
        // Y=0 gridline
        QPen pen3 = QPen(GbpController::getInstance().getYZeroLineLightModeColor());
        pen3.setStyle(Qt::CustomDashLine);
        pen3.setDashPattern({10, 6});
        zeroYvalueLineSeries->setPen(pen3);
    }
    scatterSeries->setBorderColor(Qt::transparent);    // no border on points
    scatterSeries->setMarkerSize(GbpController::getInstance().getChartPointSize());
    shadowSeries->setPointsVisible(false);
    zeroYvalueLineSeries->setPointsVisible(false);
}


void MainWindow::reduceAxisFontSize()
{
    // X axis
    QFont xAxisFont = axisX->labelsFont();
    Util::changeFontSize(xAxisFont, Util::FontResizeIntensity::AVERAGE, true,
        "Main window - X axis font");
    xAxisFontSize =  xAxisFont.pointSize(); // set for ever
    setXaxisFontSize(xAxisFontSize);

    //  Y axis
    QFont yAxisFont = axisY->labelsFont();
    Util::changeFontSize(yAxisFont, Util::FontResizeIntensity::AVERAGE, true,
        "Main window - Y axis font");
    yAxisFontSize = yAxisFont.pointSize(); // set for ever
    setYaxisFontSize(yAxisFontSize);
}


void MainWindow::setXaxisFontSize(uint fontSize){
    if (fontSize==0) {
        throw std::invalid_argument("Invalid font size of 0");
    }
    QFont xAxisFont = axisX->labelsFont();
    xAxisFont.setPointSize(fontSize);
    axisX->setLabelsFont(xAxisFont);
}


void MainWindow::setYaxisFontSize(uint fontSize){
    if (fontSize==0) {
        throw std::invalid_argument("Invalid font size of 0");
    }
    QFont yAxisFont = axisY->labelsFont();
    yAxisFont.setPointSize(fontSize);
    axisY->setLabelsFont(yAxisFont);
}


void MainWindow::setXaxisDateFormat()
{
    uint xAxisDateFormat = GbpController::getInstance().getXAxisDateFormat();
    switch(xAxisDateFormat){
        case 0:  // Locale
            axisX->setFormat(locale.dateFormat(QLocale::ShortFormat));
            break;
        case 1: // ISO 8601
            axisX->setFormat("yyyy-MM-dd");
            break;
        case 2:// ISO 8601 with 2-digits year
            axisX->setFormat("yy-MM-dd");
            break;
        default:    // should never occur
            break;
        }
}


void MainWindow::fillDailyInfoSection(const QDate& date, double amount,
    const CombinedFeStreams::DailyInfo& di)
{
    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario==nullptr){
        return; // if no scenario loaded (should not happen)
    }

    ui->ciDateLabel->setText(locale.toString(date, "yyyy-MMM-dd"));
    QColor negAmountColor = GbpController::getInstance().getExpenseColor();
    QColor posAmountColor = GbpController::getInstance().getIncomeColor();

    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromIsoCode(
        scenario->getCurrencyIsoCode(), locale.language(), found);
    if(!found){
        // should never happen
        return;
    }

    // *** Summary section ***

    double totalDeltaValue = di.totalIncomes + di.totalExpenses;
    QString totalDelta = locale.toString(totalDeltaValue,'f', currInfo.noOfDecimal);
    ui->ciDeltaLabel->setText(totalDelta);
    QPalette palette = ui->ciDeltaLabel->palette();
    if (totalDeltaValue > 0) {
        palette.setColor(QPalette::WindowText, posAmountColor);
        ui->ciDeltaLabel->setPalette(palette);
    } else {
        palette.setColor(QPalette::WindowText, negAmountColor);
        ui->ciDeltaLabel->setPalette(palette);
    }
    QString amountString = locale.toString(amount,'f', currInfo.noOfDecimal);
    ui->ciAmountLabel->setText(amountString);
    palette = ui->ciAmountLabel->palette();
    if (amount > 0) {
        palette.setColor(QPalette::WindowText, posAmountColor);
        ui->ciAmountLabel->setPalette(palette);
    } else {
        palette.setColor(QPalette::WindowText, negAmountColor);
        ui->ciAmountLabel->setPalette(palette);
    }

    bool streamFound;

    // *** list box content ***

    // we need this class in order to sort the list by Csd name
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

    // incomes
    foreach(Fe fe, di.incomesList){
        // get the Qshared pointer to the Csd
        QSharedPointer<Csd> theCsd = fe.csdPtr.toStrongRef();
        if(theCsd.isNull()){
            continue;   // should never happen
        }
        sName = theCsd->getName();
        color = theCsd->getDecorationColor();
        item = new CustomListItem(fe.toString(sName, currInfo, locale),sName);
        if( (GbpController::getInstance().getAllowDecorationColor()==true) &&
            (color.isValid())){
            item->setForeground(color);
        }
        ui->ciDetailsListWidget->addItem(item) ;  // list widget will take ownership of the item
    }

    // expenses
    foreach(Fe fe, di.expensesList){
        // get the Qshared pointer to the Csd
        QSharedPointer<Csd> theCsd = fe.csdPtr.toStrongRef();
        if(theCsd.isNull()){
            continue;   // should never happen
        }
        sName = theCsd->getName();
        color = theCsd->getDecorationColor();
        item = new CustomListItem(fe.toString(sName, currInfo, locale), sName);
        if( (GbpController::getInstance().getAllowDecorationColor()==true) && (color.isValid())){
            item->setForeground(color);
        }
        ui->ciDetailsListWidget->addItem(item) ;  // list widget will take ownership of the item
     }

}


void MainWindow::emptyDailyInfoSection()
{
    ui->ciDateLabel->setText("");
    ui->ciDeltaLabel->setText("");
    ui->ciAmountLabel->setText("");
    ui->ciDetailsListWidget->clear();
}


void MainWindow::fillGeneralInfoSection()
{
   if (!(GbpController::getInstance().isScenarioLoaded())){
    return;
   }

   ui->giNoDaysLabel->setText(locale.toString(chartRawData->getNoOfElementsUsed()));

   // Total no of events
   ui->giNoEventsLabel->setText(locale.toString(chartRawData->getNoOfFe()));
}


void MainWindow::emptyGeneralInfoSection()
{
    ui->giNoDaysLabel->setText("");
    ui->giNoEventsLabel->setText("");
}


void MainWindow::resetBaselineWidgets()
{
    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario==nullptr){
        return; // if no scenario loaded (should not happen)
    }

    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromIsoCode(
        scenario->getCurrencyIsoCode(), locale.language(), found);
    if(!found){
        // should never happen
        return;
    }

    ui->baselineDoubleSpinBox->setMaximum(CurrencyHelper::maxValueAllowedForAmountInDouble(
        currInfo.noOfDecimal));
    ui->baselineDoubleSpinBox->setMinimum(-CurrencyHelper::maxValueAllowedForAmountInDouble(
        currInfo.noOfDecimal));
    ui->baselineDoubleSpinBox->setDecimals(currInfo.noOfDecimal);
    ui->baselineDoubleSpinBox->setValue(0);
}


MainWindow::CompareWithScenarioFileResult MainWindow::compareCurrentScenarioWithFile()  {
    CompareWithScenarioFileResult result = CompareWithScenarioFileResult::CONTENT_DIFFER;

    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario==nullptr){
        // if no scenario loaded
        return CompareWithScenarioFileResult::NO_SCENARIO_LOADED;
    }

    // current scenario has never been saved on disk
    if ( GbpController::getInstance().getFullFileName() == "" ){
        return CompareWithScenarioFileResult::NOT_SAVED;
    }

    // load scenario on disk and compare to what we have in memory
    Scenario::FileResult r = Scenario::loadFromFile(GbpController::getInstance()
        .getFullFileName());
    if( r.code != Scenario::FileResultCode::SUCCESS){
        // we failed to load the scenario, for whatever reason (e.g. the file could have
        // been deleted manually)
        if (r.code==Scenario::FileResultCode::LOAD_FILE_DOES_NOT_EXIST) {
            return CompareWithScenarioFileResult::SCENARIO_FILE_GONE;
        } else {
            return CompareWithScenarioFileResult::ERROR_LOADING_SCENARIO;
        }
    }
    if ( *(r.scenarioPtr) == (*scenario) ){
        // Contents are identical
        return CompareWithScenarioFileResult::CONTENT_IDENTICAL;
    } else {
        // Contents are different
        return CompareWithScenarioFileResult::CONTENT_DIFFER;
    }
}


bool MainWindow::aboutToSwitchScenario()
{
    // To know if something has to be saved, compare the current scenario in memory
    // with its saved version on disk.
    CompareWithScenarioFileResult compareResult = compareCurrentScenarioWithFile();

    int choice;
    if (compareResult==CompareWithScenarioFileResult::CONTENT_DIFFER) {
        choice = GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::WARNING, tr("Warning"),
            tr("Current scenario has been modified, but changes have not been saved "
            "yet on disk. Do you want to save it before going forward ? If you answer \"No\", "
            "<b><font color=\"#F44336\">the changes will be lost</font></b>."),
            {tr("Cancel"),tr("No"),tr("Yes")},0,0);
        if(choice==-1){
            // ESC pressed
            return false;
        } else if (choice==0){
            // Cancel button pressed
            return false;
        } else if (choice==2){
            // Yes button pressed
            // Save current scenario and proceed.
            on_actionSave_triggered();
            return true;
        } else {
            // No button pressed
            return true;
        }
    } else if (compareResult==CompareWithScenarioFileResult::NOT_SAVED){
        // Or this can also be a new scenario that has not been saved yet in a new file yet to
        // be created.
        choice = GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::WARNING, tr("Warning"),
            tr("The new scenario has not been saved yet on disk. Do you want to "
            " save it in a file before going forward ? If you answer \"No\", "
            "<b><font color=\"#F44336\">the new scenario will be lost</font></b>."),
            {tr("Cancel"),tr("No"),tr("Yes")},0,0);
        if(choice==-1){
            // ESC pressed
            return false;
        } else if (choice==0){
            // Cancel button pressed
            return false;
        } else if (choice==2){
            // Yes button pressed
            // save new scenario and proceed. User has to choose a filename
            on_actionSave_triggered();
            return true;
        } else {
            // No button pressed
            return true;
        }
    } else if (compareResult==CompareWithScenarioFileResult::CONTENT_IDENTICAL){
        return true; // no change to current scenario, proceed
    } else if (compareResult==CompareWithScenarioFileResult::ERROR_LOADING_SCENARIO){
        // We cannot know if there is a difference, it should not happen since this scenario has
        // already been loaded in memory (unless it has been tempered with manually).
        // Log the error and go forward.
        LOG_ERROR("Cannot load the current scenario from disk (for comparison purpose).");
        return true;
    } else if (compareResult==CompareWithScenarioFileResult::SCENARIO_FILE_GONE){
        // Scenario file has been manually deleted. Log the error and go forward.
        LOG_ERROR("Current scenario file has been deleted.");
        return true;
    } else if (compareResult==CompareWithScenarioFileResult::NO_SCENARIO_LOADED){
        // This happens when the first scenario is loaded. Proceed
        return true;
    } else {
        // Should not happen
        return true;
    }

    return true;
}


void MainWindow::viewResourceFile(const QString resourceFullFileName,
    ViewResourceFileResult& result)
{
    bool carryOn = true;
    bool copyRequired = true;

    // Set destination file name in temp directory. The resource file will be copied there under
    // a different name, that includes the gbp version !
    // Name of the resource file (without path) always obeys to a strict convention.
    // Spaces are replaced with "_": a raw space in a file:// URI is left unescaped by
    // QUrl::toString() and can be mishandled by the OS's URI dispatch, causing the system
    // viewer to fail to open an otherwise valid file (e.g. "could not read file ...").
    QString baseNameDest = "gbp_";
    baseNameDest.append(QCoreApplication::applicationVersion());
    baseNameDest.append("_");
    baseNameDest.append(QFileInfo(resourceFullFileName).fileName().replace(' ', '_'));
    // Use a per-user, per-app cache directory rather than the shared system temp dir, so that
    // different OS user accounts on the same machine never collide over the same cached file.
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cacheDir.isEmpty() || !QDir().mkpath(cacheDir)) {
        LOG_ERROR(QString("->Cannot create/resolve cache directory (path: '%1')").arg(cacheDir));
        result.code = ViewResourceFileErrorCode::VRF_CACHE_DIR_UNAVAILABLE;
        result.data = "";
        carryOn = false;
    }
    QString tempFileFullName = QDir(cacheDir).filePath(baseNameDest);
    QFile tempFile(tempFileFullName);

    LOG_INFO(QString("Preparing to view %1").arg(QFileInfo(resourceFullFileName).fileName()));
    LOG_DEBUG_INFO(QString("->Resource file is %1").arg(resourceFullFileName));
    LOG_DEBUG_INFO(QString("->Destination file will be %1").arg(tempFileFullName));

    // build name of the resource file and check if it exists (it must)
    QFile resFile(resourceFullFileName);
    if(carryOn == true && resFile.exists()==false){
        LOG_ERROR(QString("->Resource file %1 does not exist").arg(resourceFullFileName));
        result.code = ViewResourceFileErrorCode::VRF_RES_FILE_DOES_NOT_EXIST;
        result.data = resourceFullFileName;
        carryOn = false;
    }

    if(carryOn == true){
        // Check if the temp file exists and has identical content to the resource file. If it is
        // the case, no copy will occur. Hash comparison catches same-version PDF updates
        // during development (size alone would miss them).
        auto fileHash = [](QFile &f) -> QByteArray {
            if (!f.open(QIODevice::ReadOnly)){
                return {};
            }
            QByteArray h = QCryptographicHash::hash(f.readAll(), QCryptographicHash::Sha1);
            f.close();
            return h;
        };
        bool tempFileAlreadyExist = tempFile.exists();
        if(tempFileAlreadyExist==true){
            LOG_DEBUG_INFO("->File with same name already exists in temp directory");

            // check if content are identical
            bool hashesDiffer = (fileHash(tempFile) != fileHash(resFile));

            if( hashesDiffer==false ){
                LOG_DEBUG_INFO("->File in temp directory has the same content, no copy required");
                copyRequired = false;
            } else {
                LOG_DEBUG_INFO("->File in temp directory has NOT the same content, copy required");

                LOG_DEBUG_INFO("->Removing existing temp file");
                if ( tempFile.remove()==false ){
                    // deletion has failed
                    LOG_ERROR(QString("->Cannot remove temp file (may be locked by another"
                        " process)"));
                    result.code = ViewResourceFileErrorCode::VRF_TEMP_FILE_DELETION_ERROR;
                    result.data = "";
                    carryOn = false;
                } else {
                    // deletion has succeeded
                    LOG_DEBUG_INFO("->Temp file deleted successfully");
                }
            }
        } else {
            LOG_DEBUG_INFO("-> File with same name does NOT already exists in temp directory");
        }

    }

    if(carryOn == true){
        // copy if required
        if(copyRequired==true){
            if ( resFile.copy(tempFileFullName) == false ){
                // copy has failed.
                bool tempDirWritable = QFileInfo(cacheDir).isWritable();
                LOG_ERROR(QString("->Copy failed : %1 (error code : %2) (temp dir writable : %3)")
                              .arg(resFile.errorString())
                              .arg(resFile.error())
                              .arg(tempDirWritable ? "yes" : "no"));
                result.code = ViewResourceFileErrorCode::VRF_RES_FILE_COPY_ERROR;
                result.data = "";
                carryOn = false;
            } else {
                // copy has succeeded.
                // Qt resource files carry a read-only attribute that QFile::copy() preserves on
                // Windows. Clear it immediately while we own the file, so we can delete or replace
                // it on the next launch without hitting "access denied".
                QFile::Permissions rw =
                    QFile::ReadOwner  | QFile::WriteOwner |
                    QFile::ReadUser   | QFile::WriteUser  |
                    QFile::ReadGroup  | QFile::ReadOther;
                if (QFile::setPermissions(tempFileFullName, rw)) {
                    LOG_DEBUG_INFO("->Read-only attribute cleared on temp file");
                } else {
                    LOG_ERROR(QString("->Failed to clear read-only attribute on temp file %1 "
                        "— manual deletion may require admin rights on next update")
                            .arg(tempFileFullName));
                    result.code = ViewResourceFileErrorCode::VRF_FAIL_CLEARING_RO_ATTRIBUTE;
                    result.data = "";
                    carryOn = false;
                }
            }
        }
    }

    QUrl theUrl;
    if(carryOn == true){
        // Get URL to view the file
        theUrl = QUrl::fromLocalFile(tempFileFullName);
        if(theUrl.isValid()==false){
            // Should never happen
            LOG_ERROR( QString("->Cannot obtain Url from local file %1")
                .arg(tempFileFullName));
            result.code = ViewResourceFileErrorCode::VRF_ERROR_OBTAINING_URL;
            result.data = "";
            carryOn = false;
        }
    }

    if(carryOn == true){
        // View the file
        bool success = QDesktopServices::openUrl(theUrl);
        if (success==true) {
            LOG_INFO("->System viewer launch succeeded");
            result.code = ViewResourceFileErrorCode::VRF_SUCCESS;
        } else {
            LOG_ERROR( QString("->System viewer launch failed"));
            result.code = ViewResourceFileErrorCode::VRF_ERROR_LAUNCHING_VIEWER;
            result.data = "";
            carryOn = false;
        }
    }

    LOG_INFO(QString("View operation completed"));
}


void MainWindow::initChart()
{
    // intercept resize event of curveWidget and resize ChartView
    ui->curveWidget->installEventFilter(this);

    // initial values for axes limits
    // we are interested only from TOMORROW to infinity
    fullFromDateX = QDateTime(GbpController::getInstance().getTomorrow(),QTime(0,0,0));
    fullToDateX = fullFromDateX.addYears(Constants::DEFAULT_DURATION_FE_GENERATION).addDays(-1);

    // Step 1 : Create the chart
    chart = new QChart();
    chart->legend()->hide();
    chart->setTitle(tr("No scenario loaded"));
    chart->setLocale(locale);
    chart->setLocalizeNumbers(true);

    // Step 2 : create X axis
    axisX = new QDateTimeAxis;
    axisX->setTickCount(11);
    //axisX->setFormat(locale.dateFormat(QLocale::ShortFormat));
    setXaxisDateFormat();
    axisX->setRange(fullFromDateX, fullToDateX);
    chart->addAxis(axisX, Qt::AlignBottom);

    // Step 3 : create Y axis
    axisY = new QValueAxis;
    axisY->setTickCount(11);
    axisY->setRange(0,1);
    // monospace prevents label shift on value change
    axisY->setLabelsFont(UiUtil::screenMonoFont("MainWindow - axis Y labels"));
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


void MainWindow::on_actionQuit_triggered()
{
    QApplication::quit();
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    // check first if the current scenario needs to be saved
    if(false == aboutToSwitchScenario()){
        // Cancel pressed
        event->ignore();
        return;
    }

    // Ask confirmation before quitting the application
    int choice = GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::QUESTION, tr("Warning"),
        tr("Do you really want to quit the application ?"), {tr("No"),tr("Yes")},0,0);
    switch(choice){
        case -1:
            event->ignore();
            return;
            break;
        case 0:
            event->ignore();
            return;
            break;
    }

    // proceed with quitting the application
    editScenarioDlg->close(); // because it has no parent
    pvCalculatorDlg->close(); // we have not set parent
    GbpController::getInstance().saveSettings();
    event->accept();
}


void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("MainWindow initial size : %1 x %2")
            .arg(width()).arg(height()));
    });
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
    if(false == aboutToSwitchScenario()){
        return; // Cancel pressed
    }

    QSharedPointer<Scenario> scenario;
    QString dir = GbpController::getInstance().getLastDir();
    QString defaultExtensionUsed = "GBP Files (*.json)";
    QString theFilter = tr("GBP Files (*.json);;All files (*)");
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open an existing scenario"),dir,
        theFilter, &defaultExtensionUsed);
    if (fileName != ""){
        // load the scenario file. Error message will be displayed in loadScenarioFile()
        bool success = loadScenarioFile(fileName);
        // if successfull, add it to the recent files list and change window title
        if (success) {
            GbpController::getInstance().recentFilenamesAdd(fileName, maxRecentFiles);
            recentFilesMenuUpdate();
        }
    }
}


void MainWindow::on_actionOpen_Example_triggered()
{
    // check first if the current scenario needs to be saved
    if(false == aboutToSwitchScenario()){
        // cancel pressed
        return;
    }

    // first, copy the json file included in the resource to the per-user, per-app cache
    // directory (rather than the shared system temp dir, so that different OS user accounts
    // on the same machine never collide over the same copy -- see viewResourceFile()).
    // Any existing copy will be overwritten without warning.
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cacheDir.isEmpty() || !QDir().mkpath(cacheDir)) {
        LOG_ERROR(QString("Opening budget example : Cannot create/resolve cache directory "
            "(path: '%1')").arg(cacheDir));
        return;
    }
    QString baseFileName = QString("gbp_Budget_Example.json");
    QString tempFile = QDir(cacheDir).filePath(baseFileName);
    LOG_INFO( QString("Opening budget example : Ready to copy in cache directory : %1")
        .arg(tempFile));
    QFile scenarioFile(":/Samples/resources/budget-example.json");
    if (scenarioFile.exists() == false ){
        LOG_ERROR( QString("Opening budget example : Cannot find the budget example file in the "
            "internal resource : %1").arg(scenarioFile.fileName()));
        return;
    }

    // Remove any existing copy
    if (QFile::exists(tempFile)) {
        if ( false == QFile::remove(tempFile) ){
            LOG_ERROR("Opening budget example : Cannot remove the old existing file");
            // do not return, but continue
        }
    }

    // Copy
    bool success = scenarioFile.copy(tempFile);
    if (success==true) {
        LOG_INFO("Opening budget example : Copy succeeded");
    } else {
        LOG_ERROR("Opening budget example : Copy failed");
        return;
    }

    // Change permission of the file so that it can be modified by the user
    bool setPermResult = QFile::setPermissions(tempFile, QFileDevice::ReadOwner |
        QFileDevice::WriteOwner);
    if (setPermResult==false) {
        LOG_ERROR( QString("Failed to set permissions on file %1")
            .arg(tempFile));
    }

    // then, just open the file. Error message will be displayed in loadScenarioFile()
    bool result = loadScenarioFile(tempFile);
    if (result==true){
        // update recent file opened list
        GbpController::getInstance().recentFilenamesAdd(tempFile, maxRecentFiles);
        recentFilesMenuUpdate();
    }
}


// A scenario must be loaded for that function to work
void MainWindow::on_actionSave_As_triggered()
{
    if ( !(GbpController::getInstance().isScenarioLoaded()) ){
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::WARNING, tr("Warning"),
            tr("No scenario loaded yet, so nothing "
            "to save."), {tr("OK")}, 0, 0);
        return;
    }

    // ask the file name
    QString dir = GbpController::getInstance().getLastDir();
    QString defaultExtensionUsed = "GBP Files (*.json)";
    QString theFilter = tr("GBP Files (*.json);;All files (*)");
    QString fileName = QFileDialog::getSaveFileName(this,tr("Choose a filename"),dir,
        theFilter, &defaultExtensionUsed);

    // Check the answer and process it
    if(fileName==""){
        return; // user canceled
    }

    // fix the filename to add the proper suffix
    QFileInfo fi(fileName);
    if(fi.suffix()==""){    // user has not specified an extension
        fileName.append(".json");
    }
    // remember last dir
    QFileInfo fileInfo(fileName);
    GbpController::getInstance().setLastDir(fileInfo.path());

    // Save the current scenario file. Error message will be displayed by saveScenario()
    Scenario::FileResult result = saveScenario(fileName);
    if (result.code == Scenario::FileResultCode::SUCCESS){
        // set new file name for current scenario
        GbpController::getInstance().setFullFileName(fileName);
        // update status bar
        msgStatusbar(tr("Scenario saved successfully"));
        // update window title (file name may have changed)
        setWindowTopTitle();
        // add it to the recent files list
        GbpController::getInstance().recentFilenamesAdd(fileName, maxRecentFiles);
        recentFilesMenuUpdate();
    }
}


// A scenario must be loaded for that function to work
// and a filename must have already been assigned
void MainWindow::on_actionSave_triggered()
{
    if ( !(GbpController::getInstance().isScenarioLoaded())){
        // no scenario loaded yet
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::WARNING, tr("Warning"),
            tr("No scenario loaded "
            "yet, so nothing to save"), {tr("OK")}, 0, 0);
        return;
    }
    QString fileName = GbpController::getInstance().getFullFileName();
    if ( fileName == "" ){
        // no file name assigned (this is a new scenario then) : use SaveAs instead
        MainWindow::on_actionSave_As_triggered();
        return;
    }
    // Error message will be displayed by saveScenario()
    Scenario::FileResult result = saveScenario(fileName);
    if(result.code == Scenario::FileResultCode::SUCCESS){
        // update status bar
        msgStatusbar(tr("Scenario saved successfully"));
    }
}


bool MainWindow::loadScenarioFile(QString fileName)
{
    LOG_INFO( QString("Attempting to load scenario from file \"%1\" ...")
        .arg(REDACT(fileName)));

    QString userErrorMessage;
    Scenario::FileResult fr ;

    // Load existing scenario from a file on disk
    try {
        fr = Scenario::loadFromFile(fileName);
        if (fr.code != Scenario::FileResultCode::SUCCESS){
            if (fr.code==Scenario::FileResultCode::LOAD_CANNOT_UPGRADE) {
                // the file is valid, but we cannot update it to new version.
                userErrorMessage = QString(tr("This file uses an older format, but cannot be "
                    "upgraded to the current version. Check that you have write permission for "
                    "the file. See the log file for details : \n%1"))
                    .arg(GbpLogger::getInstance().getLogFullFileName());
            } else {
                userErrorMessage = QString(tr("This file cannot be loaded. Error code "
                    "= \"%1\". See the log file for details : \n%2"))
                    .arg(fr.codeToString())
                    .arg(GbpLogger::getInstance().getLogFullFileName());
            }

            // Warn user
            int choice = GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR,
                tr("Error"), userErrorMessage, {tr("OK")}, 0, 0);

            // log
            QString message = QString("Loading scenario failed : error code = %1 , "
                "error message = \"%2\"")
                .arg(fr.codeToString())
                .arg(fr.logErrorMessage);
            LOG_ERROR(message);

            return false;
        }
    } catch(const std::exception& e){
        userErrorMessage = QString(tr("An unexpected error has occurred.<br><br>Details : %1")).arg(
            e.what());
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            userErrorMessage, {tr("OK")}, 0, 0);
        LOG_ERROR( QString("Loading scenario failed : unexpected exception occurred : %1")
            .arg(e.what()) );
        return false;
    } catch(...){
        // unknown type of exception received
        userErrorMessage = QString(tr("An unknown unexpected error has occurred."));
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            userErrorMessage, {tr("OK")}, 0, 0);
        LOG_ERROR(QString("Loading scenario failed : unknown unexpected exception occurred"));
        return false;
    }

    // switch scenario
    GbpController::getInstance().setScenario(fr.scenarioPtr);
    GbpController::getInstance().setFullFileName(fileName);
    chart->setTitle(fr.scenarioPtr->getName());
    setWindowTopTitle();

    // update the "last directory" used in settings
    QFileInfo fi(fileName);
    GbpController::getInstance().setLastDir(fi.path());

    // update the scenario data : recalculate flow data and refresh chart
    // Chart will take ownership of the data
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

    // update status bar
    if (fr.version1found==false) {
        msgStatusbar(tr("Scenario opened successfully"));
    } else {
        msgStatusbar(tr("Scenario opened successfully (converted from version 1 to 2)"));
    }

    // Some logging
    LOG_INFO("Scenario loaded successfully");
    LOG_INFO( QString("    File name : %1")
        .arg(REDACT(GbpController::getInstance().getFullFileName())));
    LOG_INFO( QString("    Scenario name = %1")
        .arg(REDACT(fr.scenarioPtr->getName())));
    LOG_DEBUG_INFO( QString("    Currency ISO code = %1")
        .arg(fr.scenarioPtr->getCurrencyIsoCode()));
    LOG_DEBUG_INFO( QString("    Version = %1")
        .arg(fr.scenarioPtr->getVersion()));
    LOG_DEBUG_INFO( QString("    No of periodic incomes = %1")
        .arg(fr.scenarioPtr->getIncomePeriodicCsds().size()));
    LOG_DEBUG_INFO( QString("    No of irregular incomes = %1")
        .arg(fr.scenarioPtr->getIncomeIrregularCsds().size()));
    LOG_DEBUG_INFO( QString("    No of periodic expenses = %1")
        .arg(fr.scenarioPtr->getExpensePeriodicCsds().size()));
    LOG_DEBUG_INFO( QString("    No of irregular expenses = %1")
        .arg(fr.scenarioPtr->getExpenseIrregularCsds().size()));

    // Get currency info for the current scenario about to be edited
    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromIsoCode(
        fr.scenarioPtr->getCurrencyIsoCode(), locale.language(), found);
    if(!found){
        return false; // should never happen
    }

    // display Edit Scenario Dialog
    emit signalEditScenarioPrepareContent(currInfo);

    return true;// full success
}


Scenario::FileResult MainWindow::saveScenario(QString fileName){

    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario==nullptr){
        Scenario::FileResult r ;
        return r; // if no scenario loaded (should not happen)
    }

    LOG_INFO( QString("Attempting to save scenario \"%1\" under file name \"%2\"")
        .arg(REDACT(scenario->getName())).arg(REDACT((fileName))));

    Scenario::FileResult fr = scenario->saveToFile(fileName);
    if(fr.code != Scenario::FileResultCode::SUCCESS){
        LOG_ERROR( QString("Saving scenario failed : error code=%1  "
            "error message=%2")
            .arg(fr.codeToString()).arg(fr.logErrorMessage));

        // Warn user
        int choice = GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            tr("The current scenario could not be saved. Error code = \"%1\". See the log file for"
                " details : \n%2")
                .arg(fr.codeToString())
                .arg(GbpLogger::getInstance().getLogFullFileName()),
            {tr("OK")}, 0, 0);

        return fr;
    }

    LOG_INFO( "Scenario saved successully" );

    // update the "last directory" used in settings
    QFileInfo fi(fileName);
    GbpController::getInstance().setLastDir(fi.path());

    return fr;
}


void MainWindow::on_actionEdit_triggered()
{
    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario==nullptr){
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::WARNING, tr("Warning"),
            tr("No scenario loaded yet"), {tr("OK")}, 0, 0);
        return;
    }

    // Get currency info for the curent scenario about to be edited
    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromIsoCode(
        scenario->getCurrencyIsoCode(), locale.language(), found);
    if(!found){
        return; // should never happen
    }
    // display Edit Scenario Dialog
    emit signalEditScenarioPrepareContent(currInfo);
    editScenarioDlg->show();
    editScenarioDlg->activateWindow();
}


// Request to create a new scenario
void MainWindow::on_actionNew_triggered()
{
    // check first if the current scenario needs to be saved
    if(false == aboutToSwitchScenario()){
        // Cancel pressed
        return;
    }

    // Then select a currency : see slot "slotCountryHasBeenSelected" for the follow up
    emit signalSelectCountryPrepareContent();
    selectCurrencyDialog->show();
}


void MainWindow::rescaleChart(xAxisRescale xAxisRescaleMode, bool addMarginAroundXaxis)
{
    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario==nullptr){
        return; // if no scenario loaded
    }

    // recalculate X axis scenario max limits ( "Options->Scenario years" may have changed)
    fullToDateX = fullFromDateX.addYears(scenario->getFeGenerationDuration()).addDays(-1);

    // Get chart raw data
    QList<QPointF> timeData = scatterSeries->points();
    QList<QPointF> shadowTimeData = shadowSeries->points();

    // Get current currency
    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromIsoCode(
        scenario->getCurrencyIsoCode(), locale.language(), found);
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
    Util::calculateZoomYaxis(displayYfrom, displayYto,
        GbpController::getInstance().getPercentageMainChartScaling()/100.0);
    axisY->setRange(displayYfrom, displayYto);
}


void MainWindow::regenerateRawData(QList<QPointF>& timeData, QList<QPointF>& shadowTimeData){

    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario==nullptr){
        return; // if no scenario loaded (should not happen)
    }

    // regenerate the flow data from the Scenario (can be long). Always regenerate for maximum
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

    // Get currency info for safe addition
    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromIsoCode(
        scenario->getCurrencyIsoCode(), locale.language(), found);

    // Update chart native data that will later be used to display the curves
    qsizetype timeDataSize = chartRawData->getNoOfElementsUsed();
    timeData.clear();
    timeData.reserve(timeDataSize);
    shadowTimeData.clear();
    shadowTimeData.reserve(2*timeDataSize);
    double cumulAmount = ui->baselineDoubleSpinBox->value(); // important !
    QDateTime dt;
    double dtMsec ;
    QPointF pt;
    const QList<CombinedFeStreams::DailyInfo> combinedStreams = chartRawData->getCombinedStreams();
    const qsizetype theSize = combinedStreams.size();
    QDate tomorrow = GbpController::getInstance().getTomorrow();
    for (int var = 0; var < theSize; ++var) {
        const CombinedFeStreams::DailyInfo item = combinedStreams[var];
        if(item.used==true){
            double dailyDelta = CurrencyHelper::add(item.totalIncomes, item.totalExpenses,
                currInfo.noOfDecimal);
            cumulAmount = CurrencyHelper::add(cumulAmount, dailyDelta, currInfo.noOfDecimal);
            QDate date = tomorrow.addDays(var);
            dtMsec = QDateTime(date, QTime(0,0,0)).toMSecsSinceEpoch();
            // real data
            pt = {dtMsec,cumulAmount};
            timeData.append(pt);
            // shadow data
            const qsizetype timeDataSize = timeData.size();
            if(timeDataSize>1){
                // insert fake point. Since we cant insert 2 Y values
                // for the same X value, use the trick to insert the fake one just 1 msec BEFORE
                pt = {dtMsec-1,timeData.at(timeDataSize-2).y()};
                shadowTimeData.append(pt);
            }
            pt = {dtMsec,cumulAmount};
            shadowTimeData.append(pt);
        }
    }

}


void MainWindow::replaceChartSeries(QList<QPointF> data, QList<QPointF> shadowData)
{

    // first destroy the current series and all the data they have
    chart->removeAllSeries();

    // rebuild
    scatterSeries = new QScatterSeries();       // only true data, for markers only, superimposed
    shadowSeries = new QLineSeries();           // to simulate step curve
    zeroYvalueLineSeries = new QLineSeries();   // to simulate y value = 0 gridline

    // Set scatter chart characteristics
    if (ui->showPointsCheckBox->isChecked()==true) {
        scatterSeries->show();
    } else {
        scatterSeries->hide();
    }

    // intercept point selection
    connect(scatterSeries, SIGNAL(clicked(QPointF)), this, SLOT(mypoint_clicked(QPointF)));


    // fill series with data. For the line representing the Y=0 X axis, min and max are
    // set absurdly high, to cover all the possible cases (including enormous unzoom)
    QList<QPointF> zeroLinePointList;
    zeroLinePointList.append({
        static_cast<qreal>(QDateTime(QDate(1000,1,1),QTime(0,0,0))
        .toMSecsSinceEpoch()),0});
    QDateTime absoluteMaxDate = QDateTime(QDate(4000,1,1),QTime(0,0,0));
    zeroLinePointList.append({static_cast<qreal>(absoluteMaxDate.toMSecsSinceEpoch()),0});
    zeroYvalueLineSeries->append(zeroLinePointList); // take ownership
    scatterSeries->append(data); // take ownership
    shadowSeries->append(shadowData); // take ownership

    // set visibility of Y=0line
    if (GbpController::getInstance().getShowYzeroLine()==true) {
        zeroYvalueLineSeries->setVisible(true);
    } else {
        zeroYvalueLineSeries->setVisible(false);
    }

    // attach to chart. In Qt's charting framework, the order in which series are drawn is
    // determined by the order they are added to the chart. By default, the last series added is
    // rendered on top of the others
    chart->addSeries(zeroYvalueLineSeries);    // chart takes ownership
    chart->addSeries(shadowSeries);  // chart takes ownership
    chart->addSeries(scatterSeries); // chart takes ownership

    // re-attach axis
    zeroYvalueLineSeries->attachAxis(axisX);
    shadowSeries->attachAxis(axisX);
    scatterSeries->attachAxis(axisX);
    zeroYvalueLineSeries->attachAxis(axisY);
    shadowSeries->attachAxis(axisY);
    scatterSeries->attachAxis(axisY);

    // Must be called after addSeries(): Qt Charts resets all series pens when a series is added
    // to the chart (the chart theme redecorates them). Calling setSeriesCharacteristics() before
    // addSeries() caused custom pen styles (e.g. dash pattern on zeroYvalueLineSeries) to be
    // silently overwritten.
    setSeriesCharacteristics();

    // no point have been selected yet
    indexLastPointSelected = -1;
}


void MainWindow::slotSelectCountryCompleted()
{
    // do nothing
}


void MainWindow::slotEditScenarioResult(bool regenerateData)
{
    // this is an existing scenario that has been modified (potentially).
    // Do not touch X axis is any cases : we assume user wants to keep unchanged the X axis
    if (regenerateData==true) {
        LOG_DEBUG_INFO("Regenerating the full set of data for display (CombinedStream)...");
        QList<QPointF> timeData;        // raw data for scatterSeries (real data)
        QList<QPointF> shadowTimeData;
        QList<QPointF> smoothedTimeData;
        regenerateRawData(timeData, shadowTimeData);
        LOG_DEBUG_INFO("Rebuilding charts...");
        replaceChartSeries(timeData, shadowTimeData);
        LOG_DEBUG_INFO("Rescaling charts...");
        rescaleChart({.mode=X_RESCALE::X_RESCALE_NONE}, false);
        emptyDailyInfoSection();
        // update general info section
        fillGeneralInfoSection();
    }
    // notify user
    msgStatusbar(tr("Current scenario has been modified"));

    // Reset the chart title, because we cannot know if it has changed
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario==nullptr){
        return; // if no scenario loaded (should not happen)
    }
    chart->setTitle(scenario->getName());

    // Refresh selected point info panel, because some "cosmetic" properties of Csds
    // may have changed
    // *** next version, this is not trivial ***
}


void MainWindow::slotSelectCountryResult(CurrencyInfo currInfo)
{
    QSharedPointer<Scenario> newScenario = Scenario::createBlankScenario(currInfo.isoCode);

    // Switch current scenario to this one
    GbpController::getInstance().setScenario(newScenario);

    // New scenario has not been saved yet
    GbpController::getInstance().setFullFileName("");

    // All data have to be re-generated (emoty set), chart updated
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

    // Change window title and char title
    setWindowTopTitle();
    chart->setTitle(newScenario->getName());

    // Then, prepare and display Edit Scenario Dialog
    emit signalEditScenarioPrepareContent(currInfo);
    editScenarioDlg->show();
    editScenarioDlg->activateWindow();
}


void MainWindow::slotEditScenarioCompleted()
{
    // make sure this Main Window is shown and has the focus
    this->activateWindow();
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
            editScenarioDlg->allowColoredCsdNames(
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

    // Act for "show Y=0 line" or "Y=0 line color" change
    if (impact.yzeroLine ==
        OptionsDialog::OPTIONS_IMPACT_Y_ZERO_LINE::Y_ZERO_LINE_REFRESH){
        zeroYvalueLineSeries->setVisible(GbpController::getInstance().getShowYzeroLine());
        setSeriesCharacteristics();// have to redraw with proper color
    }

    // Act for "X-Axis Date Format" change
    if (impact.xaxisDateFormat ==
        OptionsDialog::OPTIONS_IMPACT_XAXIS_DATE_FORMAT::XAXIS_DATE_FORMAT_REFRESH){
        setXaxisDateFormat();  // redraw the X Axis labels
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


void MainWindow::slotAnonymizeResult(AnonymizeDialog::AnonymizeOptions opts)
{
    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if (scenario == nullptr) {
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::WARNING, tr("Warning"),
            tr("No scenario loaded yet, so nothing to anonymize."), {tr("OK")}, 0, 0);
        return;
    }

    const QString LOREM =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor "
        "incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud "
        "exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.";

    // --- Scenario name and description ---
    scenario->setName(tr("Name of the scenario"));
    scenario->setDescription(LOREM);

    // --- Helper: apply amount factor ---
    auto applyFactor = [&](quint64 original) -> quint64 {
        double range = opts.intensity / 100.0;
        double minF = qMax(0.0, 1.0 - range);
        double maxF = 1.0 + range;
        double f = minF + QRandomGenerator::global()->generateDouble() * (maxF - minF);
        quint64 result = static_cast<quint64>(original * f);
        return qMin(result, CurrencyHelper::maxValueAllowedForAmount());
    };

    // --- Income periodic CSDs ---
    {
        auto csds = scenario->getIncomePeriodicCsds();
        int counter = 1;
        for (auto& csd : csds) {
            csd->setName(tr("Periodic Income %1").arg(counter++, 2, 10, QChar('0')));
            csd->setDesc(LOREM);
            if (opts.anonymizeAmounts) {
                csd->setAmount(applyFactor(csd->getAmount()));
            }
        }
        scenario->setIncomePeriodicCsds(csds);
    }

    // --- Income irregular CSDs ---
    {
        auto csds = scenario->getIncomeIrregularCsds();
        int counter = 1;
        for (auto& csd : csds) {
            csd->setName(tr("Irregular Income %1").arg(counter++, 2, 10, QChar('0')));
            csd->setDesc(LOREM);
            if (opts.anonymizeAmounts) {
                auto amountSet = csd->getAmountSet();
                for (auto it = amountSet.begin(); it != amountSet.end(); ++it) {
                    it->amount = applyFactor(it->amount);
                    it->notes = LOREM.left(IrregularCsd::AmountInfo::NOTES_MAX_LEN);
                }
                csd->setAmountSet(amountSet);
            } else {
                auto amountSet = csd->getAmountSet();
                for (auto it = amountSet.begin(); it != amountSet.end(); ++it) {
                    it->notes = LOREM.left(IrregularCsd::AmountInfo::NOTES_MAX_LEN);
                }
                csd->setAmountSet(amountSet);
            }
        }
        scenario->setIncomeIrregularCsds(csds);
    }

    // --- Expense periodic CSDs ---
    {
        auto csds = scenario->getExpensePeriodicCsds();
        int counter = 1;
        for (auto& csd : csds) {
            csd->setName(tr("Periodic Expense %1").arg(counter++, 2, 10, QChar('0')));
            csd->setDesc(LOREM);
            if (opts.anonymizeAmounts) {
                csd->setAmount(applyFactor(csd->getAmount()));
            }
        }
        scenario->setExpensePeriodicCsds(csds);
    }

    // --- Expense irregular CSDs ---
    {
        auto csds = scenario->getExpenseIrregularCsds();
        int counter = 1;
        for (auto& csd : csds) {
            csd->setName(tr("Irregular Expense %1").arg(counter++, 2, 10, QChar('0')));
            csd->setDesc(LOREM);
            if (opts.anonymizeAmounts) {
                auto amountSet = csd->getAmountSet();
                for (auto it = amountSet.begin(); it != amountSet.end(); ++it) {
                    it->amount = applyFactor(it->amount);
                    it->notes = LOREM.left(IrregularCsd::AmountInfo::NOTES_MAX_LEN);
                }
                csd->setAmountSet(amountSet);
            } else {
                auto amountSet = csd->getAmountSet();
                for (auto it = amountSet.begin(); it != amountSet.end(); ++it) {
                    it->notes = LOREM.left(IrregularCsd::AmountInfo::NOTES_MAX_LEN);
                }
                csd->setAmountSet(amountSet);
            }
        }
        scenario->setExpenseIrregularCsds(csds);
    }

    // --- Tags ---
    {
        Tags tags = scenario->getTags();
        QList<QUuid> tagIds = tags.getTagIdSet();
        int counter = 1;
        for (const QUuid& id : tagIds) {
            bool found;
            Tag tag = tags.getTag(id, found);
            if (!found) continue;
            tag.setName(tr("Tag %1").arg(counter++, 2, 10, QChar('0')));
            tag.setDescription(LOREM);
            tags.insert(tag);
        }
        scenario->setTags(tags);
    }

    // --- Regenerate data and refresh UI ---
    QList<QPointF> timeData;
    QList<QPointF> shadowTimeData;
    regenerateRawData(timeData, shadowTimeData);
    replaceChartSeries(timeData, shadowTimeData);
    rescaleChart({.mode=X_RESCALE::X_RESCALE_NONE}, false);
    emptyDailyInfoSection();
    fillGeneralInfoSection();
    chart->setTitle(scenario->getName());

    // Refresh Edit Scenario dialog, so it shows the anonymized names
    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromIsoCode(
        scenario->getCurrencyIsoCode(), locale.language(), found);
    if (found) {
        emit signalEditScenarioPrepareContent(currInfo);
    }

}


void MainWindow::slotAnonymizeCompleted()
{

}


// A point has been selected or unselected. This is SLOW... Optimisation required.
void MainWindow::mypoint_clicked(QPointF pt)
{

    // find the index of the point in the series
    const QList<QPointF> ptList = scatterSeries->points();
    int index = ptList.indexOf(pt);
    if(index==-1){
        // should not happen
        return;
    }

    // Find what date it is
    const QDateTime dt = QDateTime::fromMSecsSinceEpoch(pt.x());
    if (dt.isValid()==false){
        return;
    }
    const QDate date = dt.date(); // drop the time portion (get rid of the "shadow point" problem)

    // set selected points to normal color, then unselect
    //
    if (indexLastPointSelected==index) {
        // unselect the point
        scatterSeries->setPointSelected(indexLastPointSelected,false);
        indexLastPointSelected = -1;
        emptyDailyInfoSection();
    } else {
        // Select the point.
        // Those 2 are very slow with high number of points. Need optimization...
        // Tried to block signal : it does the job but then points do change color when selected...
        //scatterSeries->deselectAllPoints();
        if(indexLastPointSelected != -1){
            scatterSeries->setPointSelected(indexLastPointSelected,false);
        }
        scatterSeries->setPointSelected(index,true);

        // From the selected point, convert X coord. to date and get the corresponding DI
        // Index is the offset from tomorrow (which is index=0)
        CombinedFeStreams::DailyInfo di;
        qint64 diIndex = GbpController::getInstance().getTomorrow().daysTo(date);
        const QList<CombinedFeStreams::DailyInfo> list = chartRawData->getCombinedStreams();
        if( (diIndex>=0) && (diIndex<list.size()) ){
            di = list[diIndex];
            if (di.used==false) {
                // should never happen...
                QString msg = QString("Entry not used pt(x)=%1 pt(y)=%2"
                    " index=%3 diIndex=%4 max_val=%5: ").arg(pt.x()).arg(pt.y()).arg(index)
                    .arg(diIndex).arg(chartRawData->getNoOfDays());
                throw std::runtime_error(qUtf8Printable(msg));
            }
        } else {
            // should never happen...
            QString msg = QString("Selected point index is wrong pt(x)=%1 pt(y)=%2"
                " index=%3 diIndex=%4 max_val=%5: ").arg(pt.x()).arg(pt.y()).arg(index)
                .arg(diIndex).arg(chartRawData->getNoOfDays());
            throw std::runtime_error(qUtf8Printable(msg));
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

    QString s = locale.toString(from, "yyyy-MMM-dd");
    ui->dateRangeFromLabel->setText(s);
    s = locale.toString(to, "yyyy-MMM-dd");
    ui->dateRangeToLabel->setText(s);

    QString deltaStringYear = tr("y",this->metaObject()->className());
    QString deltaStringMonth= tr("m",this->metaObject()->className());
    QString deltaStringDay= tr("d",this->metaObject()->className());
    QString deltaString = QString("%1%2 %3%4 %5%6").arg(locale.toString(delta.years))
        .arg(deltaStringYear).arg(locale.toString(delta.months)).arg(deltaStringMonth)
        .arg(locale.toString(delta.days)).arg(deltaStringDay);
    ui->deltaRangeXLabel->setText(deltaString);
}


void MainWindow::handleYaxisRangeChange(qreal min, qreal max)
{

}


void MainWindow::msgStatusbar(QString msg){
    //ui->statusbar->showMessage(Util::elideText(msg,100,true),5000);
}


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
        QObject::connect(recentFileAction, &QAction::triggered, this,
            &MainWindow::actionRecentFile_triggered);
        recentFileActionList.append(recentFileAction);
    }
    // add "clear List", always visible and located at the end of the menu
    QAction* clearRecentFilesAction = new QAction(this);
    clearRecentFilesAction->setText(tr("Clear list"));
    clearRecentFilesAction->setVisible(true);
    ui->menuOpen_Recent->addSeparator();
    ui->menuOpen_Recent->addAction(clearRecentFilesAction);
    QObject::connect(clearRecentFilesAction, &QAction::triggered, this,
        &MainWindow::actionClear_List_triggered);
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
        QString strippedName = Util::elideText(QFileInfo(rfList.at(i)).fileName()+
            " ["+QDir::toNativeSeparators(QFileInfo(rfList.at(i)).filePath())+"]",100,true);
        recentFileActionList.at(i)->setText(strippedName);  // what is displyed in the menu item
        recentFileActionList.at(i)->setData(rfList.at(i));  // the actual full length path+filename
        recentFileActionList.at(i)->setVisible(true);
    }
    // keep hidden the unused menu items
    for (auto i = itEnd; i < maxRecentFiles; ++i){
        recentFileActionList.at(i)->setVisible(false);
    }
}


// erase the list of open files
void MainWindow::actionClear_List_triggered()
{
    GbpController::getInstance().recentFilenamesClear();
    recentFilesMenuUpdate();
}


void MainWindow::actionRecentFile_triggered(){
    QAction *action = qobject_cast<QAction *>(sender());
    if (action){
        // First get the file to load (sems to disappear sometimes after aboutToSwitchScenario...)
        QString filenameToLoad = action->data().toString();

        // check first if the current scenario needs to be saved
        // and do it if user wants it.
        if(false == aboutToSwitchScenario()){
            // Cancel pressed
            return;
        }

        // switch scenario. Error message will be displayed in loadScenarioFile()
        bool result = loadScenarioFile(filenameToLoad);
        if (result==true){
            // update recent file opened list
            GbpController::getInstance().recentFilenamesAdd(action->data().toString(),
                maxRecentFiles);
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


void MainWindow::on_toolButton_4Y_clicked()
{
    rescaleXaxis(4*12);
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


void MainWindow::on_toolButton_EOY_clicked()
{
    if (!(GbpController::getInstance().isScenarioLoaded())){
        return;
    }

    QDate tomorrow = GbpController::getInstance().getTomorrow();
    // Find end of the year containing "tomorrow"
    QDate eoy = QDate(tomorrow.year(),12,31);

    // Check if the 2 dates are at least 1 days apart, and eoy > tomorrow
    int noOfDays = tomorrow.daysTo(eoy);
    if ( noOfDays < 1 ) {
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            tr("Not enough days from tomorrow to EOY"), {tr("OK")}, 0, 0);
        return;
    }

    // proceed
    QDateTime fromDt = QDateTime(tomorrow,QTime(0,0,0));
    QDateTime toDt = QDateTime(eoy,QTime(0,0,0));
    rescaleChart({.mode=X_RESCALE::X_RESCALE_CUSTOM, .from=fromDt, .to=toDt}, true);
}


void MainWindow::on_showPointsCheckBox_stateChanged(int arg1)
{
    if (ui->showPointsCheckBox->isChecked()==true) {
        scatterSeries->show();
    } else {
        scatterSeries->hide();
    }
}

void MainWindow::on_showGridlinesCheckBox_stateChanged(int arg1)
{
    bool show = ui->showGridlinesCheckBox->isChecked();
    axisX->setGridLineVisible(show);
    axisY->setGridLineVisible(show);
}


void MainWindow::on_actionAnalysis_triggered()
{
    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario==nullptr){
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            tr(
            "No scenario loaded yet : nothing to analyse"), {tr("OK")}, 0, 0);
        return; // if no scenario loaded (should not happen)
    }

    bool found;

    // get currency, if a scenario has been loaded, otherwise create a dummy one (CAD)
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromIsoCode(
        scenario->getCurrencyIsoCode(), locale.language(), found);

    // get starting amount
    double sAmount = ui->baselineDoubleSpinBox->value();

    emit signalAnalysisPrepareContent(chartRawData.toWeakRef(), scenario->getTags(), currInfo,
        sAmount);
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
    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario==nullptr){
        // no scenario yet, must specify the file name
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::WARNING, tr("Warning"),
            tr("No scenario loaded yet, so nothing to "
            "export"), {tr("OK")}, 0, 0);
        return;
    }

    // *** get currency info for this scenario ***
    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromIsoCode(
        scenario->getCurrencyIsoCode(), locale.language(), found);
    if(!found){
        LOG_ERROR("Export failed : Cannot find the currency"); // should never happen
        return;
    }

    // *** STEP 1 : Build the columns definitions ***
    QList<CsvColumnDescriptor> columns;
    columns.append({tr("Date"), CsvColumnType::date(CsvDateFormat::YearMonthDay,
        GbpController::getInstance().getExportTextDateLocalized())});
    columns.append({tr("Total daily income"), CsvColumnType::currency(
        GbpController::getInstance().getExportTextNumberLocalized())});
    columns.append({tr("Total daily expenses"), CsvColumnType::currency(
        GbpController::getInstance().getExportTextNumberLocalized())});
    columns.append({tr("Total delta"), CsvColumnType::currency(
        GbpController::getInstance().getExportTextNumberLocalized())});
    columns.append({tr("Cumulative total"), CsvColumnType::currency(
        GbpController::getInstance().getExportTextNumberLocalized())});

    // *** STEP 2 : Populate rows ***
    QList<QList<QVariant>> data;
    double cumulAmount = ui->baselineDoubleSpinBox->value();  // start amount;
    QDate tomorrow = GbpController::getInstance().getTomorrow();
    QList<CombinedFeStreams::DailyInfo> listDi = chartRawData->getCombinedStreams();
    const qsizetype size = listDi.size();
    for (int var = 0; var < size; ++var) {
        CombinedFeStreams::DailyInfo item = listDi[var];
        if(item.used == false){
            continue;
        }
        double dailyDelta = CurrencyHelper::add(item.totalIncomes, item.totalExpenses,
            currInfo.noOfDecimal);
        cumulAmount = CurrencyHelper::add(cumulAmount, dailyDelta, currInfo.noOfDecimal);
        QDate date = tomorrow.addDays(var);
        data.append({date,item.totalIncomes,item.totalExpenses,dailyDelta,cumulAmount});
    }

    // *** STEP 3 : Call exportToCsv() and handle the result ***
    CsvExportResult result = CsvExporter::exportToCsv("Main curve",
        columns, data, locale, currInfo, '\t' );
    switch (result.status) {
        case CsvExportResult::Status::Success:
            break;
        case CsvExportResult::Status::Canceled:
            return; // user closed the dialog — nothing to do
        case CsvExportResult::Status::FileOpenError:
            GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
                tr("Export process failed. Cannot open the "
                                                           "file for saving"), {tr("OK")}, 0, 0);
            return;
        case CsvExportResult::Status::WriteError:
            GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
                tr("Export process failed. Write error."), {tr("OK")}, 0, 0);
            return;
        case CsvExportResult::Status::DataError:
            GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
                tr("Export process failed. Data error."), {tr("OK")}, 0, 0);
            return;
    }
}


void MainWindow::on_baselineDoubleSpinBox_editingFinished()
{
    if ( !(GbpController::getInstance().isScenarioLoaded())){
        // no scenario yet, do nothing
        return;
    }

    QList<QPointF> timeData;
    QList<QPointF> shadowTimeData;
    regenerateRawData(timeData, shadowTimeData);
    replaceChartSeries(timeData, shadowTimeData);
    rescaleChart({.mode=X_RESCALE::X_RESCALE_NONE}, false);
    ui->baselineDoubleSpinBox->clearFocus();
}


void MainWindow::on_customToolButton_clicked()
{
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario==nullptr){
        return; // if no scenario loaded
    }

    // "from"/"to" bound the DateIntervalDialog's date edits to the full range the scenario can
    // actually generate financial events for : tomorrow through the scenario's max calculation
    // date -- not the chart's currently zoomed-in view.
    QDate from = GbpController::getInstance().getTomorrow();
    QDate to = from.addYears(scenario->getFeGenerationDuration()).addDays(-1);

    emit signalDateIntervalPrepareContent(from,to);
    dateIntervalDlg->show();
}


void MainWindow::on_actionUser_Manual_triggered()
{
    bool french = (locale.language() == QLocale::French);
    QString resourcePath = QString(":/Doc/resources/user_manual-%1.pdf")
        .arg(french ? "fr" : "en");

    ViewResourceFileResult result;
    viewResourceFile(resourcePath, result);

    switch (result.code) {
        case ViewResourceFileErrorCode::VRF_SUCCESS:
            // Viewer launched successfully, nothing to tell the user.
            break;

        case ViewResourceFileErrorCode::VRF_RES_FILE_DOES_NOT_EXIST:
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
                tr("The User Manual document (%1) could not be found. This is likely a "
                "packaging issue — please report it.").arg(result.data), {tr("OK")}, 0, 0);
            break;

        case ViewResourceFileErrorCode::VRF_CACHE_DIR_UNAVAILABLE:
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
                tr("Cannot open the User Manual: the application's cache directory could "
                "not be created or accessed."), {tr("OK")}, 0, 0);
            break;

        case ViewResourceFileErrorCode::VRF_TEMP_FILE_DELETION_ERROR:
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
                tr("Cannot open the User Manual: a previous cached copy could not be "
                "removed (it may be locked by another process)."), {tr("OK")}, 0, 0);
            break;

        case ViewResourceFileErrorCode::VRF_RES_FILE_COPY_ERROR:
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
                tr("Cannot open the User Manual: the document could not be copied to the "
                "application cache directory."), {tr("OK")}, 0, 0);
            break;

        case ViewResourceFileErrorCode::VRF_FAIL_CLEARING_RO_ATTRIBUTE:
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::WARNING, tr("Warning"),
                tr("The User Manual could not be fully prepared for viewing due to a file "
                "permission issue."), {tr("OK")}, 0, 0);
            break;

        case ViewResourceFileErrorCode::VRF_ERROR_OBTAINING_URL:
        case ViewResourceFileErrorCode::VRF_ERROR_LAUNCHING_VIEWER:
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
                tr("The system's default PDF viewer failed to launch. You can try to open "
                "the User Manual manually from your system's temporary/cache folder."),
                {tr("OK")}, 0, 0);
            break;
    }
}


void MainWindow::showWelcomeScreen()
{
    bool french = (locale.language() == QLocale::French);
    QString resourcePath = QString(":/Doc/resources/welcome-%1.pdf")
        .arg(french ? "fr" : "en");

    ViewResourceFileResult result;
    viewResourceFile(resourcePath, result);

    switch (result.code) {
        case ViewResourceFileErrorCode::VRF_SUCCESS:
            // Viewer launched successfully, nothing to tell the user.
            break;

        case ViewResourceFileErrorCode::VRF_RES_FILE_DOES_NOT_EXIST:
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
                tr("The Welcome document (%1) could not be found. This is likely a "
                "packaging issue — please report it.").arg(result.data), {tr("OK")}, 0, 0);
            break;

        case ViewResourceFileErrorCode::VRF_CACHE_DIR_UNAVAILABLE:
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
                tr("Cannot open the Welcome document: the application's cache directory "
                "could not be created or accessed."), {tr("OK")}, 0, 0);
            break;

        case ViewResourceFileErrorCode::VRF_TEMP_FILE_DELETION_ERROR:
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
                tr("Cannot open the Welcome document: a previous cached copy could not be "
                "removed (it may be locked by another process)."), {tr("OK")}, 0, 0);
            break;

        case ViewResourceFileErrorCode::VRF_RES_FILE_COPY_ERROR:
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
                tr("Cannot open the Welcome document: it could not be copied to the "
                "application cache directory."), {tr("OK")}, 0, 0);
            break;

        case ViewResourceFileErrorCode::VRF_FAIL_CLEARING_RO_ATTRIBUTE:
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::WARNING, tr("Warning"),
                tr("The Welcome document could not be fully prepared for viewing due to a "
                "file permission issue."), {tr("OK")}, 0, 0);
            break;

        case ViewResourceFileErrorCode::VRF_ERROR_OBTAINING_URL:
        case ViewResourceFileErrorCode::VRF_ERROR_LAUNCHING_VIEWER:
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
                tr("The system's default PDF viewer failed to launch. You can try to open "
                "the Welcome document manually from your system's temporary/cache folder."),
                {tr("OK")}, 0, 0);
            break;
    }
}


void MainWindow::on_actionQuick_Tutorial_triggered()
{
    bool french = (locale.language() == QLocale::French);
    QString resourcePath = QString(":/Doc/resources/quick_tutorial-%1.pdf")
        .arg(french ? "fr" : "en");

    ViewResourceFileResult result;
    viewResourceFile(resourcePath, result);

    switch (result.code) {
    case ViewResourceFileErrorCode::VRF_SUCCESS:
        // Viewer launched successfully, nothing to tell the user.
        break;

    case ViewResourceFileErrorCode::VRF_RES_FILE_DOES_NOT_EXIST:
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            tr("The Quick Tutorial document (%1) could not be found. This is likely a "
            "packaging issue — please report it.").arg(result.data), {tr("OK")}, 0, 0);
        break;

    case ViewResourceFileErrorCode::VRF_CACHE_DIR_UNAVAILABLE:
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            tr("Cannot open the Quick Tutorial: the application's cache directory could "
            "not be created or accessed."), {tr("OK")}, 0, 0);
        break;

    case ViewResourceFileErrorCode::VRF_TEMP_FILE_DELETION_ERROR:
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            tr("Cannot open the Quick Tutorial: a previous cached copy could not be "
            "removed (it may be locked by another process)."), {tr("OK")}, 0, 0);
        break;

    case ViewResourceFileErrorCode::VRF_RES_FILE_COPY_ERROR:
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            tr("Cannot open the Quick Tutorial: the document could not be copied to the "
            "application cache directory."), {tr("OK")}, 0, 0);
        break;

    case ViewResourceFileErrorCode::VRF_FAIL_CLEARING_RO_ATTRIBUTE:
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::WARNING, tr("Warning"),
            tr("The Quick Tutorial could not be fully prepared for viewing due to a file "
            "permission issue. Please try again."), {tr("OK")}, 0, 0);
        break;

    case ViewResourceFileErrorCode::VRF_ERROR_OBTAINING_URL:
    case ViewResourceFileErrorCode::VRF_ERROR_LAUNCHING_VIEWER:
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            tr("The system's default PDF viewer failed to launch. You can try to open "
            "the Quick Tutorial manually from your system's temporary/cache folder."),
            {tr("OK")}, 0, 0);
        break;
    }
}



void MainWindow::on_actionProperties_triggered()
{
    if ( !(GbpController::getInstance().isScenarioLoaded()) ){
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::WARNING, tr("Warning"),
            tr("No scenario loaded yet"), {tr("OK")}, 0, 0);
        return;
    }

    // from this point , scenario can be also new but unsaved...
    emit signalScenarioPropertiesPrepareContent();
    scenarioPropertiesDlg->show();
    scenarioPropertiesDlg->activateWindow();
}


// Visualize the Change Log file
void MainWindow::on_actionChange_Log_triggered()
{
    ViewResourceFileResult result;
    // Always in english
    viewResourceFile(":/Doc/resources/changelog-en.pdf", result);

    switch (result.code) {
        case ViewResourceFileErrorCode::VRF_SUCCESS:
            // Viewer launched successfully, nothing to tell the user.
            break;

        case ViewResourceFileErrorCode::VRF_RES_FILE_DOES_NOT_EXIST:
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
                tr("The Change Log document (%1) could not be found. This is likely a "
                "packaging issue — please report it.").arg(result.data), {tr("OK")}, 0, 0);
            break;

        case ViewResourceFileErrorCode::VRF_CACHE_DIR_UNAVAILABLE:
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
                tr("Cannot open the Change Log: the application's cache directory could "
                "not be created or accessed."), {tr("OK")}, 0, 0);
            break;

        case ViewResourceFileErrorCode::VRF_TEMP_FILE_DELETION_ERROR:
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
                tr("Cannot open the Change Log: a previous cached copy could not be removed "
                "(it may be locked by another process)."), {tr("OK")}, 0, 0);
            break;

        case ViewResourceFileErrorCode::VRF_RES_FILE_COPY_ERROR:
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
                tr("Cannot open the Change Log: the document could not be copied to the "
                "application cache directory."), {tr("OK")}, 0, 0);
            break;

        case ViewResourceFileErrorCode::VRF_FAIL_CLEARING_RO_ATTRIBUTE:
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::WARNING, tr("Warning"),
                tr("The Change Log could not be fully prepared for viewing due to a file "
                "permission issue."), {tr("OK")}, 0, 0);
            break;

        case ViewResourceFileErrorCode::VRF_ERROR_OBTAINING_URL:
        case ViewResourceFileErrorCode::VRF_ERROR_LAUNCHING_VIEWER:
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
                tr("The system's default PDF viewer failed to launch. You can try to open "
                "the Change Log manually from your system's temporary/cache folder."),
                {tr("OK")}, 0, 0);
            break;
    }
}

void MainWindow::changeYaxisLabelFormat(){
    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario==nullptr){
        return; // if no scenario loaded
    }

    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromIsoCode(
        scenario->getCurrencyIsoCode(), locale.language(), found);
    if(!found){
        // should never happen
        return;
    }
    QString yValFormat = QString("\%.%1f").arg(currInfo.noOfDecimal);
    axisY->setLabelFormat(yValFormat);
}




// Change the Main Window Title according to the current scenario loaded
void MainWindow::setWindowTopTitle()
{

    if (!(GbpController::getInstance().isScenarioLoaded())){
        // no scenario yet
        this->setWindowTitle(tr("Graphical Budget Planner"));
        return;
    } else {
        QString ffn = GbpController::getInstance().getFullFileName();
        if (""==ffn) {
            // new scenario not yet saved
            this->setWindowTitle(tr("Not saved yet %1 GBP").arg(QChar(0x2014)));
            return;
        } else {
            QFileInfo fileInfo(ffn);
            QString baseName = Util::elideText(fileInfo.fileName(),50,true);
            this->setWindowTitle(tr("%1 %2 GBP").arg(baseName).arg(QChar(0x2014)));
            return;
        }
    }

}


void MainWindow::adjustMenuItemLength()
{
    int maxLen;
    QFontMetrics metrics =ui->menuTools->fontMetrics();  // assuming all menus have the same fonts

    QList<QAction *> menubarActions = ui->menubar->actions();
    for (QAction *action : menubarActions){
        if (QMenu *menu = action->menu()) {
            // Iterate through each action to find the widest text
            int maxLen = 0;
            QList<QAction*> menuActions = menu->actions();
            for (QAction *menuAction : menuActions) {
                QString fullText = QString("%1...  Ctrl+W").arg(menuAction->text());
                int width = metrics.horizontalAdvance(fullText);
                if (width>=maxLen) {
                    maxLen = width;
                }
            }
            menu->setMinimumWidth(maxLen*1.1); // padding for icon placement (set experimentally...)
        }
    }
}


void MainWindow::on_actionPV_Calculator_triggered()
{
    emit signalPvDialogPrepareContent();
    pvCalculatorDlg->show();
}


void MainWindow::on_actionAnonymize_triggered()
{
    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario==nullptr){
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            tr("No scenario loaded yet"), {tr("OK")}, 0, 0);
        return;
    }

    emit signalAnonymizePrepareContent();
    anonymizeDlg->show();
}


/**
 * @brief Discard the current scenario modifications and reload the same scenario
 * from disk.
 */
void MainWindow::on_actionReload_triggered()
{
    // Get the full file path of the current scenario
    QString fullFileName = GbpController::getInstance().getFullFileName();

    // Check if the current scenario has been saved to disk
    if (fullFileName.isEmpty()) {
        int choice = GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::WARNING,
            tr("Warning"),
            tr("The current scenario has not been saved to disk yet. Nothing to reload from."),
            {tr("OK")},
            0, 0);
        return;
    }

    // Check if the file still exists on disk
    if (!QFile::exists(fullFileName)) {
        int choice = GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::WARNING,
            tr("Warning"),
            tr("The scenario file could not be found at the following location (it may have been "
                "moved or deleted) :\n%1")
                .arg(fullFileName),
            {tr("OK")}, 0, 0);
        return;
    }

    // Warn the user if there are unsaved changes
    CompareWithScenarioFileResult comparisonResult = compareCurrentScenarioWithFile();

    if (comparisonResult == CompareWithScenarioFileResult::CONTENT_DIFFER) {
        int choice = GbpQMessage::messageBoxQuestion(this,
            GbpQMessage::Type::WARNING, tr("Warning"),
            tr("The current scenario has been modified.<br>"
                "Do you want to discard these changes and reload the scenario from disk?"),
            {tr("Cancel"), tr("Yes")},
            0,  // default button: Cancel
            0); // escape button: Cancel

        if (choice != 1) {  // 1 is the "Yes" button (second button, index 1)
            return; // User pressed Cancel
        }
    }

    // Reload the scenario from disk. Error message will be displayed in loadScenarioFile()
    loadScenarioFile(fullFileName);
}


