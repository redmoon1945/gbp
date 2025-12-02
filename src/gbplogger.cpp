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

#include "gbplogger.h"
#include <QDir>
#include <QStandardPaths>
#include <QTime>
#include <QDateTime>
#include <QDirIterator>
#include <QFileInfo>
#include <QCoreApplication>
#include <QStringConverter>

GbpLogger::GbpLogger()
{
    loggingEnabled = true;
    logPrivacy = LogPrivacy::PUBLIC_ONLY;
    logVerbosity = LogVerbosity::NORMAL;

    // Parse command-line arguments for log level and privacy
    QStringList argList = QCoreApplication::arguments();

    // Process log arguments
    for (int i = 0; i < argList.size(); ++i) {
        if (argList.at(i) == "-logprivacy=ALLOW_PRIVATE") {
            logPrivacy = LogPrivacy::ALLOW_PRIVATE;
            qInfo().noquote() << "Log privacy: ALLOW_PRIVATE enabled "
                "(sensitive data will be logged locally in the log file)";
        } else if (argList.at(i) == "-logprivacy=PUBLIC_ONLY") {
            logPrivacy = LogPrivacy::PUBLIC_ONLY;
            qInfo().noquote() << "Log privacy: PUBLIC_ONLY (no sensitive data logged)";
        } else if (argList.at(i) == "-logverbosity=DEBUG") {
            logVerbosity = LogVerbosity::DEBUG;
            qInfo().noquote() << "Log level: DEBUG";
        } else if (argList.at(i) == "-logverbosity=NORMAL") {
            logVerbosity = LogVerbosity::NORMAL;
            qInfo().noquote() << "Log verbosity: NORMAL";
        }
    }

    // Determine where to store the logs.
    // Try QStandardPaths::AppDataLocation/logs first
    // (linux: ~/.local/share/graphical-budget-planner/logs)
    // Otherwise use QDir::tempPath()/gbp
    bool success = false;

    QString p = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!p.isEmpty()) {
        QDir dir(p);
        if (dir.mkpath("logs")) {
            dir.cd("logs");
            logFolder = dir.absolutePath();
            success = true;
        }
    }
    if (!success) {
        logFolder = QDir::tempPath();
        QDir dir(logFolder);
        if (dir.mkpath("gbp")) {
            dir.cd("gbp");
            logFolder = dir.absolutePath();
        }
    }

    // Create log file with timestamp
    QDate today = QDateTime::currentDateTime().toLocalTime().date();
    logFullFileName = QString("%1/%2__%3.txt")
        .arg(logFolder)
        .arg(today.toString("yyyy-MM-dd"))
        .arg(QTime::currentTime().toString("hh_mm_ss"));

    logFile.setFileName(logFullFileName);
    success = logFile.open(QIODevice::Append | QIODevice::WriteOnly | QIODevice::Text);

    if (!success) {
        loggingEnabled = false;
        QString errorString = QString("Logging is disabled (cannot create a log file in %1)")
                                  .arg(logFolder);
        qWarning().noquote() << errorString;
    } else {
        loggingEnabled = true;
        QString successString = QString("Log file created : %1").arg(logFullFileName);
        qInfo().noquote() << successString;
    }

    logOutStream.setDevice(&logFile);
    logOutStream.setEncoding(QStringConverter::Utf8);
}

GbpLogger::~GbpLogger()
{
    if (logFile.isOpen()) {
        log(LogVerbosity::DEBUG, LogType::INFO, "Closing log file...");
        logOutStream.flush();
        logFile.close();
    }
}


GbpLogger& GbpLogger::getInstance()
{
    static GbpLogger instance;
    return instance;
}


void GbpLogger::logInfo(const QString &message)
{
    log(LogVerbosity::NORMAL, LogType::INFO, message);
}


void GbpLogger::logWarning(const QString &message)
{
    log(LogVerbosity::NORMAL, LogType::WARNING, message);
}


void GbpLogger::logError(const QString &message)
{
    log(LogVerbosity::NORMAL, LogType::ERROR, message);
}


void GbpLogger::log(LogVerbosity verbosity, LogType type, const QString& message)
{
    if (!loggingEnabled) {
        return;
    }

    // Check if message verbosity exceeds current setting
    if ( logVerbosity==LogVerbosity::NORMAL && verbosity==LogVerbosity::DEBUG ){
        return;
    }

    QMutexLocker locker(&logMutex);  // Thread-safe

    QString typeString;
    switch (type) {
        case LogType::INFO:
            typeString = "INFO";
            break;
        case LogType::WARNING:
            typeString = "WARNING";
            break;
        case LogType::ERROR:
            typeString = "ERROR";
            break;
    }

    QDateTime dt = QDateTime::currentDateTime();
    QString outString = QString("%1\t%2\t%3\n")
        .arg(dt.toString(Qt::ISODate))
        .arg(typeString)
        .arg(message);

    logOutStream << outString;
    logOutStream.flush();
}

QString GbpLogger::redact(const QString& privateData, const QString& redactedText) const
{
    return (logPrivacy == LogPrivacy::ALLOW_PRIVATE) ? privateData : redactedText;
}

void GbpLogger::cleanUpLogs(const QDate& today)
{
    // Clean up old logs to prevent accumulation. Remove logs older than 100 days
    qint64 nowInJulianDay = today.toJulianDay();
    QDirIterator it(logFolder, {"*.txt"}, QDir::Files);
    QStringList toBeDeleted;

    while (it.hasNext()) {
        QFile f(it.next());
        QFileInfo fileInfo(f.fileName());
        if (!fileInfo.isFile()) {
            continue;
        }

        QString baseName = fileInfo.fileName();
        if (baseName.length() < 8) {
            continue;  // not a gbp log file
        }

        QString dateString = baseName.mid(0, 10);
        QDate d = QDate::fromString(dateString, Qt::ISODate);
        if (!d.isValid()) {
            continue;
        }

        if (100 < (nowInJulianDay - d.toJulianDay())) {
            toBeDeleted.append(f.fileName());
        }
    }

    for (const QString& fName : toBeDeleted) {
        bool deletionSuccess = QFile::remove(fName);
        log(LogVerbosity::NORMAL, LogType::INFO, QString("Deleting old log file %1 : success=%2")
            .arg(redact(fName, "[LOG_FILE]"))
            .arg(deletionSuccess));
    }

}


QString GbpLogger::getLogFullFileName() const
{
    return logFullFileName;
}


GbpLogger::LogVerbosity GbpLogger::getLogVerbosity() const
{
    return logVerbosity;
}


GbpLogger::LogPrivacy GbpLogger::getLogPrivacy() const
{
    return logPrivacy;
}
