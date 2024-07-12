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

#include "editirregulardialog.h"
#include "qcolordialog.h"
#include "ui_editirregulardialog.h"
#include "gbpcontroller.h"
#include <QMessageBox>
#include <QFont>
#include <QCoreApplication>


EditIrregularDialog::EditIrregularDialog(QLocale aLocale, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EditIrregularDialog),
    locale(aLocale)
{
    ui->setupUi(this);

    // force description widget to be small (cant do it in Qt Designer...)
    QFontMetrics fm = ui->descPlainTextEdit->fontMetrics();
    ui->descPlainTextEdit->setFixedHeight(fm.height()*4); // 4 lines

    // use smaller font for description list
    QFont descFont = ui->descPlainTextEdit->font();
    uint oldFontSize = descFont.pointSize();
    uint newFontSize = Util::changeFontSize(false,true, oldFontSize);
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Edit Irregular Dialog - Description - Font size set from %1 to %2").arg(oldFontSize).arg(newFontSize));
    descFont.setPointSize(newFontSize);
    ui->descPlainTextEdit->setFont(descFont);

    // use smaller font for the warning Label (for past events)
    QFont warninglabelFont = ui->warningLabel->font();
    oldFontSize = warninglabelFont.pointSize();
    newFontSize = Util::changeFontSize(false,true, oldFontSize);
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Edit Irregular Dialog - Warning Label - Font size set from %1 to %2").arg(oldFontSize).arg(newFontSize));
    warninglabelFont.setPointSize(newFontSize);
    ui->warningLabel->setFont(warninglabelFont);

    // set the model (no internal data for now)
    tableModel = new EditIrregularModel(aLocale);
    ui->itemsTableView->setModel(tableModel);

    // adjust table
    fm = ui->itemsTableView->fontMetrics();
    ui->itemsTableView->setColumnWidth(0,fm.averageCharWidth()*35);  // date
    ui->itemsTableView->setColumnWidth(1,fm.averageCharWidth()*25);  // amount
    QFont defaultTableFont = ui->itemsTableView->font();
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(defaultTableFont.pointSize());
    tableModel->setDefaultTableFont(defaultTableFont);
    tableModel->setMonoTableFont(mono);

    // the edit add element dialog
    eie = new EditIrregularElementDialog(locale,this);        // auto-destroyed by Qt
    eie->setModal(true);

    // Plain Text Edition Dialog
    editDescriptionDialog = new PlainTextEditionDialog(this);     // auto-destroyed by Qt because it is a child
    editDescriptionDialog->setModal(true);

    // the import dialog
    importDlg = new LoadIrregularTextFileDialog(aLocale,this);        // auto-destroyed by Qt
    importDlg->setModal(true);

    // connect emitters & receivers for Dialogs : Description Edition
    QObject::connect(this, &EditIrregularDialog::signalPlainTextDialogPrepareContent, editDescriptionDialog, &PlainTextEditionDialog::slotPrepareContent);
    QObject::connect(editDescriptionDialog, &PlainTextEditionDialog::signalPlainTextEditionResult, this, &EditIrregularDialog::slotPlainTextEditionResult);
    QObject::connect(editDescriptionDialog, &PlainTextEditionDialog::signalPlainTextEditionCompleted, this, &EditIrregularDialog::slotPlainTextEditionCompleted);

    // connections with edition of irregular element
    QObject::connect(this, &EditIrregularDialog::signalEditElementPrepareContent, eie, &EditIrregularElementDialog::slotPrepareContent);
    QObject::connect(eie, &EditIrregularElementDialog::signalEditElementResult, this, &EditIrregularDialog::slotEditElementResult);
    QObject::connect(eie, &EditIrregularElementDialog::signalEditElementCompleted, this, &EditIrregularDialog::slotEditElementCompleted);

    // connections with import dialog
    QObject::connect(this, &EditIrregularDialog::signalImportPrepareContent, importDlg, &LoadIrregularTextFileDialog::slotPrepareContent);
    QObject::connect(importDlg, &LoadIrregularTextFileDialog::signalImportResult, this, &EditIrregularDialog::slotImportResult);
    QObject::connect(importDlg, &LoadIrregularTextFileDialog::signalImportCompleted, this, &EditIrregularDialog::slotImportCompleted);

}


