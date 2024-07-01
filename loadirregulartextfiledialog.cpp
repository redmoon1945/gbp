/*
 *  Copyright (C) 2024 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
 *  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QFileDialog>
#include <QDir>
#include <QRegularExpression>
#include <QMessageBox>
#include <QDate>
#include <QCoreApplication>
#include "loadirregulartextfiledialog.h"
#include "ui_loadirregulartextfiledialog.h"
#include "currencyhelper.h"
#include "gbpcontroller.h"


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
    QString errorStringUI, errorStringLog, line, lineTrimmed;
    QStringList tokens;
    QString fileName = ui->fileNameLineEdit->text();
    int lineNo = 0;

    GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Info, QString("Attempting to import a text file containing values for an Irregular income/expense : file name=\"%1\"").arg(fileName));

    // open the file
    QFile file(fileName);   // file is closed automatically by Qt
    if (!file.exists()){
        errorStringUI = QString(tr("File %1 does not exist")).arg(fileName);
        errorStringLog = QString("File %1 does not exist").arg(fileName);
        QMessageBox::critical(nullptr,tr("Import failed"), errorStringUI);
        GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Error, QString("Import failed : %1").arg(errorStringLog));
        return;
    }
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)){

        try {
            // read and parse lines
            QTextStream stream(&file);
            stream.setAutoDetectUnicode(true);
            QMap<QDate,IrregularFeStreamDef::AmountInfo> data;
            while (!stream.atEnd()){
                line = stream.readLine();
                lineNo++;
                // process line
                if ( !line.isNull() ){

                    lineTrimmed = line.trimmed();
                    tokens = lineTrimmed.split('\t');
                    if ( (tokens.length() != 3) && (tokens.length() != 2) ) {
                        errorStringUI = tr("Bad format for line no %1 (no of tokens is not 3 or 2, but %2).").arg(lineNo).arg(tokens.size());
                        errorStringLog = QString("Bad format for line no %1 (no of tokens is not 3 or 2, but %2).").arg(lineNo).arg(tokens.size());
                        QMessageBox::critical(nullptr,tr("Import failed"), errorStringUI);
                        GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Error, QString("Import failed : %1").arg(errorStringLog));
                        return;
                    }
                    // convert the date
                    QDate date = QDate::fromString(tokens[0],Qt::ISODate);
                    if (!date.isValid()) {
                        errorStringUI = tr("Date \"%1\" is invalid at line no %2.").arg(tokens[0]).arg(lineNo);
                        errorStringLog = QString("Date \"%1\" is invalid at line no %2.").arg(tokens[0]).arg(lineNo);
                        QMessageBox::critical(nullptr,tr("Import failed"), errorStringUI);
                        GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Error, QString("Import failed : %1").arg(errorStringLog));
                        return;
                    }
                    // convert the amount
                    bool ok;
                    double d = tokens[1].toDouble(&ok);
                    if ( ok==false ){
                        errorStringUI = tr("Amount \"%1\" is not a valid number at line no %2.").arg(tokens[1]).arg(lineNo);
                        errorStringLog = QString("Amount \"%1\" is not a valid number at line no %2.").arg(tokens[1]).arg(lineNo);
                        QMessageBox::critical(nullptr,tr("Import failed"), errorStringUI);
                        GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Error, QString("Import failed : %1").arg(errorStringLog));
                        return;
                    }
                    if (d<0) {
                        errorStringUI = tr("Amount \"%1\" is smaller than 0 at line %2.").arg(tokens[1]).arg(lineNo);
                        errorStringLog = QString("Amount \"%1\" is smaller than 0 at line %2.").arg(tokens[1]).arg(lineNo);
                        QMessageBox::critical(nullptr,tr("Import failed"), errorStringUI);
                        GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Error, QString("Import failed : %1").arg(errorStringLog));
                        return;
                    }
                    if ( d > CurrencyHelper::maxValueAllowedForAmountInDouble(currInfo.noOfDecimal) ) {
                        double maxAllowed =  CurrencyHelper::maxValueAllowedForAmountInDouble(currInfo.noOfDecimal);
                        QString maxAllowedString = QString::number(maxAllowed, 'f', currInfo.noOfDecimal);
                        errorStringUI = tr("Amount \"%1\" is bigger than the maximum allowed of %2 at line %3.").arg(tokens[1]).arg(maxAllowedString).arg(lineNo);
                        errorStringLog = QString("Amount \"%1\" is bigger than the maximum allowed of %2 at line %3.").arg(tokens[1]).arg(maxAllowedString).arg(lineNo);
                        QMessageBox::critical(nullptr,tr("Import failed"), errorStringUI);
                        GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Error, QString("Import failed : %1").arg(errorStringLog));
                        return;
                    }
                    int res;
                    qint64 amountDecimal = CurrencyHelper::amountDoubleToQint64(d,currInfo.noOfDecimal,res);
                    if (res != 0) {
                        errorStringUI = tr("Amount \"%1\" cannot be processed at line %2.").arg(tokens[1]).arg(lineNo);
                        errorStringLog = QString("Amount \"%1\" cannot be processed at line %2.").arg(tokens[1]).arg(lineNo);
                        QMessageBox::critical(nullptr,tr("Import failed"), errorStringUI);
                        GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Error, QString("Import failed : %1").arg(errorStringLog));
                        return;
                    }
                    // get the notes and check length. Only if there is a third token
                    QString notes;
                    if (tokens.size()==3) {
                        notes = tokens[2];
                        if (notes.length() > IrregularFeStreamDef::AmountInfo::NOTES_MAX_LEN) {
                            errorStringUI = tr("Notes length (%1 char.) is longer than the max allowed of %2 at line %3.").arg(notes.length()).arg(IrregularFeStreamDef::AmountInfo::NOTES_MAX_LEN).arg(lineNo);
                            errorStringLog = QString("Notes length (%1 char.) is longer than the max allowed of %2 at line %3.").arg(notes.length()).arg(IrregularFeStreamDef::AmountInfo::NOTES_MAX_LEN).arg(lineNo);
                            QMessageBox::critical(nullptr,tr("Import failed"), errorStringUI);
                            GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Error, QString("Import failed : %1").arg(errorStringLog));
                            return;
                        }
                    }

                    // record the data
                    IrregularFeStreamDef::AmountInfo ai = {.amount=amountDecimal, .notes=notes};
                    data.insert(date, ai);

                }
            }

            // send back the result and close the window
            QMessageBox::information(nullptr,tr("Import succeeded"),QString(tr("%1 entry(ies) have been imported.")).arg(data.size()));
            emit signalImportResult(data);
            this->hide();
            GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Info, QString("Import succeededfailed : %1"));

        } catch (...) {
            std::exception_ptr p = std::current_exception();
            errorStringUI = tr("An unexpected error has occured.\n\nDetails : %1").arg((p ? p.__cxa_exception_type()->name() : "null"));
            errorStringLog = QString("An unexpected error has occured.\n\nDetails : %1").arg((p ? p.__cxa_exception_type()->name() : "null"));
            QMessageBox::critical(nullptr,tr("Import failed"), errorStringUI);
            GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Error, QString("Import failed : %1").arg(errorStringLog));
            return;
        }

    } else {
        errorStringUI = tr("Cannot open file %1 in read-only mode").arg(fileName);
        errorStringLog = QString("Cannot open file %1 in read-only mode").arg(fileName);
        QMessageBox::critical(nullptr,tr("Import failed"), errorStringUI);
        GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Error, QString("Import failed : %1").arg(errorStringLog));
        return ;
    }

}


void LoadIrregularTextFileDialog::on_browsePushButton_clicked()
{
    QString errorString;

    QString fileName = QFileDialog::getOpenFileName(this, tr("Select a Unicode text file"),QDir::homePath());
    if (fileName != ""){
        QFile file(fileName);
        if (!file.exists()){
            errorString = tr("File %1 does not exist").arg(fileName);
            QMessageBox::critical(nullptr,tr("Import failed"), errorString);
            return;
        } else {
            ui->fileNameLineEdit->setText(file.fileName());
        }
    }
}


void LoadIrregularTextFileDialog::on_LoadIrregularTextFileDialog_rejected()
{
    on_cancelPushButton_clicked();
}
