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

#include "gbpcontroller.h"
#include "gbplogger.h"
#include "qfont.h"
#include <QDir>
#include <QString>
#include <QStandardPaths>
#include <QApplication>



GbpController::GbpController()
    : factorySettings{}  // const ini
{

    // no current scenario yet.
    scenario=nullptr;

    // Set default value in case no ini file exist, outside factory settings
    lastDir = QDir::homePath();
    lastDirImport = QDir::homePath();;
    lastDirExport = QDir::homePath();
    fullFileName = "";
    recentFilenames.clear();
    initialSystemApplicationFont = QApplication::font().toString();
    noSettingsFileAtStartup = false;
    systemDesktopDarkTheme = false;

    // Set default value for settings from factory settings
    chartPointSize = factorySettings.chartPointSize;
    chartTheming = factorySettings.chartTheming;
    darkModeCurveColor = factorySettings.darkModeCurveColor;
    lightModeCurveColor = factorySettings.lightModeCurveColor;
    darkModePointColor = factorySettings.darkModePointColor;
    lightModePointColor = factorySettings.lightModePointColor;
    darkModeSelectedPointColor = factorySettings.darkModeSelectedPointColor;
    lightModeSelectedPointColor = factorySettings.lightModeSelectedPointColor;
    exportTextNumberLocalized = factorySettings.exportTextNumberLocalized;
    exportTextDateLocalized = factorySettings.exportTextDateLocalized;
    percentageMainChartScaling = factorySettings.percentageMainChartScaling;
    useDefaultSystemFont = factorySettings.useDefaultSystemFont;
    customApplicationFont = factorySettings.customApplicationFont;
    todayUseSystemDate = factorySettings.todayUseSystemDate;
    todayCustomDate = factorySettings.todayCustomDate;
    allowDecorationColor = factorySettings.allowDecorationColor;
    usePresentValue = factorySettings.usePresentValue;
    pvDiscountRate = factorySettings.pvDiscountRate;
    wheelRotatedAwayZoomIn = factorySettings.wheelRotatedAwayZoomIn;
    showYzeroLine = factorySettings.showYzeroLine;
    yZeroLineLightModeColor = factorySettings.yZeroLineLightModeColor;
    yZeroLineDarkModeColor = factorySettings.yZeroLineDarkModeColor;
    gridlinesDarkModeColor = factorySettings.gridlinesDarkModeColor;
    gridlinesLightModeColor = factorySettings.gridlinesLightModeColor;
    xAxisDateFormat = factorySettings.xAxisDateFormat;
    showTooltips = factorySettings.showTooltips;

    // Init the value of "today". THIS WILL BE OVERRIDDEN BY LOAD() if "CustomDate" if required.
    // Set the date corresponding to "today" and "tomorrow".
    // There are 3 reasons why we dont want the app to call everywhere "QDate::currentDate()"
    // 1) the app could have been started near midnight and the transition to another day while
    //    running could mess up things (for log for example)
    // 2) we want to offer the option of setting the "today" date as configuration parameter
    //    (in the "options")
    // 3) we want to make sure today is in LOCAL time
    today = QDateTime::currentDateTime().toLocalTime().date(); // same as factory settings
    tomorrow = today.addDays(1);

    // *** SETTINGS ***

    // Parse workspace argument from command line
    QString workspace;
    QStringList argList = QCoreApplication::arguments();
    for (int i = 0; i < argList.size(); ++i) {
        if (argList.at(i).startsWith("-workspace=")) {
            QString temp = argList.at(i).mid(11);  // Remove "-workspace=" prefix
            // Validate: 1-20 characters, alphanumeric only
            if (temp.length() >= 1 && temp.length() <= 20) {
                bool valid = true;
                for (const QChar &ch : temp) {
                    if (!ch.isLetterOrNumber()) {
                        valid = false;
                        break;
                    }
                }
                if (valid) {
                    workspace = temp;
                }
            }
        }
    }
    this->workspace = workspace;  // save for later retrieval

    // define default path for settings before creating GbpController (we dont want OrganizationName
    // be part of the path). One choose INI file structure (favor decentralization, portability and
    // human readability)
    if (workspace.isEmpty()) {
        settingsFullFileName = QString("%1/%2.ini").arg(
            QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)).arg(
            QCoreApplication::applicationName());
    } else {
        settingsFullFileName = QString("%1/%2_%3.ini").arg(
            QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)).arg(
            QCoreApplication::applicationName()).arg(workspace);
    }
    QFile iniFile(settingsFullFileName);
    if (iniFile.exists()==false){
        noSettingsFileAtStartup = true;
    }

    // Create QSettings using std::unique_ptr with std::make_unique
    settingsPtr = std::make_unique<QSettings>(settingsFullFileName, QSettings::IniFormat);
    // Check if it failed
    if (settingsPtr != nullptr) {
        // From QT doc : Be aware that QSettings delays performing some operations. For this reason,
        // you might want to call sync() to ensure that the data stored in QSettings is written to
        // disk before calling status().
        settingsPtr->sync();
        // Check if any problem occurred
        QSettings::Status status = settingsPtr->status();
        if (status != QSettings::NoError) {
            // a fatal error occurred
            QString statusErrorString;
            switch (status) {
                case QSettings::AccessError:
                    statusErrorString = "Access error";
                    break;
                case QSettings::FormatError:
                    statusErrorString = "Format error";
                    break;
                default:
                    statusErrorString = "Unknown error";
                    break;
            }
            QString fInfo = QString("%1").arg(Q_FUNC_INFO);
            QString errorString = QString("%1: A problem occurred when trying to read or create the "
                "settings from file %2 : %3")
                .arg(fInfo).arg(settingsFullFileName).arg(statusErrorString);
            throw std::logic_error(errorString.toStdString());
        }
    } else {
        // Weird fatal error...
        QString fInfo = QString("%1").arg(Q_FUNC_INFO);
        QString errorString = QString("%1: Cannot create or read ini file (null pointer returned)"
            " from %1").arg(fInfo).arg(settingsFullFileName);
        throw std::logic_error(errorString.toStdString());
    }

    QString nativeSettingsFullFileName = QDir::toNativeSeparators(settingsFullFileName);
    QString configFileString;
    if (noSettingsFileAtStartup) {
        configFileString = QString("New configuration file will be created with default "
            "values at %1").arg(nativeSettingsFullFileName);
    } else {
        configFileString = QString("Configuration file : %1").arg(nativeSettingsFullFileName);
    }
    qInfo().noquote() << configFileString;
}



