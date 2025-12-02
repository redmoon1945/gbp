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

#ifndef GBPCONTROLLER_H
#define GBPCONTROLLER_H

#include "scenario.h"
#include <QColor>
#include <QSettings>
#include <QSharedPointer>
#include <memory>

/**
 * @brief Singleton owning the current scenario, the settings and the knowledge of
 * what "today" and "tomorrow" means.
 */
class GbpController
{

public:

    /**
     * @brief Define ow the QtChart theming will be set. From 1.7.0.
     * @note 1.6.3- uses a bool with true that is equivalent to FORCE_DARK and
     * false that is equivalent to FORCE_LIGHT. FOLLOW_DESKTOP_THEME does not exist in 1.6.3-.
     */
    enum class ChartTheming {FORCE_LIGHT, FORCE_DARK, FOLLOW_DESKTOP_THEME};


    /**
     * @brief Regroup all the GBP default values for all settings parameters.
     */
    struct FactorySettings{
        uint chartPointSize = 10;
        ChartTheming chartTheming = ChartTheming::FOLLOW_DESKTOP_THEME;
        QColor darkModeCurveColor = QColor(128, 0, 0);
        QColor lightModeCurveColor = QColor(0, 0, 128);
        QColor darkModePointColor = QColor(255, 0, 0);
        QColor lightModePointColor = QColor(0, 0, 255);
        QColor darkModeSelectedPointColor = QColor(0, 255, 0);
        QColor lightModeSelectedPointColor = QColor(0, 128, 0);
        bool exportTextAmountLocalized = false;
        bool exportTextDateLocalized = false;
        uint percentageMainChartScaling = 5;
        bool useDefaultSystemFont = true;
        QString customApplicationFont = "";
        bool todayUseSystemDate = true;
        QDate todayCustomDate = QDateTime::currentDateTime().toLocalTime().date();
        bool allowDecorationColor = true;
        bool usePresentValue = false;
        double pvDiscountRate = 0;
        bool wheelRotatedAwayZoomIn = false;
        bool showYzeroLine = true;
        QColor yZeroLineLightModeColor = QColor(0, 128,128);
        QColor yZeroLineDarkModeColor = QColor(0, 128, 128);
        uint xAxisDateFormat =0;
        bool showTooltips = true;
        QColor incomeColor = Util::getOptimizedGreen();
        QColor expenseColor = Util::getOptimizedRed();
    };


    // methods
    static GbpController& getInstance();
    void closeScenario();
    bool isScenarioLoaded() const;

    /**
     * @brief Load the application settings from the config "ini" file. This must be done
     * only ONCE, as EARLY AS possible after the application is started.
     * @details It is possible there is no INI file to load from. In that case, a NEW one will
     * be created with factory settings in.
     */
    void loadSettings();


    void saveSettings();
    void resetSettings();
    void recentFilenamesAdd(QString newFilename, int maxNoOfEntries);
    void recentFilenamesClear();

    /**
     * @brief Get a weak reference to the current scenario. A weak reference is returned
     * because GbpController is the sole owner.
     * @details If no scenario loaded yet, an empty QWeakPointer<Scenario> is returned.
     * @return A weak reference to the currently loaded scenario.
     */
    QWeakPointer<Scenario> getScenario() const;

    /**
     * @brief Convert an ChartTheming enum value to an int. This is used when serializing
     * to a JSON object or as a QVariant.
     * @details The following is always true :
     *   0 -> ChartTheming::FORCE_LIGHT
     *   1 -> ChartTheming::FORCE_DARK
     *   2 -> ChartTheming::FOLLOW_DESKTOP_THEME
     * @note GBP 1.6.3 used a bool with true=FORCE_DARK and false=FORCE_LIGHT
     * @throws std::invalid_argument if theme is unknown.
     * @param theme ChartTheming enum value to convert.
     * @return The equivalent int.
     */
    static int chartThemingFromEnumToInt(ChartTheming theme);

    /**
     * @brief Convert an int to a ChartTheming enum value. This is used when serializing
     * from a JSON object or as a QVariant.
     * @details The following is always true :
     *   ChartTheming::FORCE_LIGHT -> 0
     *   ChartTheming::FORCE_DARK -> 1
     *   ChartTheming::FOLLOW_DESKTOP_THEME -> 2
     * @param value The int to convert.
     * @param convertedTheme Result of the conversion. Valid only if conversion succeeded.
     * @return True if the conversion succeeded, false otherwise (int value is invalid).
     */
    static bool chartThemingFromIntToEnum(int value, ChartTheming& convertedTheme);

