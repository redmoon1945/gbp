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
#include <QLoggingCategory>
#include <QToolTip>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif


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


/**
 * @brief Count the log files belonging to one workspace.
 * @details Log filenames follow "yyyy-MM-dd__hh_mm_ss.txt" for the default workspace, or
 * "yyyy-MM-dd__hh_mm_ss_WORKSPACE.txt" for a named workspace (see GbpLogger). The exact
 * length check avoids one workspace's name matching as a substring of another's.
 * @param logFiles All *.txt files found in the log directory.
 * @param workspaceName Workspace name, or an empty string for the default workspace.
 * @return Number of log files belonging to that workspace.
 */
static int countLogFiles(const QFileInfoList &logFiles, const QString &workspaceName)
{
    const int timestampLength = 20; // "yyyy-MM-dd__hh_mm_ss"
    int count = 0;

    if (workspaceName.isEmpty()) {
        for (const QFileInfo &logInfo : logFiles) {
            if (logInfo.fileName().length() == timestampLength + 4) { // + ".txt"
                count++;
            }
        }
    } else {
        QString suffix = "_" + workspaceName + ".txt";
        for (const QFileInfo &logInfo : logFiles) {
            QString name = logInfo.fileName();
            if (name.endsWith(suffix) && name.length() == timestampLength + suffix.length()) {
                count++;
            }
        }
    }
    return count;
}

/**
 * @brief List all workspaces found in the config directory.
 * @details Scans .ini files matching the application name pattern and extracts workspace names.
 * Shows how many log files belong to each workspace. The default workspace is always listed
 * first.
 */
static void listWorkspaces()
{
    // Note: We can't use QStandardPaths yet because QCoreApplication hasn't
    // been initialized. We need to create a temporary QCoreApplication first.
    int dummyArgc = 1;
    char appName[] = "gbp";
    char* dummyArgv[] = {appName, nullptr};
    QCoreApplication tempApp(dummyArgc, dummyArgv);
    QCoreApplication::setApplicationName(Constants::APP_NAME);

    // This is the same path as the one that will later be used by GbpController
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString appNameStr = Constants::APP_NAME;

    QDir dir(configPath);
    if (!dir.exists()) {
        std::cout << "  No workspaces found.\n";
        std::cout << "  (Workspaces are created on first application run)\n";
        exit(0);
    }

    QStringList filters;
    filters << appNameStr + ".ini" << appNameStr + "_*.ini";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    if (files.isEmpty()) {
        std::cout << "  No workspaces found.\n";
        std::cout << "  (Workspaces are created on first application run)\n";
    } else {
        // Locate the log directory the same way GbpLogger does, without creating it:
        // AppDataLocation/logs normally, falling back to tempPath()/gbp.
        QString logFolder = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
            + "/logs";
        QDir logDir(logFolder);
        if (!logDir.exists()) {
            logDir.setPath(QDir::tempPath() + "/gbp");
        }
        QFileInfoList logFiles;
        if (logDir.exists()) {
            logFiles = logDir.entryInfoList({"*.txt"}, QDir::Files);
        }

        std::cout << "Found " << files.size() << " workspace(s):\n\n";

        for (const QFileInfo &fileInfo : files) {
            QString fileName = fileInfo.fileName();

            // Extract workspace name if present
            // Pattern: graphical-budget-planner_WORKSPACE.ini
            QString prefix = appNameStr + "_";
            QString workspaceName;
            QString displayName;
            if (fileName.startsWith(prefix) && fileName.endsWith(".ini")) {
                int start = prefix.length();
                int len = fileName.length() - start - 4;
                workspaceName = fileName.mid(start, len);
                displayName = workspaceName;
            } else {
                displayName = "(default)";
            }

            int logCount = countLogFiles(logFiles, workspaceName);
            std::cout << "  " << displayName.toStdString()
                      << "  (" << logCount << " log file(s))\n";
        }

        std::cout << "\nUse -workspace_cleanup to remove all non-default workspaces.\n\n";
    }

    exit(0);
}

