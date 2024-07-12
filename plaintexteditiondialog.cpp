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

#include "plaintexteditiondialog.h"
#include "ui_plaintexteditiondialog.h"

PlainTextEditionDialog::PlainTextEditionDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PlainTextEditionDialog)
{
    ui->setupUi(this);
    readOnly = false;
}

PlainTextEditionDialog::~PlainTextEditionDialog()
{
    delete ui;
}


void PlainTextEditionDialog::slotPrepareContent(QString title, QString content, bool isItReadOnly)
{
    readOnly = isItReadOnly;
    ui->plainTextEdit->setPlainText(content);
    this->setWindowTitle(title);
    if (readOnly){
        ui->plainTextEdit->setReadOnly(true);
        ui->applyChangesPushButton->setVisible(false);
        ui->cancelPushButton->setText("Close");
    } else{
        ui->plainTextEdit->setReadOnly(false);
        ui->applyChangesPushButton->setVisible(true);
        ui->cancelPushButton->setText("Cancel");
    }

}


void PlainTextEditionDialog::on_applyChangesPushButton_clicked()
{
    QString content = ui->plainTextEdit->toPlainText();
    emit signalPlainTextEditionResult(content);
    this->hide();
    ui->plainTextEdit->setPlainText(""); // dont hold the text, no use for that now
    emit signalPlainTextEditionCompleted();
}


void PlainTextEditionDialog::on_cancelPushButton_clicked()
{
    this->hide();
    ui->plainTextEdit->setPlainText(""); // dont hold the text, no use for that now
    emit signalPlainTextEditionCompleted();
}


void PlainTextEditionDialog::on_PlainTextEditionDialog_rejected()
{
        on_cancelPushButton_clicked();
}

