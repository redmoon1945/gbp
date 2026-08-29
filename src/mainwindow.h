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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "anonymizedialog.h"
#include "editscenariodialog.h"
#include "presentvaluecalculatordialog.h"
#include "scenariopropertiesdialog.h"
#include "selectcurrencydialog.h"
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
    /**
     * @brief Constructs the main window and initializes the full UI.
     * @details Builds all child dialogs, sets up chart series, connects signals/slots,
     * and adjusts fonts for widget groups that require a size reduction relative to the
     * base font. QApplication::font() is always used as the base for all calculations,
     * and fonts are set explicitly on every widget group including toolbar buttons and
     * menus. This ensures consistent behaviour across all platforms and desktop
     * environments: on KDE, QToolButton and QMenuBar/QMenu would otherwise be overridden
     * with separate Toolbar/Menu font roles that ignore QApplication::setFont().
     * @param systemLocale Locale derived from the system or from a command-line override,
     *        used for number and date formatting throughout the application.
     * @param parent Optional parent widget; nullptr makes this a top-level window.
     */
    MainWindow(QLocale systemLocale, QWidget *parent = nullptr);
    ~MainWindow();

    /**
     * @brief Displays the first-run welcome screen (English or French, based on `locale`).
     * @details Called once from main(), right after construction, when no settings file was
     * found at startup. Delegates to viewResourceFile().
     */
    void showWelcomeScreen();

signals:
    // For Edit Scenario : prepare content before edition
    void signalEditScenarioPrepareContent(CurrencyInfo currInfo);
    // For Options Edition : prepare content before edition
    void signalOptionsPrepareContent();
    // For Analysis Dialog : prepare content before edition
    void signalAnalysisPrepareContent(QWeakPointer<CombinedFeStreams> chartRawDataRef,
        Tags availabletags, CurrencyInfo currInfo, double startingAmount);
    // For DateInterval Dialog : prepare content before edition
    void signalDateIntervalPrepareContent(QDate from, QDate to);
    //
    void signalSelectCountryPrepareContent();
    // For Scenario Properties display : prepare content before edition
    void signalScenarioPropertiesPrepareContent();
    // For About Dialog : prepare content before edition
    void signalAboutDialogPrepareContent(QLocale theLocale);
    // For PV Dialog : prepare content before edition
    void signalPvDialogPrepareContent();
    // For AnonymizeDialog : : prepare content before edition
    void signalAnonymizePrepareContent();

public slots:
    /**
     * @brief Follow-up to "New Scenario" menu selection (2nd step). From this point, a new empty
     * scenario will be created and will become the current scenario.
     * @param currInfo Currency Information.
     */
    void slotSelectCountryResult(CurrencyInfo currInfo);

    void slotSelectCountryCompleted();
    // From Edit Scenario : result and edition completion notification

    /**
     * @brief Scenario Edition has been "applied" : update accordingly.
     * @param regenerateData True means all FE list must be recalculated from scratch.
     */
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
    // From Anonymize Edit : result and edition completion notification
    void slotAnonymizeResult(AnonymizeDialog::AnonymizeOptions opts);
    void slotAnonymizeCompleted();
    /**
     * @brief to catch point selection signal in main chart.
     * @param pt Selected point on the curve.
     */
    void mypoint_clicked(QPointF pt);

    // to catch scale change in an axis in main chart
    void handleXaxisRangeChange(QDateTime dtFrom, QDateTime dtTo);
    void handleYaxisRangeChange(qreal min, qreal max);

protected:
    void closeEvent(QCloseEvent * event) override;
    void showEvent(QShowEvent * event) override;