EditIrregularDialog::~EditIrregularDialog()
{
    delete ui;
    delete tableModel; // dont forget, because we have not set "parent" !
}


void EditIrregularDialog::slotPrepareContent(bool isNewStreamDef, bool isIncome, IrregularFeStreamDef irStreamDef, CurrencyInfo newCurrInfo)
{
    // remember some variables
    this->editingExistingStreamDef = !isNewStreamDef;
    this->currInfo = newCurrInfo;
    this->isIncome = isIncome;

    // Set the currency info for the table model and then
    // update model (the view will be automatically updated)
    tableModel->setCurrInfo(currInfo);
    tableModel->setItems(irStreamDef.getAmountSet());

    // decoration color
    if (isNewStreamDef) {
        decorationColor = QColor(); // use normal color for new Stream Def
    } else {
        decorationColor = irStreamDef.getDecorationColor(); // can be normal or custom
    }
    if (decorationColor.isValid()==false) {
        // use normal color
        ui->decorationColorCheckBox->setChecked(false);
        ui->decorationColorTextLabel->setEnabled(false);
        ui->decorationColorPushButton->setVisible(false);
    } else {
        // Use custom color for text
        ui->decorationColorCheckBox->setChecked(true);
        ui->decorationColorTextLabel->setEnabled(true);
        ui->decorationColorPushButton->setEnabled(true);
        ui->decorationColorPushButton->setVisible(true);
    }
    setDecorationColorInfo();

    if(editingExistingStreamDef){
        // *** EXISTING ***

        // remember the id
        initialId = irStreamDef.getId();

        if(isIncome){
            this->setWindowTitle(tr("Editing Income of Category Type \"Irregular\""));
        } else {
            this->setWindowTitle(tr("Editing Expense of Category Type \"Irregular\""));
        }
        ui->applyPushButton->setText(tr("Apply"));
        ui->cancelPushButton->setText(tr("Cancel"));
        ui->nameLineEdit->setText(irStreamDef.getName());
        ui->descPlainTextEdit->setPlainText(irStreamDef.getDesc());
        if (irStreamDef.getActive()){
            ui->activeYesRadioButton->setChecked(true);
        } else {
            ui->activeNoRadioButton->setChecked(true);
        }

    } else{

        // *** CREATING ***

        // clean up the window
        cleanUpForNewStreamDef();

        initialId = QUuid::createUuid();
        // set some UI elements
        if(isIncome){
            this->setWindowTitle(tr("Creating Irregular Income"));
        } else {
            this->setWindowTitle(tr("Creating Irregular Expense"));
        }
        ui->applyPushButton->setText(tr("Create"));
        ui->cancelPushButton->setText(tr("Close"));
    }

    ui->nameLineEdit->setFocus();
}


void EditIrregularDialog::slotPlainTextEditionResult(QString result)
{
    ui->descPlainTextEdit->setPlainText(result);
}


void EditIrregularDialog::slotPlainTextEditionCompleted()
{
}


