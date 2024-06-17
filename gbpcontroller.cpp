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

#include "gbpcontroller.h"
#include "qfont.h"
#include <QDir>
#include <QString>
#include <QStandardPaths>
#include <QApplication>
#include <QDirIterator>



GbpController::GbpController()
{
    scenario=nullptr;
    lastDir = QDir::homePath();
    fullFileName = "";
    chartExportImageQuality = 75;
    chartExportImageType = "PNG";
    recentFilenames.clear();
    logLevel = LogLevel::Minimal;
    useDefaultSystemFont = true;
    customApplicationFont = "";
    initialSystemApplicationFont = QApplication::font().toString();
    todayUseSystemDate = true;
    todayCustomDate = QDate();

    QStringList argList = QCoreApplication::arguments();

    // Process arguments that could affect options
    for (int i = 0; i < argList.size(); ++i) {      // search arg that set loglevel
        if ( argList.at(i).length() != 11){
            continue;
        }
        if (false == argList.at(i).startsWith("-loglevel=")) {
            continue;
        }
        QString s = argList.at(i).last(1);
        bool ok;
        int d = s.toInt(&ok);
        if (ok==false){
            continue;
        }
        if(d==1){
            logLevel = LogLevel::Minimal;
        } else if(d==2){
            logLevel = LogLevel::Debug;
        } else {
            logLevel = LogLevel::NONE;
            loggingEnabled = false;
        }
    }

    // Initi the value of "today".
    // Set ONCE the date corresponding to "today" and "tomorrow". There are 3 reasons why we dont want the app to call everywhere "QDate::currentDate()"
    // 1) the app could have been started near midnight and the transition to another day while running could mess up things (for log for example)
    // 2) we want to offer the option of setting the "today" date as configuration parameter (in the "options")
    // 3) we want to make sure today is in LOCAL time
    today = QDateTime::currentDateTime().toLocalTime().date();
    tomorrow = today.addDays(1);

    // *** LOG FILES ***

    // determine where to store the logs. try in QStandardPaths::AppDataLocation/logs (linux : ~/.local/share/graphical-budget-planner/logs), else use QDir::tempPath()/gbp
    bool success=false;

    QString p = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!p.isEmpty()){
        QDir dir(p);
        if (true == dir.mkpath("logs") ){
            dir.cd("logs");
            logFolder = dir.absolutePath();
            success = true;
        }
    }
    if(success==false){
        logFolder = QDir::tempPath();
        QDir dir(logFolder);
        if ( true == dir.mkpath("gbp") ) {
            dir.cd("gbp");
            logFolder = dir.absolutePath();
        }
    }

    // open log file and create out stream
    QString logFileName = QString("%1/%2__%3.txt").arg(logFolder).arg(today.toString("yyyy-MM-dd")).arg(QTime::currentTime().toString("hh_mm_ss"));
    logFile.setFileName(logFileName);
    success = logFile.open(QIODevice::Append | QIODevice::WriteOnly | QIODevice::Text);
    if (success == false){
        loggingEnabled = false;
        QString errorString = QString("Logging is disabled (cannot create a log file in %1)").arg(logFolder);
        qWarning().noquote() << errorString;
    } else {
        loggingEnabled = true;
        QString successString = QString("Log file created : %1, log level=%2").arg(logFileName).arg(logLevel);
        qInfo().noquote() << successString;
    }
    logOutStream.setDevice(&logFile);
    logOutStream.setEncoding(QStringConverter::Utf8);

    // *** SETTINGS ***

    // define default path for settings before creating GbpController (we dont want OrganizationName be part of the path)
    // One choose INI file structure (favor decentralization, portability and human readability)
    QString settingsFile = QString("%1/%2.ini").arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)).arg(QCoreApplication::applicationName());
    settingsPtr = new QSettings(settingsFile, QSettings::IniFormat);

}


GbpController::~GbpController()
{
    delete settingsPtr;
}


