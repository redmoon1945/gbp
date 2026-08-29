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

#include "aboutdialog.h"
#include <QTimer>
#include "ui_aboutdialog.h"
#include "gbpcontroller.h"
#include "gbplogger.h"
#include "uiutil.h"
#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QDate>
#include <QTime>
#include <QDateTime>
#include <qstandardpaths.h>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);

    ui->plainTextEdit->document()->setDefaultFont(QApplication::font());

    ui->appLabel->setText(QCoreApplication::applicationName() + "  " +
        QCoreApplication::applicationVersion());
    // "Built on" label is set in slotAboutDialogPrepareContent(), once the locale is known -
    // __DATE__/__TIME__ are fixed-format, always-English compile-time strings and need to be
    // parsed before they can be shown in the application's locale.

    QFontMetrics fm(ui->configFilePlainTextEdit->font());
    ui->configFilePlainTextEdit->setFixedHeight(fm.height()*3);
    ui->logFilePlainTextEdit->setFixedHeight(fm.height()*3);
    ui->cachePlainTextEdit->setFixedHeight(fm.height()*3);

    // set first tab as current
    ui->tabWidget->setCurrentIndex(0);
}


AboutDialog::~AboutDialog()
{
    delete ui;
}


void AboutDialog::slotAboutDialogPrepareContent(const QLocale &theLocale)
{
    // Built on : __DATE__/__TIME__ are fixed-format, always-English compile-time strings
    // (e.g. "Aug 28 2026", "14:32:07") - parse them back into real QDate/QTime so they can be
    // shown using the application's locale, consistent with the rest of the app. For a
    // single-digit day, __DATE__ pads with a space instead of a zero (e.g. "Aug  8 2026",
    // two spaces) - simplified() collapses that back to one space so it matches the format.
    QDate buildDate = QDate::fromString(QString(__DATE__).simplified(), "MMM d yyyy");
    QTime buildTime = QTime::fromString(QString(__TIME__), "hh:mm:ss");
    QDateTime buildDateTime(buildDate, buildTime);
    QString builtOn = tr("Built on : %1")
        .arg(theLocale.toString(buildDateTime, "yyyy-MMM-dd HH:mm:ss"));
    ui->buildOnLabel->setText(builtOn);

    ui->configFilePlainTextEdit->setPlainText(
        QDir::toNativeSeparators(GbpController::getInstance().getSettingsFullFileName()));
    ui->logFilePlainTextEdit->setPlainText(
        QDir::toNativeSeparators(GbpLogger::getInstance().getLogFullFileName()));
    ui->workspaceLineEdit->setText(GbpController::getInstance().getWorkspace());
    ui->cachePlainTextEdit->setText(
        QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)));

    // Locale
    QString locString = QString("%1 (%2 %3)").
        arg(theLocale.name()).
        arg(theLocale.nativeLanguageName()).
        arg(theLocale.nativeTerritoryName());
    ui->localeLineEdit->setText(locString);

    // Log verbosity
    GbpLogger::LogVerbosity verbosity = GbpLogger::getInstance().getLogVerbosity();
    QString verbosityStr = (verbosity == GbpLogger::LogVerbosity::DEBUG)
        ? tr("Debug") : tr("Normal");
    ui->logVerbosityLineEdit->setText(verbosityStr);

    // Log privacy
    GbpLogger::LogPrivacy privacy = GbpLogger::getInstance().getLogPrivacy();
    QString privacyStr = (privacy == GbpLogger::LogPrivacy::ALLOW_PRIVATE)
        ? tr("Allow private") : tr("Public only");
    ui->logPrivacyLineEdit->setText(privacyStr);

    ui->closePushButton->setFocus();

}


void AboutDialog::on_AboutDialog_rejected()
{
    on_closePushButton_clicked();
}


void AboutDialog::on_closePushButton_clicked()
{
    hide();
}


void AboutDialog::on_viewLogPushButton_clicked()
{
    QDesktopServices::openUrl(
        QUrl::fromLocalFile(GbpLogger::getInstance().getLogFullFileName()));
}



void AboutDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("AboutDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
    });
}