private slots:

    void on_actionQuit_triggered(); //> this generate a "close-event"
    void on_actionAbout_triggered();
    void on_actionAbout_Qt_triggered();
    void on_actionOpen_triggered();
    void on_actionOpen_Example_triggered();
    void on_actionSave_As_triggered();
    void on_actionSave_triggered();
    void on_actionEdit_triggered();
    void on_actionProperties_triggered();
    void on_actionNew_triggered();
    void on_actionOptions_triggered();
    void on_actionAnalysis_triggered();
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
    void on_toolButton_Max_clicked();
    void on_toolButton_Fit_clicked();
    void on_toolButton_EOY_clicked();
    void on_customToolButton_clicked();
    void on_showPointsCheckBox_stateChanged(int arg1);
    void on_showGridlinesCheckBox_stateChanged(int arg1);
    void on_toolButton_Right_clicked();
    void on_toolButton_Left_clicked();
    void on_exportTextFilePushButton_clicked();
    void on_baselineDoubleSpinBox_editingFinished();
    void on_actionUser_Manual_triggered();
    void on_actionQuick_Tutorial_triggered();
    void on_actionChange_Log_triggered();
    void on_actionPV_Calculator_triggered();
    void on_actionReload_triggered();
    void on_actionAnonymize_triggered();

    // We do the explicit connection, so we drop the on_... naming convention entirely
    void actionClear_List_triggered();
    void actionRecentFile_triggered();


