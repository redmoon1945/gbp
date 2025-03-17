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

#include "dateintervaldialog.h"
#include "ui_dateintervaldialog.h"
#include "gbpcontroller.h"
#include <QMessageBox>
#include <QDate>



DateIntervalDialog::DateIntervalDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DateIntervalDialog)
{
    ui->setupUi(this);
    QDate from = GbpController::getInstance().getTomorrow();
    QDate to = from.addYears(1).addDays(-1);
    ui->fromDateEdit->setDate(from);
    ui->toDateEdit->setDate(to);
}


DateIntervalDialog::~DateIntervalDialog()
{
    delete ui;
}


void DateIntervalDialog::slotPrepareContent(QDate from, QDate to)
{
    if (to<=from) {
        QMessageBox::critical(nullptr,tr("Error"),
            tr("\"To\" date must occur after \"From\" date"));
        return;
    }
    if( (from.isValid()==false) || (to.isValid()==false) ){
        return;
    }
    ui->fromDateEdit->setDate(from);
    ui->toDateEdit->setDate(to);
}


void DateIntervalDialog::on_applyPushButton_clicked()
{
    QDate from = ui->fromDateEdit->date();
    QDate to = ui->toDateEdit->date();
    if (to<=from) {
        QMessageBox::critical(nullptr,tr("Error"),
            tr("\"To\" date must occur after \"From\" date"));
        return;
    }
    emit signalDateIntervalResult(from, to);
    hide();

}


void DateIntervalDialog::on_cancelPushButton_clicked()
{
    hide();
    emit signalDateIntervalCompleted();
}


void DateIntervalDialog::on_DateIntervalDialog_rejected()
{
    on_cancelPushButton_clicked();
}

