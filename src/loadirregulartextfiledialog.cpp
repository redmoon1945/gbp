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

#include <QFileDialog>
#include <QDir>
#include <QRegularExpression>
#include <QMessageBox>
#include <QDate>
#include <QCoreApplication>
#include "loadirregulartextfiledialog.h"
#include "gbpcontroller.h"
#include "ui_loadirregulartextfiledialog.h"
#include "currencyhelper.h"
#include "gbplogger.h"


LoadIrregularTextFileDialog::LoadIrregularTextFileDialog(QLocale aLocale, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoadIrregularTextFileDialog)
{
    ui->setupUi(this);

    theLocale = aLocale;
}

LoadIrregularTextFileDialog::~LoadIrregularTextFileDialog()
{
    delete ui;
}


void LoadIrregularTextFileDialog::slotPrepareContent(CurrencyInfo currencyInfo)
{
    currInfo = currencyInfo;
}


void LoadIrregularTextFileDialog::on_cancelPushButton_clicked()
{
    this->hide();
    emit signalImportCompleted();
}


void LoadIrregularTextFileDialog::on_importPushButton_clicked()
{
    QString userErrorMessage, logErrorMessage, line, lineTrimmed;
    QStringList tokens;
    QString fileName = ui->fileNameLineEdit->text();
    int lineNo = 0;

    LOG_INFO( QString("Attempting to import a text file containing values for an irregular "
        "cash stream definition : file name=\"%1\"").arg(REDACT(fileName)));

    // open the file
    QFile file(fileName);   // file is closed automatically by Qt
    if (!file.exists()){
        userErrorMessage = QString(tr("File %1 does not exist")).arg(fileName);
        logErrorMessage = QString("File %1 does not exist").arg(fileName);
        QMessageBox::critical(nullptr,tr("Error"), userErrorMessage);
        LOG_ERROR( QString("Import failed : %1").arg(logErrorMessage) );
        return;
    }
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)){

        try {
            // read and parse lines
            QTextStream stream(&file);
            stream.setAutoDetectUnicode(true);
            QMap<QDate,IrregularCsd::AmountInfo> data;
            while (!stream.atEnd()){
                line = stream.readLine();
                lineNo++;
                // process line
                if ( !line.isNull() ){

                    lineTrimmed = line.trimmed();
                    tokens = lineTrimmed.split('\t');
                    if ( (tokens.length() != 3) && (tokens.length() != 2) ) {
                        userErrorMessage = tr("Bad format for line no %1 (no of tokens "
                            "is not 3 or 2, but %2).").arg(lineNo).arg(tokens.size());
                        logErrorMessage = QString("Bad format for line no %1 (no of tokens is "
                            "not 3 or 2, but %2).").arg(lineNo).arg(tokens.size());
                        QMessageBox::critical(nullptr,tr("Error"), userErrorMessage);
                        LOG_ERROR( QString("Import failed : %1").arg(logErrorMessage) );
                        return;
                    }
                    // convert the date
                    QDate date = QDate::fromString(tokens[0],Qt::ISODate);
                    if (!date.isValid()) {
                        userErrorMessage = tr("Date \"%1\" is invalid at line no %2.")
                            .arg(tokens[0]).arg(lineNo);
                        logErrorMessage = QString("Date \"%1\" is invalid at line no %2.")
                            .arg(tokens[0]).arg(lineNo);
                        QMessageBox::critical(nullptr,tr("Error"), userErrorMessage);
                        LOG_ERROR( QString("Import failed : %1").arg(logErrorMessage) );
                        return;
                    }
                    // convert the amount
                    bool ok;
                    double d = tokens[1].toDouble(&ok);
                    if ( ok==false ){
                        userErrorMessage = tr("Amount \"%1\" is not a valid number at line no %2.")
                            .arg(tokens[1]).arg(lineNo);
                        logErrorMessage = QString("Amount \"%1\" is not a valid number "
                            "at line no %2.").arg(tokens[1]).arg(lineNo);
                        QMessageBox::critical(nullptr,tr("Error"), userErrorMessage);
                        LOG_ERROR( QString("Import failed : %1").arg(logErrorMessage) );
                        return;
                    }
                    if (d<0) {
                        userErrorMessage = tr("Amount \"%1\" is smaller than 0 at line %2.")
                            .arg(tokens[1]).arg(lineNo);
                        logErrorMessage = QString("Amount \"%1\" is smaller than 0 at line %2.")
                            .arg(tokens[1]).arg(lineNo);
                        QMessageBox::critical(nullptr,tr("Error"), userErrorMessage);
                        LOG_ERROR( QString("Import failed : %1").arg(logErrorMessage) );
                        return;
                    }
                    if ( d > CurrencyHelper::maxValueAllowedForAmountInDouble(
                            currInfo.noOfDecimal) ) {
                        double maxAllowed =  CurrencyHelper::maxValueAllowedForAmountInDouble(
                            currInfo.noOfDecimal);
                        QString maxAllowedString = QString::number(maxAllowed, 'f',
                            currInfo.noOfDecimal);
                        userErrorMessage = tr("Amount \"%1\" is bigger than the maximum allowed of %2"
                            " at line %3.").arg(tokens[1]).arg(maxAllowedString).arg(lineNo);
                        logErrorMessage = QString("Amount \"%1\" is bigger than the maximum"
                            " allowed of %2 at line %3.").arg(tokens[1]).arg(maxAllowedString)
                            .arg(lineNo);
                        QMessageBox::critical(nullptr,tr("Error"), userErrorMessage);
                        LOG_ERROR( QString("Import failed : %1").arg(logErrorMessage) );
                        return;
                    }
                    int res;
                    qint64 amountDecimal = CurrencyHelper::amountDoubleToQint64(d,
                        currInfo.noOfDecimal,res);
                    if (res != 0) {
                        userErrorMessage = tr("Amount \"%1\" cannot be processed at line %2.")
                            .arg(tokens[1]).arg(lineNo);
                        logErrorMessage = QString("Amount \"%1\" cannot be processed at line %2.")
                            .arg(tokens[1]).arg(lineNo);
                        QMessageBox::critical(nullptr,tr("Error"), userErrorMessage);
                        LOG_ERROR( QString("Import failed : %1").arg(logErrorMessage) );
                        return;
                    }
                    // get the notes and check length. Only if there is a third token
                    QString notes;
                    if (tokens.size()==3) {
                        notes = tokens[2];
                        if (notes.length() > IrregularCsd::AmountInfo::NOTES_MAX_LEN) {
                            userErrorMessage = tr("Notes length (%1 char.) is longer than the max"
                                " allowed of %2 at line %3.").arg(notes.length()).arg(
                                IrregularCsd::AmountInfo::NOTES_MAX_LEN).arg(lineNo);
                            logErrorMessage = QString("Notes length (%1 char.) is longer than"
                                " the max allowed of %2 at line %3.").arg(notes.length())
                                .arg(IrregularCsd::AmountInfo::NOTES_MAX_LEN).arg(lineNo);
                            QMessageBox::critical(nullptr,tr("Error"), userErrorMessage);
                            LOG_ERROR( QString("Import failed : %1").arg(logErrorMessage) );
                            return;
                        }
                    }

                    // record the data. we know the amount is >= 0
                    IrregularCsd::AmountInfo ai = {.amount=static_cast<quint64>(amountDecimal),
                        .notes=notes};
                    data.insert(date, ai);

                }
            }

            // send back the result and close the window
            QMessageBox::information(nullptr,tr("Information"),QString(
                tr("Import succeeded, %1 entries have been imported.")).arg(data.size()));
            emit signalImportResult(data);
            this->hide();
            LOG_INFO( QString("Import succeeded") );

        } catch (const std::exception& e) {
            userErrorMessage = tr("An unexpected error has occurred.\n\nDetails : %1").arg(e.what());
            logErrorMessage = QString("An unexpected error has occurred.\n\nDetails : %1")
                .arg(e.what());
            QMessageBox::critical(nullptr,tr("Error"), userErrorMessage);
            LOG_ERROR( QString("Import failed : %1").arg(logErrorMessage) );
            return;
        }

    } else {
        userErrorMessage = tr("Cannot open file %1 in read-only mode").arg(fileName);
        logErrorMessage = QString("Cannot open file %1 in read-only mode")
            .arg(REDACT(fileName));
        QMessageBox::critical(nullptr,tr("Error"), userErrorMessage);
        LOG_ERROR(  QString("Import failed : %1").arg(logErrorMessage) );
        return ;
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
            QMessageBox::critical(nullptr,tr("Error"), errorString);
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
