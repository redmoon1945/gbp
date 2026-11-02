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

#include <QFileDialog>
#include <QTimer>
#include <QDir>
#include <QRegularExpression>
#include <QMessageBox>
#include <QDate>
#include <QCoreApplication>
#include "loadirregulartextfiledialog.h"
#include "gbpcontroller.h"
#include "ui_loadirregulartextfiledialog.h"
#include "currencyhelper.h"
#include "csvimporter.h"
#include "gbplogger.h"
#include "uiutil.h"
#include "gbpqmessage.h"


LoadIrregularTextFileDialog::LoadIrregularTextFileDialog(QLocale aLocale, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoadIrregularTextFileDialog)
{
    ui->setupUi(this);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);

    // Make smaller the font used in the instructions panel
    QFont appFont = QApplication::font();
    QFont font = appFont;
    Util::changeFontSize(font, Util::FontResizeIntensity::AVERAGE, true,
        "LoadIrregularTextFileDialog - instructions");
    ui->instructionsTextEdit->setFont(font);
    ui->instructionsTextEdit->setHtml(tr(
        "<h3>Importing events</h3>"
        "<p>Loads the events of this irregular Cash Stream Definition from a file. "
        "<b>The current events are deleted.</b> This is useful when there are many events, or "
        "when they come from a model outside GBP, such as dividends from a stock portfolio.</p>"

        "<h3>File format</h3>"
        "<ul>"
        "<li>Unicode text file (UTF-8, UTF-16 or UTF-32).</li>"
        "<li>One event per line, with no empty lines.</li>"
        "<li>The first line can be a header : it is ignored if at least one of its fields is "
        "not valid, and the final message then shows it.</li>"
        "<li>[TAB] is the TAB character (0x09).</li>"
        "</ul>"
        "<p>Format of a line : <code>&lt;date&gt;[TAB]&lt;amount&gt;[TAB]&lt;notes&gt;</code><br>"
        "Example : <code>2025-12-31[TAB]1250.50[TAB]Dividends</code></p>"

        "<h3>Fields</h3>"
        "<table border=\"1\" cellspacing=\"0\" cellpadding=\"4\">"
        "<tr><th>Field</th><th>Format</th></tr>"
        "<tr><td><b>Date</b></td><td>ISO 8601 (YYYY-MM-DD), e.g. 2025-12-31</td></tr>"
        "<tr><td><b>Amount</b></td><td>No thousands separator, &quot;.&quot; as the decimal "
        "separator, between 0 and the maximum below</td></tr>"
        "<tr><td><b>Notes</b></td><td><i>Optional.</i> Max 100 characters, with no TAB or line "
        "break. Can be in double quotes, with inner quotes doubled, as in the CSV export : "
        "<code>&quot;say &quot;&quot;hi&quot;&quot;&quot;</code></td></tr>"
        "</table>"

        "<h3>Maximum amount</h3>"
        "<table border=\"1\" cellspacing=\"0\" cellpadding=\"4\">"
        "<tr><th>Currency decimals</th><th>Maximum</th></tr>"
        "<tr><td>0 (e.g. Yen)</td><td>99 999 999 999 999</td></tr>"
        "<tr><td>1</td><td>9 999 999 999 999.9</td></tr>"
        "<tr><td>2</td><td>999 999 999 999.99</td></tr>"
        "<tr><td>3</td><td>99 999 999 999.999</td></tr>"
        "<tr><td>4</td><td>9 999 999 999.9999</td></tr>"
        "</table>"

        "<p>All imported events are marked as active, so they count in the Cash Balance "
        "curve.</p>"));

    theLocale = aLocale;
}

LoadIrregularTextFileDialog::~LoadIrregularTextFileDialog()
{
    delete ui;
}


void LoadIrregularTextFileDialog::slotPrepareContent(CurrencyInfo currencyInfo)
{
    currInfo = currencyInfo;

    ui->fileNameLineEdit->setFocus();
}


void LoadIrregularTextFileDialog::on_cancelPushButton_clicked()
{
    this->hide();
    emit signalImportCompleted();
}


