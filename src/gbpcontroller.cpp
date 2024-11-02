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
    recentFilenames.clear();
    loggingEnabled = true;
    logLevel = LogLevel::Minimal;
    useDefaultSystemFont = true;
    customApplicationFont = "";
    initialSystemApplicationFont = QApplication::font().toString();
    todayUseSystemDate = true;
    todayCustomDate = QDate();
    allowDecorationColor = true;
    usePresentValue = false;
    pvDiscountRate = 0; // disable conversion to present values
    noSettingsFileAtStartup = false;
    isDarkModeSet = false;
    exportTextAmountLocalized = false;
    exportTextDateLocalized = false;
    chartPointSize = 10;
    wheelRotatedAwayZoomIn = false;

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

    // Init the value of "today". Set ONCE the date corresponding to "today" and "tomorrow".
    // There are 3 reasons why we dont want the app to call everywhere "QDate::currentDate()"
    // 1) the app could have been started near midnight and the transition to another day while
    //    running could mess up things (for log for example)
    // 2) we want to offer the option of setting the "today" date as configuration parameter
    //    (in the "options")
    // 3) we want to make sure today is in LOCAL time
    today = QDateTime::currentDateTime().toLocalTime().date();
    tomorrow = today.addDays(1);

    // *** LOG FILES ***

    // determine where to store the logs. try in QStandardPaths::AppDataLocation/logs
    // (linux : ~/.local/share/graphical-budget-planner/logs), else use QDir::tempPath()/gbp
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
    logFullFileName = QString("%1/%2__%3.txt").arg(logFolder).arg(today.toString("yyyy-MM-dd")).arg(
        QTime::currentTime().toString("hh_mm_ss"));
    logFile.setFileName(logFullFileName);
    success = logFile.open(QIODevice::Append | QIODevice::WriteOnly | QIODevice::Text);
    if (success == false){
        loggingEnabled = false;
        QString errorString = QString("Logging is disabled (cannot create a log file in %1)").arg(
            logFolder);
        qWarning().noquote() << errorString;
    } else {
        loggingEnabled = true;
        QString successString = QString("Log file created : %1, log level=%2").arg(
            logFullFileName).arg(logLevel);
        qInfo().noquote() << successString;
    }
    logOutStream.setDevice(&logFile);
    logOutStream.setEncoding(QStringConverter::Utf8);

    // *** SETTINGS ***

    // define default path for settings before creating GbpController (we dont want OrganizationName
    // be part of the path). One choose INI file structure (favor decentralization, portability and
    // human readability)
    settingsFullFileName = QString("%1/%2.ini").arg(QStandardPaths::writableLocation(
        QStandardPaths::ConfigLocation)).arg(QCoreApplication::applicationName());
    QFile iniFile(settingsFullFileName);
    if (iniFile.exists()==false){
        noSettingsFileAtStartup = true;
    }
    settingsPtr = new QSettings(settingsFullFileName, QSettings::IniFormat);


    QString configFileString = QString("Configuration file is : %1").arg(settingsFullFileName);
    qInfo().noquote() << configFileString;

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