GbpController& GbpController::getInstance()
{
    static GbpController _instance;
    return _instance;
}


void GbpController::closeScenario()
{
    scenario=nullptr;
    fullFileName = "";
}


bool GbpController::isScenarioLoaded() const
{
    return ( (scenario==nullptr) ? (false) : (true) );
}


// This should be done only ONCE, as early as possible after the applicationis started
void GbpController::loadSettings()
{
    // Ensure loading has not been already done
    if (settingsLoaded==true) {
        return;
    }

    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Attempting to load settings from %1").arg(settingsPtr->fileName()));

    QVariant v;
    // recent open files list
    if (settingsPtr->contains("recent_files")){
        v = settingsPtr->value("recent_files");
        recentFilenames = v.toStringList(); // empty list if conversion failed
    } else{
        recentFilenames = {};
    }

    // no of years scenario's Fe list is generated
    if (settingsPtr->contains("scenario_max_years")){
        bool ok;
        v = settingsPtr->value("scenario_max_years");
        scenarioMaxYearsSpan = v.toInt(&ok);
        if ( (!ok) || (scenarioMaxYearsSpan<1) || (scenarioMaxYearsSpan>100) )  {
            // settings is invalid
            scenarioMaxYearsSpan = 50;
        }
    } else{
        scenarioMaxYearsSpan = 50;
    }

    // dark mode for chart
    if (settingsPtr->contains("chart_dark_mode")){
        v = settingsPtr->value("chart_dark_mode");
        bool ok = Util::isValidBoolString(v.toString());
        if(ok){
            chartDarkMode = v.toBool();
        } else {
            chartDarkMode = false;
        }
    } else{
        chartDarkMode = false;;
    }

    // curve color for dark mode for chart
    if (settingsPtr->contains("chart_dark_mode_curve_color")){
        v = settingsPtr->value("chart_dark_mode_curve_color");
        QString s = v.toString();
        curveDarkModeColor = QColor(s);
        if (curveDarkModeColor.isValid()==false) {
            curveDarkModeColor = QColor(180, 0, 0);
        }
    } else{
        curveDarkModeColor = QColor(180, 0, 0);
    }

    // curve color for light mode for chart
    if (settingsPtr->contains("chart_light_mode_curve_color")){
        v = settingsPtr->value("chart_light_mode_curve_color");
        QString s = v.toString();
        curveLightModeColor = QColor(s);
        if (curveLightModeColor.isValid()==false) {
            curveLightModeColor = QColor(0, 0, 180);
        }
    } else{
        curveLightModeColor = QColor(0, 0, 180);
    }

    // Chart Export Image Quality
    if (settingsPtr->contains("chart_export_image_quality")){
        bool ok;
        v = settingsPtr->value("chart_export_image_quality");
        chartExportImageQuality = v.toInt(&ok);
        if ( (!ok) || (chartExportImageQuality<1) || (chartExportImageQuality>100) )  {
            // settings is invalid
            chartExportImageQuality = 75;
        }
    } else{
        chartExportImageQuality = 75;
    }

    // Chart Export Image Type
    if (settingsPtr->contains("chart_export_image_type")){
        bool ok;
        v = settingsPtr->value("chart_export_image_type");
        chartExportImageType = v.toString();
        if ( (chartExportImageType!="PNG") && (chartExportImageType!="JPG") )  {
            // settings is invalid
            chartExportImageType = "PNG";
        }
    } else{
        chartExportImageType = "PNG";
    }

    // amount in exported text are localized or not (in which case format is : no thousand separator, decimal sep = "."
    if (settingsPtr->contains("export_text_amount_localized")){
        v = settingsPtr->value("export_text_amount_localized");
        bool ok = Util::isValidBoolString(v.toString());
        if(ok){
            exportTextAmountLocalized = v.toBool();
        } else {
            exportTextAmountLocalized = false;
        }

    } else{
        exportTextAmountLocalized = false;;
    }

    // last directory
    if (settingsPtr->contains("last_dir")){
        v = settingsPtr->value("last_dir");
        lastDir = v.toString();
        // check that the directory is valid and still exists
        QFileInfo fi(lastDir);
        if( (fi.exists()==false) || (fi.isDir()==false) ){
            lastDir = QDir::homePath(); // overwrite with a known value
        }
    } else{
        lastDir = QDir::homePath();
    }

    // Main chart scaling. Values are in percentage (uint, [0-5]. 0 = 100%, 5 = 105%)
    // This is to prevent situation where border points fall exactly on axis
    // in which case they are difficult to see and click on
    // 0 means no additional space (points may fall on axis), [1..5] means factor of [1.02-1.05]
    if (settingsPtr->contains("main_chart_scaling_percentage")){
        bool ok;
        v = settingsPtr->value("main_chart_scaling_percentage");
        int anInt = v.toInt(&ok);
        if ( (!ok) || (percentageMainChartScaling<0) || (percentageMainChartScaling>5) )  {
            // settings is invalid
            percentageMainChartScaling = 2;
        } else {
            percentageMainChartScaling = anInt;
        }
    } else{
        percentageMainChartScaling = 2; // default is 102%
    }

    // use default system font for the application font
    if (settingsPtr->contains("use_default_system_font")){
        v = settingsPtr->value("use_default_system_font");
        bool ok = Util::isValidBoolString(v.toString());
        if (ok){
            useDefaultSystemFont = v.toBool();
        } else {
            useDefaultSystemFont = true;    // default if data is invalid
        }
    } else{
            useDefaultSystemFont = true;
    }

    // Custom Application Font
    if (settingsPtr->contains("custom_application_font")){
        v = settingsPtr->value("custom_application_font");
        QString s = v.toString().trimmed();
        if( s.length() != 0){
            QFont f ;
            bool ok = f.fromString(s);  // test if it is a valid font string description
            if ( ok )  {
                customApplicationFont = s;  // valid font string description
            } else {
                customApplicationFont = ""; // invalid font string description
            }
        } else {
            customApplicationFont = "";
        }

    } else{
        customApplicationFont = "";
    }

    // Date of Today
    todayUseSystemDate = true;
    todayCustomDate = QDate();
    if (settingsPtr->contains("today_use_system_date")){
        v = settingsPtr->value("today_use_system_date");
        bool ok = Util::isValidBoolString(v.toString());
        if(ok==true){
            if (v.toBool()==false) {
                // grab the specific date
                if (settingsPtr->contains("today_specific_date")){
                    v = settingsPtr->value("today_specific_date");
                    QString s = v.toString().trimmed();
                    QDate date = QDate::fromString(s, Qt::DateFormat::ISODate) ;
                    if ( (date.isNull()==false) && (date.isValid()==true) ) {
                        todayUseSystemDate = false;
                        todayCustomDate = date;
                        // update "today"
                        today = todayCustomDate;
                        tomorrow = today.addDays(1);
                    }
                }
            }
        }
    }

    // loaded is completed and successful
    settingsLoaded = true;

    // Some loggings
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info, QString("Settings loaded successfully"));
    GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Info, QString("    recent_files = %1").arg(recentFilenames.join(",")));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info, QString("    scenario_max_years = %1").arg(scenarioMaxYearsSpan));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info, QString("    chart_dark_mode = %1").arg(chartDarkMode));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info, QString("    chart_dark_mode_curve_color = %1").arg(curveDarkModeColor.name(QColor::HexRgb)));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info, QString("    chart_light_mode_curve_color = %1").arg(curveLightModeColor.name(QColor::HexRgb)));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info, QString("    chart_export_image_type = %1").arg(chartExportImageType));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info, QString("    chart_export_image_quality = %1").arg(chartExportImageQuality));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info, QString("    export_text_amount_localized = %1").arg(exportTextAmountLocalized));
    GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Info, QString("    last_dir = %1").arg(lastDir));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info, QString("    main_chart_scaling_percentage = %1").arg(percentageMainChartScaling));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info, QString("    use_default_system_font = %1").arg(useDefaultSystemFont));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info, QString("    custom_application_font = %1").arg(customApplicationFont));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info, QString("    today_use_system_date = %1").arg(todayUseSystemDate));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info, QString("    today_custom_date = %1").arg(todayCustomDate.toString(Qt::DateFormat::ISODate)));

}