void LoadIrregularTextFileDialog::on_importPushButton_clicked()
{
    QString fileName = ui->fileNameLineEdit->text();

    LOG_INFO( QString("Attempting to import a text file containing values for an irregular "
        "cash stream definition : file name=\"%1\"").arg(REDACT(fileName)));

    // Show an error to the user and log it
    auto importFailed = [](const QString &userErrorMessage, const QString &logErrorMessage) {
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            userErrorMessage, {tr("OK")}, 0, 0);
        LOG_ERROR( QString("Import failed : %1").arg(logErrorMessage) );
    };

    try {
        // read and check the lines
        CsvImportResult result = CsvImporter::importFile(fileName, {
            {tr("Date"), CsvImportColumn::Type::Date},
            {tr("Amount"), CsvImportColumn::Type::Number},
            {tr("Notes"), CsvImportColumn::Type::Text, true}
        });
        if (!result.success) {
            importFailed(result.userErrorMessage, result.logErrorMessage);
            return;
        }

        // check the meaning of the values
        QMap<QDate,IrregularCsd::AmountInfo> data;
        for (const CsvImportRow &row : result.rows) {
            QDate date = row.cells[0].toDate();
            double d = row.cells[1].toDouble();
            const QString &amountString = row.rawCells[1];
            if (d<0) {
                importFailed(tr("Amount \"%1\" is smaller than 0 at line %2.")
                    .arg(amountString).arg(row.lineNo),
                    QString("Amount \"%1\" is smaller than 0 at line %2.")
                    .arg(amountString).arg(row.lineNo));
                return;
            }
            double maxAllowed = CurrencyHelper::maxValueAllowedForAmountInDouble(
                currInfo.noOfDecimal);
            if (d > maxAllowed) {
                QString maxAllowedString = theLocale.toString(maxAllowed, 'f',
                    currInfo.noOfDecimal);
                importFailed(tr("Amount \"%1\" is bigger than the maximum"
                    " allowed of %2 at line %3.").arg(amountString).arg(maxAllowedString)
                    .arg(row.lineNo),
                    QString("Amount \"%1\" is bigger than the maximum"
                    " allowed of %2 at line %3.").arg(amountString).arg(maxAllowedString)
                    .arg(row.lineNo));
                return;
            }
            int res;
            qint64 amountDecimal = CurrencyHelper::amountDoubleToQint64(d,
                currInfo.noOfDecimal,res);
            if (res != 0) {
                importFailed(tr("Amount \"%1\" cannot be processed at line %2.")
                    .arg(amountString).arg(row.lineNo),
                    QString("Amount \"%1\" cannot be processed at line %2.")
                    .arg(amountString).arg(row.lineNo));
                return;
            }
            QString notes = row.cells[2].toString();
            if (notes.length() > IrregularCsd::AmountInfo::NOTES_MAX_LEN) {
                importFailed(tr("Notes length (%1 char.) is longer than the max allowed of %2"
                    " at line %3.").arg(notes.length())
                    .arg(IrregularCsd::AmountInfo::NOTES_MAX_LEN).arg(row.lineNo),
                    QString("Notes length (%1 char.) is longer than the max allowed of %2"
                    " at line %3.").arg(notes.length())
                    .arg(IrregularCsd::AmountInfo::NOTES_MAX_LEN).arg(row.lineNo));
                return;
            }

            // record the data. we know the amount is >= 0
            IrregularCsd::AmountInfo ai = {.amount=static_cast<quint64>(amountDecimal),
                .notes=notes};
            data.insert(date, ai);
        }

        // send back the result and close the window
        GbpQMessage::messageBoxQuestion(nullptr,
            GbpQMessage::Type::INFORMATION, tr("Information"),
            CsvImporter::successMessage(data.size(), result), {tr("OK")}, 0, 0);
        emit signalImportResult(data);
        this->hide();
        LOG_INFO( QString("Import succeeded") );

    } catch (const std::exception& e) {
        importFailed(tr("An unexpected error has occurred.<br><br>Details : %1").arg(e.what()),
            QString("An unexpected error has occurred.\n\nDetails : %1").arg(e.what()));
    }
}


// Choose the file to import
void LoadIrregularTextFileDialog::on_browsePushButton_clicked()
{
    QString errorString;

    // Use "import" last directory
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select a Unicode text file"),
        GbpController::getInstance().getLastDirImport());
    if (fileName != ""){
        QFile file(fileName);
        if (!file.exists()){
            errorString = tr("File %1 does not exist").arg(fileName);
            GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
                errorString, {tr("OK")}, 0, 0);
            return;
        } else {
            ui->fileNameLineEdit->setText(file.fileName());
            // remember the directory
            QString d = QFileInfo(file).absolutePath();
            GbpController::getInstance().setLastDirImport(d);
        }
    }
}


void LoadIrregularTextFileDialog::on_LoadIrregularTextFileDialog_rejected()
{
    on_cancelPushButton_clicked();
}


void LoadIrregularTextFileDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("LoadIrregularTextFileDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
    });
}