// This should be done only ONCE, as EARLY AS possible after the applicationis started
void GbpController::loadSettings()
{
    // Ensure loading has not been already done
    if (settingsLoaded==true) {
        return;
    }

    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        QString("Attempting to load settings from %1").arg(settingsPtr->fileName()));

    QVariant v;
    // recent open files list
    if (settingsPtr->contains("recent_files")){
        v = settingsPtr->value("recent_files");
        recentFilenames = v.toStringList(); // empty list if conversion failed
    } else{
        recentFilenames = {};
    }

    // Chart Point Size in pixels
    if (settingsPtr->contains("chart_point_size")){
        bool ok;
        v = settingsPtr->value("chart_point_size");
        int anInt = v.toInt(&ok);
        if ( (!ok) || (anInt<5) || (anInt>25) )  {
            // settings is invalid
            chartPointSize = 10;
        } else {
            chartPointSize = anInt;
        }
    } else{
        chartPointSize = 10; // default is 10 pixels
    }

    // dark mode for all charts : enabled or not
    if (settingsPtr->contains("chart_dark_mode")){
        v = settingsPtr->value("chart_dark_mode");
        bool ok = Util::isValidBoolString(v.toString());
        if(ok){
            isDarkModeSet = v.toBool();
        } else {
            isDarkModeSet = false;
        }
    } else{
        isDarkModeSet = false;;
    }

    // curve color for dark mode for chart
    if (settingsPtr->contains("chart_dark_mode_curve_color")){
        v = settingsPtr->value("chart_dark_mode_curve_color");
        QString s = v.toString();
        darkModeCurveColor = QColor(s);
        if (darkModeCurveColor.isValid()==false) {
            darkModeCurveColor = QColor(192, 0, 0);
        }
    } else{
        darkModeCurveColor = QColor(192, 0, 0);
    }

    // curve color for light mode for chart
    if (settingsPtr->contains("chart_light_mode_curve_color")){
        v = settingsPtr->value("chart_light_mode_curve_color");
        QString s = v.toString();
        lightModeCurveColor = QColor(s);
        if (lightModeCurveColor.isValid()==false) {
            lightModeCurveColor = QColor(0, 0, 192);
        }
    } else{
        lightModeCurveColor = QColor(0, 0, 192);
    }

    // point color for dark mode for chart
    if (settingsPtr->contains("chart_dark_mode_point_color")){
        v = settingsPtr->value("chart_dark_mode_point_color");
        QString s = v.toString();
        darkModePointColor= QColor(s);
        if (darkModePointColor.isValid()==false) {
            darkModePointColor = QColor(255, 0, 0);
        }
    } else{
        darkModePointColor = QColor(255,0, 0);
    }

    // point color for light mode for chart
    if (settingsPtr->contains("chart_light_mode_point_color")){
        v = settingsPtr->value("chart_light_mode_point_color");
        QString s = v.toString();
        lightModePointColor = QColor(s);
        if (lightModePointColor.isValid()==false) {
            lightModePointColor = QColor(0, 0, 255);
        }
    } else{
        lightModePointColor = QColor(0, 0, 255);
    }

    // selected point color for dark mode for chart
    if (settingsPtr->contains("chart_dark_mode_selected_point_color")){
        v = settingsPtr->value("chart_dark_mode_selected_point_color");
        QString s = v.toString();
        darkModeSelectedPointColor = QColor(s);
        if (darkModeSelectedPointColor.isValid()==false) {
            darkModeSelectedPointColor = QColor(0, 255, 0);
        }
    } else{
        darkModeSelectedPointColor = QColor(0,255, 0);
    }

    // selected point color for light mode for chart
    if (settingsPtr->contains("chart_light_mode_selected_point_color")){
        v = settingsPtr->value("chart_light_mode_selected_point_color");
        QString s = v.toString();
        lightModeSelectedPointColor = QColor(s);
        if (lightModeSelectedPointColor.isValid()==false) {
            lightModeSelectedPointColor = QColor(0, 192, 0);
        }
    } else{
        lightModeSelectedPointColor = QColor(0, 192, 0);
    }

    // Amounts in exported text are localized or not (in which case format is : no thousand
    // separator, decimal sep = "."
    if (settingsPtr->contains("export_text_amount_localized")){
        v = settingsPtr->value("export_text_amount_localized");
        bool ok = Util::isValidBoolString(v.toString());
        if(ok){
            exportTextAmountLocalized = v.toBool();
        } else {
            exportTextAmountLocalized = false;
        }

    } else{
        exportTextAmountLocalized = false;
    }

    // Dates in exported text are localized or not (in which case format is : YYYY-MM-DD
    if (settingsPtr->contains("export_text_date_localized")){
        v = settingsPtr->value("export_text_date_localized");
        bool ok = Util::isValidBoolString(v.toString());
        if(ok){
            exportTextDateLocalized = v.toBool();
        } else {
            exportTextDateLocalized = false;
        }

    } else{
        exportTextDateLocalized = false;
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
        if ( (!ok) || (anInt<0) || (anInt>10) )  {
            // settings is invalid
            percentageMainChartScaling = 5;
        } else {
            percentageMainChartScaling = anInt;
        }
    } else{
        percentageMainChartScaling = 5; // default is 105%
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

    // allow the use of decoration color for incomes/expenses
    if (settingsPtr->contains("allow_decoration_color")){
        v = settingsPtr->value("allow_decoration_color");
        bool ok = Util::isValidBoolString(v.toString());
        if (ok){
            allowDecorationColor = v.toBool();
        } else {
            allowDecorationColor = true;    // default if data is invalid
        }
    } else{
        allowDecorationColor = true;
    }

    // use of Present Value conversion for all FE amounts
    if (settingsPtr->contains("use_present_value")){
        v = settingsPtr->value("use_present_value");
        bool ok = Util::isValidBoolString(v.toString());
        if (ok){
            usePresentValue = v.toBool();

            // this is the annual discount rate to be used when "use_present_value" is true.
            if (settingsPtr->contains("pv_discount_rate")){
                v = settingsPtr->value("pv_discount_rate");
                bool ok;
                double aDouble = v.toDouble(&ok);
                if ( (!ok) || (aDouble<0) || (aDouble>100) )  {
                    // settings is invalid, revert back to default
                    pvDiscountRate = 0;
                } else {
                    pvDiscountRate = aDouble;
                }
            } else{
                pvDiscountRate = 0; // revert to default
            }

        } else {
            usePresentValue = false; // revert to default if data is invalid
            pvDiscountRate = 0;
        }
    } else{
        // cant find the Use Present Value flag (could be older version of GBP)
        usePresentValue = false;

        pvDiscountRate = 0;
    }

    // mouse wheel moving away from user : effect on zoom
    // allow the use of decoration color for incomes/expenses
    if (settingsPtr->contains("wheel_rotated_away_zoom_in")){
        v = settingsPtr->value("wheel_rotated_away_zoom_in");
        bool ok = Util::isValidBoolString(v.toString());
        if (ok){
            wheelRotatedAwayZoomIn = v.toBool();
        } else {
            wheelRotatedAwayZoomIn = false;    // default if data is invalid
        }
    } else{
        wheelRotatedAwayZoomIn = false;
    }

     // loaded is completed and successful
    settingsLoaded = true;

    // Some loggings
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info,
        QString("Settings loaded successfully"));
    GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Info,
        QString("    recent_files = %1").arg(recentFilenames.join(",")));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info,
       QString("    chart_point_size = %1").arg(chartPointSize));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info,
        QString("    chart_dark_mode = %1").arg(isDarkModeSet));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info,
        QString("    chart_dark_mode_curve_color = %1").arg(darkModeCurveColor.name(
        QColor::HexRgb)));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info,
        QString("    chart_light_mode_curve_color = %1").arg(lightModeCurveColor.name(
        QColor::HexRgb)));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info,
        QString("    point_color_dark_mode = %1").arg(darkModePointColor.name(QColor::HexRgb)));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info,
        QString("    point_color_light_mode = %1").arg(lightModePointColor.name(QColor::HexRgb)));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info,
        QString("    selected_point_dark_mode = %1").arg(darkModeSelectedPointColor.name(
        QColor::HexRgb)));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info,
        QString("    selected_point_light_mode = %1").arg(lightModeSelectedPointColor.name(
        QColor::HexRgb)));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info,
        QString("    export_text_amount_localized = %1").arg(exportTextAmountLocalized));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info,
        QString("    export_text_date_localized = %1").arg(exportTextDateLocalized));
    GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Info,
        QString("    last_dir = %1").arg(lastDir));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info,
        QString("    main_chart_scaling_percentage = %1").arg(percentageMainChartScaling));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info,
        QString("    use_default_system_font = %1").arg(useDefaultSystemFont));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info,
        QString("    custom_application_font = %1").arg(customApplicationFont));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info,
        QString("    today_use_system_date = %1").arg(todayUseSystemDate));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info,
        QString("    today_custom_date = %1").arg(todayCustomDate.toString(
        Qt::DateFormat::ISODate)));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info,
        QString("    allow_decoration_color = %1").arg(allowDecorationColor));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info,
        QString("    use_present_value = %1").arg(usePresentValue));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info,
        QString("    pv_discount_rate = %1").arg(pvDiscountRate));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal,GbpController::Info,
        QString("    wheel_rotated_away_zoom_in = %1").arg(wheelRotatedAwayZoomIn));

}