/**
 * @brief Extract the workspace name from a log file name.
 * @details Log filenames follow "yyyy-MM-dd__hh_mm_ss.txt" for the default workspace, or
 * "yyyy-MM-dd__hh_mm_ss_WORKSPACE.txt" for a named workspace (see GbpLogger).
 * @param fileName The log file's base name.
 * @return The workspace name, or an empty string if this is a default-workspace log (no
 * suffix) or the name doesn't match the expected pattern. The default workspace's logs must
 * never be reported as belonging to a named workspace, so any ambiguity resolves to empty.
 */
static QString workspaceNameFromLogFileName(const QString &fileName)
{
    const int timestampLength = 20; // "yyyy-MM-dd__hh_mm_ss"
    if (!fileName.endsWith(".txt") || fileName.length() <= timestampLength + 4
        || fileName.at(timestampLength) != QChar('_')) {
        return QString();
    }
    int start = timestampLength + 1;
    int len = fileName.length() - start - 4;
    if (len <= 0) {
        return QString();
    }
    return fileName.mid(start, len);
}

/**
 * @brief Remove all non-default workspace config and log files, plus orphaned log files.
 * @details Deletes all .ini files except the default configuration file (without workspace
 * suffix), and deletes all associated log files for each removed workspace. Also deletes
 * "orphaned" log files: logs whose workspace name has no matching .ini file at all (e.g. left
 * behind by a workspace removed by hand before this tool existed). The default workspace's
 * config and log files are never touched. Shows a preview of what will be deleted and always
 * asks for confirmation before doing anything irreversible.
 */
