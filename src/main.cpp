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

#include "globaltooltipfilter.h"
#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QStandardPaths>
#include <QDir>
#include <QDesktopServices>
#include "gbpcontroller.h"
#include "gbplogger.h"
#include <QStyleFactory>
#include "constants.h"
#include <QProcess>
#include <iostream>


// Enum for system theme mode
enum class SystemThemeMode {
    Dark,
    Light,
    Other
};

// Enum for XWayland behavior
enum class XWaylandBehavior {
    Forced,      // -xwayland switch was passed
    Prevented,   // -noxwayland switch was passed
    Unspecified  // neither switch was passed
};

// Function to create a dark palette
static QPalette createDarkPalette()
{
    QPalette darkPalette;

    // Window colors - softer, more sophisticated
    darkPalette.setColor(QPalette::Window, QColor(45, 45, 48));        // Softer dark gray
    darkPalette.setColor(QPalette::WindowText, QColor(230, 230, 230)); // Off-white, not pure white

    // Base colors for input widgets
    darkPalette.setColor(QPalette::Base, QColor(30, 30, 32));          // Slightly darker
    darkPalette.setColor(QPalette::AlternateBase, QColor(50, 50, 53)); // Slightly lighter
    darkPalette.setColor(QPalette::Text, QColor(230, 230, 230));       // Off-white

    // Button colors
    darkPalette.setColor(QPalette::Button, QColor(45, 45, 48));
    darkPalette.setColor(QPalette::ButtonText, QColor(230, 230, 230));

    // Tooltips - dark theme appropriate
    darkPalette.setColor(QPalette::ToolTipBase, QColor(30, 30, 32));   // Dark background
    darkPalette.setColor(QPalette::ToolTipText, QColor(230, 230, 230)); // Light text

    // Highlights - muted, softer blue
    darkPalette.setColor(QPalette::Highlight, QColor(58, 120, 180));   // Softer blue
    darkPalette.setColor(QPalette::HighlightedText, QColor(230, 230, 230));

    // Links
    darkPalette.setColor(QPalette::Link, QColor(100, 180, 255));       // Lighter blue for visibility

    // Disabled text
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(120, 120, 120));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 120, 120));

    return darkPalette;
}


/**
 * @brief Detects if system theme is dark across platforms.
 * @details Queries platform-specific settings to determine if the theme is dark.
 * Supports Windows 10/11, Linux (X11/XWayland/native Wayland) with GNOME, Cinnamon,
 * XFCE, KDE/Plasma.
 *
 * Detection strategy (Windows):
 *   - Queries Registry for AppsUseLightTheme (0x0=dark, 0x1=light)
 *
 * Detection strategy (Linux):
 *   - If desktop is GNOME: Check GNOME color-scheme, then GTK theme
 *   - If desktop is KDE: Check KDE 6, then KDE 5 color scheme
 *   - If desktop is Cinnamon: Check Cinnamon GTK theme, then GTK config
 *   - If desktop is XFCE: Check XFCE theme via xfconf, then GTK config
 *   - Otherwise: Return false (unknown desktop)
 *
 * @param desktop Desktop environment name (Linux only, e.g., "GNOME", "KDE", "Cinnamon").
 * Ignored on Windows.
 * @return bool: true if dark theme was confirmed, false otherwise
 *
 * @note Each detection method has a 1-second timeout to prevent hangs.
 */
