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

#include "loadvariablegrowthtextfiledialog.h"
#include "gbpcontroller.h"
#include "gbplogger.h"
#include "growth.h"
#include "ui_loadvariablegrowthtextfiledialog.h"
#include "util.h"
#include "uiutil.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include "gbpqmessage.h"


LoadVariableGrowthTextFileDialog::LoadVariableGrowthTextFileDialog(
    const QString newGrowthName,QLocale aLocale, QWidget *parent)
    : QDialog(parent) , ui(new Ui::LoadVariableGrowthTextFileDialog)
{
    ui->setupUi(this);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);

    QFont appFont = QApplication::font();
    QFont font = appFont;
    Util::changeFontSize(font, Util::FontResizeIntensity::AVERAGE, true,
        "LoadVariableGrowthTextFileDialog - instructions");
    ui->instructionsPlainTextEdit->setFont(font);

    growthName = newGrowthName.isEmpty()
        ? newGrowthName
        : newGrowthName[0].toLower() + newGrowthName.mid(1);
    QString s = QString(tr("Importing variable %1")).arg(growthName);
    this->setWindowTitle(s);

    theLocale = aLocale;
}


LoadVariableGrowthTextFileDialog::~LoadVariableGrowthTextFileDialog()
{
    delete ui;
}


void LoadVariableGrowthTextFileDialog::slotPrepareContent()
{
    ui->fileNameLineEdit->setFocus();
}


void LoadVariableGrowthTextFileDialog::on_cancelPushButton_clicked()
{
    this->hide();
    emit signalImportCompleted();
}


void LoadVariableGrowthTextFileDialog::on_importPushButton_clicked()
{
    QString userErrorMessage, logErrorMessage, line;
    QString fileName = ui->fileNameLineEdit->text();
    int lineNo = 0;

    LOG_INFO(QString("Attempting to import a text file containing variable %1 entries : "
        "file name=\"%2\"").arg(growthName).arg(REDACT(fileName)));

    QFile file(fileName);
    if (!file.exists()) {
        userErrorMessage = QString(tr("File %1 does not exist")).arg(fileName);
        logErrorMessage  = QString("File %1 does not exist").arg(fileName);
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            userErrorMessage, {tr("OK")}, 0, 0);
        LOG_ERROR(QString("Import failed : %1").arg(logErrorMessage));
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        userErrorMessage = tr("Cannot open file %1 in read-only mode").arg(fileName);
        logErrorMessage  = QString("Cannot open file %1 in read-only mode").arg(REDACT(fileName));
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            userErrorMessage, {tr("OK")}, 0, 0);
        LOG_ERROR(QString("Import failed : %1").arg(logErrorMessage));
        return;
    }

    try {
        QTextStream stream(&file);
        stream.setAutoDetectUnicode(true);
        QMap<QDate,qint64> data;

        while (!stream.atEnd()) {
            line = stream.readLine();
            lineNo++;
            if (line.isNull()) continue;

            QString lineTrimmed = line.trimmed();
            QStringList tokens = lineTrimmed.split('\t');
            if (tokens.length() != 2) {
                userErrorMessage = tr("Bad format for line no %1 (expected 2 TAB-separated "
                    "tokens, got %2).").arg(lineNo).arg(tokens.size());
                logErrorMessage  = QString("Bad format for line no %1 (expected 2 tokens, "
                    "got %2).").arg(lineNo).arg(tokens.size());
                GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
                    userErrorMessage, {tr("OK")}, 0, 0);
                LOG_ERROR(QString("Import failed : %1").arg(logErrorMessage));
                return;
            }

            QDate date = QDate::fromString(tokens[0], Qt::ISODate);
            if (!date.isValid()) {
                userErrorMessage = tr("Date \"%1\" is invalid at line no %2.")
                    .arg(tokens[0]).arg(lineNo);
                logErrorMessage  = QString("Date \"%1\" is invalid at line no %2.")
                    .arg(tokens[0]).arg(lineNo);
                GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
                    userErrorMessage, {tr("OK")}, 0, 0);
                LOG_ERROR(QString("Import failed : %1").arg(logErrorMessage));
                return;
            }

            bool ok;
            double growth = tokens[1].toDouble(&ok);
            if (!ok) {
                userErrorMessage = tr("%1 value \"%2\" is not a valid number at line no %3.")
                    .arg(growthName).arg(tokens[1]).arg(lineNo);
                logErrorMessage  = QString("%1 value \"%2\" is not a valid number "
                    "at line no %3.").arg(growthName).arg(tokens[1]).arg(lineNo);
                GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
                    userErrorMessage, {tr("OK")}, 0, 0);
                LOG_ERROR(QString("Import failed : %1").arg(logErrorMessage));
                return;
            }
            if (growth < Growth::MIN_GROWTH_DOUBLE || growth > Growth::MAX_GROWTH_DOUBLE) {
                userErrorMessage = tr("%1 value \"%2\" is out of the allowed range "
                    "[%3, %4] at line no %5.")
                    .arg(growthName)
                    .arg(tokens[1])
                    .arg(Growth::MIN_GROWTH_DOUBLE)
                    .arg(Growth::MAX_GROWTH_DOUBLE)
                    .arg(lineNo);
                logErrorMessage = QString("%1 value \"%2\" out of range at line no %3.")
                    .arg(growthName).arg(tokens[1]).arg(lineNo);
                GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
                    userErrorMessage, {tr("OK")}, 0, 0);
                LOG_ERROR(QString("Import failed : %1").arg(logErrorMessage));
                return;
            }

            data.insert(date, Growth::fromDoubleToDecimal(growth));
        }

        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::INFORMATION, tr("Information"),
            QString(tr("Import succeeded, %1 entries have been imported.")).arg(data.size()), {tr("OK")}, 0, 0);
        emit signalImportResult(data);
        this->hide();
        LOG_INFO(QString("Import succeeded"));

    } catch (const std::exception& e) {
        userErrorMessage = tr("An unexpected error has occurred.<br><br>Details : %1").arg(e.what());
        logErrorMessage  = QString("An unexpected error has occurred. Details : %1").arg(e.what());
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            userErrorMessage, {tr("OK")}, 0, 0);
        LOG_ERROR(QString("Import failed : %1").arg(logErrorMessage));
    }
}


void LoadVariableGrowthTextFileDialog::on_browsePushButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select a CSV file"),
        GbpController::getInstance().getLastDirImport());
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.exists()) {
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            tr("File %1 does not exist").arg(fileName), {tr("OK")}, 0, 0);
        return;
    }
    ui->fileNameLineEdit->setText(file.fileName());
    GbpController::getInstance().setLastDirImport(QFileInfo(file).absolutePath());
}


void LoadVariableGrowthTextFileDialog::on_LoadVariableGrowthTextFileDialog_rejected()
{
    on_cancelPushButton_clicked();
}


void LoadVariableGrowthTextFileDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("LoadVariableGrowthTextFileDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
    });
}