    /**
     * @brief Convert a ChartTheming enum to an English string representation.
     * @param theme ChartTheming enum value to convert.
     * @return The string representation
     */
    static QString chartThemingToString(ChartTheming theme);

    /**
     * @brief Indicates if Qcharts should be set to dark mode.
     * @details Return true if :
     *   chartTheming = FORCE_DARK
     *     OR
     *   (chartTheming = FOLLOW_DESKTOP_THEME) AND (systemDesktopDarkTheme=true)
     * @return true if Qcharts should be set to dark mode, false otherwise.
     */
    bool useDarkModeForChart();

    // methods forbidden for a singleton
    GbpController(const GbpController&) = delete;
    GbpController(GbpController&&) = delete;
    GbpController& operator=(const GbpController&) = delete;
    GbpController& operator=(GbpController&&) = delete;

    // Getters and setters, for elements in config file
    QStringList getRecentFilenames() const;
    void setRecentFilenames(const QStringList &newRecentFilenames);
    uint getChartPointSize() const;
    void setChartPointSize(uint newChartPointSize);
    GbpController::ChartTheming getChartTheming() const;
    void setChartTheming(GbpController::ChartTheming newchartTheming);
    QColor getDarkModeCurveColor() const;
    void setDarkModeCurveColor(const QColor &newDarkModeCurveColor);
    QColor getLightModeCurveColor() const;
    void setLightModeCurveColor(const QColor &newLightModeCurveColor);
    QColor getDarkModePointColor() const;
    void setDarkModePointColor(const QColor &newDarkModePointColor);
    QColor getLightModePointColor() const;
    void setLightModePointColor(const QColor &newLightModePointColor);
    QColor getDarkModeSelectedPointColor() const;
    void setDarkModeSelectedPointColor(const QColor &newDarkModeSelectedPointColor);
    QColor getLightModeSelectedPointColor() const;
    void setLightModeSelectedPointColor(const QColor &newLightModeSelectedPointColor);
    bool getExportTextAmountLocalized() const;
    void setExportTextAmountLocalized(bool newExportTextAmountLocalized);
    bool getExportTextDateLocalized() const;
    void setExportTextDateLocalized(bool newExportTextDateLocalized);
    QString getLastDir() const;
    void setLastDir(const QString &newLastDir);
    uint getPercentageMainChartScaling() const;
    void setPercentageMainChartScaling(uint newPercentageMainChartScaling);
    bool getUseDefaultSystemFont() const;
    void setUseDefaultSystemFont(bool newUseDefaultSystemFont);
    QString getCustomApplicationFont() const;
    void setCustomApplicationFont(const QString &newCustomApplicationFont);
    bool getTodayUseSystemDate() const;
    void setTodayUseSystemDate(bool newTodayUseSystemDate);
    QDate getTodayCustomDate() const;
    void setTodayCustomDate(const QDate &newTodayCustomDate);
    bool getAllowDecorationColor() const;
    void setAllowDecorationColor(bool newAllowDecorationColor);
    bool getUsePresentValue() const;
    void setUsePresentValue(bool newUsePresentValue);
    double getPvDiscountRate() const;
    void setPvDiscountRate(double newPvDiscountRate);
    QString getFullFileName() const;
    void setFullFileName(const QString &newFullFileName);
    void setScenario(QSharedPointer<Scenario> newScenario);
    bool getWheelRotatedAwayZoomIn() const;
    void setWheelRotatedAwayZoomIn(bool newWheelRotatedAwayZoomIn);
    bool getShowYzeroLine() const;
    void setShowYzeroLine(bool newShowYzeroLine);
    QColor getYZeroLineLightModeColor() const;
    void setYZeroLineLightModeColor(const QColor &newYZeroLineLightModeColor);
    QColor getYZeroLineDarkModeColor() const;
    void setYZeroLineDarkModeColor(const QColor &newYZeroLineDarkModeColor);
    uint getXAxisDateFormat() const;
    void setXAxisDateFormat(uint newXAxisDateFormat);
    bool getShowTooltips() const;
    void setShowTooltips(bool newShowTooltips);
    QColor getIncomeColor() const;
    void setIncomeColor(const QColor &newIncomeColor);
    QColor getExpenseColor() const;
    void setExpenseColor(const QColor &newExpenseColor);
    QString getLastDirImport() const;
    void setLastDirImport(const QString &newLastDirImport);
    QString getLastDirExport() const;
    void setLastDirExport(const QString &newLastDirExport);

