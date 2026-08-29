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

#include "editirregularelementdialog.h"
#include "gbplogger.h"
#include <QTimer>
#include "ui_editirregularelementdialog.h"
#include <QMessageBox>
#include <QCoreApplication>
#include "irregularcsd.h"
#include "gbpcontroller.h"
#include "uiutil.h"
#include "gbpqmessage.h"


EditIrregularElementDialog::EditIrregularElementDialog(const QLocale &aLocale, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EditIrregularElementDialog)
{
    ui->setupUi(this);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);

    this->locale = aLocale;
    ui->notesLineEdit->setMaxLength(IrregularCsd::AmountInfo::NOTES_MAX_LEN);

    // widen Date Widget
    UiUtil::widenDateEdit(ui->dateEdit);

    QFont appFont = QApplication::font();

    // Make note font smaller
    QFont noteFont = appFont;
    Util::changeFontSize(noteFont, Util::FontResizeIntensity::AVERAGE, true,
        "EditIrregularElementDialog - note");
    ui->notesLineEdit->setFont(noteFont);
}


EditIrregularElementDialog::~EditIrregularElementDialog()
{
    delete ui;
}


// For "Create", currentDate is not used
void EditIrregularElementDialog::slotPrepareContent(bool isIncome, bool newEditMode,
    const CurrencyInfo &cInfo, const QList<QDate> &newExistingDates, QDate currentDate,
    double amount, const QString &notes)
{

    editMode = newEditMode;
    existingDates = newExistingDates;
    currInfo = cInfo;
    latestOldDate = currentDate;    // date may or may not change during an edit, so remember the "old" one
    ui->currencyLabel->setText(currInfo.isoCode);
    ui->amountDoubleSpinBox->setDecimals(currInfo.noOfDecimal);
    ui->amountDoubleSpinBox->setMinimum(0);
    ui->amountDoubleSpinBox->setMaximum(CurrencyHelper::maxValueAllowedForAmountInDouble(
        currInfo.noOfDecimal));

    if (editMode) {
        // *** existing ***
        QString title = QString(tr("Editing an irregular item (%1)"))
            .arg((isIncome)?(tr("Income")):(tr("Expense")));
        this->setWindowTitle(title);
        ui->applyPushButton->setText(tr("Apply changes"));
        ui->closePushButton->setText(tr("Cancel"));
        ui->dateEdit->setDate(currentDate);
        ui->amountDoubleSpinBox->setValue(amount);
        ui->notesLineEdit->setText(notes);
    } else {
        // *** new ***
        QString title = QString(tr("Creating an irregular item (%1)"))
            .arg((isIncome)?(tr("Income")):(tr("Expense")));
        this->setWindowTitle(title);
        ui->applyPushButton->setText(tr("Create"));
        ui->closePushButton->setText(tr("Close"));
        ui->amountDoubleSpinBox->setValue(0);
        ui->notesLineEdit->setText("");
        QDate newDate;
        if (existingDates.size() != 0){
            // as default value, offer the next month following the last in the current list
            newDate = existingDates.last().addMonths(1);
            ui->dateEdit->setDate(newDate);
        } else{
            // offer TOMORROW as default value
            ui->dateEdit->setDate(GbpController::getInstance().getTomorrow());
        }
    }

    ui->dateEdit->setFocus();
}


void EditIrregularElementDialog::on_applyPushButton_clicked()
{
    QDate newDate = ui->dateEdit->date();
    double amount = ui->amountDoubleSpinBox->value();

    // validate the new date
    if ( !newDate.isValid() ){
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            tr("Date entered is invalid"), {tr("OK")}, 0, 0);
        return;
    }
    // validate amount
    if (amount<0){
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            tr("Amount cannot be smaller than 0"), {tr("OK")}, 0, 0);
        return;
    }
    if (amount>CurrencyHelper::maxValueAllowedForAmountInDouble(currInfo.noOfDecimal) ) {
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            QString(tr("Amount is bigger than the "
            "maximum allowed of %1"))
            .arg(CurrencyHelper::maxValueAllowedForAmountInDouble(currInfo.noOfDecimal)), {tr("OK")}, 0, 0);
        return;
    }

    if (!editMode) {
        // *** CREATION ***
        // date must not exist for Element Creation
        if(existingDates.contains(newDate)){
            QString errorString = tr("This date has already an amount defined");
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
                errorString, {tr("OK")}, 0, 0);
            return;
        } else{
            // add to the list of existing dates
            existingDates.append(newDate);
        }
    } else {
        // *** EDITION ***
        // replace old date if a change occurred for the date and
        // without warning...
        if (latestOldDate != newDate) {
            int index = existingDates.indexOf(latestOldDate); // will always succeed
            if (index == -1){
                return; // should never happen
            }
            existingDates.removeAt(index);
            // add to the list of existing dates
            existingDates.append(newDate);
        }
    }

    // send result back to caller. For "Create", latestOldDate can be anything because
    // it will not be used by caller.
    emit signalEditElementResult(editMode, latestOldDate, newDate, amount,
        ui->notesLineEdit->text());
    if (editMode){
        hide();
        emit signalEditElementCompleted();
    } else {
        // increase datedisplayed by 1 month, erase amount and continue creating items
        ui->dateEdit->setDate(newDate.addMonths(1));
        ui->amountDoubleSpinBox->setValue(0);
    }

}


void EditIrregularElementDialog::on_closePushButton_clicked()
{
    hide();
    emit signalEditElementCompleted();
}


void EditIrregularElementDialog::on_EditIrregularElementDialog_rejected()
{
    on_closePushButton_clicked();
}


void EditIrregularElementDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    // QTimer::singleShot(0) defers execution until after all show-related events have been
    // processed, including the platform style's focusInEvent which calls selectAll() on the
    // focused QLineEdit. Calling deselect() directly in slotPrepareContent() has no effect
    // because that slot runs before the dialog is shown.
    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("EditIrregularElementDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
        ui->notesLineEdit->deselect();
    });
}