private:

    Ui::MainWindow *ui;

    // *** Struct and enums ***

    enum class X_RESCALE {X_RESCALE_NONE, X_RESCALE_CUSTOM, X_RESCALE_DATA_MAX,
        X_RESCALE_SCENARIO_MAX};

    /**
     * @brief Used when comparing scenario in memory with counter part on disk.
     */
    enum class CompareWithScenarioFileResult {CONTENT_IDENTICAL, CONTENT_DIFFER,
        NOT_SAVED, NO_SCENARIO_LOADED, SCENARIO_FILE_GONE, ERROR_LOADING_SCENARIO};

    struct xAxisRescale{
        X_RESCALE mode;
        QDateTime from; // used only when mode = X_RESCALE_CUSTOM=1
        QDateTime to;   // used only when mode = X_RESCALE_CUSTOM=1
    };

    /**
     * @brief Outcome codes for MainWindow::viewResourceFile().
     */
    enum class ViewResourceFileErrorCode {
        VRF_SUCCESS,                    ///< File was cached (or already up to date) and opened OK.
        VRF_RES_FILE_DOES_NOT_EXIST,    ///< The requested resource file does not exist.
        VRF_CACHE_DIR_UNAVAILABLE,      ///< The per-user cache directory could not be resolved or
                                        ///< created.
        VRF_TEMP_FILE_DELETION_ERROR,   ///< Stale cached copy could not be deleted (e.g. locked by
                                        ///< another process).
        VRF_RES_FILE_COPY_ERROR,        ///< Resource file could not be copied to the cache dir.
        VRF_FAIL_CLEARING_RO_ATTRIBUTE, ///< Read-only attribute could not be cleared on the cached
                                        ///< copy.
        VRF_ERROR_OBTAINING_URL,        ///< Could not build a valid QUrl from the cached file's
                                        ///< local path.
        VRF_ERROR_LAUNCHING_VIEWER      ///< OS declined or failed to launch a viewer for the file.
    };

    /**
     * @brief Result of a MainWindow::viewResourceFile() call.
     */
    struct ViewResourceFileResult{
        ViewResourceFileErrorCode code = ViewResourceFileErrorCode::VRF_SUCCESS; ///< Outcome code.
        QString data; ///< Optional extra context (e.g. offending file name), empty if unused.
    };

    // *** Children Dialogs pointers ***

    EditScenarioDialog* editScenarioDlg; // no parent
    PresentValueCalculatorDialog* pvCalculatorDlg; // no parent
    SelectCurrencyDialog* selectCurrencyDialog;
    OptionsDialog* optionsDlg;
    AboutDialog* aboutDlg;
    AnalysisDialog* analysisDlg;
    DateIntervalDialog* dateIntervalDlg;
    ScenarioPropertiesDialog* scenarioPropertiesDlg;
    AnonymizeDialog* anonymizeDlg;


    // *** misc variables ***

    int maxRecentFiles = 10;
    QList<QAction*> recentFileActionList;
    QLocale locale;                 // default locale to use, as determined at startup


    // *** variables for the chart ***

    double chartScalingFactor;      // see QChart. 1 means no zoom
    CustomQChartView *chartView;
    QChart *chart ;
    QLineSeries *shadowSeries;          // shadow the points just to trace line between them
    QScatterSeries* scatterSeries;      // contains only the real points
    QLineSeries *zeroYvalueLineSeries;  // line at y value = 0
    QDateTimeAxis *axisX;
    QValueAxis *axisY;
    QDateTime fullFromDateX;        // tomorrow
    QDateTime fullToDateX;          // max date of calculation for the current scenario
    uint xAxisFontSize;
    uint yAxisFontSize;
    int indexLastPointSelected = -1;
    /**
     * @brief The final stream of financial events generated for the scenario. This is
     * what is displayed in the Cash Balance curve an Analysis module. MainWindow object
     * is the owner of chartRawData.
     */
    QSharedPointer<CombinedFeStreams> chartRawData; // generated data for Cash Balance curve


    // *** methods ***

    // Chart-related stuff

    /**
     * @brief Regenerate the Scenario Flow Data (that is the chartRawData). Do not touch the chart,
     * series or axis. All references to CSD QSharedPointers are destroyed and rebuilt.
     * @param timeData List of final amount per day.
     * @param shadowTimeData List of final amount per day, plus fake points to simulate steps
     * in line curve
     */
    void regenerateRawData(QList<QPointF>& timeData, QList<QPointF>& shadowTimeData);

    /**
     * @brief From existing chartRawData (which is not modified), rebuild chart's series and set
     * characteristics (like Colors). data and shadowData are coming from chartRawData.
     * Does not update the Chart (rescaling).
     * @param data List of final amount per day.
     * @param shadowData List of final amount per day, plus fake points to simulate steps in
     * line curve
     */
    void replaceChartSeries(QList<QPointF> data, QList<QPointF> shadowData);

    /**
     * @brief Rescale both axis of the chart. Y axis is always auto-scaled. Chart's data is
     * NOT changed.A scenario must be loaded.
     * @param xAxisRescaleMode How the xAxis will be rescaled.
     * @param addMarginAroundXaxis Normally true. It means X axis limit are extended by the
     * rescaling factor" set in Options, in order to prevent border point to fall directly on Y
     * axis. For "shitfing" however, we want this turned Off (false)
     */
    void rescaleChart(xAxisRescale xAxisRescaleMode, bool addMarginAroundXaxis);

    /**
     * @brief Used ONLY by the toolbuttons above the Cash Balance curve. A scenario must be loaded.
     * Y axis is rescaled accordingly.
     * @param noOfMonths No of month to rescale the X axis.
     */
    void rescaleXaxis(uint noOfMonths);

    void shiftGraph(bool toTheRight);
    void themeChanged();
    void setSeriesCharacteristics();
    void reduceAxisFontSize();
    void setXaxisFontSize(uint fontSize);
    void setYaxisFontSize(uint fontSize);

    /**
     * @brief The format is set in the Options.
     */
    void setXaxisDateFormat();

    void initChart();

    // misc

    /**
     * @brief Load a scenario file into a Scenario object.
     * @param fileName The file name to load from.
     * @return true if successful, false otherwise
     */
    bool loadScenarioFile(QString fileName);

    /**
     * @brief Save current scenario under the provided filename. No error message is displayed,
     * but loggin is performed.
     * @param fileName Name of the file to save the scenario into.
     * @return Info about the operation result.
     */
    Scenario::FileResult saveScenario(QString fileName);

    /**
     * @brief Deprecated : now do nothing
     * @param msg Message to display.
     */
    void msgStatusbar(QString msg);

    /**
     * @brief Rebuild the menu from scratch, empty it (no recent files).
     */
    void recentFilesMenuInit();

    void recentFilesMenuUpdate();
    bool eventFilter(QObject *object, QEvent *event) override;

    /**
     * @brief Fill the "Daily Info" Section related to the point selected on the Cash Balance curve.
     * @param date Date of the selected point.
     * @param amount Total cummultaive amount for the selected date.
     * @param di Full info about the selected point.
     */
    void fillDailyInfoSection(const QDate& date, double amount,
        const CombinedFeStreams::DailyInfo& di);


    void emptyDailyInfoSection();

    /**
     * @brief Refresh the content of the "General Info" section.
     * @details A scenario must have been loaded.
     */
    void fillGeneralInfoSection();

    /**
     * @brief Erase the content of the "General Info" section.
     */
    void emptyGeneralInfoSection();

    /**
     * @brief Current Scenario has changed, reset the baseline widgets.
     */
    void resetBaselineWidgets();

    /**
     * @brief Compare current scenario in memory with its counterpart on disk.
     * @return
     */
    CompareWithScenarioFileResult compareCurrentScenarioWithFile();

    /**
     * @brief We are about to switch the current scenario and proceeed to the next task.
     * @details Ask the user if he wants to save the current "modified" scenario.
     * 3 possible answers :
     *   Yes : Save currently modified (or unsaved new) scenario and proceed to the next task.
     *   No : Do not save currently modified (or unsaved new) scenario and proceed to the next task.
     *   Cancel : Do nothing and do NOT proceed to the next task.
     * @return true : we can proceeed further (user may be asked), false : do NOT proceed
     * forward (Cancel button or ESC pressed)
     */
    bool aboutToSwitchScenario();

    /**
     * @brief Displays a bundled Qt resource file (e.g. a PDF) using the OS default viewer.
     *
     * @details A resource file (":/...") lives inside the compiled binary and cannot be handed
     * directly to an external application. This method therefore:
     *   1. Checks that the resource file exists.
     *   2. Copies it to a per-user, per-app cache directory
     *      (QStandardPaths::CacheLocation), under a destination name that embeds the current
     *      application version (e.g. "gbp_1.8.0_gbp_changelog - en.pdf"), so a version upgrade
     *      never reuses a stale cached copy.
     *   3. If a cached copy with that name already exists, compares its SHA-1 hash against the
     *      resource file's content and skips the copy when they match (avoids redundant writes
     *      on repeated views; also catches resource content changes made during development
     *      without a version bump).
     *   4. Clears the read-only attribute on the cached copy (Qt resource files are read-only,
     *      and QFile::copy() preserves that bit on Windows), so it can be deleted/replaced on a
     *      later call without an "access denied" error.
     *   5. Opens the cached file via QDesktopServices::openUrl(), which launches whatever
     *      application the OS associates with the file's extension.
     *
     * Resource file naming convention (input parameter): all lowercase, words separated by "_",
     * always ending with a language suffix (e.g. "- en"), with a mandatory file extension.
     *
     * @param resourceFullFileName Full path of the resource file to view
     *        (e.g. ":/Doc/resources/gbp_changelog - en.pdf"). Must already exist in the
     *        application's resources.
     * @param result [out] Outcome of the operation. On success, result.code is set to
     *        ViewResourceFileErrorCode::VRF_SUCCESS. On failure, result.code identifies which
     *        step failed (see ViewResourceFileErrorCode) and result.data optionally carries
     *        extra context (e.g. the offending file name).
     */
    void viewResourceFile(const QString resourceFullFileName, ViewResourceFileResult &result);

    void changeYaxisLabelFormat();
    void setWindowTopTitle();
    void adjustMenuItemLength();
};
#endif // MAINWINDOW_H
