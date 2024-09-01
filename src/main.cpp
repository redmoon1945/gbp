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

#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QStandardPaths>
#include <QDir>

#include "gbpcontroller.h"

#define APP_NAME "graphical-budget-planner"
#define APP_VERSION "1.3.0"

int main(int argc, char *argv[])
{
    // First thing to do. Do NOT set Organization Name (e.g. will affect logs location)
    QApplication a(argc, argv);
    QCoreApplication::setApplicationName(APP_NAME);         // used by QSettings
    QCoreApplication::setApplicationVersion(APP_VERSION);

    // Create the Controller BEFORE the UI is built. it holds the scenario, settings, enable the logging system
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Application started"));

    // Clean up the old log files and load settings
    GbpController::getInstance().cleanUpLogs();
    GbpController::getInstance().loadSettings();

    // Default application font override if user has decided so in Settings.
    // If option "-usesystemfont" is passed, it forces the use of system font
    // which is very handy if user messed up with weird font.
    QStringList argList = QCoreApplication::arguments();
    bool forceSystemFont = false;
    for (int i = 0; i < argList.size(); ++i) {      // search arg that set "-usesystemfont"
        if ( "-usesystemfont"== argList.at(i)) {
            forceSystemFont = true;
            break;
        }
    }
    if (forceSystemFont == false){
        QFont fo = QApplication::font();
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "Default system font as reported by Qt : "+fo.toString());
        if (false == GbpController::getInstance().getUseDefaultSystemFont()){
            GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "Custom font to be used as the application font... Attempting font change");
            // settings ask to use a custom application font
            QString fString =GbpController::getInstance().getCustomApplicationFont(); // should be valid, but take no chance
            GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "Custom font to be used is : " + fString);
            if ( fString.trimmed().length() != 0){
                QFont f ;
                bool ok = f.fromString(fString);  // test if it is a valid font string description
                if ( ok )  {
                    a.setFont(f);  // valid font string description
                    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "Success of the custom font installation");
                } else {
                    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Warning, "Custom font cannot be installed : keeping system font");
                }
            }
        } else {
            GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "Default system font will be used as the application font");
        }
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "Default system font forced to be used as the application font");
    }


    // get the system Locale
    QLocale sysLocale = QLocale::system();

    // Useful info to log
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, "Application's current directory : " + QDir::currentPath());
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, "Directory that contains the application executable : " + QCoreApplication::applicationDirPath());
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "System Locale is : " +sysLocale.name());
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "    Decimal point : " +sysLocale.decimalPoint());
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "    Group separator : " +sysLocale.groupSeparator());
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "    Language : " + QLocale::languageToString(sysLocale.language()));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "    Territory : " + QLocale::territoryToString(sysLocale.territory()));

    // set translation mechanism asap
    QTranslator translator;
    QString trFileName = "gbp_"+sysLocale.name()+".qm" ;
    QString pathToTranslationFiles =  QCoreApplication::applicationDirPath(); // required for AppImage (they are "mounted" in a temp dir by the system)
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "Attempting to load translation file " + trFileName + " from directory " + pathToTranslationFiles + "... ");
    bool result = translator.load(trFileName,pathToTranslationFiles);
    if (result==true) {
        a.installTranslator(&translator);
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "Translation file found and loaded");
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Warning,"No Translation file found for this locale");
    }

    // init some Classes before starting
    Util::init();

    MainWindow w(sysLocale);
    w.show();
    int r = a.exec();

    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Application ended"));
    return r;

}
