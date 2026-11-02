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
#include "csvimporter.h"
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
    ui->instructionsTextEdit->setFont(font);
    ui->instructionsTextEdit->setHtml(tr(
        "<h3>Importing entries</h3>"
        "<p>Loads a list of entries from a file. <b>The current entries are deleted.</b> This is "
        "useful when the values are computed by an external model and entering them manually "
        "would be tedious.</p>"

        "<h3>File format</h3>"
        "<ul>"
        "<li>Unicode text file (UTF-8, UTF-16 or UTF-32).</li>"
        "<li>One entry per line, with no empty lines.</li>"
        "<li>The first line can be a header : it is ignored if at least one of its fields is "
        "not valid, and the final message then shows it.</li>"
        "<li>[TAB] is the TAB character (0x09).</li>"
        "<li>If two lines share the same date, the last one wins.</li>"
        "</ul>"
        "<p>Format of a line : <code>&lt;date&gt;[TAB]&lt;growth&gt;</code><br>"
        "Example : <code>2025-01-01[TAB]8.9234</code></p>"

        "<h3>Fields</h3>"
        "<table border=\"1\" cellspacing=\"0\" cellpadding=\"4\">"
        "<tr><th>Field</th><th>Format</th></tr>"
        "<tr><td><b>Date</b></td><td>Transition date, the first day the new value applies. "
        "ISO 8601 (YYYY-MM-DD), e.g. 2025-01-01</td></tr>"
        "<tr><td><b>Growth</b></td><td>Annual growth (or inflation) rate in percentage, with "
        "&quot;.&quot; as the decimal separator : 8.9234 means 8.9234%. From -100 to 10000, up "
        "to 5 decimal places stored.</td></tr>"
        "<tr><td><b>Third field</b></td><td><i>Optional.</i> The monthly value written by the "
        "CSV export. It must be a number, and it is not used.</td></tr>"
        "</table>"));

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
    QString fileName = ui->fileNameLineEdit->text();

    LOG_INFO(QString("Attempting to import a text file containing variable %1 entries : "
        "file name=\"%2\"").arg(growthName).arg(REDACT(fileName)));

    // Show an error to the user and log it
    auto importFailed = [](const QString &userErrorMessage, const QString &logErrorMessage) {
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            userErrorMessage, {tr("OK")}, 0, 0);
        LOG_ERROR(QString("Import failed : %1").arg(logErrorMessage));
    };

    try {
        // read and check the lines. The monthly value, written by the CSV export, is not used
        // since it is derived from the annual value.
        QString capitalizedName = growthName.isEmpty()
            ? growthName
            : growthName[0].toUpper() + growthName.mid(1);
        CsvImportResult result = CsvImporter::importFile(fileName, {
            {tr("Date"), CsvImportColumn::Type::Date},
            {capitalizedName, CsvImportColumn::Type::Number},
            {tr("Monthly %1").arg(growthName), CsvImportColumn::Type::Number, true}
        });
        if (!result.success) {
            importFailed(result.userErrorMessage, result.logErrorMessage);
            return;
        }

        // check the meaning of the values
        QMap<QDate,qint64> data;
        for (const CsvImportRow &row : result.rows) {
            double growth = row.cells[1].toDouble();
            if (growth < Growth::MIN_GROWTH_DOUBLE || growth > Growth::MAX_GROWTH_DOUBLE) {
                importFailed(tr("%1 value \"%2\" is out of the allowed range "
                    "[%3, %4] at line no %5.")
                    .arg(growthName)
                    .arg(row.rawCells[1])
                    .arg(Growth::MIN_GROWTH_DOUBLE)
                    .arg(Growth::MAX_GROWTH_DOUBLE)
                    .arg(row.lineNo),
                    QString("%1 value \"%2\" out of range at line no %3.")
                    .arg(growthName).arg(row.rawCells[1]).arg(row.lineNo));
                return;
            }

            data.insert(row.cells[0].toDate(), Growth::fromDoubleToDecimal(growth));
        }

        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::INFORMATION, tr("Information"),
            CsvImporter::successMessage(data.size(), result), {tr("OK")}, 0, 0);
        emit signalImportResult(data);
        this->hide();
        LOG_INFO(QString("Import succeeded"));

    } catch (const std::exception& e) {
        importFailed(tr("An unexpected error has occurred.<br><br>Details : %1").arg(e.what()),
            QString("An unexpected error has occurred. Details : %1").arg(e.what()));
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