static bool detectDarkTheme(const QString& desktop = "")
{
#if defined(Q_OS_WIN)
    // // Windows 10/11: Check Registry for dark theme preference
    // QProcess process;
    // process.start("reg", QStringList() << "query"
    //     << "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows"
    //     << "\\CurrentVersion\\Themes\\Personalize"
    //     << "/v" << "AppsUseLightTheme");
    // process.waitForFinished(1000);

    // if (process.exitCode() == 0) {
    //     QString output = process.readAllStandardOutput();
    //     // AppsUseLightTheme: 0x0 = dark, 0x1 = light
    //     if (output.contains("0x0", Qt::CaseInsensitive)) {
    //         return true;
    //     }
    // }
    // return false;

    QSettings settings(
        R"(HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)",
        QSettings::NativeFormat
        );

    // AppsUseLightTheme: 0 = dark, 1 = light
    // Default to light (1) if key doesn't exist (fresh installs)
    int lightTheme = settings.value("AppsUseLightTheme", 1).toInt();
    bool isDark = (lightTheme == 0);
    return isDark;


#elif defined(Q_OS_LINUX)
    QProcess process;

    // Optimize detection based on known desktop environment
    if (desktop.contains("GNOME", Qt::CaseInsensitive)) {
        // GNOME: Try color-scheme first, then GTK theme
        process.start("gsettings",
            QStringList() << "get"
            << "org.gnome.desktop.interface"
            << "color-scheme");
        process.waitForFinished(1000);

        if (process.exitCode() == 0) {
            QString output = process.readAllStandardOutput().trimmed();
            if (output.contains("dark", Qt::CaseInsensitive) ||
                output.contains("'1'")) {
                return true;
            } else if (!output.contains("'default'", Qt::CaseInsensitive)) {
                return false;
            }
        }

        // Fallback to GTK theme name
        process.start("gsettings",
            QStringList() << "get"
            << "org.gnome.desktop.interface"
            << "gtk-theme");
        process.waitForFinished(1000);

        if (process.exitCode() == 0) {
            QString output = process.readAllStandardOutput().trimmed();
            if (output.contains("dark", Qt::CaseInsensitive)) {
                return true;
            } else {
                return false;
            }
        }

    } else if (desktop.contains("KDE", Qt::CaseInsensitive) ||
               desktop.contains("Plasma", Qt::CaseInsensitive)) {
        // KDE: Try kreadconfig6, then kreadconfig5
        process.start("kreadconfig6",
            QStringList() << "--file" << "kdeglobals"
            << "--group" << "General"
            << "--key" << "ColorScheme");
        process.waitForFinished(1000);

        if (process.exitCode() == 0) {
            QString output = process.readAllStandardOutput().trimmed();
            if (output.contains("dark", Qt::CaseInsensitive)) {
                return true;
            } else {
                return false;
            }
        }

        // Fallback to kreadconfig5
        process.start("kreadconfig5",
            QStringList() << "--file" << "kdeglobals"
            << "--group" << "General"
            << "--key" << "ColorScheme");
        process.waitForFinished(1000);

        if (process.exitCode() == 0) {
            QString output = process.readAllStandardOutput().trimmed();
            if (output.contains("dark", Qt::CaseInsensitive)) {
                return true;
            } else {
                return false;
            }
        }

    } else if (desktop.contains("Cinnamon", Qt::CaseInsensitive)) {
        // Cinnamon (LMDE, Linux Mint): Try Cinnamon settings, then GTK theme, then GTK config
        process.start("gsettings",
            QStringList() << "get"
            << "org.cinnamon.desktop.interface"
            << "gtk-theme");
        process.waitForFinished(1000);

        if (process.exitCode() == 0) {
            QString output = process.readAllStandardOutput().trimmed();
            if (output.contains("dark", Qt::CaseInsensitive)) {
                return true;
            } else {
                return false;
            }
        }

        // Fallback to GNOME GTK theme
        process.start("gsettings",
            QStringList() << "get"
            << "org.gnome.desktop.interface"
            << "gtk-theme");
        process.waitForFinished(1000);

        if (process.exitCode() == 0) {
            QString output = process.readAllStandardOutput().trimmed();
            if (output.contains("dark", Qt::CaseInsensitive)) {
                return true;
            } else {
                return false;
            }
        }

    } else if (desktop.contains("XFCE", Qt::CaseInsensitive)) {
        // XFCE: Use xfconf to query the theme
        process.start("xfconf-query",
            QStringList() << "-c" << "xsettings"
            << "-p" << "/Net/ThemeName");
        process.waitForFinished(1000);

        if (process.exitCode() == 0) {
            QString output = process.readAllStandardOutput().trimmed();
            if (output.contains("dark", Qt::CaseInsensitive)) {
                return true;
            } else {
                return false;
            }
        }

        // Fallback to GTK config files
        QStringList gtkConfigPaths = {
            QDir::homePath() + "/.config/gtk-4.0/settings.ini",
            QDir::homePath() + "/.config/gtk-3.0/settings.ini"
        };

        for (const QString& configPath : gtkConfigPaths) {
            QFile configFile(configPath);
            if (configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString content = configFile.readAll();
                configFile.close();
                if (content.contains("gtk-application-prefer-dark-mode=true", Qt::CaseInsensitive)) {
                    return true;
                } else if (content.contains("gtk-application-prefer-dark-mode=false", Qt::CaseInsensitive)) {
                    return false;
                }
                if (content.contains("dark", Qt::CaseInsensitive)) {
                    return true;
                }
            }
        }
    } else {
        // Unknown desktop environment on Linux - cannot detect theme
        return false;
    }

    return false;
#else
    // macOS and other platforms: not implemented yet
    return false;
#endif
}