    // For elements NOT in INI file
    bool getNoSettingsFileAtStartup() const;
    QDate getToday() const;
    QDate getTomorrow() const;
    QString getInitialSystemApplicationFont() const;
    QString getSettingsFullFileName() const;
    FactorySettings getFactorySettings() const;
    bool getSystemDesktopDarkTheme() const;
    void setSystemDesktopDarkTheme(bool newSystemDesktopDarkTheme);

private:

    // immutable instance
    const FactorySettings factorySettings;

    // ************* data stored in the settings, in .ini file ****************
    // ************* Required getters / setters                ****************

    // List of recent full file names used for scenario (open, save as)
    QStringList recentFilenames;

    // charts characteristics
    uint chartPointSize;
    GbpController::ChartTheming chartTheming;
    QColor darkModeCurveColor;
    QColor lightModeCurveColor;
    QColor darkModePointColor;
    QColor lightModePointColor;
    QColor darkModeSelectedPointColor;
    QColor lightModeSelectedPointColor;

    // Specifies if amounts in Exported CSV file should be localized
    bool exportTextAmountLocalized;

    // Specifies if dates in Exported CSV file should be localized
    bool exportTextDateLocalized;

    // last dir used for opening/saving scenario.
    QString lastDir;

    // last dir used for importing CSV files. New in 1.7.0.
    QString lastDirImport;

    // last dir used for exporting PNG or CSV files. New in 1.7.0.
    QString lastDirExport;

    // how much space is given on the chart above X&Y axis min/max, in percentage over 100%
    uint percentageMainChartScaling;

    // Fonts
    bool useDefaultSystemFont;
    QString customApplicationFont;

    // If true, today's date if determined by real date-time (this is the default). If false,
    // it is set using the value "todayCustomDate"
    bool todayUseSystemDate;
    QDate todayCustomDate;

    // Allow names of Financial Stream Def to have specific colors
    bool allowDecorationColor;

    // if true, all calculated FE amounts are converted to present value using the pvDiscountRate
    bool usePresentValue;

    // ANNUAL discount rate for PV calculation, in percentage
    double pvDiscountRate;

    // If true : vertical wheel rotating AWAY from the user will ZOOM IN
    // If False : vertical wheel rotating TOWARD the user will ZOOM IN
    bool wheelRotatedAwayZoomIn;

    // If true : show Y=0 line on the chart as grey dash line
    // If False : don't show
    bool showYzeroLine;

    // If showYzeroLine==true, this is the color of the line drawn.
    // If false, value is irrelevant
    QColor yZeroLineLightModeColor;
    QColor yZeroLineDarkModeColor;

    // X-Axis Date format. 0=Locale  1=ISO  2=ISO with 2-digits year
    uint xAxisDateFormat;

    // Show or hide tooltips for the application
    bool showTooltips;

    /**
     * @brief Color for income (or positive) amount or income barchart. Since 1.7.
     */
    QColor incomeColor;

    /**
     * @brief Color for expense (or negative) amount or expense barchart. Since 1.7.
     */
    QColor expenseColor;


    // ****** Variables with getter/setters but not saved in settings file ******

    // path+file name for current scenario. Empty means no file yet assigned (e.g. for new)
    QString fullFileName;

    /**
     * @brief Current loaded Scenario. If equal nullptr, it means there is no loaded scenario yet.
     */
    QSharedPointer<Scenario> scenario;



    // ***** misc other variables NOT stored in settings, with getter only ****

    bool noSettingsFileAtStartup;   // indicates if no ini file were found when gbp started
    // Date of "today" in local time, set ONCE, when the settings is loaded
    QDate today;
    // derived from "today"
    QDate tomorrow;
    // font upon app starts, before anything changed
    QString initialSystemApplicationFont;
    // full file name for the config file
    QString settingsFullFileName;
    // Indicate if system desktop has a dark theme
    bool systemDesktopDarkTheme;

    // ***** Purely internal variables without getters/setters *****
    std::unique_ptr<QSettings> settingsPtr;
    bool settingsLoaded=false;  // to prevent more than 1 loading

    // *** methods ***
    GbpController();
    ~GbpController();
};

//extern GbpController gbpController;
#endif // GBPCONTROLLER_H
