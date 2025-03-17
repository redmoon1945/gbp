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

#include "editvariablegrowthdialog.h"
#include "ui_editvariablegrowthdialog.h"
#include <QString>
#include <QMessageBox>
#include <QItemSelectionModel>
#include <QCoreApplication>
#include "editvariablegrowthmodel.h"
#include "gbpcontroller.h"


EditVariableGrowthDialog::EditVariableGrowthDialog(QString newGrowthName, QLocale locale,
    QWidget *parent) : QDialog(parent), ui(new Ui::EditVariableGrowthDialog)
{
    this->locale = locale;
    ui->setupUi(this);

    // set the model (no internal data for now)
    tableModel = new EditVariableGrowthModel(newGrowthName, this->locale);
    ui->growthTableView->setModel(tableModel);
    // force equal with of columns
    ui->growthTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // Makes note characters smaller and italic
    QFont noteFont = ui->noteLabel->font();
    uint oldFontSize = noteFont.pointSize();
    uint newFontSize = Util::changeFontSize(1,true, oldFontSize);
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        QString("EdityVariableGrowth - Note : Font size set from %1 to %2")
        .arg(oldFontSize).arg(newFontSize));
    noteFont.setPointSize(newFontSize);
    ui->noteLabel->setFont(noteFont);

    // the edit add element dialog
    ege = new EditGrowthElementDialog(newGrowthName,locale,this);        // auto-destroyed by Qt
    ege->setModal(true);

    // connections with edition of growth element
    QObject::connect(this, &EditVariableGrowthDialog::signalEditElementPrepareContent, ege,
        &EditGrowthElementDialog::slotPrepareContent);
    QObject::connect(ege, &EditGrowthElementDialog::signalEditElementResult, this,
        &EditVariableGrowthDialog::slotEditElementResult);
    QObject::connect(ege, &EditGrowthElementDialog::signalEditElementCompleted, this,
        &EditVariableGrowthDialog::slotEditElementCompleted);
}


EditVariableGrowthDialog::~EditVariableGrowthDialog()
{
    delete ui;
    delete tableModel; // dont forget, because we have not set "parent" !
}


// Prepare for a new editing session.
// To be called before displaying the EditComplexGrowthDialog window, to setup content
void EditVariableGrowthDialog::slotPrepareContent(Growth newGrowth)
{
    QString tmp = Util::wordCapitalize(true,QString(tr("Edit variable %1"))
        .arg(tableModel->getGrowthName()));
    this->setWindowTitle(tmp);

    ui->noteLabel->setText(QString(tr("%1 : Value is 0 before the oldest transition date is "
        "defined. It is always applied on a monthly basis, even if defined on an annual basis "
        "(for convenience purpose). Value stays the same until a new transition date + value is "
        "defined.")).arg(tableModel->getGrowthName()));
    tableModel->setGrowthName(tableModel->getGrowthName());
    // update model (the view will be automatically updated)
    tableModel->setGrowthData(newGrowth);
}


// This can be for an edition of existing element or the definition of a new element
void EditVariableGrowthDialog::slotEditElementResult(bool isEdition, QDate oldDate,
    QDate newDate, double growthInPercentage)
{
    // get the current data from the model
    Growth ag = tableModel->getGrowthData();   // necessarily a Variable type

    // convert growth in percentge to growth in hundredth of percentage
    qint64 gInt = Growth::fromDoubleToDecimal(growthInPercentage); // necessarily always valid

    // update the data
    QMap<QDate,qint64> factors = ag.getAnnualVariableGrowth();
    if ( isEdition && (oldDate!=newDate) ){
        // delete the old entry since date has changed
        factors.remove(oldDate);
    }
    factors.insert(newDate,gInt); // if existing date, value is replaced
    Growth newAg = Growth::fromVariableDataAnnualBasisDecimal(factors);

    // update the model (the table view will be updated automatically)
    tableModel->setGrowthData(newAg);

    // select the edited item
    int rowToSelect = tableModel->getPositionForDate(newDate);
    if (rowToSelect != -1){ // should never happen
        ui->growthTableView->selectRow(rowToSelect);
    }
}


void EditVariableGrowthDialog::slotEditElementCompleted()
{
    ui->growthTableView->setFocus();
}



void EditVariableGrowthDialog::on_applyPushButton_clicked()
{
    Growth g = tableModel->getGrowthData();
    emit signalEditVariableGrowthResult(g);
    hide();
    emit signalEditVariableGrowthCompleted();
}


void EditVariableGrowthDialog::on_cancelPushButton_clicked()
{
    hide();
    emit signalEditVariableGrowthCompleted();
}


void EditVariableGrowthDialog::on_addPushButton_clicked()
{
    QList<QDate> existingDates = tableModel->getGrowthData().getAnnualVariableGrowth().keys();
    emit signalEditElementPrepareContent(false,existingDates,GbpController::getInstance()
        .getTomorrow(),0); // last 2 values are dummy
    ege->show();
}


void EditVariableGrowthDialog::on_editPushButton_clicked()
{
    QMap<QDate,qint64> factors = tableModel->getGrowthData().getAnnualVariableGrowth();
    QList<QDate> existingDates = factors.keys();
    QList<int> selectedRows = getSelectedRows();
    if (selectedRows.size()!=1){
        QMessageBox::critical(this,tr("Error"),tr("Select exactly one row"));
        ui->growthTableView->setFocus(); // fix strange behavior
        return;
    }
    QDate aDate = existingDates.at(selectedRows.at(0));
    qint64 growthInt = (factors.value(aDate));
    double growthDouble = Growth::fromDecimalToDouble(growthInt);   // necessarily always valid
    emit signalEditElementPrepareContent(true,existingDates,aDate,growthDouble);
    ege->show();
}


void EditVariableGrowthDialog::on_deletePushButton_clicked()
{
    QMap<QDate,qint64> factors = tableModel->getGrowthData().getAnnualVariableGrowth();
    QList<QDate> existingDates = factors.keys();
    QList<int> selectedRows = getSelectedRows();
    if (selectedRows.size()==0){
        QMessageBox::critical(this,tr("Error"),tr("Select at least one row"));
        ui->growthTableView->setFocus(); // fix strange behavior
        return;
    }
    // remove selected rows from factors
    foreach(int row,selectedRows ){
        factors.remove(existingDates.at(row));
    }
    // update model data and redisplay
    Growth newAg = Growth::fromVariableDataAnnualBasisDecimal(factors);
    tableModel->setGrowthData(newAg); // view will update itself
}



QList<int> EditVariableGrowthDialog::getSelectedRows()
{
    QItemSelectionModel* selectionModel = ui->growthTableView->selectionModel();
    QModelIndexList selectedIndexes = selectionModel->selectedRows();
    QList<int> selectedRows;
    foreach (const QModelIndex& index, selectedIndexes) {
        selectedRows.append(index.row());
    }
    return selectedRows;
}


void EditVariableGrowthDialog::on_growthTableView_doubleClicked(const QModelIndex &index)
{
    on_editPushButton_clicked();
}


// dialog has been closed ("x" button) manually by user
void EditVariableGrowthDialog::on_EditVariableGrowthDialog_rejected()
{
    on_cancelPushButton_clicked();
}


void EditVariableGrowthDialog::on_SelectAllPushButton_clicked()
{
    ui->growthTableView->selectAll();
    ui->growthTableView->setFocus(); // fix strange behavior or bug
}


void EditVariableGrowthDialog::on_unselectAllPushButton_clicked()
{
    ui->growthTableView->clearSelection();
}

