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

#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "gbpcontroller.h"
#include <QDesktopServices>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    ui->appLabel->setText(QCoreApplication::applicationName() + "  " + QCoreApplication::applicationVersion());
    QString builtOn = QString(tr("Built on : %1 %2")).arg(__DATE__).arg(__TIME__);
    ui->buildOnLabel->setText(builtOn);

    QFontMetrics fm(ui->configFilePlainTextEdit->font());
    ui->configFilePlainTextEdit->setFixedHeight(fm.height()*3); // 2 lines min
    ui->logFilePlainTextEdit->setFixedHeight(fm.height()*3); // 2 lines min
    // set first tab as current
    ui->tabWidget->setCurrentIndex(0);
}


AboutDialog::~AboutDialog()
{
    delete ui;
}


void AboutDialog::slotAboutDialogPrepareContent(QLocale theLocale)
{
    ui->configFilePlainTextEdit->setPlainText(GbpController::getInstance().getSettingsFullFileName());
    ui->logFilePlainTextEdit->setPlainText(GbpController::getInstance().getLogFullFileName());

    // Locale
    QString locString = QString("%1 (%2 %3)").
        arg(theLocale.name()).
        arg(theLocale.nativeLanguageName()).
        arg(theLocale.nativeTerritoryName());
    ui->localeLineEdit->setText(locString);
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
    // then, use the system defaut application to read the file
    bool success = QDesktopServices::openUrl(QUrl::fromLocalFile(ui->logFilePlainTextEdit->toPlainText()));
    if (success==true) {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal,
            GbpController::Info, QString("Viewing Log File : Viewer Launch succeeded"));
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal,
            GbpController::Error, QString("Viewing Log File : Viewer Launch failed"));
    }
}

