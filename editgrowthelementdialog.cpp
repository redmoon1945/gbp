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

#include "editgrowthelementdialog.h"
#include "ui_editgrowthelementdialog.h"
#include "growth.h"
#include "gbpcontroller.h"

#include <QMessageBox>
#include <QCoreApplication>

EditGrowthElementDialog::EditGrowthElementDialog(QString newGrowthName, QLocale aLocale, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditGrowthElementDialog)
{
    growthName = newGrowthName;
    locale = aLocale;
    ui->setupUi(this);

    // reset programmatically min, max and precision for growth value. We stay with Monthly value
    ui->growthDoubleSpinBox->setMinimum(Growth::MIN_GROWTH_DOUBLE);
    ui->growthDoubleSpinBox->setMaximum(Growth::MAX_GROWTH_DOUBLE);
    ui->growthDoubleSpinBox->setDecimals(Growth::NO_OF_DECIMALS);

    // fill combobox for month names
    fillMonthComboBox();

    // set some text labels
    ui->growthLabel->setText(QString(tr("%1 on annual basis :")).arg(growthName));
    ui->monthlyLabel->setText(QString(tr("Monthly %1 equivalent :")).arg(growthName));
}

EditGrowthElementDialog::~EditGrowthElementDialog()
{
    delete ui;
}


// growthInPercentage is monthly growth in percentage, but expressed on an annual basis
void EditGrowthElementDialog::slotPrepareContent(bool newEditMode, QList<QDate> newExistingDates, QDate currentDate, double growthInPercentage)
{
    editMode = newEditMode;
    existingDates = newExistingDates;

    latestOldDate = currentDate;      // date may or may not change during an edit, so remember the "old" one

    QDate theDate = QDate(currentDate.year(), currentDate.month(),1); // reset day to 1, to be sure

    if(editMode){
        // *** existing ***
        this->setWindowTitle(QString(tr("Edit A Monthly %1 Value")).arg(growthName));
        ui->applyPushButton->setText(QString(tr("Apply Changes")));
        ui->closePushButton->setText(QString(tr("Cancel")));
        ui->monthComboBox->setCurrentIndex(theDate.month()-1);
        ui->yearSpinBox->setValue(theDate.year());
        ui->growthDoubleSpinBox->setValue(growthInPercentage);
        // calculate the monthly basis equivalent
        double monthlyGrowth = Util::annualToMonthlyGrowth(growthInPercentage);
        QString s = locale.toString(monthlyGrowth,'g',6);
        ui->monthlyLabel->setText(s);
    } else{
        // *** new ***
        this->setWindowTitle(QString(tr("Add a New Monthly %1 Value")).arg(growthName));
        ui->applyPushButton->setText(QString(tr("Create")));
        ui->closePushButton->setText(QString(tr("Close")));
        QDate newDate;
        if (existingDates.size() != 0){
            // as default value, offer the next month following the last in the current list
            newDate = existingDates.last().addMonths(1);
            ui->yearSpinBox->setValue(newDate.year());
            ui->monthComboBox->setCurrentIndex(newDate.month()-1);
        } else{
            // offer month/year of TOMORROW as default value
            ui->monthComboBox->setCurrentIndex(GbpController::getInstance().getTomorrow().month()-1);
            ui->yearSpinBox->setValue(GbpController::getInstance().getTomorrow().year());
        }
        ui->growthDoubleSpinBox->setValue(0);
        ui->monthlyLabel->setText("0");
    }
}



void EditGrowthElementDialog::on_closePushButton_clicked()
{
    hide();
    emit signalEditElementCompleted();
}


void EditGrowthElementDialog::on_applyPushButton_clicked(){

    QDate newDate = QDate(ui->yearSpinBox->value(),ui->monthComboBox->currentIndex()+1,1);
    double growthValueAnnualBasis = ui->growthDoubleSpinBox->value();

    // validate date
    if ( !newDate.isValid() ){
        QMessageBox::critical(this,tr("Invalid Date"),QString(tr("Date entered is invalid")));
        return;
    }
    // validate growth
    if (growthValueAnnualBasis<Growth::MIN_GROWTH_DOUBLE){
        QMessageBox::critical(this,tr("Invalid Value"),QString(tr("%1 value is smaller than the minimum allowed of %2")).arg(growthName).arg(Growth::MIN_GROWTH_DOUBLE));
        return;
    }
    if (growthValueAnnualBasis>Growth::MAX_GROWTH_DOUBLE){
        QMessageBox::critical(this,tr("Invalid Value"),QString(tr("%1 value is bigger than the maximum allowed of %2")).arg(growthName).arg(Growth::MAX_GROWTH_DOUBLE));
        return;
    }
    // date must not exist for Element Creation
    if(!editMode){
        // *** CREATION ***
        if(existingDates.contains(newDate)){
            QString errorString = QString(tr("A %1 value is already defined for that date")).arg(growthName);
            QMessageBox::critical(this,tr("Invalid Date"),errorString);
            return;
        } else {
            // add to the list of existing years
            existingDates.append(newDate);
        }
    } else {
        // *** EDITION ***
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

    emit signalEditElementResult(editMode, latestOldDate, newDate, growthValueAnnualBasis);
    if (editMode){
        hide();
        emit signalEditElementCompleted();
    } else {
        // increase datedisplayed by 1 month and continue creating items
        QDate aNewDate = newDate.addMonths(1);
        ui->monthComboBox->setCurrentIndex(aNewDate.month()-1);
        ui->yearSpinBox->setValue(aNewDate.year());
        ui->growthDoubleSpinBox->setValue(0);
        ui->monthlyLabel->setText("0");
    }
}


// Dialog manually closed by user
void EditGrowthElementDialog::on_EditGrowthElementDialog_rejected()
{
    on_closePushButton_clicked();
}


void EditGrowthElementDialog::fillMonthComboBox()
{
    for(int i=1;i<=12;i++){
        ui->monthComboBox->addItem(locale.monthName(i),i);
    }
}



void EditGrowthElementDialog::on_growthDoubleSpinBox_valueChanged(double arg1)
{
    // update the monthly basis equivalent
    double annualGrowth = ui->growthDoubleSpinBox->value();
    double monthlyGrowth = Util::annualToMonthlyGrowth(annualGrowth);
    QString s = QString("%1 %2").arg(locale.toString(monthlyGrowth,'g',6)).arg("%");
    ui->monthlyLabel->setText(s);
}

