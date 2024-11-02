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
#include <QDesktopServices>
#include "gbpcontroller.h"

void showWelcomeScreen(bool french);


#define APP_NAME "graphical-budget-planner"
#define APP_VERSION "1.5.1"

int main(int argc, char *argv[])
{
    // First thing to do. Do NOT set Organization Name (e.g. will affect logs location)
    QApplication a(argc, argv);
    QCoreApplication::setApplicationName(APP_NAME);         // used by QSettings
    QCoreApplication::setApplicationVersion(APP_VERSION);

    // Create the Controller BEFORE the UI is built. It holds the scenario, settings, enable
    // the logging system
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        QString("Application started"));

    // Clean up the old log files and load settings
    GbpController::getInstance().cleanUpLogs();
    GbpController::getInstance().loadSettings();

    // get the Locale to use throughout the application
    bool systemLocaleUsed;
    QLocale sysLocale = Util::getLocale(QCoreApplication::arguments(), systemLocaleUsed);
    if (systemLocaleUsed==false) {
        QLocale::setDefault(sysLocale); // force the "default" to this one
    }

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
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
            "Default system font as reported by Qt : "+fo.toString());
        if (false == GbpController::getInstance().getUseDefaultSystemFont()){
            GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
                "Custom font to be used as the application font... Attempting font change");
            // settings ask to use a custom application font. // should be valid, but take no chance
            QString fString =GbpController::getInstance().getCustomApplicationFont();
            GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
                "Custom font to be used is : " + fString);
            if ( fString.trimmed().length() != 0){
                QFont f ;
                bool ok = f.fromString(fString);  // test if it is a valid font string description
                if ( ok )  {
                    a.setFont(f);  // valid font string description
                    GbpController::getInstance().log(GbpController::LogLevel::Minimal,
                        GbpController::Info, "Success of the custom font installation");
                } else {
                    GbpController::getInstance().log(GbpController::LogLevel::Minimal,
                        GbpController::Warning, "Custom font cannot be installed : keeping"
                            " system font");
                }
            }
        } else {
            GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
                "Default system font will be used as the application font");
        }
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
            "Default system font forced to be used as the application font");
    }



    // Useful info to log
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info,
        "Application's current directory : " + QDir::currentPath());
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info,
        "Directory that contains the application executable : " +
        QCoreApplication::applicationDirPath());
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        "System Locale is : " +sysLocale.name());
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        "    Decimal point : " +sysLocale.decimalPoint());
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        "    Group separator : " +sysLocale.groupSeparator());
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        "    Language : " + QLocale::languageToString(sysLocale.language()));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        "    Territory : " + QLocale::territoryToString(sysLocale.territory()));

    // set translation mechanism asap. Only language is considered in the Locale, not the territory
    QTranslator translator;
    QString trFileName = "gbp_"+sysLocale.languageToCode(sysLocale.language())+".qm" ;
    // required for AppImage (they are "mounted" in a temp dir by the system)
    QString pathToTranslationFiles =  QCoreApplication::applicationDirPath();
    //
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        "Attempting to load translation file " + trFileName + " from directory " +
        pathToTranslationFiles + "... ");
    bool result = translator.load(trFileName,pathToTranslationFiles);
    if (result==true) {
        a.installTranslator(&translator);
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
            "Translation file found and loaded");
    } else {
        QString langString = QString(
            "No Translation file found for this language \"%1\", English will be used.")
            .arg(sysLocale.languageToCode(sysLocale.language()));
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Warning,
            langString);
    }

    // init some Classes before starting
    Util::init();

    MainWindow w(sysLocale);

    if (GbpController::getInstance().getNoSettingsFileAtStartup()==true) {
        // no settings file found : this must be the first time the app is run
        // display a welcome screen
        showWelcomeScreen( (sysLocale.language()==QLocale::French) ? (true) : (false));
    }

    w.show();
    int r = a.exec();

    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        QString("Application ended"));
    return r;

}

void showWelcomeScreen(bool french)
{
    // first, copy the changelog included in the resource to a tmp directory
    // Name of the file in temp dir is dependant on the version !
    QString baseFileName = QString("/gbp_Welcome-%1-%2.pdf").arg((french)?("fr"):("en")).
        arg(QCoreApplication::applicationVersion());
    QString tempFileFullName = QDir::tempPath().append(baseFileName);
    QFile tempFile(tempFileFullName);

    // build resource name and check if it exists (it should)
    QFile welcomeFile(QString(":/Doc/resources/Graphical Budget Planner - Welcome-%1.pdf").
        arg((french)?("fr"):("en")));
    if(welcomeFile.exists()==false){
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Error,
            QString("Viewing Welcome : %1 does not exist in the resource file").arg(
            welcomeFile.fileName()));
        return;
    }

    //  check if the temp file exist. Copy only if non existent
    bool success;
    if (tempFile.exists()==true) {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
            "Viewing Welcome : File already exists in temp directory, not copied");
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
            QString("Viewing Welcome : Ready to copy Change Log in tmp directory : %1").arg(
            tempFileFullName));
        success = welcomeFile.copy(tempFileFullName);
        if (success==true) {
            GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
                QString("Viewing Welcome : Copy succeeded"));

        } else {
            GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Error,
                QString("Viewing Welcome : Copy failed"));
            return;
        }
    }

    // then, use the system defaut application to read the file
    success = QDesktopServices::openUrl(QUrl::fromLocalFile(tempFileFullName));
    if (success==true) {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
            QString("Viewing Welcome : PDF Viewer Launch succeeded"));
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Error,
            QString("Viewing Welcome : PDF Viewer Launch failed"));
    }
}