void GbpController::saveSettings()
{
    settingsPtr->setValue("recent_files",recentFilenames);
    settingsPtr->setValue("scenario_max_years",scenarioMaxYearsSpan);
    settingsPtr->setValue("chart_dark_mode",chartDarkMode);
    settingsPtr->setValue("chart_dark_mode_curve_color",curveDarkModeColor.name(QColor::HexRgb));
    settingsPtr->setValue("chart_light_mode_curve_color",curveLightModeColor.name(QColor::HexRgb));
    settingsPtr->setValue("chart_export_image_type",chartExportImageType);
    settingsPtr->setValue("chart_export_image_quality",chartExportImageQuality);
    settingsPtr->setValue("export_text_amount_localized",exportTextAmountLocalized);
    settingsPtr->setValue("last_dir",lastDir);
    settingsPtr->setValue("main_chart_scaling_percentage",percentageMainChartScaling);
    settingsPtr->setValue("use_default_system_font",useDefaultSystemFont);
    settingsPtr->setValue("custom_application_font",customApplicationFont);
    settingsPtr->setValue("today_use_system_date",todayUseSystemDate);
    settingsPtr->setValue("today_specific_date",todayCustomDate.toString(Qt::DateFormat::ISODate));
}


// insert on top of the recentFileNmae list a new file name.
// If the file already exist in the list, it is moved on the top.
// Keep a max of 20
void GbpController::recentFilenamesAdd(QString newFilename, int maxNoOfEntries)
{
    // remove duplicate if any
    recentFilenames.removeAll(newFilename);
    // add to the top of the list
    recentFilenames.prepend(newFilename);
    // trim the list
    if (recentFilenames.size()>maxNoOfEntries){
        recentFilenames.removeLast();
    }
}


