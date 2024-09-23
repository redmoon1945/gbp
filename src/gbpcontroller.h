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
    void recentFilenamesAdd(QString newFilename, int maxNoOfEntries);
    void recentFilenamesClear();
    void log(LogLevel level, LogType type, QString message);
    void cleanUpLogs();
    QDate getToday() const;
    QDate getTomorrow() const;

    // methods forbidden for a singleton
    GbpController(const GbpController&) = delete;
    GbpController(GbpController&&) = delete;
    GbpController& operator=(const GbpController&) = delete;
    GbpController& operator=(GbpController&&) = delete;

    // Getters and setters
    QSharedPointer<Scenario> getScenario() const;
    void setScenario(QSharedPointer<Scenario> newScenario);
    QString getFullFileName() const;
    void setFullFileName(const QString &newFullFileName);
    QString getLastDir() const;
    void setLastDir(const QString &newLastDir);
    QStringList getRecentFilenames() const;
    void setRecentFilenames(const QStringList &newRecentFilenames);
    bool getChartDarkMode() const;
    void setChartDarkMode(bool newChartDarkMode);
    QColor getCurveDarkModeColor() const;
    void setCurveDarkModeColor(const QColor &newCurveDarkModeColor);
    QColor getCurveLightModeColor() const;
    void setCurveLightModeColor(const QColor &newCurveLightModeColor);
    bool getExportTextAmountLocalized() const;
    void setExportTextAmountLocalized(bool newExportTextAmountLocalized);
    uint getPercentageMainChartScaling() const;
    void setPercentageMainChartScaling(uint newPercentageMainChartScaling);
    bool getUseDefaultSystemFont() const;
    void setUseDefaultSystemFont(bool newUseDefaultSystemFont);
    QString getCustomApplicationFont() const;
    void setCustomApplicationFont(const QString &newCustomApplicationFont);
    QString getInitialSystemApplicationFont() const;
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
    QString getSettingsFullFileName() const;
    QString getLogFullFileName() const;


    bool getNoSettingsFileAtStartup() const;

private:

    // ************* data stored in the settings on file ****************

    // List of recent full file names used for scenario (open, save as)
    QStringList recentFilenames;

    bool chartDarkMode=false;
    QColor curveDarkModeColor;
    QColor curveLightModeColor;
    bool exportTextAmountLocalized=false;

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

    bool allowDecorationColor;

    // if true, all calculated FE amounts are converted to present value using the pvDiscountRate
    bool usePresentValue;

    // ANNUAL discount rate for PV calculation, in percentage
    double pvDiscountRate;

    // ****************************************************


    // *** misc variables ***

    // Date of "today" in local time, set ONCE, when the settings is loaded
    QDate today;

     // derived from "today"
    QDate tomorrow;

    // path+file name for current scenario. Empty means no file yet assigned (e.g. for new)
    QString fullFileName;

    QSharedPointer<Scenario> scenario;
    QTextStream logOutStream;
    QString logFolder;
    bool loggingEnabled=false;
    QFile logFile;
    QString logFullFileName;
    LogLevel logLevel;

    // font upon app starts, before anything changed
    QString initialSystemApplicationFont;

    // Settings
    QString settingsFullFileName;   // full file name for the config file
    QSettings* settingsPtr=nullptr; // cant find a way to use QSharedPointer...
    bool settingsLoaded=false;  // to prevent more than 1 loading
    bool noSettingsFileAtStartup;   // no ini file found when gbp has started

    // *** methods ***
    GbpController();
    ~GbpController();
};

//extern GbpController gbpController;
#endif // GBPCONTROLLER_H
