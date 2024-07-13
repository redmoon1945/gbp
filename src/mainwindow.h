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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "editscenariodialog.h"
#include "selectcountrydialog.h"
#include "optionsdialog.h"
#include "aboutdialog.h"
#include "analysisdialog.h"
#include "dateintervaldialog.h"
#include <QMainWindow>
#include <QFileDialog>
#include <QChart>
#include <QChartView>
#include <QDateTimeAxis>
#include <QValueAxis>
#include "combinedfestreams.h"
#include "qcustomplot.h"
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
    void signalEditScenarioPrepareContent(bool isNewScenario,QString countryCode, CurrencyInfo currInfo);
    // For Options Edition : prepare content before edition
    void signalOptionsPrepareContent();
    // For Analysis Dialog : prepare content before edition
    void signalAnalysisPrepareContent(QMap<QDate,CombinedFeStreams::DailyInfo> chartRawData, CurrencyInfo currInfo);
    // For DateInterval Dialog : prepare content before edition
    void signalDateIntervalPrepareContent();
    //
    void signalSelectCountryPrepareContent();

public slots:
    // From SelectCountry Dialog : receive result and notification of edition completion
    void slotSelectCountryResult(QString countryCode, CurrencyInfo currInfo);
    void slotSelectCountryCompleted();
    // From Edit Scenario : result and edition completion notification
    void slotEditScenarioResult(bool currentlyEditingNewScenario);
    void slotEditScenarioCompleted();
    // From Options Edit : result and edition completion notification
    void slotOptionsResult(OptionsDialog::OptionsChangesImpact impact);
    void slotOptionsCompleted();
    // From DateInterval Edit : result and edition completion notification
    void slotDateIntervalResult(QDate from, QDate to);
    void slotDateIntervalCompleted();
    // to catch point selection signal
    void selectionChangedByUser();
    // to catch scale change in an axis
    void rangeChangedX(const QCPRange &newRange);
    void rangeChangedY(const QCPRange &newRange);


protected:
    void closeEvent(QCloseEvent * event) override;

private slots:

    void on_actionQuit_triggered();
    void on_actionAbout_triggered();
    void on_actionAbout_Qt_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_As_triggered();
    void on_actionSave_triggered();
    void on_actionEdit_triggered();
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

private:

    Ui::MainWindow *ui;

    // Dialogs
    EditScenarioDialog* editScenarioDlg;
    SelectCountryDialog* selectCountryDialog;
    OptionsDialog* optionsDlg;
    AboutDialog* aboutDlg;
    AnalysisDialog* analysisDlg;
    DateIntervalDialog* dateIntervalDlg;

    // variables
    int maxRecentFiles = 10;
    QList<QAction*> recentFileActionList;
    double chartScalingFactor;
    QLocale locale;                 // system locale, as determined when constructor un

    // variables for the chart
    QCustomPlot* chart;
    QMap<QDate,CombinedFeStreams::DailyInfo> chartRawData;
    QCPTextElement *chartTitle = nullptr ;
    QDate fullFromDateX;        // tomorrow
    QDate fullToDateX;          // max date of calculation
    double fullFromDoubleX;     // "no fo sec from epoch" version of fullFromDateX
    double fullToDoubleX;       // "no fo sec from epoch" version of fullToDateX
    double fitFromDoubleX;      // "no fo sec from epoch" version of fitFromDateX
    double fitToDoubleX;        // "no fo sec from epoch" version of fitToDateX
    double fullMinY;            // min value of Y for the toal range of raw data
    double fullMaxY;            // max value of Y for the toal range of raw data

    // methods
    void updateScenarioDataDisplayed(bool rescaleXaxis, bool resetBaselineValue);
    bool loadScenarioFile(QString fileName);
    bool saveScenario(QString fileName);
    void msgStatusbar(QString msg);
    void recentFilesMenuInit();
    void recentFilesMenuUpdate();
    bool eventFilter(QObject *object, QEvent *event) override;
    void setChartTitle(QString theTitle);
    void setVisibilityZoomButtons();
    void rescaleYaxis(uint noOfMonths);
    void shiftGraph(bool toTheRight);
    void setChartColors();
    void fillDailyInfoSection(const QDate& date, double amount, const CombinedFeStreams::DailyInfo& di);
    void emptyDailyInfoSection();
    void updateBaselineWidgets(CurrencyInfo currInfo);
    void checkIfCurrentScenarioMatchesDiskVersion(bool& match) const;


};
#endif // MAINWINDOW_H