GbpController::~GbpController()
{
}


GbpController::FactorySettings GbpController::getFactorySettings() const
{
    return factorySettings;
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


void GbpController::loadSettings()
{
    // Ensure loading has not been already done
    if (settingsLoaded==true) {
        return;
    }

    LOG_INFO(QString("Attempting to load settings from %1")
        .arg(REDACT(settingsPtr->fileName())));

    QVariant v;

    // recent open files list. For 1.6.3 and before, the recent_list of open files is stored as a
    // string with "," as separator. For 1.7.0 and above, separator is now "|||", because it greatly
    // reduces the risk of collisions with full file name (not impossible, but very unlikely).
    if (settingsPtr->contains("recent_files_v2")){
        // new format 1.7.0 and above
        v = settingsPtr->value("recent_files_v2");
        QString s = v.toString().trimmed();
        QStringList sl = s.split("|||", Qt::SkipEmptyParts);
        recentFilenames = sl;
    } else if (settingsPtr->contains("recent_files")){
        // OLD 1.6.3 format, with "," separator.
        v = settingsPtr->value("recent_files");
        recentFilenames = v.toStringList(); // empty list if conversion failed
        // do not delete the old entry, it wont be used by 1.7.0, but could be by 1.6.3
    } else{
        // No recent list found
        recentFilenames = {};
    }
    LOG_INFO(QString("    recent_files = %1")
        .arg(REDACT(recentFilenames.join("|||"))));

    // Chart Point Size in pixels
    if (settingsPtr->contains("chart_point_size")){
        bool ok;
        v = settingsPtr->value("chart_point_size");
        int anInt = v.toInt(&ok);
        if ( (!ok) || (anInt<5) || (anInt>25) )  {
            // settings is invalid
            chartPointSize = factorySettings.chartPointSize;
        } else {
            chartPointSize = anInt;
        }
    } else{
        chartPointSize = factorySettings.chartPointSize;
    }
    LOG_INFO(QString("    chart_point_size = %1").arg(chartPointSize));

    // Determine Chart Theme for all QCharts. In 1.6.3-, this was a bool under "chart_dark_mode"
    // with "true" equivalent to FORCE_DARK, "false" equivalent to FORCE_LIGHT
    if (settingsPtr->contains("chart_theming")) {
        // for 1.7.0+ , we must start with this
        v = settingsPtr->value("chart_theming");
        bool ok;
        int anInt = v.toInt(&ok);
        if (ok==true) {
            if ( false==chartThemingFromIntToEnum(anInt, chartTheming) ) {
                chartTheming = factorySettings.chartTheming;
            }
        } else {
            chartTheming = factorySettings.chartTheming;
        }
    } else if (settingsPtr->contains("chart_dark_mode")){
        // for 1.6.3- : convert to 1.7.0 model
        v = settingsPtr->value("chart_dark_mode");
        bool ok = Util::isValidBoolString(v.toString());
        if(ok){
            if (v.toBool()==true) {
                chartTheming = ChartTheming::FORCE_DARK;
            } else {
                chartTheming = ChartTheming::FORCE_LIGHT;
            }

        } else {
            chartTheming = factorySettings.chartTheming;
        }
    } else {
        chartTheming = factorySettings.chartTheming;
    }
    settingsPtr->remove("chart_dark_mode"); // remove entry from 1.6.3-
    LOG_INFO(QString("    chart_theming = %1").arg(chartThemingToString(chartTheming)));

    // curve color for dark mode for chart
    if (settingsPtr->contains("chart_dark_mode_curve_color")){
        v = settingsPtr->value("chart_dark_mode_curve_color");
        QString s = v.toString();
        darkModeCurveColor = QColor(s);
        if (darkModeCurveColor.isValid()==false) {
            darkModeCurveColor = factorySettings.darkModeCurveColor;
        }
    } else{
        darkModeCurveColor = factorySettings.darkModeCurveColor;
    }
    LOG_INFO(QString("    chart_dark_mode_curve_color = %1")
        .arg(darkModeCurveColor.name(QColor::HexRgb)));

    // curve color for light mode for chart
    if (settingsPtr->contains("chart_light_mode_curve_color")){
        v = settingsPtr->value("chart_light_mode_curve_color");
        QString s = v.toString();
        lightModeCurveColor = QColor(s);
        if (lightModeCurveColor.isValid()==false) {
            lightModeCurveColor = factorySettings.lightModeCurveColor;
        }
    } else{
        lightModeCurveColor = factorySettings.lightModeCurveColor;
    }
    LOG_INFO(QString("    chart_light_mode_curve_color = %1")
        .arg(lightModeCurveColor.name(QColor::HexRgb)));

    // point color for dark mode for chart
    if (settingsPtr->contains("chart_dark_mode_point_color")){
        v = settingsPtr->value("chart_dark_mode_point_color");
        QString s = v.toString();
        darkModePointColor= QColor(s);
        if (darkModePointColor.isValid()==false) {
            darkModePointColor = factorySettings.darkModePointColor;
        }
    } else{
        darkModePointColor = factorySettings.darkModePointColor;
    }
    LOG_INFO(QString("    point_color_dark_mode = %1")
        .arg(darkModePointColor.name(QColor::HexRgb)));

    // point color for light mode for chart
    if (settingsPtr->contains("chart_light_mode_point_color")){
        v = settingsPtr->value("chart_light_mode_point_color");
        QString s = v.toString();
        lightModePointColor = QColor(s);
        if (lightModePointColor.isValid()==false) {
            lightModePointColor = factorySettings.lightModePointColor;
        }
    } else{
        lightModePointColor = factorySettings.lightModePointColor;
    }
    LOG_INFO(QString("    point_color_light_mode = %1")
        .arg(lightModePointColor.name(QColor::HexRgb)));

    // selected point color for dark mode for chart
    if (settingsPtr->contains("chart_dark_mode_selected_point_color")){
        v = settingsPtr->value("chart_dark_mode_selected_point_color");
        QString s = v.toString();
        darkModeSelectedPointColor = QColor(s);
        if (darkModeSelectedPointColor.isValid()==false) {
            darkModeSelectedPointColor = factorySettings.darkModeSelectedPointColor;
        }
    } else{
        darkModeSelectedPointColor = factorySettings.darkModeSelectedPointColor;
    }
    LOG_INFO(QString("    selected_point_dark_mode = %1")
        .arg(darkModeSelectedPointColor.name(QColor::HexRgb)));

    // selected point color for light mode for chart
    if (settingsPtr->contains("chart_light_mode_selected_point_color")){
        v = settingsPtr->value("chart_light_mode_selected_point_color");
        QString s = v.toString();
        lightModeSelectedPointColor = QColor(s);
        if (lightModeSelectedPointColor.isValid()==false) {
            lightModeSelectedPointColor = factorySettings.lightModeSelectedPointColor;
        }
    } else{
        lightModeSelectedPointColor = factorySettings.lightModeSelectedPointColor;
    }
    LOG_INFO(QString("    selected_point_light_mode = %1")
        .arg(lightModeSelectedPointColor.name(QColor::HexRgb)));

    // Numbers in exported text are localized or not (in which case format is : no thousand
    // separator, decimal sep = "."
    if (settingsPtr->contains("export_text_amount_localized")){
        v = settingsPtr->value("export_text_amount_localized");
        bool ok = Util::isValidBoolString(v.toString());
        if(ok){
            exportTextNumberLocalized = v.toBool();
        } else {
            exportTextNumberLocalized = factorySettings.exportTextNumberLocalized;
        }

    } else{
        exportTextNumberLocalized = factorySettings.exportTextNumberLocalized;
    }
    LOG_INFO(QString("    export_text_amount_localized = %1")
        .arg(exportTextNumberLocalized));

    // Dates in exported text are localized or not (in which case format is : YYYY-MM-DD
    if (settingsPtr->contains("export_text_date_localized")){
        v = settingsPtr->value("export_text_date_localized");
        bool ok = Util::isValidBoolString(v.toString());
        if(ok){
            exportTextDateLocalized = v.toBool();
        } else {
            exportTextDateLocalized = factorySettings.exportTextDateLocalized;
        }

    } else{
        exportTextDateLocalized = factorySettings.exportTextDateLocalized;
    }
    LOG_INFO(QString("    export_text_date_localized = %1")
        .arg(exportTextDateLocalized));

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
    LOG_INFO(QString("    last_dir = %1")
        .arg(REDACT(lastDir)));

    // last directory for file imports. New in 1.7.0.
    if (settingsPtr->contains("last_dir_import")){
        v = settingsPtr->value("last_dir_import");
        lastDirImport = v.toString();
        // check that the directory is valid and still exists
        QFileInfo fi(lastDirImport);
        if( (fi.exists()==false) || (fi.isDir()==false) ){
            lastDirImport = QDir::homePath(); // overwrite with a known value
        }
    } else{
        lastDirImport = QDir::homePath();
    }
    LOG_INFO(QString("    last_dir_import = %1")
        .arg(REDACT(lastDirImport)));

    // last directory for file exports. New in 1.7.0.
    if (settingsPtr->contains("last_dir_export")){
        v = settingsPtr->value("last_dir_export");
        lastDirExport = v.toString();
        // check that the directory is valid and still exists
        QFileInfo fi(lastDirExport);
        if( (fi.exists()==false) || (fi.isDir()==false) ){
            lastDirExport = QDir::homePath(); // overwrite with a known value
        }
    } else{
        lastDirExport = QDir::homePath();
    }
    LOG_INFO(QString("    last_dir_export = %1")
        .arg(REDACT(lastDirExport)));

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
            percentageMainChartScaling = factorySettings.percentageMainChartScaling;
        } else {
            percentageMainChartScaling = anInt;
        }
    } else{
        percentageMainChartScaling = factorySettings.percentageMainChartScaling;
    }
    LOG_INFO(QString("    main_chart_scaling_percentage = %1")
        .arg(percentageMainChartScaling));

    // use default system font for the application font AND Custom Application Font.
    // If there is a problem with custom font, we have to force useDefaultSystemFont=true
    if (settingsPtr->contains("use_default_system_font")){
        v = settingsPtr->value("use_default_system_font");
        bool ok = Util::isValidBoolString(v.toString());
        if (ok){
            useDefaultSystemFont = v.toBool();
        } else {
            useDefaultSystemFont = factorySettings.useDefaultSystemFont;
        }
    } else{
            useDefaultSystemFont = factorySettings.useDefaultSystemFont;
    }
    bool customFontOk = false;
    if (settingsPtr->contains("custom_application_font")){
        v = settingsPtr->value("custom_application_font");
        QString s = v.toString().trimmed();
        if( s.length() != 0){
            QFont f ;
            bool ok = f.fromString(s);  // test if it is a valid font string description
            if ( ok )  {
                customApplicationFont = s;  // valid font string description
                customFontOk = true;
            } else {
                customApplicationFont = factorySettings.customApplicationFont;
            }
        } else {
            customApplicationFont = factorySettings.customApplicationFont;
        }
    } else{
        customApplicationFont = factorySettings.customApplicationFont;
    }
    if ( (customFontOk == false) && (useDefaultSystemFont==false) ) {
        useDefaultSystemFont = true;
    }
    LOG_INFO(QString("    use_default_system_font = %1")
        .arg(useDefaultSystemFont));
    LOG_INFO(QString("    custom_application_font = %1")
        .arg(customApplicationFont));

    // Date of Today AND custom today date. If there is a problem converting the custom date,
    // we have to revert back to todayUseSystemDate=true
    if (settingsPtr->contains("today_use_system_date")){
        v = settingsPtr->value("today_use_system_date");
        bool ok = Util::isValidBoolString(v.toString());
        if(ok==true){
            todayUseSystemDate = v.toBool();
        } else {
            todayUseSystemDate = factorySettings.todayUseSystemDate;
        }
    } else {
        todayUseSystemDate = factorySettings.todayUseSystemDate;
    }
    bool customTodayDateOk = false;
    if (settingsPtr->contains("today_specific_date")){
        v = settingsPtr->value("today_specific_date");
        QString s = v.toString().trimmed();
        QDate date = QDate::fromString(s, Qt::DateFormat::ISODate) ;
        if ( (date.isNull()==false) && (date.isValid()==true) ) {
            todayCustomDate = date;
            customTodayDateOk = true;
        } else {
            todayCustomDate = factorySettings.todayCustomDate;
        }
    } else {
        todayCustomDate = factorySettings.todayCustomDate;
    }
    if ( (todayUseSystemDate==false) && (customTodayDateOk==false) ) {
        todayUseSystemDate = true;
    }
    LOG_INFO(QString("    today_use_system_date = %1")
        .arg(todayUseSystemDate));
    LOG_INFO(QString("    today_specific_date = %1")
        .arg(todayCustomDate.toString(Qt::DateFormat::ISODate)));

    // OVERRIDE "today" and "tomorrow" values (already set in the constructor for "use system date")
    // if custom date is chosen, from the values gotten from config file.
    if(todayUseSystemDate==false){
        today = todayCustomDate;
        tomorrow = today.addDays(1);
    }

    // allow the use of decoration color for incomes/expenses
    if (settingsPtr->contains("allow_decoration_color")){
        v = settingsPtr->value("allow_decoration_color");
        bool ok = Util::isValidBoolString(v.toString());
        if (ok){
            allowDecorationColor = v.toBool();
        } else {
            allowDecorationColor = factorySettings.allowDecorationColor;    // default if data is invalid
        }
    } else{
        allowDecorationColor = factorySettings.allowDecorationColor;
    }
    LOG_INFO(QString("    allow_decoration_color = %1")
        .arg(allowDecorationColor));

    // Use of Present Values AND pv discount rate.
    // If there is a problem converting the discount rate, we have to revert back to
    // usePresentValue=false
    if (settingsPtr->contains("use_present_value")){
        v = settingsPtr->value("use_present_value");
        bool ok = Util::isValidBoolString(v.toString());
        if (ok){
            usePresentValue = v.toBool();
        } else {
            usePresentValue = factorySettings.usePresentValue; // revert to default if data is invalid
        }
    } else{
        usePresentValue = factorySettings.usePresentValue;
    }
    bool discountRateOk = false;
    if (settingsPtr->contains("pv_discount_rate")){
        v = settingsPtr->value("pv_discount_rate");
        bool ok;
        double aDouble = v.toDouble(&ok);
        if ( (!ok) || (aDouble<0) || (aDouble>100) )  {
            // settings is invalid, revert back to default
            pvDiscountRate = factorySettings.pvDiscountRate;
        } else {
            pvDiscountRate = aDouble;
            discountRateOk = true;
        }
    } else{
        pvDiscountRate = factorySettings.pvDiscountRate;
    }
    if( (usePresentValue==true) && (discountRateOk==false) ){
        usePresentValue = false;
    }
    LOG_INFO(QString("    use_present_value = %1 pvDiscountRate=%2")
        .arg(usePresentValue).arg(pvDiscountRate));

    // mouse wheel moving away from user : effect on zoom
    // allow the use of decoration color for incomes/expenses
    if (settingsPtr->contains("wheel_rotated_away_zoom_in")){
        v = settingsPtr->value("wheel_rotated_away_zoom_in");
        bool ok = Util::isValidBoolString(v.toString());
        if (ok){
            wheelRotatedAwayZoomIn = v.toBool();
        } else {
            wheelRotatedAwayZoomIn = factorySettings.wheelRotatedAwayZoomIn;    // default if data is invalid
        }
    } else{
        wheelRotatedAwayZoomIn = factorySettings.wheelRotatedAwayZoomIn;
    }
    LOG_INFO(QString("    wheel_rotated_away_zoom_in = %1")
        .arg(wheelRotatedAwayZoomIn));

    // show Y zero line
    if (settingsPtr->contains("show_y_zero_line")){
        v = settingsPtr->value("show_y_zero_line");
        bool ok = Util::isValidBoolString(v.toString());
        if (ok){
            showYzeroLine = v.toBool();
        } else {
            showYzeroLine = factorySettings.showYzeroLine;    // default if data is invalid
        }
    } else{
        showYzeroLine = factorySettings.showYzeroLine;
    }
    LOG_INFO(QString("    show_y_zero_line = %1")
        .arg(showYzeroLine));

    // Yzero line Dark color
    if (settingsPtr->contains("y_zero_line_dark_mode_color")){
        v = settingsPtr->value("y_zero_line_dark_mode_color");
        QString s = v.toString();
        yZeroLineDarkModeColor = QColor(s);
        if (yZeroLineDarkModeColor.isValid()==false) {
            yZeroLineDarkModeColor = factorySettings.yZeroLineDarkModeColor;
        }
    } else{
        // Default color
        yZeroLineDarkModeColor = factorySettings.yZeroLineDarkModeColor;
    }
    LOG_INFO(QString("    y_zero_line_dark_mode_color = %1")
        .arg(yZeroLineDarkModeColor
                                                                                             .name(QColor::HexRgb)));

    // Yzero line Light color
    if (settingsPtr->contains("y_zero_line_light_mode_color")){
        v = settingsPtr->value("y_zero_line_light_mode_color");
        QString s = v.toString();
        yZeroLineLightModeColor = QColor(s);
        if (yZeroLineLightModeColor.isValid()==false) {
            yZeroLineLightModeColor = factorySettings.yZeroLineLightModeColor;
        }
    } else{
        // Default color
        yZeroLineLightModeColor = factorySettings.yZeroLineLightModeColor;
    }
    LOG_INFO(QString("    y_zero_line_light_mode_color = %1")
        .arg(yZeroLineLightModeColor
                                                                                              .name(QColor::HexRgb)));

    // Gridlines dark color
    if (settingsPtr->contains("gridlines_color_dark_mode")){
        v = settingsPtr->value("gridlines_color_dark_mode");
        QString s = v.toString();
        gridlinesDarkModeColor = QColor(s);
        if (gridlinesDarkModeColor.isValid()==false) {
            gridlinesDarkModeColor = factorySettings.gridlinesDarkModeColor;
        }
    } else{
        gridlinesDarkModeColor = factorySettings.gridlinesDarkModeColor;
    }
    LOG_INFO(QString("    gridlines_color_dark_mode = %1")
        .arg(gridlinesDarkModeColor.name(QColor::HexRgb)));

    // Gridlines light color
    if (settingsPtr->contains("gridlines_color_light_mode")){
        v = settingsPtr->value("gridlines_color_light_mode");
        QString s = v.toString();
        gridlinesLightModeColor = QColor(s);
        if (gridlinesLightModeColor.isValid()==false) {
            gridlinesLightModeColor = factorySettings.gridlinesLightModeColor;
        }
    } else{
        gridlinesLightModeColor = factorySettings.gridlinesLightModeColor;
    }
    LOG_INFO(QString("    gridlines_color_light_mode = %1")
        .arg(gridlinesLightModeColor.name(QColor::HexRgb)));

    // X-Axis Date Format
    if (settingsPtr->contains("xaxis_date_format")){
        bool ok;
        v = settingsPtr->value("xaxis_date_format");
        int anInt = v.toInt(&ok);
        if ( (!ok) || (anInt<0) || (anInt>2) )  {
            // settings is invalid
            xAxisDateFormat = factorySettings.xAxisDateFormat;
        } else {
            xAxisDateFormat = anInt;
        }
    } else{
        // support old version of scenario file
        xAxisDateFormat = factorySettings.xAxisDateFormat; // Locale
    }
    LOG_INFO(QString("    xaxis_date_format = %1")
        .arg(xAxisDateFormat));

    // show tooltips
    if (settingsPtr->contains("show_tooltips")){
        v = settingsPtr->value("show_tooltips");
        bool ok = Util::isValidBoolString(v.toString());
        if (ok){
            showTooltips = v.toBool();
        } else {
            showTooltips = factorySettings.showTooltips;    // default if data is invalid
        }
    } else{
        showTooltips = factorySettings.showTooltips;
    }
    LOG_INFO(QString("    show_tooltips = %1")
        .arg(showTooltips));

    // color for income or positive amount (since 1.7).
    if (settingsPtr->contains("income_color")){
        v = settingsPtr->value("income_color");
        QString s = v.toString();
        incomeColor= QColor(s);
        if (incomeColor.isValid()==false) {
            incomeColor = factorySettings.incomeColor;
        }
    } else{
        incomeColor = factorySettings.incomeColor;
    }
    LOG_INFO(QString("    income_color = %1")
        .arg(incomeColor.name(QColor::HexRgb)));

    // color for expense or negative amount (since 1.7)
    if (settingsPtr->contains("expense_color")){
        v = settingsPtr->value("expense_color");
        QString s = v.toString();
        expenseColor = QColor(s);
        if (expenseColor.isValid()==false) {
            expenseColor = factorySettings.expenseColor;
        }
    } else{
        expenseColor = factorySettings.expenseColor;
    }
    LOG_INFO(QString("    expense_color = %1")
        .arg(expenseColor.name(QColor::HexRgb)));

     // loaded is completed and successful
    settingsLoaded = true;

    // Log the success and completion of loading
    LOG_INFO(QString("Settings loaded successfully"));

}