// This can be for an edition of existing element or the definition of a new element
void EditIrregularDialog::slotEditElementResult(bool isEdition, QDate oldDate, QDate newDate, double editedAmount, QString editedNotes)
{
    // get the current data from the model
    QMap<QDate, IrregularFeStreamDef::AmountInfo> items = tableModel->getItems();

    // convert amount to decimal form
    int ok;
    qint64 amount = CurrencyHelper::amountDoubleToQint64(editedAmount, currInfo.noOfDecimal, ok); // necessarily always valid
    if (ok != 0) {
        // we should not have an error because the widget is limiting the value entered
        return;
    }

    // update the data
    IrregularFeStreamDef::AmountInfo ai = {.amount=amount, .notes=editedNotes};
    if ( isEdition && (oldDate!=newDate) ){
        // delete the old entry since date has changed
        items.remove(oldDate);
    }
    items.insert(newDate,ai); // old content is replaced if it exists already for that date

    // update the model (the table view will be updated automatically)
    tableModel->setItems(items);

    // select the edited item
    int rowToSelect = tableModel->getPositionForDate(newDate);
    if (rowToSelect != -1){ // should never happen
        ui->itemsTableView->selectRow(rowToSelect);
    }
}


void EditIrregularDialog::slotEditElementCompleted()
{
    ui->itemsTableView->setFocus();
}


void EditIrregularDialog::slotImportResult(QMap<QDate, IrregularFeStreamDef::AmountInfo> items)
{
    // update the model (the table view will be updated automatically)
    tableModel->setItems(items);
}


void EditIrregularDialog::slotImportCompleted()
{

}


void EditIrregularDialog::on_loadPushButton_clicked()
{
    emit signalImportPrepareContent(currInfo);
    importDlg->show();
}


void EditIrregularDialog::on_cancelPushButton_clicked()
{
    hide();
    emit signalEditIrregularStreamDefCompleted();
}


void EditIrregularDialog::on_applyPushButton_clicked()
{
    if (editingExistingStreamDef) {
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("Attempting to modify Irregular item \"%1\" ...").arg(ui->nameLineEdit->text()));
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("Attempting to create new Irregular item \"%1\" ...").arg(ui->nameLineEdit->text()));
    }

    QMap<QDate, IrregularFeStreamDef::AmountInfo> items = tableModel->getItems();
    IrregularFeStreamDef irStreamDef(items, initialId, ui->nameLineEdit->text(), ui->descPlainTextEdit->toPlainText(),
                                     ui->activeYesRadioButton->isChecked(), isIncome, decorationColor);

    emit signalEditIrregularStreamDefResult(ui->activeYesRadioButton->isChecked(), irStreamDef);

    if (editingExistingStreamDef) {
        hide();
        emit signalEditIrregularStreamDefCompleted();
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("    Modifications applied to existing Irregular item"));
    } else {
        cleanUpForNewStreamDef();
        ui->nameLineEdit->setFocus();
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("    New Irregular item created"));
    }

}


void EditIrregularDialog::on_addPushButton_clicked()
{
    QList<QDate> existingDates = tableModel->getItems().keys();
    emit signalEditElementPrepareContent(isIncome, false,currInfo, existingDates,GbpController::getInstance().getTomorrow(),0,""); // last 2 values are dummy
    eie->show();
}


void EditIrregularDialog::on_editPushButton_clicked()
{
    QMap<QDate, IrregularFeStreamDef::AmountInfo> items = tableModel->getItems();
    QList<QDate> existingDates = items.keys();
    QList<int> selectedRows = getSelectedRows();
    if (selectedRows.size()!=1){
        QMessageBox::critical(this,tr("Invalid Selection"),tr("Select exactly one row"));
        ui->itemsTableView->setFocus(); // fix strange behavior
        return;
    }
    QDate aDate = existingDates.at(selectedRows.at(0));
    IrregularFeStreamDef::AmountInfo ai = items.value(aDate);
    int ok;
    double amount = CurrencyHelper::amountQint64ToDouble(ai.amount, currInfo.noOfDecimal,ok);   // necessarily always valid
    if (ok != 0) {
        // should never happen at this stage
        return;
    }
    emit signalEditElementPrepareContent(isIncome, true,currInfo, existingDates,aDate,amount, ai.notes); // last 2 values are dummy
    eie->show();
}


