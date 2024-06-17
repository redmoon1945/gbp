/*
 *  Copyright (C) 2024 Claude Dumas <claudedumas63@protonmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
class GbpController
{

public:

    enum LogType{Info, Warning, Error};
    enum LogLevel{NONE=0, Minimal=1, Debug=2};  // Minimal (defaut) contains no data that can be considered as "private", that is
                                                // no file content, no dir or file name, no income or expense name.
                                                // Debug is for full debug process and may contain private data

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
    qint16 getScenarioMaxYearsSpan() const;
    void setScenarioMaxYearsSpan(qint16 newScenarioMaxYearsSpan);
    bool getChartDarkMode() const;
    void setChartDarkMode(bool newChartDarkMode);
    QColor getCurveDarkModeColor() const;
    void setCurveDarkModeColor(const QColor &newCurveDarkModeColor);
    QColor getCurveLightModeColor() const;
    void setCurveLightModeColor(const QColor &newCurveLightModeColor);
    int getChartExportImageQuality() const;
    void setChartExportImageQuality(int newChartExportImageQuality);
    QString getChartExportImageType() const;
    void setChartExportImageType(const QString &newChartExportImageType);
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

private:

    // *** data stored in the settings on file ***
    QStringList recentFilenames;            // List of recent full file names used for scenario (open, save as)
    qint16 scenarioMaxYearsSpan=25;         // no of years to be used to calculate the whole set of Fe by a scenario
    bool chartDarkMode=false;
    QColor curveDarkModeColor;
    QColor curveLightModeColor;
    int chartExportImageQuality;            // 1 to 100
    QString chartExportImageType;           // "PNG" or "JPG"
    bool exportTextAmountLocalized=false;
    QString lastDir;                        // last dir used for opening/saving scenario.
    uint percentageMainChartScaling;        // how much space is given on the chart above X&Y axis min/max, in percentage over 100%
    bool useDefaultSystemFont;
    QString customApplicationFont;
    bool todayUseSystemDate;                 // If true, today's date if determined by real date-time (this is the default). If false, it is set using the value "todayCustomDate"
    QDate todayCustomDate;

    // *** misc variables ***
    QDate today;                        // Date of "today" in local time, set ONCE, when the settings is loaded
    QDate tomorrow;                     // derived from "today"
    QString fullFileName;               // path+file name for current scenario. Empty means no file yet assigned (e.g. for new)
    // logging
    QSharedPointer<Scenario> scenario;
    QTextStream logOutStream;
    QString logFolder;
    bool loggingEnabled=false;
    QFile logFile;
    LogLevel logLevel;
    QString initialSystemApplicationFont;   // font upon app starts, before anything changed
    // Settings
    QSettings* settingsPtr=nullptr; // cant find a way to use QSharedPointer...
    bool settingsLoaded=false;  // to prevent more than 1 loading

    // *** methods ***
    GbpController();
    ~GbpController();
};

//extern GbpController gbpController;
#endif // GBPCONTROLLER_H