void GbpController::recentFilenamesClear()
{
    recentFilenames.clear();
}


void GbpController::log(LogLevel level, LogType type, QString message)
{
    if (loggingEnabled==false){
        return;
    }
    if (level > logLevel) {
        return;
    }
    QString filteredMessage = message.replace('\n',' ');
    QString outString = QString("[%1] - %2 - %3\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")).arg((type==Info)?("INFO"):((type==Warning)?("WARNING"):("ERROR"))).arg(message);
    logOutStream << outString;
    logOutStream.flush();
}


void GbpController::cleanUpLogs()
{
    // clean up old logs to prevent accumulation. Remove logs older than 100 days
    qint64 nowInJulianDay = today.toJulianDay();
    QDirIterator it(logFolder,{"*.txt"}, QDir::Files);
    QStringList toBeDeleted;
    while (it.hasNext()) {
        QFile f(it.next());
        QFileInfo fileInfo(f.fileName()); // strip path
        if ( !fileInfo.isFile() ){
            continue; // not to sure about QDir::Files ...
        }
        QString baseName = fileInfo.fileName();
        if (baseName.length()<8){
            continue;   // not a gbp log file
        }
        QString dateString = baseName.mid(0,10);
        QDate d = QDate::fromString(dateString, Qt::ISODate);
        if ( !d.isValid()){
            continue;
        }
        if ( 100 < (nowInJulianDay-d.toJulianDay()) ){
            toBeDeleted.append(f.fileName());
        }
    }
    foreach(QString fName, toBeDeleted){
        bool deletionSuccess = QFile::remove(fName); // return False if failed;
        GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info, QString("Deleting old log file %1 : success=%2").arg(fName).arg(deletionSuccess));
    }
}


QDate GbpController::getToday() const
{
    return today;
}