void EditIrregularDialog::on_deletePushButton_clicked()
{
    QMap<QDate, IrregularFeStreamDef::AmountInfo> items = tableModel->getItems();
    QList<QDate> existingDates = items.keys();
    QList<int> selectedRows = getSelectedRows();
    if (selectedRows.size()==0){
        QMessageBox::critical(this,tr("Invalid Selection"),tr("Select at least one row"));
        ui->itemsTableView->setFocus(); // fix strange behavior
        return;
    }
    // remove selected rows from factors
    foreach(int row,selectedRows ){
        items.remove(existingDates.at(row));
    }
    // update model data and redisplay
    tableModel->setItems(items); // view will update itself
}


void EditIrregularDialog::on_itemsTableView_doubleClicked(const QModelIndex &index)
{
    on_editPushButton_clicked();
}


void EditIrregularDialog::on_fullViewPushButton_clicked()
{
    emit signalPlainTextDialogPrepareContent(tr("Edit Description"), ui->descPlainTextEdit->toPlainText(), false);
    editDescriptionDialog->show();
}


void EditIrregularDialog::on_EditIrregularDialog_rejected()
{
    on_cancelPushButton_clicked();
}


QList<int> EditIrregularDialog::getSelectedRows()
{
    QItemSelectionModel* selectionModel = ui->itemsTableView->selectionModel();
    QModelIndexList selectedIndexes = selectionModel->selectedRows();
    QList<int> selectedRows;
    foreach (const QModelIndex& index, selectedIndexes) {
        selectedRows.append(index.row());
    }
    return selectedRows;
}


void EditIrregularDialog::cleanUpForNewStreamDef(){
    ui->nameLineEdit->setText("");
    ui->descPlainTextEdit->setPlainText("");
    ui->activeYesRadioButton->setChecked(true);
    initialId = QUuid::createUuid();
    tableModel->setItems(QMap<QDate, IrregularFeStreamDef::AmountInfo>());// erase data
    // decoration color
    ui->decorationColorCheckBox->setChecked(false);
    on_decorationColorCheckBox_clicked();
}


void EditIrregularDialog::setDecorationColorInfo()
{
    QString COLOR_STYLE("QPushButton { background-color : %1; border: none;}");

    if (decorationColor.isValid()) {
        ui->decorationColorPushButton->setStyleSheet(COLOR_STYLE.arg(decorationColor.name()));
        QColor c = decorationColor.name(QColor::HexRgb);
        ui->decorationColorTextLabel->setText(Util::buildColorDisplayName(c));
    } else {
        ui->decorationColorTextLabel->setText("");
        ui->decorationColorPushButton->setStyleSheet(""); // reset to border and default background color
    }

}


void EditIrregularDialog::on_selectAllPushButton_clicked()
{
    ui->itemsTableView->selectAll();
}


void EditIrregularDialog::on_unselectAllPushButton_clicked()
{
    ui->itemsTableView->clearSelection();
}



void EditIrregularDialog::on_decorationColorPushButton_clicked()
{
    //QColorDialog::ColorDialogOptions opt = QColorDialog::DontUseNativeDialog;
    QColor color;
    color = QColorDialog::getColor(decorationColor, this, tr("Color Chooser"));
    if (color.isValid()==false) {
        return; // user cancelled
    } else {
        decorationColor = color;
        setDecorationColorInfo();
    }
}


void EditIrregularDialog::on_decorationColorCheckBox_clicked()
{
    if (ui->decorationColorCheckBox->isChecked()){
        // normal to custom color
        ui->decorationColorTextLabel->setEnabled(true);
        ui->decorationColorPushButton->setVisible(true);
        decorationColor = QColor::fromRgb(128,128,128); // default custom color (we dont remember the last one used)
        setDecorationColorInfo();   // take note of it
        on_decorationColorPushButton_clicked(); // user must select a color now (cancelling is allowed)
    } else{
        // custom to normal color
        ui->decorationColorTextLabel->setEnabled(false);
        ui->decorationColorPushButton->setVisible(false);
        decorationColor = QColor();
        setDecorationColorInfo();
    }
}

