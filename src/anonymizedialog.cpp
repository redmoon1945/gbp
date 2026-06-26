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

#include "anonymizedialog.h"
#include "ui_anonymizedialog.h"
#include "util.h"
#include "uiutil.h"
#include "gbplogger.h"
#include <QShowEvent>
#include <QTimer>


AnonymizeDialog::AnonymizeDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AnonymizeDialog)
{
    ui->setupUi(this);

    QFont appFont = QApplication::font();
    QFont font = appFont;
    Util::changeFontSize(font, Util::FontResizeIntensity::AVERAGE, true,
        "AnonymizeDialog - instructions");
    ui->infoLabel->setFont(font);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);
}


AnonymizeDialog::~AnonymizeDialog()
{
    delete ui;
}


void AnonymizeDialog::slotPrepareContent(){
    ui->anonymizePushButton->setFocus();
}


void AnonymizeDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    int w = width();
    adjustSize();
    resize(w, qMin(height(), MAX_DIALOG_HEIGHT));
    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("AnonymizeDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
    });
}


void AnonymizeDialog::on_anonymizePushButton_clicked()
{
    AnonymizeOptions result =   {
        .anonymizeAmounts = ui->anonymizeAmountsRadioButton->isChecked(),
        .intensity = ui->intensitySlider->value()
    };
    emit signalResult(result);
    hide();
}


void AnonymizeDialog::on_cancelPushButton_clicked()
{
    emit signalCompleted();
    hide();
}


void AnonymizeDialog::on_anonymizeAmountsRadioButton_toggled(bool checked)
{
    ui->intensityLabel->setEnabled(checked);
    ui->intensitySlider->setEnabled(checked);
    ui->intensityValueLabel->setEnabled(checked);
}


void AnonymizeDialog::on_intensitySlider_valueChanged(int value)
{
    ui->intensityValueLabel->setText(QString("%1%").arg(value));
}

void AnonymizeDialog::on_AnonymizeDialog_rejected()
{
    on_cancelPushButton_clicked();
}