QDate GbpController::getTomorrow() const
{
    return tomorrow;
}


// Getters and Setters

QSharedPointer<Scenario> GbpController::getScenario() const
{
    return scenario;
}

void GbpController::setScenario(QSharedPointer<Scenario> newScenario)
{
    scenario = newScenario;
}

QString GbpController::getFullFileName() const
{
    return fullFileName;
}

void GbpController::setFullFileName(const QString &newFullFileName)
{
    fullFileName = newFullFileName;
}

QString GbpController::getLastDir() const
{
    return lastDir;
}

void GbpController::setLastDir(const QString &newLastDir)
{
    lastDir = newLastDir;
}

QStringList GbpController::getRecentFilenames() const
{
    return recentFilenames;
}

void GbpController::setRecentFilenames(const QStringList &newRecentFilenames)
{
    recentFilenames = newRecentFilenames;
}

qint16 GbpController::getScenarioMaxYearsSpan() const
{
    return scenarioMaxYearsSpan;
}

void GbpController::setScenarioMaxYearsSpan(qint16 newScenarioMaxYearsSpan)
{
    scenarioMaxYearsSpan = newScenarioMaxYearsSpan;
}

bool GbpController::getChartDarkMode() const
{
    return chartDarkMode;
}

void GbpController::setChartDarkMode(bool newChartDarkMode)
{
    chartDarkMode = newChartDarkMode;
}

QColor GbpController::getCurveDarkModeColor() const
{
    return curveDarkModeColor;
}

void GbpController::setCurveDarkModeColor(const QColor &newCurveDarkModeColor)
{
    curveDarkModeColor = newCurveDarkModeColor;
}

QColor GbpController::getCurveLightModeColor() const
{
    return curveLightModeColor;
}

void GbpController::setCurveLightModeColor(const QColor &newCurveLightModeColor)
{
    curveLightModeColor = newCurveLightModeColor;
}

int GbpController::getChartExportImageQuality() const
{
    return chartExportImageQuality;
}

void GbpController::setChartExportImageQuality(int newChartExportImageQuality)
{
    chartExportImageQuality = newChartExportImageQuality;
}

QString GbpController::getChartExportImageType() const
{
    return chartExportImageType;
}

void GbpController::setChartExportImageType(const QString &newChartExportImageType)
{
    chartExportImageType = newChartExportImageType;
}

bool GbpController::getExportTextAmountLocalized() const
{
    return exportTextAmountLocalized;
}

void GbpController::setExportTextAmountLocalized(bool newExportTextAmountLocalized)
{
    exportTextAmountLocalized = newExportTextAmountLocalized;
}

uint GbpController::getPercentageMainChartScaling() const
{
    return percentageMainChartScaling;
}

void GbpController::setPercentageMainChartScaling(uint newPercentageMainChartScaling)
{
    percentageMainChartScaling = newPercentageMainChartScaling;
}

bool GbpController::getUseDefaultSystemFont() const
{
    return useDefaultSystemFont;
}

void GbpController::setUseDefaultSystemFont(bool newUseDefaultSystemFont)
{
    useDefaultSystemFont = newUseDefaultSystemFont;
}

QString GbpController::getCustomApplicationFont() const
{
    return customApplicationFont;
}

void GbpController::setCustomApplicationFont(const QString &newCustomApplicationFont)
{
    customApplicationFont = newCustomApplicationFont;
}

QString GbpController::getInitialSystemApplicationFont() const
{
    return initialSystemApplicationFont;
}

bool GbpController::getTodayUseSystemDate() const
{
    return todayUseSystemDate;
}

void GbpController::setTodayUseSystemDate(bool newTodayUseSystemDate)
{
    todayUseSystemDate = newTodayUseSystemDate;
}

QDate GbpController::getTodayCustomDate() const
{
    return todayCustomDate;
}

void GbpController::setTodayCustomDate(const QDate &newTodayCustomDate)
{
    todayCustomDate = newTodayCustomDate;
}




