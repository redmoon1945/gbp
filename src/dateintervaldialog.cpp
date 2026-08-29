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

#include "dateintervaldialog.h"
#include "gbplogger.h"
#include <QTimer>
#include "ui_dateintervaldialog.h"
#include "gbpcontroller.h"
#include "uiutil.h"
#include <QMessageBox>
#include <QDate>
#include "gbpqmessage.h"



DateIntervalDialog::DateIntervalDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DateIntervalDialog)
{
    ui->setupUi(this);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);

    QDate from = GbpController::getInstance().getTomorrow();
    QDate to = from.addYears(1).addDays(-1);
    ui->fromDateEdit->setDate(from);
    ui->toDateEdit->setDate(to);

    // Make buttons' font smaller
    QFont appFont = QApplication::font();
    QFont font = appFont;
    Util::changeFontSize(font, Util::FontResizeIntensity::AVERAGE, true,
        "Date interval dialog - buttons");
    ui->setEoyPushButton->setFont(font);
    ui->setTomorrowPushButton->setFont(font);
}


DateIntervalDialog::~DateIntervalDialog()
{
    delete ui;
}


void DateIntervalDialog::slotPrepareContent(QDate from, QDate to)
{
    if (to<=from) {
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            tr("\"To\" date must occur after \"From\" date"), {tr("OK")}, 0, 0);
        return;
    }
    if( (from.isValid()==false) || (to.isValid()==false) ){
        return;
    }

    // "from"/"to" define the allowed [min,max] range for both date edits, not the values to
    // display. The previously selected values are preserved across invocations of this dialog ;
    // QDateEdit automatically clamps its current value into the new range if it no longer fits,
    // so no additional clamping logic is needed here.
    ui->fromDateEdit->setDateRange(from, to);
    ui->toDateEdit->setDateRange(from, to);

    ui->fromDateEdit->setFocus();
}


void DateIntervalDialog::on_applyPushButton_clicked()
{
    QDate from = ui->fromDateEdit->date();
    QDate to = ui->toDateEdit->date();
    if (to<=from) {
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            tr("\"To\" date must occur after \"From\" date"), {tr("OK")}, 0, 0);
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


void DateIntervalDialog::on_setTomorrowPushButton_clicked()
{
    QDate from = GbpController::getInstance().getTomorrow();
    ui->fromDateEdit->setDate(from);
}


void DateIntervalDialog::on_setEoyPushButton_clicked()
{
    QDate to = ui->toDateEdit->date();
    ui->toDateEdit->setDate(QDate(to.year(),12,31));
}


void DateIntervalDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("DateIntervalDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
    });
}