void GbpController::saveSettings()
{
    settingsPtr->setValue("recent_files",recentFilenames);
    settingsPtr->setValue("chart_point_size",chartPointSize);
    settingsPtr->setValue("chart_dark_mode",isDarkModeSet);
    settingsPtr->setValue("chart_dark_mode_curve_color",darkModeCurveColor.name(QColor::HexRgb));
    settingsPtr->setValue("chart_light_mode_curve_color",lightModeCurveColor.name(QColor::HexRgb));
    settingsPtr->setValue("chart_light_mode_point_color",
        lightModePointColor.name(QColor::HexRgb));
    settingsPtr->setValue("chart_dark_mode_point_color",
        darkModePointColor.name(QColor::HexRgb));
    settingsPtr->setValue("chart_light_mode_selected_point_color",
        lightModeSelectedPointColor.name(QColor::HexRgb));
    settingsPtr->setValue("chart_dark_mode_selected_point_color",
        darkModeSelectedPointColor.name(QColor::HexRgb));
    settingsPtr->setValue("export_text_amount_localized",exportTextAmountLocalized);
    settingsPtr->setValue("export_text_date_localized",exportTextDateLocalized);
    settingsPtr->setValue("last_dir",lastDir);
    settingsPtr->setValue("main_chart_scaling_percentage",percentageMainChartScaling);
    settingsPtr->setValue("use_default_system_font",useDefaultSystemFont);
    settingsPtr->setValue("custom_application_font",customApplicationFont);
    settingsPtr->setValue("today_use_system_date",todayUseSystemDate);
    settingsPtr->setValue("today_specific_date",todayCustomDate.toString(Qt::DateFormat::ISODate));
    settingsPtr->setValue("allow_decoration_color",allowDecorationColor);
    settingsPtr->setValue("use_present_value",usePresentValue);
    settingsPtr->setValue("pv_discount_rate",pvDiscountRate);
    settingsPtr->setValue("wheel_rotated_away_zoom_in",wheelRotatedAwayZoomIn);
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
    QString outString = QString("[%1] - %2 - %3\n").arg(
        QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")).arg(
        (type==Info)?("INFO"):((type==Warning)?("WARNING"):("ERROR"))).arg(message);
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
        GbpController::getInstance().log(
            GbpController::LogLevel::Minimal,GbpController::Info, QString(
            "Deleting old log file %1 : success=%2").arg(fName).arg(deletionSuccess));
    }
}


