#include "editamountinfodialog.h"
#include "ui_editamountinfodialog.h"
#include "currencyhelper.h"
#include <QMessageBox>

EditAmountInfoDialog::EditAmountInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditAmountInfoDialog)
{
    ui->setupUi(this);
}

EditAmountInfoDialog::~EditAmountInfoDialog()
{
    delete ui;
}


void EditAmountInfoDialog::slotPrepareContent(bool newEditMode, CurrencyInfo newCurrInfo, QList<QDate> newExistingDates, QDate dateToEdit, qint64 amountToEdit)
{
    editMode = newEditMode;
    existingDates = newExistingDates;
    currInfo = newCurrInfo;

    ui->amountDoubleSpinBox->setDecimals(currInfo.noOfDecimal);
    ui->warningLabel->setText("");
    ui->dateLabel->setText("Date : ");
    ui->dateDateEdit->setMinimumDate(QDate(1800,1,1));
    ui->dateDateEdit->setMaximumDate(QDate(3000,12,31));

    if(editMode){
        // we are editing an existing element
        this->setWindowTitle(QString("Edit Existing Date-Amount"));
        ui->applyPushButton->setText(QString("Apply Changes"));
        ui->closePushButton->setText(QString("Cancel"));
        ui->dateDateEdit->setDate(dateToEdit);
        int result;
        double d = CurrencyHelper::amountQint64ToDouble(amountToEdit,quint8(currInfo.noOfDecimal), result);
        if (result!=0){
            // amount or currInfo.noOfDecimal is too big. Should not happen. Abort
            return;
        }
        ui->amountDoubleSpinBox->setValue(d);

    } else{
        // we are creating a new element
        this->setWindowTitle(QString("Create New Date-Amount"));
        ui->applyPushButton->setText(QString("Create"));
        ui->closePushButton->setText(QString("Close"));
        QDate newDate;
        if (existingDates.size() != 0){
            // as default value, offer the date following the last in the current list
            newDate = existingDates.last().addDays(1);
        } else{
            // offer tomorrow date as default value
            newDate = QDate::currentDate().addDays(1);
        }
        ui->dateDateEdit->setDate(newDate);
    }
}


void EditAmountInfoDialog::on_EditAmountInfoDialog_rejected()
{
    on_closePushButton_clicked();
}


void EditAmountInfoDialog::on_closePushButton_clicked()
{
    hide();
    emit signalEditElementCompleted();
}


void EditAmountInfoDialog::on_applyPushButton_clicked()
{
    QDate enteredDate = ui->dateDateEdit->date();

    // check for Feb 29
    if ( (enteredDate.month()==2) && (enteredDate.daysInMonth()==29)) {
        QMessageBox::critical(this,"Invalid Date","February 29 is not allowed");
        return;
    }
    int result;
    double d = ui->amountDoubleSpinBox->value();
    qint64 amount = CurrencyHelper::amountDoubleToQint64(d,quint8(currInfo.noOfDecimal),result);
    if(result!=0){
        if (result==-1){
            // amount entered is bigger than the maximum allowed
            double maxDouble = CurrencyHelper::maxValueAllowedForAmountInDouble(quint8(currInfo.noOfDecimal));
            QString errorString = QString("The amount entered %1 is bigger than the maximum allowed of %2").arg(d,maxDouble);
            QMessageBox::critical(this,"Invalid amount",errorString);
            return;
        } else {
            // no of decimal too big or unnow error : should not happen
            return;
        }
    }
    if(!editMode){
        // validate year : must not exist for Element Creation
        if(existingDates.contains(enteredDate)){
            QMessageBox::critical(this,"Invalid Date","Date already exists");
            return;
        }
        // add to the list of existing years
        existingDates.append(enteredDate);
    }
    emit signalEditElementResult(enteredDate, amount);
    if (editMode){
        hide();
        emit signalEditElementCompleted();
    } else {
        // increase date in display and continue creating items
        QDate newDate = enteredDate.addDays(1);
        ui->dateDateEdit->setDate(newDate);
    }
}

