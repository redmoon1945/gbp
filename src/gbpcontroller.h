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
#include <QFile>
#include <QColor>
#include <QSettings>
#include <QSharedPointer>

// Singleton intended to :
//     - Be the owner of the current scenario
//     - Manage the configuration settings
//     - Provide logging facilities
//     - Single source of truth as for what "today" is
class GbpController
{

public:

    enum LogType{Info, Warning, Error};

    // no file content, no dir or file name, no income or expense name.
    // Debug is for full debug process and may contain private data
    enum LogLevel{NONE=0, Minimal=1, Debug=2};

    // methods
    static GbpController& getInstance();
    void closeScenario();
    bool isScenarioLoaded() const;
    void loadSettings();
    void saveSettings();
    void resetSettings();
    void recentFilenamesAdd(QString newFilename, int maxNoOfEntries);
    void recentFilenamesClear();
    void log(LogLevel level, LogType type, QString message);
    void cleanUpLogs();


    // methods forbidden for a singleton
    GbpController(const GbpController&) = delete;
    GbpController(GbpController&&) = delete;
    GbpController& operator=(const GbpController&) = delete;
    GbpController& operator=(GbpController&&) = delete;

    // Getters and setters
    QStringList getRecentFilenames() const;
    void setRecentFilenames(const QStringList &newRecentFilenames);
    uint getChartPointSize() const;
    void setChartPointSize(uint newChartPointSize);
    bool getIsDarkModeSet() const;
    void setIsDarkModeSet(bool newIsDarkModeSet);
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
    QSharedPointer<Scenario> getScenario() const;
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

    // Getters only
    bool getNoSettingsFileAtStartup() const;
    QDate getToday() const;
    QDate getTomorrow() const;
    QString getLogFullFileName() const;
    QString getInitialSystemApplicationFont() const;
    QString getSettingsFullFileName() const;
    LogLevel getLogLevel() const;



private:

    // ************* data stored in the settings, in .ini file ****************
    // ************* Required getters / setters                ****************

    // List of recent full file names used for scenario (open, save as)
    QStringList recentFilenames;
    // charts characteristics
    uint chartPointSize;
    bool isDarkModeSet;
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

    // ****** Variables with getter/setters ******
    // path+file name for current scenario. Empty means no file yet assigned (e.g. for new)
    QString fullFileName;
    // Current loaded Scenario
    QSharedPointer<Scenario> scenario;


    // ***** misc other variables with getter only ****
    bool noSettingsFileAtStartup;   // indicates if no ini file were found when gbp started
    // Date of "today" in local time, set ONCE, when the settings is loaded
    QDate today;
    // derived from "today"
    QDate tomorrow;
    // full name of the lo file
    QString logFullFileName;
    // font upon app starts, before anything changed
    QString initialSystemApplicationFont;
    // full file name for the config file
    QString settingsFullFileName;
    // Level of debugging
    LogLevel logLevel;


    // ***** Purely internal variables without getters/setters *****
    QTextStream logOutStream;
    QString logFolder;
    QFile logFile;
    bool loggingEnabled;
    QSettings* settingsPtr=nullptr; // cant find a way to use QSharedPointer...
    bool settingsLoaded=false;  // to prevent more than 1 loading


    // *** methods ***
    GbpController();
    ~GbpController();
};

//extern GbpController gbpController;
#endif // GBPCONTROLLER_H
