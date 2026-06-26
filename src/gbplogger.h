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

#ifndef GBPLOGGER_H
#define GBPLOGGER_H

#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDate>
#include <QString>


// Convenience macro for logging
#define REDACT_EXPLICIT(data, placeholder) GbpLogger::getInstance().redact(data, placeholder)
#define REDACT(data) GbpLogger::getInstance().redact(data)
#define LOG_INFO(msg)    GbpLogger::getInstance().logInfo(msg)
#define LOG_WARNING(msg) GbpLogger::getInstance().logWarning(msg)
#define LOG_ERROR(msg)   GbpLogger::getInstance().logError(msg)
#define LOG_DEBUG_INFO(msg)   GbpLogger::getInstance().log(GbpLogger::LogVerbosity::DEBUG, \
                              GbpLogger::LogType::INFO, msg)

/**
 * @brief Singleton class responsible for all logging functionality in GBP.
 * @details This class provides centralized logging with privacy controls,
 * automatic log rotation, and thread-safe operations. Verbosity and Privacy
 * are set only from the command line options received by gbp upon start up. They
 * are not read from settings nor written to settings.
 */
class GbpLogger
{
public:
    /**
     * @brief Logging privacy level enumeration. Controls whether sensitive information
     * (file paths, amounts, scenario names, user data) is included in log files.
     */
    enum class LogPrivacy {
        /**
         * @brief Public information only (safe default). No private data is logged. File paths,
         * amounts, scenario names, and other sensitive information is replaced with placeholders.
         */
        PUBLIC_ONLY,

        /**
         * @brief Allow private data in logs. Full details including file paths, amounts, scenario
         * names are logged. Only enable when debugging on a secure system.
         */
        ALLOW_PRIVATE
    };

    /**
     * @brief Type of log message.
     */
    enum class LogType {
        INFO,
        WARNING,
        ERROR
    };

    /**
     * @brief Define log verbosity. Normal or Debug mode.
     */
    enum class LogVerbosity {
        NORMAL,  /// Always logged (errors, warnings, key operations)
        DEBUG    /// Only when debug mode enabled (verbose logging)
    };

    // Singleton access
    static GbpLogger& getInstance();

    // Delete copy/move constructors and assignment operators
    GbpLogger(const GbpLogger&) = delete;
    GbpLogger(GbpLogger&&) = delete;
    GbpLogger& operator=(const GbpLogger&) = delete;
    GbpLogger& operator=(GbpLogger&&) = delete;


    /**
     * @brief Long version of logging a message, with specified verbosity and type.
     * @details When written to the log file, the messagee is formatted to fit in a
     * CSV file (tab separated fields).s
     * @param verbosity Minimum log verbosity required (Normal=always, Debug=only if enabled)
     * @param type Type of message (Info, Warning, Error)
     * @param message The message to log
     */
    void log(LogVerbosity verbosity, LogType type, const QString& message);

    /**
     * @brief Shorten version of logging a message. Type = INFO, Level = NORMAL.
     * @param message The message to log.
     */
    void logInfo(const QString& message);

    /**
     * @brief Shorten version of logging a message. Type = WARNING, Level = NORMAL.
     * @param message The message to log.
     */
    void logWarning(const QString& message);

    /**
     * @brief Shorten version of logging a message. Type = ERROR, Level = NORMAL.
     * @param message The message to log.
     */
    void logError(const QString& message);


    /**
     * @brief Redact sensitive data based on current privacy setting.
     *
     * @details This function returns either the actual sensitive data or a
     * placeholder text depending on the current log privacy setting. When privacy
     * is set to PUBLIC_ONLY (the safe default), the redactedText placeholder is
     * returned. When set to ALLOW_PRIVATE, the actual privateData is returned.
     *
     * @param privateData The actual sensitive information (file path, amount, name, etc.)
     * @param redactedText Placeholder text to show when privacy is disabled (default: "[REDACTED]")
     * @return QString Either privateData (if ALLOW_PRIVATE) or redactedText (if PUBLIC_ONLY)
     *
     * @example
     * @code
     * QString filePath = "/home/user/secret.gbp";
     * QString msg = QString("Loading from %1").arg(GbpLogger::getInstance().redact(filePath, "[FILE]"));
     * // PUBLIC_ONLY:    "Loading from [FILE]"
     * // ALLOW_PRIVATE:  "Loading from /home/user/secret.gbp"
     * @endcode
     */
    QString redact(const QString& privateData, const QString& redactedText = "[REDACTED]") const;

    /**
     * @brief Clean up old log files to prevent accumulation.
     * @details Removes log files older than 100 days before an arbitrary "today", regardless
     * of workspace. Both default and workspace-specific logs are deleted based on age alone.
     * @param today Reference date for calculating file age
     */
    void cleanUpLogs(const QDate& today);

    /**
     * @brief Get the log folder path.
     * @details Returns the folder where log files are stored, as determined during construction.
     * @return Absolute path to the log folder.
     */
    QString getLogFolder() const;

    /**
     * @brief Remove log files for specified workspaces.
     * @details Deletes all log files whose names match the workspace suffix pattern for each
     * given workspace. Log filenames follow the format yyyy-MM-dd__hh_mm_ss_WORKSPACE.txt
     * (25 + workspace length characters).
     * @param workspaces List of workspace names to remove logs for. If empty, nothing is deleted.
     * @param deleted Populated with filenames that were successfully deleted.
     * @param failed Populated with filenames that could not be deleted.
     */
    void removeWorkspaceLogs( const QStringList& workspaces, QStringList& deleted,
        QStringList& failed);

    // Getters
    QString getLogFullFileName() const;
    LogPrivacy getLogPrivacy() const;
    LogVerbosity getLogVerbosity() const;


private:
    GbpLogger();
    ~GbpLogger();

    /**
     * @brief Current logging privacy setting.
     * @details Determines whether sensitive information is included in log output. Default is
     * PUBLIC_ONLY for security. Can be changed via command-line (-logprivacy=1).
     */
    LogPrivacy logPrivacy;

    LogVerbosity logVerbosity;

    // Log file management
    QString logFolder;
    QString logFullFileName;
    QFile logFile;
    QTextStream logOutStream;
    bool loggingEnabled;

    // Thread safety
    QMutex logMutex;


};

#endif // GBPLOGGER_H