void GbpController::saveSettings()
{
    // we store the recent_files in a different format, in order to choose a different separator
    // than ",". Using "|||" as separator can still produce collisions, but very unlikely.
    // We still can read the old format, when loading, but is is converted to the new one on
    // the fly.
    settingsPtr->setValue("recent_files_v2",recentFilenames.join("|||"));

    settingsPtr->setValue("chart_point_size",chartPointSize);
    settingsPtr->setValue("chart_theming",chartThemingFromEnumToInt(chartTheming));
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
    settingsPtr->setValue("export_text_amount_localized",exportTextNumberLocalized);
    settingsPtr->setValue("export_text_date_localized",exportTextDateLocalized);
    settingsPtr->setValue("last_dir",lastDir);
    settingsPtr->setValue("last_dir_import",lastDirImport);
    settingsPtr->setValue("last_dir_export",lastDirExport);
    settingsPtr->setValue("main_chart_scaling_percentage",percentageMainChartScaling);
    settingsPtr->setValue("use_default_system_font",useDefaultSystemFont);
    settingsPtr->setValue("custom_application_font",customApplicationFont);
    settingsPtr->setValue("today_use_system_date",todayUseSystemDate);
    settingsPtr->setValue("today_specific_date",todayCustomDate.toString(Qt::DateFormat::ISODate));
    settingsPtr->setValue("allow_decoration_color",allowDecorationColor);
    settingsPtr->setValue("use_present_value",usePresentValue);
    settingsPtr->setValue("pv_discount_rate",pvDiscountRate);
    settingsPtr->setValue("wheel_rotated_away_zoom_in",wheelRotatedAwayZoomIn);
    settingsPtr->setValue("show_y_zero_line",showYzeroLine);
    settingsPtr->setValue("y_zero_line_dark_mode_color", yZeroLineDarkModeColor
        .name(QColor::HexRgb));
    settingsPtr->setValue("y_zero_line_light_mode_color", yZeroLineLightModeColor
        .name(QColor::HexRgb));
    settingsPtr->setValue("gridlines_color_dark_mode", gridlinesDarkModeColor.name(QColor::HexRgb));
    settingsPtr->setValue("gridlines_color_light_mode", gridlinesLightModeColor.name(QColor::HexRgb));
    settingsPtr->setValue("xaxis_date_format",xAxisDateFormat);
    settingsPtr->setValue("show_tooltips",showTooltips);
    settingsPtr->setValue("income_color", incomeColor.name(QColor::HexRgb));
    settingsPtr->setValue("expense_color", expenseColor.name(QColor::HexRgb));

    settingsPtr->sync();
}