void showWelcomeScreen(bool french);

/**
 * @brief Get the Windows look style from command-line arguments.
 * @param argc Number of command-line arguments
 * @param argv Array of command-line arguments
 * @return QString: "fusion" (default), "vista", or "native"
 */
static QString getWindowsLookStyle(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromUtf8(argv[i]);
        if (arg.startsWith("-windowslook=")) {
            QString style = arg.mid(13); // Remove "-windowslook=" prefix
            if (style == "native" || style == "vista" || style == "fusion") {
                return style;
            }
        }
    }
    return "fusion"; // Default to Fusion style
}

/**
 * @brief Check if -h or --help was passed and display help, then exit.
 * @param argc Number of command-line arguments
 * @param argv Array of command-line arguments
 */
static void checkHelpArgument(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromUtf8(argv[i]);
        if (arg == "-h" || arg == "--help") {
            std::string appName = Constants::APP_NAME.toStdString();
            std::string appVer = Constants::APP_VERSION.toStdString();
            std::cout <<  appName << " v" << appVer << "\n";
            std::cout << "USAGE:\n";
            std::cout << "  gbp [options]\n";
            std::cout << "OPTIONS:\n";
            std::cout << "  -h, --help\n";
            std::cout << "    Display help message and exit\n";
            std::cout << "\n  APPEARANCE OPTIONS:\n";
            std::cout << "  -usesystemfont\n";
            std::cout << "    Force the use of the default system font (all platforms)\n";
            std::cout << "  -windowslook=STYLE\n";
            std::cout << "    Set Windows style (Windows only). STYLE can be:\n";
            std::cout << "      fusion  - Cross-platform Fusion style (default)\n";
            std::cout << "      native  - Windows 11 native style\n";
            std::cout << "      vista   - Windows Vista style\n";
            std::cout << "\n  DISPLAY SERVER OPTIONS:\n";
            std::cout << "  -xwayland\n";
            std::cout << "    Force XWayland instead of using native Wayland (Linux only)\n";
            std::cout << "  -noxwayland\n";
            std::cout << "    Prevent switching to XWayland, use native Wayland only (Linux only)\n";
            std::cout << "\n  LOCALIZATION OPTIONS:\n";
            std::cout << "  -locale=LANG-TERRITORY\n";
            std::cout << "    Override system locale (e.g., -locale=en-US) (all platforms)\n";
            std::cout << "\n  LOGGING OPTIONS:\n";
            std::cout << "  -logprivacy=ALLOW_PRIVATE\n";
            std::cout << "    Allow sensitive data in logs (all platforms)\n";
            std::cout << "  -logprivacy=PUBLIC_ONLY\n";
            std::cout << "    Hide sensitive data from logs, default (all platforms)\n";
            std::cout << "  -logverbosity=DEBUG\n";
            std::cout << "    Set verbose debug logging (all platforms)\n";
            std::cout << "  -logverbosity=NORMAL\n";
            std::cout << "    Set normal logging verbosity, default (all platforms)\n\n";

            exit(0);
        }
    }
}

/**
 * @brief Check required XWayland behavior from command-line arguments.
 * @param argc Number of command-line arguments
 * @param argv Array of command-line arguments
 * @return XWaylandBehavior: Forced if -xwayland, Prevented if -noxwayland, Unspecified otherwise
 */
