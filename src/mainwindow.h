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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "editscenariodialog.h"
#include "scenariopropertiesdialog.h"
#include "selectcountrydialog.h"
#include "optionsdialog.h"
#include "aboutdialog.h"
#include "analysisdialog.h"
#include "dateintervaldialog.h"
#include <QMainWindow>
#include <QFileDialog>
#include <QLocale>
#include <QChart>
#include <QChartView>
#include <QDateTimeAxis>
#include <QValueAxis>
#include "combinedfestreams.h"
#include "scenario.h"
#include "customqchartview.h"
#include <QUuid>

QT_BEGIN_NAMESPACE
namespace Ui {class MainWindow;}
QT_END_NAMESPACE



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QLocale systemLocale, QWidget *parent = nullptr);
    ~MainWindow();

signals:
    // For Edit Scenario : prepare content before edition
    void signalEditScenarioPrepareContent(QString countryCode, CurrencyInfo currInfo);
    // For Options Edition : prepare content before edition
    void signalOptionsPrepareContent();
    // For Analysis Dialog : prepare content before edition
    void signalAnalysisPrepareContent(QMap<QDate,CombinedFeStreams::DailyInfo> chartRawData,
        CurrencyInfo currInfo);
    // For DateInterval Dialog : prepare content before edition
    void signalDateIntervalPrepareContent(QDate from, QDate to);
    //
    void signalSelectCountryPrepareContent();
    // For Scenario Properties display : prepare content before edition
    void signalScenarioPropertiesPrepareContent();
    // For About Dialog : prepare content before edition
    void signalAboutDialogPrepareContent(QLocale theLocale);

public slots:
    // From SelectCountry Dialog : receive result and notification of edition completion
    void slotSelectCountryResult(QString countryCode, CurrencyInfo currInfo);
    void slotSelectCountryCompleted();
    // From Edit Scenario : result and edition completion notification
    void slotEditScenarioResult(bool regenerateData);
    void slotEditScenarioCompleted();
    // From Options Edit : result and edition completion notification
    void slotOptionsResult(OptionsDialog::OptionsChangesImpact impact);
    void slotOptionsCompleted();
    // From DateInterval Edit : result and edition completion notification
    void slotDateIntervalResult(QDate from, QDate to);
    void slotDateIntervalCompleted();
    // From ScenarioProperties : completion notification
    void slotScenarioPropertiesCompleted();
    // to catch point selection signal in main chart
    void mypoint_clicked(QPointF pt);
    // to catch scale change in an axis in main chart
    void handleXaxisRangeChange(QDateTime dtFrom, QDateTime dtTo);
    void handleYaxisRangeChange(qreal min, qreal max);

protected:
    void closeEvent(QCloseEvent * event) override;

private slots:

    void on_actionQuit_triggered();
    void on_actionAbout_triggered();
    void on_actionAbout_Qt_triggered();
    void on_actionOpen_triggered();
    void on_actionOpen_Example_triggered();
    void on_actionSave_As_triggered();
    void on_actionSave_triggered();
    void on_actionEdit_triggered();
    void on_actionProperties_triggered();
    void on_actionNew_triggered();
    void on_actionClear_List_triggered();
    void on_actionRecentFile_triggered();
    void on_actionOptions_triggered();
    void on_actionAnalysis_triggered();
    void on_toolButton_Max_clicked();
    void on_toolButton_3M_clicked();
    void on_toolButton_1M_clicked();
    void on_toolButton_6M_clicked();
    void on_toolButton_1Y_clicked();
    void on_toolButton_2Y_clicked();
    void on_toolButton_3Y_clicked();
    void on_toolButton_4Y_clicked();
    void on_toolButton_5Y_clicked();
    void on_toolButton_10Y_clicked();
    void on_toolButton_15Y_clicked();
    void on_toolButton_20Y_clicked();
    void on_toolButton_25Y_clicked();
    void on_toolButton_50Y_clicked();
    void on_toolButton_Fit_clicked();
    void on_showPointsCheckBox_stateChanged(int arg1);
    void on_toolButton_Right_clicked();
    void on_toolButton_Left_clicked();
    void on_exportTextFilePushButton_clicked();
    void on_baselineDoubleSpinBox_editingFinished();
    void on_customToolButton_clicked();
    void on_actionUser_Manual_triggered();
    void on_actionQuick_Tutorial_triggered();
    void on_actionChange_Log_triggered();