// Completely empty the config file
void GbpController::resetSettings()
{
    settingsPtr->clear();
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


QWeakPointer<Scenario> GbpController::getScenario() const
{
    if (scenario==nullptr) {
        return QWeakPointer<Scenario>();
    } else {
        return scenario.toWeakRef();
    }
}


int GbpController::chartThemingFromEnumToInt(ChartTheming theme)
{
    switch (theme) {
        case ChartTheming::FORCE_LIGHT:
            return 0;
        case ChartTheming::FORCE_DARK:
            return 1;
        case ChartTheming::FOLLOW_DESKTOP_THEME:
            return 2;
        default:
            throw std::invalid_argument(QString("%1: unknown chart theme")
                .arg(Q_FUNC_INFO).toStdString());// Should never happen
    }
}


bool GbpController::chartThemingFromIntToEnum(int value, ChartTheming &convertedTheme)
{
    switch (value) {
        case 0:
            convertedTheme = ChartTheming::FORCE_LIGHT;
            return true;
        case 1:
            convertedTheme = ChartTheming::FORCE_DARK;
            return true;
        case 2:
            convertedTheme = ChartTheming::FOLLOW_DESKTOP_THEME;
            return true;
        default:
            return false;// illegal int value
    }
}


QString GbpController::chartThemingToString(ChartTheming theme)
{
    switch (theme) {
        case ChartTheming::FORCE_LIGHT:
            return "Force Light mode";
        case ChartTheming::FORCE_DARK:
            return "Force Dark mode";
        case ChartTheming::FOLLOW_DESKTOP_THEME:
            return "Follow desktop theme";
        default:
            throw std::invalid_argument(QString("%1: unknown chart theme")
                .arg(Q_FUNC_INFO).toStdString());// Should never happen
    }
}


bool GbpController::useDarkModeForChart()
{
    if ( (chartTheming == ChartTheming::FORCE_DARK) ||
        ((systemDesktopDarkTheme==true) && (chartTheming == ChartTheming::FOLLOW_DESKTOP_THEME)) ) {
        return true;
    } else {
        return false;
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

GbpController::ChartTheming GbpController::getChartTheming() const
{
    return chartTheming;
}

void GbpController::setChartTheming(ChartTheming newchartTheming)
{
    chartTheming = newchartTheming;
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

bool GbpController::getExportTextNumberLocalized() const
{
    return exportTextNumberLocalized;
}

void GbpController::setExportTextNumberLocalized(bool newExportTextNumberLocalized)
{
    exportTextNumberLocalized = newExportTextNumberLocalized;
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


void GbpController::setScenario(QSharedPointer<Scenario> newScenario)
{
    scenario = newScenario;
}


QColor GbpController::getIncomeColor() const
{
    return incomeColor;
}

void GbpController::setIncomeColor(const QColor &newIncomeColor)
{
    incomeColor = newIncomeColor;
}

QColor GbpController::getExpenseColor() const
{
    return expenseColor;
}

void GbpController::setExpenseColor(const QColor &newExpenseColor)
{
    expenseColor = newExpenseColor;
}


// *** Getters only ***

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

QString GbpController::getInitialSystemApplicationFont() const
{
    return initialSystemApplicationFont;
}

QString GbpController::getSettingsFullFileName() const
{
    return settingsFullFileName;
}

bool GbpController::getSystemDesktopDarkTheme() const
{
    return systemDesktopDarkTheme;
}

void GbpController::setSystemDesktopDarkTheme(bool newSystemDesktopDarkTheme)
{
    systemDesktopDarkTheme = newSystemDesktopDarkTheme;
}

QString GbpController::getWorkspace() const
{
    return workspace;
}

QString GbpController::getLastDirImport() const
{
    return lastDirImport;
}

void GbpController::setLastDirImport(const QString &newLastDirImport)
{
    lastDirImport = newLastDirImport;
}

QString GbpController::getLastDirExport() const
{
    return lastDirExport;
}

void GbpController::setLastDirExport(const QString &newLastDirExport)
{
    lastDirExport = newLastDirExport;
}


QColor GbpController::getYZeroLineLightModeColor() const
{
    return yZeroLineLightModeColor;
}

void GbpController::setYZeroLineLightModeColor(const QColor &newYZeroLineLightModeColor)
{
    yZeroLineLightModeColor = newYZeroLineLightModeColor;
}

QColor GbpController::getYZeroLineDarkModeColor() const
{
    return yZeroLineDarkModeColor;
}

void GbpController::setYZeroLineDarkModeColor(const QColor &newYZeroLineDarkModeColor)
{
    yZeroLineDarkModeColor = newYZeroLineDarkModeColor;
}

QColor GbpController::getGridlinesDarkModeColor() const
{
    return gridlinesDarkModeColor;
}

void GbpController::setGridlinesDarkModeColor(const QColor &newGridlinesDarkModeColor)
{
    gridlinesDarkModeColor = newGridlinesDarkModeColor;
}

QColor GbpController::getGridlinesLightModeColor() const
{
    return gridlinesLightModeColor;
}

void GbpController::setGridlinesLightModeColor(const QColor &newGridlinesLightModeColor)
{
    gridlinesLightModeColor = newGridlinesLightModeColor;
}

bool GbpController::getShowTooltips() const
{
    return showTooltips;
}

void GbpController::setShowTooltips(bool newShowTooltips)
{
    showTooltips = newShowTooltips;
}

uint GbpController::getXAxisDateFormat() const
{
    return xAxisDateFormat;
}

void GbpController::setXAxisDateFormat(uint newXAxisDateFormat)
{
    xAxisDateFormat = newXAxisDateFormat;
}

bool GbpController::getShowYzeroLine() const
{
    return showYzeroLine;
}

void GbpController::setShowYzeroLine(bool newShowYzeroLine)
{
    showYzeroLine = newShowYzeroLine;
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