// *** Getters and Setters ***

QStringList GbpController::getRecentFilenames() const
{
    return recentFilenames;
}

void GbpController::setRecentFilenames(const QStringList &newRecentFilenames)
{
    recentFilenames = newRecentFilenames;
}

bool GbpController::getIsDarkModeSet() const
{
    return isDarkModeSet;
}

void GbpController::setIsDarkModeSet(bool newIsDarkModeSet)
{
    isDarkModeSet = newIsDarkModeSet;
}

QColor GbpController::getDarkModeCurveColor() const
{
    return darkModeCurveColor;
}

void GbpController::setDarkModeCurveColor(const QColor &newDarkModeCurveColor)
{
    darkModeCurveColor = newDarkModeCurveColor;
}

QColor GbpController::getLightModeCurveColor() const
{
    return lightModeCurveColor;
}

void GbpController::setLightModeCurveColor(const QColor &newLightModeCurveColor)
{
    lightModeCurveColor = newLightModeCurveColor;
}

QColor GbpController::getDarkModePointColor() const
{
    return darkModePointColor;
}

void GbpController::setDarkModePointColor(const QColor &newDarkModePointColor)
{
    darkModePointColor = newDarkModePointColor;
}

QColor GbpController::getLightModePointColor() const
{
    return lightModePointColor;
}