static void cleanupWorkspaces()
{
    // Note: We can't use QStandardPaths yet because QCoreApplication hasn't
    // been initialized. We need to create a temporary QCoreApplication first.
    int dummyArgc = 1;
    char appName[] = "gbp";
    char* dummyArgv[] = {appName, nullptr};
    QCoreApplication tempApp(dummyArgc, dummyArgv);
    QCoreApplication::setApplicationName(Constants::APP_NAME);

    // This is the same path as the one that will later be used by GbpController
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString appNameStr = Constants::APP_NAME;

    QDir dir(configPath);
    if (!dir.exists()) {
        std::cout << "  Configuration directory does not exist yet.\n";
        std::cout << "  Nothing to clean up.\n";
        exit(0);
    }

    QStringList filters;
    filters << appNameStr + ".ini" << appNameStr + "_*.ini";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    if (files.isEmpty()) {
        std::cout << "  No configuration files found.\n";
        std::cout << "  Nothing to clean up.\n";
        exit(0);
    }

    QString defaultConfigName = appNameStr + ".ini";

    // Build the list of workspaces that would actually be deleted, without touching anything
    // yet, so the user can be shown a preview before we do anything irreversible.
    QString prefix = appNameStr + "_";
    QStringList workspacesToDelete;
    for (const QFileInfo &fileInfo : files) {
        QString fileName = fileInfo.fileName();
        if (fileName != defaultConfigName && fileName.startsWith(prefix)
            && fileName.endsWith(".ini")) {
            int start = prefix.length();
            int len = fileName.length() - start - 4;
            workspacesToDelete.append(fileName.mid(start, len));
        }
    }

    // Also find orphaned log files: logs whose workspace name has no corresponding .ini
    // file at all. The log directory is located the same way GbpLogger does, without
    // creating it (this is a read-only preview at this point). The default workspace
    // (no suffix) is never a candidate here: workspaceNameFromLogFileName() only returns
    // a name for suffixed, non-default logs.
    QString logFolder = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + "/logs";
    QDir logDir(logFolder);
    if (!logDir.exists()) {
        logDir.setPath(QDir::tempPath() + "/gbp");
    }
    QStringList orphanedWorkspaces;
    QFileInfoList logFiles;
    if (logDir.exists()) {
        logFiles = logDir.entryInfoList({"*.txt"}, QDir::Files);
        for (const QFileInfo &logInfo : logFiles) {
            QString ws = workspaceNameFromLogFileName(logInfo.fileName());
            if (!ws.isEmpty() && !workspacesToDelete.contains(ws)
                && !orphanedWorkspaces.contains(ws)) {
                orphanedWorkspaces.append(ws);
            }
        }
    }

    if (workspacesToDelete.isEmpty() && orphanedWorkspaces.isEmpty()) {
        std::cout << "  No non-default workspace configuration files found.\n";
        std::cout << "  Nothing to clean up.\n";
        exit(0);
    }

    if (!workspacesToDelete.isEmpty()) {
        std::cout << "The following workspace(s) will be permanently deleted:\n\n";
        for (const QString &workspaceName : workspacesToDelete) {
            std::cout << "  " << workspaceName.toStdString() << "\n";
        }
        std::cout << "\n";
    }

    if (!orphanedWorkspaces.isEmpty()) {
        std::cout << "The following workspace(s) have no configuration file, only leftover "
                     "log files.\nTheir log files will also be permanently deleted:\n\n";
        for (const QString &workspaceName : orphanedWorkspaces) {
            std::cout << "  " << workspaceName.toStdString() << "\n";
        }
        std::cout << "\n";
    }

    std::cout << "The default workspace is never touched.\n";
    std::cout << "This cannot be undone.\n\n";

#ifdef _WIN32
    // gbp is a GUI-subsystem app on Windows: launching it from cmd.exe/PowerShell does not
    // make the shell wait for it, so the shell immediately starts reading its own next
    // command from the same console gbp is attached to - racing gbp for the user's
    // keystrokes at an interactive prompt (typing "yes" can end up being run as a command
    // by the shell instead of answering this prompt). Proceed without asking for
    // confirmation on Windows instead; the preview above is the safeguard here.
    std::cout << "No interactive confirmation available on Windows - proceeding "
                 "(see preview above).\n\n";
#else
    std::cout << "Type 'yes' to continue, anything else to cancel: ";
    std::cout.flush();
    std::string response;
    std::getline(std::cin, response);
    QString confirmation = QString::fromStdString(response).trimmed();
    if (confirmation.compare("yes", Qt::CaseInsensitive) != 0) {
        std::cout << "\nCancelled. No files were deleted.\n";
        exit(0);
    }
    std::cout << "\n";
#endif

    QStringList deletedWorkspaces;

    std::cout << "Scanning configuration files...\n\n";

    for (const QFileInfo &fileInfo : files) {
        QString fileName = fileInfo.fileName();

        // Skip the default configuration file
        if (fileName == defaultConfigName) {
            std::cout << "  Keeping: " << fileName.toStdString() << " (default config)\n";
            continue;
        }

        // Delete workspace-specific config files
        QFile file(fileInfo.absoluteFilePath());
        if (file.remove()) {
            std::cout << "  Deleted: " << fileName.toStdString() << "\n";
            // Extract workspace name for log cleanup
            if (fileName.startsWith(prefix) && fileName.endsWith(".ini")) {
                int start = prefix.length();
                int len = fileName.length() - start - 4;
                deletedWorkspaces.append(fileName.mid(start, len));
            }
        } else {
            std::cout << "  Failed to delete: " << fileName.toStdString() << "\n";
        }
    }

    // Delete log files for removed workspaces, plus any orphaned logs found earlier (the
    // default workspace is never included in either list). This reuses the log file
    // listing already gathered above, deleting directly rather than going through
    // GbpLogger::getInstance() - constructing that singleton here would create a new log
    // file of its own as a side effect, which would be a confusing thing for a cleanup
    // command to do.
    QStringList deletedLogs;
    QStringList failedLogs;
    QStringList workspacesToPurgeLogsFor = deletedWorkspaces + orphanedWorkspaces;
    if (!workspacesToPurgeLogsFor.isEmpty()) {
        std::cout << "\nDeleting associated log files...\n\n";
        for (const QString &ws : workspacesToPurgeLogsFor) {
            int deletedForWorkspace = 0;
            for (const QFileInfo &logInfo : logFiles) {
                if (workspaceNameFromLogFileName(logInfo.fileName()) != ws) {
                    continue;
                }
                if (QFile::remove(logInfo.absoluteFilePath())) {
                    deletedLogs.append(logInfo.fileName());
                    deletedForWorkspace++;
                } else {
                    failedLogs.append(logInfo.fileName());
                    std::cout << "  Failed to delete: " << logInfo.fileName().toStdString()
                              << "\n";
                }
            }
            std::cout << "  " << ws.toStdString() << ": " << deletedForWorkspace
                      << " log file(s) deleted\n";
        }
    }

    std::cout << "\n  Log files deleted: " << deletedLogs.size() << "\n\n";

    exit(0);
}

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
 * @brief Check if -h, --help, -workspace_list, or -workspace_cleanup was passed and handle them.
 * @details Rejects multiple workspace commands (-workspace=, -workspace_list, -workspace_cleanup)
 * if used simultaneously.
 * @param argc Number of command-line arguments
 * @param argv Array of command-line arguments
 */