private:

    Ui::MainWindow *ui;

    enum class X_RESCALE {X_RESCALE_NONE, X_RESCALE_CUSTOM, X_RESCALE_DATA_MAX,
        X_RESCALE_SCENARIO_MAX};
    struct xAxisRescale{
        X_RESCALE mode;
        QDateTime from; // used only when mode = X_RESCALE_CUSTOM=1
        QDateTime to;   // used only when mode = X_RESCALE_CUSTOM=1
    };

    // Used when comparing scenario in memory with counter part on disk
    enum CompareWithScenarioFileResult {CONTENT_IDENTICAL, CONTENT_DIFFER,
        NOT_SAVED, NO_SCENARIO_LOADED, ERROR_LOADING_SCENARIO};

    // Dialogs
    EditScenarioDialog* editScenarioDlg;
    SelectCountryDialog* selectCountryDialog;
    OptionsDialog* optionsDlg;
    AboutDialog* aboutDlg;
    AnalysisDialog* analysisDlg;
    DateIntervalDialog* dateIntervalDlg;
    ScenarioPropertiesDialog* scenarioPropertiesDlg;

    // variables
    int maxRecentFiles = 10;
    QList<QAction*> recentFileActionList;
    double chartScalingFactor;      // see QChart. 1 means no zoom
    QLocale locale;                 // system locale, as determined when constructor un

    // variables for the chart
    CustomQChartView *chartView;
    QChart *chart ;
    QLineSeries *shadowSeries;          // shadow the points just to trace line between them
    QScatterSeries* scatterSeries;      // contains only the real points
    QLineSeries *zeroYvalueLineSeries;  // line at y value = 0
    QDateTimeAxis *axisX;
    QValueAxis *axisY;
    QMap<QDate,CombinedFeStreams::DailyInfo> chartRawData;
    QDateTime fullFromDateX;        // tomorrow
    QDateTime fullToDateX;          // max date of calculation for the current scenario
    uint xAxisFontSize;
    uint yAxisFontSize;
    int indexLastPointSelected = -1;

    // *** methods ***

    // Chart-related stuff
    void regenerateRawData(QList<QPointF>& timeData, QList<QPointF>& shadowTimeData);
    void replaceChartSeries(QList<QPointF> data, QList<QPointF> shadowData);
    void rescaleChart(xAxisRescale xAxisRescaleMode, bool addMarginAroundXaxis);
    void rescaleXaxis(uint noOfMonths);
    void shiftGraph(bool toTheRight);
    void themeChanged();
    void setSeriesCharacteristics();
    void reduceAxisFontSize();
    void setXaxisFontSize(uint fontSize);
    void setYaxisFontSize(uint fontSize);
    void setXaxisDateFormat();
    void initChart();

    // misc
    bool loadScenarioFile(QString fileName);
    Scenario::FileResult saveScenario(QString fileName);
    void msgStatusbar(QString msg);
    void recentFilesMenuInit();
    void recentFilesMenuUpdate();
    bool eventFilter(QObject *object, QEvent *event) override;
    void fillDailyInfoSection(const QDate& date, double amount,
        const CombinedFeStreams::DailyInfo& di);
    void emptyDailyInfoSection();
    void fillGeneralInfoSection();
    void emptyGeneralInfoSection();
    void resetBaselineWidgets();
    CompareWithScenarioFileResult compareCurrentScenarioWithFile();
    bool aboutToSwitchScenario();
    void changeYaxisLabelFormat();
    uint calculateTotalNoOfEvents();
    void setWindowTopTitle();
    void adjustMenuItemLength();
};
#endif // MAINWINDOW_H