void GbpController::setLightModePointColor(const QColor &newLightModePointColor)
{
    lightModePointColor = newLightModePointColor;
}

QColor GbpController::getDarkModeSelectedPointColor() const
{
    return darkModeSelectedPointColor;
}

void GbpController::setDarkModeSelectedPointColor(const QColor &newDarkModeSelectedPointColor)
{
    darkModeSelectedPointColor = newDarkModeSelectedPointColor;
}

QColor GbpController::getLightModeSelectedPointColor() const
{
    return lightModeSelectedPointColor;
}

void GbpController::setLightModeSelectedPointColor(const QColor &newLightModeSelectedPointColor)
{
    lightModeSelectedPointColor = newLightModeSelectedPointColor;
}

bool GbpController::getExportTextAmountLocalized() const
{
    return exportTextAmountLocalized;
}

void GbpController::setExportTextAmountLocalized(bool newExportTextAmountLocalized)
{
    exportTextAmountLocalized = newExportTextAmountLocalized;
}

QString GbpController::getLastDir() const
{
    return lastDir;
}

void GbpController::setLastDir(const QString &newLastDir)
{
    lastDir = newLastDir;
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

bool GbpController::getAllowDecorationColor() const
{
    return allowDecorationColor;
}

void GbpController::setAllowDecorationColor(bool newAllowDecorationColor)
{
    allowDecorationColor = newAllowDecorationColor;
}

bool GbpController::getUsePresentValue() const
{
    return usePresentValue;
}

void GbpController::setUsePresentValue(bool newUsePresentValue)
{
    usePresentValue = newUsePresentValue;
}

double GbpController::getPvDiscountRate() const
{
    return pvDiscountRate;
}

void GbpController::setPvDiscountRate(double newPvDiscountRate)
{
    pvDiscountRate = newPvDiscountRate;
}

QString GbpController::getFullFileName() const
{
    return fullFileName;
}

void GbpController::setFullFileName(const QString &newFullFileName)
{
    fullFileName = newFullFileName;
}

QSharedPointer<Scenario> GbpController::getScenario() const
{
    return scenario;
}

void GbpController::setScenario(QSharedPointer<Scenario> newScenario)
{
    scenario = newScenario;
}




// *** Getters ***

bool GbpController::getNoSettingsFileAtStartup() const
{
    return noSettingsFileAtStartup;
}

QDate GbpController::getToday() const
{
    return today;
}

QDate GbpController::getTomorrow() const
{
    return tomorrow;
}

QString GbpController::getLogFullFileName() const
{
    return logFullFileName;
}

QString GbpController::getInitialSystemApplicationFont() const
{
    return initialSystemApplicationFont;
}

QString GbpController::getSettingsFullFileName() const
{
    return settingsFullFileName;
}

GbpController::LogLevel GbpController::getLogLevel() const
{
    return logLevel;
}

bool GbpController::getExportTextDateLocalized() const
{
    return exportTextDateLocalized;
}

void GbpController::setExportTextDateLocalized(bool newExportTextDateLocalized)
{
    exportTextDateLocalized = newExportTextDateLocalized;
}

bool GbpController::getWheelRotatedAwayZoomIn() const
{
    return wheelRotatedAwayZoomIn;
}

void GbpController::setWheelRotatedAwayZoomIn(bool newWheelRotatedAwayZoomIn)
{
    wheelRotatedAwayZoomIn = newWheelRotatedAwayZoomIn;
}

uint GbpController::getChartPointSize() const
{
    return chartPointSize;
}

void GbpController::setChartPointSize(uint newChartPointSize)
{
    chartPointSize = newChartPointSize;
}