static void checkHelpArgument(int argc, char *argv[])
{
#ifdef _WIN32
    // GUI subsystem has no console. Attach to the parent (cmd/PowerShell) so that
    // --help and workspace commands can print, and so -workspace_cleanup can read the
    // user's confirmation. Fails silently when launched without a console (e.g.
    // double-click), leaving stdout/stdin unconnected as usual.
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        freopen("CONIN$", "r", stdin);
        std::cout << "\n";
    }
#endif
    // Count workspace-related commands and reject if more than one
    int workspaceCmdCount = 0;
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromUtf8(argv[i]);
        if (arg == "-workspace_list" || arg == "-workspace_cleanup"
            || arg.startsWith("-workspace=")) {
            workspaceCmdCount++;
        }
    }
    if (workspaceCmdCount > 1) {
        std::cerr << "Error: only one workspace command (-workspace=, "
                     "-workspace_list, -workspace_cleanup) can be used at a time.\n";
        exit(1);
    }

    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromUtf8(argv[i]);

        // Handle -workspace_list (needs special initialization)
        if (arg == "-workspace_list") {
            listWorkspaces();  // This function calls exit(0)
        }

        // Handle -workspace_cleanup (needs special initialization)
        if (arg == "-workspace_cleanup") {
            cleanupWorkspaces();  // This function calls exit(0)
        }

        if (arg == "-h" || arg == "--help") {
            std::string appName = Constants::APP_NAME.toStdString();
            std::string appVer = Constants::APP_VERSION.toStdString();
            std::cout <<  appName << " v" << appVer << "\n";
            std::cout << "\nUSAGE:\n";
            std::cout << "  gbp [options]\n";
            std::cout << "\nOPTIONS:\n";
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
            std::cout << "    Prevent switching to XWayland, use native Wayland only "
                         "(Linux only)\n";
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
            std::cout << "    Set normal logging verbosity, default (all platforms)\n";
            std::cout << "\n  WORKSPACE OPTIONS:\n";
            std::cout << "  -workspace=WORKSPACE\n";
            std::cout << "    Specify workspace identifier (1-20 alphanumeric characters)\n";
            std::cout << "    Appends WORKSPACE to log and settings file names\n";
            std::cout << "    Each workspace uses separate settings and logs (all platforms)\n";
            std::cout << "  -workspace_list\n";
            std::cout << "    List all workspace configuration files and exit\n";
            std::cout << "    Shows the configuration directory location, workspace names,\n";
            std::cout << "    and the number of log files for each\n";
            std::cout << "  -workspace_cleanup\n";
            std::cout << "    Remove all non-default workspace .ini files and their\n";
            std::cout << "    associated log files, then exit\n";
            std::cout << "    Only keeps the default configuration file\n";
            std::cout << "    Use -workspace_list to preview before cleaning\n";
            std::cout << "    Shows the files to be deleted and asks for confirmation\n";
            std::cout << "    (Linux/macOS only - proceeds without asking on Windows)\n\n";

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

    // Suppress Qt font-database fallback warnings. These are emitted whenever Qt searches for
    // a font capable of rendering a script it has no dedicated font for (e.g. Arabic, CJK).
    // They are purely informational — rendering still works via system fallback fonts — but
    // they flood the console when the currency picker populates its list of all world currencies.
    QLoggingCategory::setFilterRules("qt.text.font.db=false");

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

    // Report the cache directory location, same as the config/log file locations reported
    // above (see GbpController::loadSettings() and GbpLogger's constructor).
    QString cacheDirString = QString("Cache directory : %1")
        .arg(QDir::toNativeSeparators(
            QStandardPaths::writableLocation(QStandardPaths::CacheLocation)));
    qInfo().noquote() << cacheDirString;

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

    // Log default mono font


    /*!
     * @note Item-view font on Windows: Qt's Windows platform theme registers a class-specific
     * font for @c QAbstractItemView (the icon-title font, typically Segoe UI 9pt on Windows 11).
     * Class-specific fonts have higher priority than the application-wide font set by
     * @c QApplication::setFont(), so every @c QTableWidget, @c QListWidget and @c QTreeWidget
     * in the app would silently inherit Segoe UI 9pt regardless of the configured application
     * font — making item-view text visibly smaller than all other widgets.
     *
     * This call re-registers the current application font as the class-specific font for
     * @c QAbstractItemView immediately after the application font has been finalised. Because a
     * class-specific registration always beats the app-wide default, this overrides the
     * platform's Segoe UI 9pt with the user's chosen font.
     *
     * @note This global override is a best-effort fix. In practice it is not sufficient on its
     * own: Qt's item delegate resolves cell fonts through a path that can still fall back to the
     * platform's class-specific entry, so cell text may still render at Segoe UI 9pt.
     * Each dialog therefore also calls @c widget->setFont(appFont) explicitly in its constructor
     * for every affected item view. A widget-level font has the highest priority in Qt's
     * resolution chain and cannot be overridden by any class-specific or platform font.
     * If a new item-view widget is added to a dialog, its constructor must include the same
     * explicit @c setFont(QApplication::font()) call.
     *
     * Trade-off: native Windows convention uses a smaller font in lists and tables to show more
     * rows at once. By overriding this we depart from that convention, which is acceptable here
     * because the app already uses Fusion style (not the native Windows style), so there is no
     * broader native consistency to preserve, and the user's font choice should apply everywhere.
     */
    QApplication::setFont(QApplication::font(), "QAbstractItemView");
    LOG_INFO(QString("QAbstractItemView font override applied : \"%1\"")
        .arg(QApplication::font().toString()));
    QToolTip::setFont(QApplication::font());

    /*!
     * @note Spacer scaling strategy: fixed-pixel QSpacerItem sizes defined in .ui files are
     * overridden in each form constructor via QSpacerItem::changeSize(), using values derived
     * from QFontMetrics. This makes spacing proportional to the application font, so the UI
     * remains visually consistent across different font sizes, DPI settings, and HiDPI displays.
     * Hardcoded pixel spacers appear too small at high DPI or large font sizes, whereas
     * font-metric-based spacers scale automatically.
     *
     * Each constructor reads the original pixel size directly from the spacer at runtime via
     * sizeHint(), then scales it using the following linear rules
     * (mA = fm.horizontalAdvance("M"), mH = fm.height(), fm = QFontMetrics(font())):
     *   - Horizontal spacers: newWidth  = qRound(sizeHint().width()  * mA / 20.0)
     *   - Vertical spacers:   newHeight = qRound(sizeHint().height() * mH / 30.0)
     * Reading from sizeHint() means resizing a spacer in Qt Designer automatically reflects
     * here with no code change required.
     */
    // log font metrics that are used as the spacing unit in the layouts
    QFontMetrics appFontFm(a.font());
    int mh = appFontFm.height();
    int mw = appFontFm.horizontalAdvance("M");
    QString appFontString = QString("Application system font applied : "
        " Width of capital M=%1  Full line height=%2").arg(mw).arg(mh);
    LOG_INFO(appFontString);

    // Scale checkbox and radio button indicators to match the application font size.
    // Qt styles use hardcoded pixel sizes for indicators that don't follow font changes.
    int indicatorSize = QFontMetrics(QApplication::font()).height() / 2;
    a.setStyleSheet(a.styleSheet() +
        QString("QCheckBox::indicator { width: %1px; height: %1px; }"
                "QRadioButton::indicator { width: %1px; height: %1px; }").arg(indicatorSize));

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
        w.showWelcomeScreen();
    }

    w.show();
    int r = a.exec();

    LOG_INFO("Application ended normally");
    return r;

}