static XWaylandBehavior isXWaylandBehavior(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromUtf8(argv[i]);
        if (arg == "-xwayland") {
            return XWaylandBehavior::Forced;
        }
        if (arg == "-noxwayland") {
            return XWaylandBehavior::Prevented;
        }
    }
    return XWaylandBehavior::Unspecified;
}

/**
 * @brief Get the value of an environmental variable, taken into account the possibility
 * it does not exist.
 * @param name The name of the variable
 * @return The value.
 */
static QString safeGetEnv(const char* name)
{
    const char* val = std::getenv(name);
    return val ? QString::fromUtf8(val) : QString("unknown");
}

/**
 * @brief Write to the terminal the app name and version, plus some system info.
 * @param sessionType Value of the associated env variable.
 * @param desktop Value of the associated env variable.
 */
static void getSessionTypeAndDesktop(QString& sessionType,QString& desktop )
{
    sessionType = "unknown";
    desktop = "unknown";

#if defined(Q_OS_LINUX)
    sessionType = safeGetEnv("XDG_SESSION_TYPE");
    desktop     = safeGetEnv("XDG_CURRENT_DESKTOP");
#elif defined(Q_OS_WIN)
    sessionType = "win32";
    desktop     = "Windows Desktop";
#elif defined(Q_OS_MACOS)
    sessionType = "cocoa";
    desktop     = "macOS Desktop";
#else
    // Fallback: whatever Qt reports
    sessionType = QGuiApplication::platformName();
    desktop     = QGuiApplication::platformName();
#endif
}


int main(int argc, char *argv[])
{
    // Check for help argument early, before any Qt initialization
    checkHelpArgument(argc, argv);

    /**
     * @brief Accumulate log messages until we are ready to write them in the log
     */
    QStringList logBuffer;


    // Print app name/version, desktop, session type
    QString sessionType;
    QString desktop;
    getSessionTypeAndDesktop(sessionType,desktop);
    QString introString = QString("%1 %2, running on %3 %4")
        .arg(Constants::APP_NAME).arg(Constants::APP_VERSION).arg(desktop).arg(sessionType);
    qInfo().noquote() << introString;
    logBuffer.append(introString);


#if defined(Q_OS_LINUX)

    /** We had no decoration on Gnome Wayland (works though on KDE,
     *  because it implements SSD, which is something Gnome Wayland refuses
     *  to do. Same behavior for LMDE / Mint Wayland.
     *  As of nov 2025, we are not aware of another Wayland desktop that
     *  behaves like this.
     *
     *  From Grok :
     *  "What changed in Qt 6.9.x : Starting around Qt 6.9, upstream removed
     *  that fallback CSD path: Qt Wayland requires an external decoration
     *  plugin (from wayland-decoration-client), otherwise you get the
     *  message: No decoration plugins available. Running with no
     *  decorations. This change was intentional: the fallback was
     *  considered inconsistent and "un-themed" (looked out of place on
     *  GNOME). So → what you're seeing is not a bug but an upstream
     *  behavior change between Qt 6.8.x and 6.9.x."
     *  So we need to fall back to X11 on Gnome Wayland if we detect
     *  Gnome Wayland.
     *
     *  The goal of this section is to analyse if we have to switch to XWayland
    */
    if (sessionType.compare("wayland", Qt::CaseInsensitive) == 0) {

        XWaylandBehavior behavior = isXWaylandBehavior(argc, argv);

        // Check if user forced XWayland via command-line argument
        if (behavior == XWaylandBehavior::Forced) {
            qputenv("QT_QPA_PLATFORM", QByteArray("xcb"));
            sessionType = "x11";
            qInfo().noquote() << "XWayland forced by -xwayland argument";
            logBuffer.append("XWayland forced by -xwayland argument");
        } else if (behavior == XWaylandBehavior::Prevented) {
            // User explicitly prevented XWayland, use native Wayland
            qInfo().noquote() << "Native Wayland enforced by -noxwayland argument";
            logBuffer.append("Native Wayland enforced by -noxwayland argument");
        } else {
            // behavior == XWaylandBehavior::Unspecified (automatic detection)
            if (desktop.contains("GNOME", Qt::CaseInsensitive)) {
                // Force XWayland for GNOME Wayland sessions (needed for window decorations)
                qputenv("QT_QPA_PLATFORM", QByteArray("xcb"));
                sessionType = "x11";
                qInfo().noquote() << "Switching to XWayland";
                logBuffer.append("Wayland + Gnome detected : => Switching to XWayland");
            } else if (desktop.contains("Cinnamon", Qt::CaseInsensitive)){
                // Force XWayland for Cinnamon Wayland sessions (needed for window decorations)
                qputenv("QT_QPA_PLATFORM", QByteArray("xcb"));
                sessionType = "x11";
                qInfo().noquote() << "Switching to XWayland";
                logBuffer.append("Wayland + Cinnamon detected : => Switching to XWayland");
            }
        }
    }

    // Set platform theme based on desktop environment
    // The basic principle is that if we are running on Wayland, we set xdgdesktopportal
    // and do not interfere with dark theme management.
    // If on X11, we set the "Fusion" theme and set custom dark theme if required.
    if (sessionType.compare("wayland", Qt::CaseInsensitive) == 0) {
        // --- WAYLAND ---
        // Does support theming through xdgdesktopportal
        qputenv("QT_QPA_PLATFORMTHEME", QByteArray("xdgdesktopportal"));
        QString msg("Wayland: using xdgdesktopportal theme");
        qInfo().noquote() << msg.toStdString();
        logBuffer.append(msg);
    } else {
        // --- X11 ---
        // Do not support theming, so we implement our own simulated one for dark theme
        // Will be applied later
    }


#endif

    // First thing to do. Do NOT set Organization Name (e.g. will affect logs location)
    QApplication a(argc, argv);
    QCoreApplication::setApplicationName(Constants::APP_NAME);         // used by QSettings
    QCoreApplication::setApplicationVersion(Constants::APP_VERSION);

    // *** Dark theme detection  ***
    bool darkThemeDetected = detectDarkTheme(desktop);
    if (darkThemeDetected) {
        QString msg("Dark theme detected for system desktop");
        qInfo().noquote() << msg.toStdString();
        logBuffer.append(msg);
    }


#if defined(Q_OS_WIN)
    // Get Windows look style argument (default: fusion)
    QString windowsLookStyle = getWindowsLookStyle(argc, argv);

    if (windowsLookStyle == "native") {
        // Use native Windows 11 style
        QString msg("Windows: using native Windows 11 style");
        qInfo().noquote() << msg.toStdString();
        logBuffer.append(msg);
    } else if (windowsLookStyle == "vista") {
        // Use Windows Vista style (uses Windows native colors)
        QApplication::setStyle(QStyleFactory::create("windowsvista"));
        QString msg("Windows: using Windows Vista style");
        qInfo().noquote() << msg.toStdString();
        logBuffer.append(msg);
    } else if (windowsLookStyle == "fusion") {
        // Use Fusion style for consistent cross-platform behavior (default)
        QApplication::setStyle(QStyleFactory::create("Fusion"));
        if (darkThemeDetected) {
            QApplication::setPalette(createDarkPalette());
            QString msg("Windows: using Fusion style with dark palette");
            qInfo().noquote() << msg.toStdString();
            logBuffer.append(msg);
        } else {
            QString msg("Windows: using Fusion style");
            qInfo().noquote() << msg.toStdString();
            logBuffer.append(msg);
        }
    }

#elif defined(Q_OS_LINUX)

    // Apply Fusion style + dark palette for X11 and XWayland
    // Must be applied after QApplication a(argc, argv)
    if (sessionType.compare("wayland", Qt::CaseInsensitive) != 0) {
        // Apply fusion style whatever the theme
        QApplication::setStyle(QStyleFactory::create("Fusion"));
        QString msg("X11: applying Fusion style");
        qInfo().noquote() << msg.toStdString();
        logBuffer.append(msg);
        // If Dark mode theme detected, apply custom palette
        if (darkThemeDetected==true) {
            QApplication::setPalette(createDarkPalette());
            msg = "X11: applying custom dark palette";
            logBuffer.append(msg);
            qInfo().noquote() << msg.toStdString();
        }
    }

#endif

    LOG_INFO(QString("Application started"));

    // Unpack the log buffer now that we are ready to do so.
    for (const QString &item : logBuffer){
        LOG_INFO(item);
    }

    // Clean up the old log files, use "real today" as reference date
    GbpLogger::getInstance().cleanUpLogs(QDate::currentDate());

    // Star the GBP Controller by loading settings
    GbpController::getInstance().loadSettings();

    // Indicate if we have detected Dark Theme for the desktop
    GbpController::getInstance().setSystemDesktopDarkTheme(darkThemeDetected);

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
        LOG_INFO("Default system font as reported by Qt : "+fo.toString());
        if (false == GbpController::getInstance().getUseDefaultSystemFont()){
            QString fString =GbpController::getInstance().getCustomApplicationFont();

            // settings ask to use a custom application font. // should be valid, but take no chance
            if ( fString.trimmed().length() != 0){
                QFont f ;
                bool ok = f.fromString(fString);  // test if it is a valid font string description
                if ( ok==true )  {
                    a.setFont(f);
                    LOG_INFO(QString("Custom font \"%1\" will be used as the "
                        "application font...").arg(fString));
                } else {
                    LOG_ERROR(QString("Custom font \"%1\"s cannot be installed : keeping default"
                        " system font").arg(fString));
                }
            }
        } else {
            LOG_INFO("Default system font will be used as the application font");
        }
    } else {
        LOG_INFO("Default system font forced to be used as the application font");
    }

    // Useful info to log
    LOG_INFO("Application's current directory : " + REDACT(QDir::currentPath()));
    LOG_INFO("Directory that contains the application executable : " +
        REDACT(QCoreApplication::applicationDirPath()));
    LOG_INFO("System Locale is : " + sysLocale.name());
    LOG_INFO("    Decimal point : " + sysLocale.decimalPoint());
    LOG_INFO("    Group separator : " + sysLocale.groupSeparator());
    LOG_INFO("    Language : " + QLocale::languageToString(sysLocale.language()));
    LOG_INFO("    Territory : " + QLocale::territoryToString(sysLocale.territory()));

    // set translation mechanism asap. Only language is considered in the Locale, not the territory
    QTranslator translator;
    QString trFileName = "gbp_"+sysLocale.languageToCode(sysLocale.language())+".qm" ;
    // required for AppImage (they are "mounted" in a temp dir by the system)
    QString pathToTranslationFiles =  QCoreApplication::applicationDirPath();
    //
    LOG_INFO("Attempting to load translation file " +
        trFileName + " from directory " + REDACT(pathToTranslationFiles) + "... ");
    bool result = translator.load(trFileName,pathToTranslationFiles);
    if (result==true) {
        a.installTranslator(&translator);
        LOG_INFO("Translation file found and loaded");
    } else {
        QString langString = QString(
            "No translation file found for this language \"%1\", English will be used.")
            .arg(sysLocale.languageToCode(sysLocale.language()));
        LOG_WARNING(langString);
    }


    // Block all tooltips if required in the Options
    GlobalTooltipFilter filter;
    if (GbpController::getInstance().getShowTooltips() == false ){
        a.installEventFilter(&filter);
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

    LOG_INFO("Application ended normally");
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
        LOG_ERROR(
            QString("Viewing welcome : %1 does not exist in the resource file").arg(
            welcomeFile.fileName()));
        return;
    }

    //  check if the temp file exist. Copy only if non existent
    bool success;
    if (tempFile.exists()==true) {
        LOG_INFO("Viewing welcome : File already exists in temp directory, not copied");
    } else {
        LOG_INFO(QString("Viewing welcome : Ready to copy change log in tmp directory : %1")
            .arg(tempFileFullName));
        success = welcomeFile.copy(tempFileFullName);
        if (success==true) {
            LOG_INFO(QString("Viewing welcome : Copy succeeded"));

        } else {
            LOG_ERROR(QString("Viewing welcome : Copy failed"));
            return;
        }
    }

    // then, use the system defaut application to read the file
    success = QDesktopServices::openUrl(QUrl::fromLocalFile(tempFileFullName));
    if (success==true) {
        LOG_INFO(QString("Viewing welcome : PDF viewer launch succeeded"));
    } else {
        LOG_ERROR(QString("Viewing welcome : PDF viewer launch failed"));
    }
}

