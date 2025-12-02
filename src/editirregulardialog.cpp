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

#include "editirregulardialog.h"
#include "qcolordialog.h"
#include "ui_editirregulardialog.h"
#include "gbpcontroller.h"
#include "gbplogger.h"
#include <QMessageBox>
#include <QFont>
#include <QCoreApplication>
#include <QFileDialog>
#include <qforeach.h>


EditIrregularDialog::EditIrregularDialog(QLocale aLocale, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EditIrregularDialog),
    locale(aLocale)
{
    ui->setupUi(this);

    // use smaller font for description list
    QFont descFont = ui->descPlainTextEdit->font();
    Util::changeFontSize(descFont, Util::FontResizeIntensity::WEAK, true);
    ui->descPlainTextEdit->setFont(descFont);

    // force description widget to be small (cant do it in Qt Designer...)
    QFontMetrics fm = ui->descPlainTextEdit->fontMetrics();
    ui->descPlainTextEdit->setFixedHeight(fm.height()*(2*1.2)); // 2 lines

    // use smaller font for the warning Label (for past events)
    QFont warningLabelFont = ui->warningLabel->font();
    Util::changeFontSize(warningLabelFont, Util::FontResizeIntensity::WEAK, true);
    ui->warningLabel->setFont(warningLabelFont);

    // use smaller font for the "future" Label
    QFont futurLabelFont = ui->futurLabel->font();
    Util::changeFontSize(futurLabelFont, Util::FontResizeIntensity::WEAK, true);
    ui->futurLabel->setFont(futurLabelFont);

    // use smaller font for tag list
    QFont tagFont = ui->tagsEdit->font();
    Util::changeFontSize(tagFont, Util::FontResizeIntensity::AVERAGE, true);
    ui->tagsEdit->setFont(tagFont);

    // make the tag list not too high (must be done after font setting)
    QFontMetrics fm2(ui->tagsEdit->font());
    ui->tagsEdit->setFixedHeight(fm2.height()*(2 * 1.2)); // 2 lines

    // set the model (no internal data for now)
    tableModel = new EditIrregularModel(aLocale);
    ui->itemsTableView->setModel(tableModel);

    // adjust table abd set fonts
    fm = ui->itemsTableView->fontMetrics();
    ui->itemsTableView->setColumnWidth(0,fm.averageCharWidth()*15);  // date (based on trials)
    ui->itemsTableView->setColumnWidth(1,fm.averageCharWidth()*25);  // amount (based on trials)
    QFont defaultTableFont = ui->itemsTableView->font();
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(defaultTableFont.pointSize());
    QFont italic = defaultTableFont;
    italic.setItalic(true);
    tableModel->setDefaultTableFont(defaultTableFont);
    tableModel->setMonoTableFont(mono);
    tableModel->setItalicTableFont(italic);

    // force max len for name (not possible for Description)
    ui->nameLineEdit->setMaxLength(Csd::NAME_MAX_LEN);

    // the edit add element dialog
    eie = new EditIrregularElementDialog(locale,this);        // auto-destroyed by Qt
    eie->setModal(true);

    // Plain Text Edition Dialog
    editDescriptionDialog = new PlainTextEditionDialog(this);     // auto-destroyed by Qt
    editDescriptionDialog->setModal(true);
    // the import dialog
    importDlg = new LoadIrregularTextFileDialog(aLocale,this);        // auto-destroyed by Qt
    importDlg->setModal(true);
    // Visualize occurrences  Dialog
    visualizeOccurrencesDialog = new VisualizeOccurrencesDialog(locale,this);// auto-destroyed by Qt
    visualizeOccurrencesDialog->setModal(true);

    // connect emitters & receivers for Dialogs : Description Edition
    QObject::connect(this, &EditIrregularDialog::signalPlainTextDialogPrepareContent,
        editDescriptionDialog, &PlainTextEditionDialog::slotPrepareContent);
    QObject::connect(editDescriptionDialog, &PlainTextEditionDialog::signalPlainTextEditionResult,
        this, &EditIrregularDialog::slotPlainTextEditionResult);
    QObject::connect(editDescriptionDialog,
        &PlainTextEditionDialog::signalPlainTextEditionCompleted, this,
        &EditIrregularDialog::slotPlainTextEditionCompleted);

    // connections with edition of irregular element
    QObject::connect(this, &EditIrregularDialog::signalEditElementPrepareContent, eie,
        &EditIrregularElementDialog::slotPrepareContent);
    QObject::connect(eie, &EditIrregularElementDialog::signalEditElementResult, this,
        &EditIrregularDialog::slotEditElementResult);
    QObject::connect(eie, &EditIrregularElementDialog::signalEditElementCompleted, this,
        &EditIrregularDialog::slotEditElementCompleted);
    // connections with import dialog
    QObject::connect(this, &EditIrregularDialog::signalImportPrepareContent, importDlg,
        &LoadIrregularTextFileDialog::slotPrepareContent);
    QObject::connect(importDlg, &LoadIrregularTextFileDialog::signalImportResult, this,
        &EditIrregularDialog::slotImportResult);
    QObject::connect(importDlg, &LoadIrregularTextFileDialog::signalImportCompleted, this,
        &EditIrregularDialog::slotImportCompleted);
    // connect emitters & receivers for Dialogs : Visualize occurrences
    QObject::connect(this, &EditIrregularDialog::signalVisualizeOccurrencesPrepareContent,
        visualizeOccurrencesDialog, &VisualizeOccurrencesDialog::slotPrepareContent);
    QObject::connect(visualizeOccurrencesDialog, &VisualizeOccurrencesDialog::signalCompleted,
        this, &EditIrregularDialog::slotVisualizeOccurrencesCompleted);
}


EditIrregularDialog::~EditIrregularDialog()
{
    delete ui;
    delete tableModel; // dont forget, because we have not set "parent" !
}


void EditIrregularDialog::slotPrepareContent(bool isNewCsd, bool isIncome,
    QWeakPointer<IrregularCsd> iCsd, CurrencyInfo newCurrInfo, QDate maxDateScenarioFeGeneration,
    QSet<QUuid> associatedTagIds, Tags availTags)
{
    // If we are editing a existing Csd, convert csd reference to strong pointer
    QSharedPointer<IrregularCsd> irCsd;
    if (isNewCsd==false) {
        irCsd = iCsd.toStrongRef();
        if (irCsd.isNull()) {
            return; // Should never happen
        }
    }

    // remember some variables
    this->editingExistingCsd = !isNewCsd;
    this->currInfo = newCurrInfo;
    this->isIncome = isIncome;
    this->maxDateFeGeneration = maxDateScenarioFeGeneration;
    tagIdSet = associatedTagIds;    // ids of the Tag this FSD is associated with
    availableTags = availTags; // All the tags defined in the scenario

    // Set the currency info for the table model and then
    // update model (the view will be automatically updated)
    tableModel->setCurrInfo(currInfo);
    if (isNewCsd==false) {
        tableModel->setItems(irCsd->getAmountSet());
    } else {
        tableModel->setItems({});
    }

    // decoration color
    if (isNewCsd) {
        decorationColor = QColor(); // use normal color for new Stream Def
    } else {
        decorationColor = irCsd->getDecorationColor(); // can be normal or custom
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

    // Show the futur label only if "Convert to present values" is enabled in the Options Dialog.
    // This way, we dont confuse the user if he does not know about this option (or dont care)
    bool usePvConversion = GbpController::getInstance().getUsePresentValue();
    double pvAnnualDiscountRate = GbpController::getInstance().getPvDiscountRate();
    if ( (usePvConversion==true)&&(pvAnnualDiscountRate!=0) ){
        ui->futurLabel->setVisible(true);
    } else {
        ui->futurLabel->setVisible(false);
    }

    if(editingExistingCsd){
        // *** EXISTING ***

        // remember the id
        initialId = irCsd->getId();

        // Set title
        if(isIncome){
            this->setWindowTitle(tr("Editing irregular income"));
        } else {
            this->setWindowTitle(tr("Editing irregular expense"));
        }
        ui->applyPushButton->setText(tr("Apply"));
        ui->cancelPushButton->setText(tr("Cancel"));
        ui->nameLineEdit->setText(irCsd->getName());
        ui->descPlainTextEdit->setPlainText(irCsd->getDesc());
        if (irCsd->getActive()){
            ui->activeYesRadioButton->setChecked(true);
        } else {
            ui->activeNoRadioButton->setChecked(true);
        }

        // Tags
        updateTagListTextBox();

    } else{

        // *** CREATING ***

        // clean up the window
        cleanUpForNewCsd();

        initialId = QUuid::createUuid();
        // set some UI elements
        if(isIncome){
            this->setWindowTitle(tr("Creating irregular income"));
        } else {
            this->setWindowTitle(tr("Creating irregular expense"));
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


void EditIrregularDialog::slotEditElementResult(bool isEdition, QDate oldDate, QDate newDate,
    double editedAmount, QString editedNotes)
{
    // get the current data from the model
    QMap<QDate, IrregularCsd::AmountInfo> items = tableModel->getItems();

    // convert amount to decimal form
    int ok;

    // necessarily always valid. Amount is always >=0.
    quint64 amount = static_cast<quint64>(CurrencyHelper::amountDoubleToQint64(editedAmount,
        currInfo.noOfDecimal, ok));
    if (ok != 0) {
        // we should not have an error because the widget is limiting the value entered
        return;
    }

    // update the data
    IrregularCsd::AmountInfo ai = {.amount=amount, .notes=editedNotes};
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


void EditIrregularDialog::slotImportResult(QMap<QDate, IrregularCsd::AmountInfo> items)
{
    // update the model (the table view will be updated automatically)
    tableModel->setItems(items);
}


void EditIrregularDialog::slotImportCompleted()
{

}


void EditIrregularDialog::slotVisualizeOccurrencesCompleted()
{
    // Log the operation
    LOG_DEBUG_INFO("Visualize occurences completed");
}


void EditIrregularDialog::on_loadPushButton_clicked()
{
    emit signalImportPrepareContent(currInfo);
    importDlg->show();
}


void EditIrregularDialog::on_cancelPushButton_clicked()
{
    hide();
    emit signalEditIrregularCsdCompleted();
}


void EditIrregularDialog::on_applyPushButton_clicked()
{
    if (editingExistingCsd) {
        LOG_INFO( QString("Attempting to modify Irregular csd \"%1\" ...")
            .arg(REDACT(ui->nameLineEdit->text())));
    } else {
        LOG_INFO(QString("Attempting to create new Irregular csd \"%1\" ...")
            .arg(REDACT(ui->nameLineEdit->text())));
    }

    QMap<QDate, IrregularCsd::AmountInfo> items = tableModel->getItems();
    QSharedPointer<IrregularCsd> irCsd = QSharedPointer<IrregularCsd> (
        new IrregularCsd(items, initialId, (ui->nameLineEdit->text().trimmed())
        .left(Csd::NAME_MAX_LEN), ui->descPlainTextEdit->toPlainText()
        .left(Csd::DESC_MAX_LEN),  ui->activeYesRadioButton->isChecked(), isIncome,
        decorationColor));

    emit signalEditIrregularCsdResult(ui->activeYesRadioButton->isChecked(), irCsd);

    if (editingExistingCsd) {
        hide();
        emit signalEditIrregularCsdCompleted();
        LOG_INFO("    Modifications applied");
    } else {
        cleanUpForNewCsd();
        ui->nameLineEdit->setFocus();
        LOG_INFO("    New Irregular csd created");
    }
    LOG_INFO("End of edition");

}


void EditIrregularDialog::on_addPushButton_clicked()
{
    QList<QDate> existingDates = tableModel->getItems().keys();
    emit signalEditElementPrepareContent(isIncome, false,currInfo, existingDates,
        GbpController::getInstance().getTomorrow(),0,""); // last 2 values are dummy
    eie->show();
}


void EditIrregularDialog::on_editPushButton_clicked()
{
    QMap<QDate, IrregularCsd::AmountInfo> items = tableModel->getItems();
    QList<QDate> existingDates = items.keys();
    QList<int> selectedRows = getSelectedRows();
    if (selectedRows.size()!=1){
        QMessageBox::critical(this,tr("Error"),tr("Select exactly one row"));
        ui->itemsTableView->setFocus(); // fix strange behavior
        return;
    }
    QDate aDate = existingDates.at(selectedRows.at(0));
    IrregularCsd::AmountInfo ai = items.value(aDate);
    int ok;

    // necessarily always valid
    double amount = CurrencyHelper::amountQint64ToDouble(ai.amount, currInfo.noOfDecimal,ok);

    if (ok != 0) {
        // should never happen at this stage
        return;
    }
    emit signalEditElementPrepareContent(isIncome, true,currInfo, existingDates,aDate,amount,
        ai.notes); // last 2 values are dummy
    eie->show();
}


void EditIrregularDialog::on_deletePushButton_clicked()
{
    QMap<QDate, IrregularCsd::AmountInfo> items = tableModel->getItems();
    QList<QDate> existingDates = items.keys();
    QList<int> selectedRows = getSelectedRows();
    if (selectedRows.size()==0){
        QMessageBox::critical(this,tr("Error"),tr("Select at least one row"));
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
    emit signalPlainTextDialogPrepareContent(tr("Edit description"),
        ui->descPlainTextEdit->toPlainText(), false);
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


void EditIrregularDialog::cleanUpForNewCsd(){
    ui->nameLineEdit->setText("");
    ui->descPlainTextEdit->setPlainText("");
    ui->activeYesRadioButton->setChecked(true);
    initialId = QUuid::createUuid();
    tableModel->setItems(QMap<QDate, IrregularCsd::AmountInfo>());// erase data

    // decoration color
    ui->decorationColorCheckBox->setChecked(false);
    on_decorationColorCheckBox_clicked();

    // tags
    ui->tagsEdit->clear();
    tagIdSet.clear();   // new fsd will have no association with tags when created
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
        // reset to border and default background color
        ui->decorationColorPushButton->setStyleSheet("");
    }

}


QString EditIrregularDialog::convertTagIDSetToString()
{
    QString result;
    QUuid id;
    Tag tag;
    QStringList sl;
    bool found;

    if (tagIdSet.size()==0) {
        return result;
    } else {
        auto i = tagIdSet.begin();
        while (i != tagIdSet.end()) {
            id = *i;
            tag = availableTags.getTag(id,found);
            if (found==false) {
                return result; // should never happen
            }
            sl.append(tag.getName());
            ++i;
        }
        sl.sort();
        result = sl.join(" | ");
    }

    return result;
}


void EditIrregularDialog::updateTagListTextBox()
{
    QString tagsString = convertTagIDSetToString();
    ui->tagsEdit->setPlainText(tagsString);
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
    color = QColorDialog::getColor(decorationColor, this, tr("Color chooser"));
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

        // default custom color (we dont remember the last one used)
        decorationColor = QColor::fromRgb(128,128,128);

        setDecorationColorInfo();   // take note of it
        // user must select a color now (cancelling is allowed)
        on_decorationColorPushButton_clicked();
    } else{
        // custom to normal color
        ui->decorationColorTextLabel->setEnabled(false);
        ui->decorationColorPushButton->setVisible(false);
        decorationColor = QColor();
        setDecorationColorInfo();
    }
}




void EditIrregularDialog::on_visualizeOccurrencesPushButton_clicked()
{
    // Build a new IrregularCsd using data in the fields
    QMap<QDate, IrregularCsd::AmountInfo> items = tableModel->getItems();
    QSharedPointer<IrregularCsd> irCsd = QSharedPointer<IrregularCsd>(
        new IrregularCsd(items, initialId, ui->nameLineEdit->text(),
        ui->descPlainTextEdit->toPlainText(),
        ui->activeYesRadioButton->isChecked(), isIncome, decorationColor));

    // Log the operation
    LOG_INFO( QString("About to visualize occurrences for irregular item name = %1")
        .arg(REDACT(ui->nameLineEdit->text())));

    emit signalVisualizeOccurrencesPrepareContent(currInfo, Growth(), maxDateFeGeneration,
        irCsd.toWeakRef());
    visualizeOccurrencesDialog->show();
}



void EditIrregularDialog::on_exportPushButton_clicked()
{
    // *** get a file name ***
    QString defaultExtensionUsed = "CSV files (*.csv *.CSV)";
    QString filter = tr("CSV files (*.csv *.CSV);;Text files (*.txt *.TXT);;All files (*)");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Select a file"),
        GbpController::getInstance().getLastDirExport(), filter, &defaultExtensionUsed);
    if (fileName == ""){
        return;
    }
    // *** fix the filename to add the proper suffix ***
    QFileInfo fi(fileName);
    if(fi.suffix()==""){    // user has not specified an extension
        fileName.append(".csv");
    }
    GbpController::getInstance().setLastDirExport(fi.absolutePath());
    LOG_INFO( QString("Attempting to export Irregular Csd \"%1\" Financial events to text "
        "file \"%2\" ...").arg(REDACT(ui->nameLineEdit->text().trimmed())).arg(REDACT(fileName)));

    QFile file(fileName);
    if (false == file.open(QFile::WriteOnly | QFile::Truncate)){
        QMessageBox::critical(nullptr,tr("Error"),tr("Cannot open the file for writing"));
        LOG_ERROR("    Export failed : Cannot open the file for saving");
        return;
    }

    // *** export to the file ***
    QString s;

    // write header
    s = QString("%1\t%2\t%3\n").arg(tr("Date"),tr("Amount"), tr("Notes"));
    file.write(s.toUtf8());

    // set date format
    QString dateFormat = "yyyy-MM-dd";  // ISO
    if (GbpController::getInstance().getExportTextDateLocalized()==true) {
        dateFormat = locale.dateFormat(QLocale::ShortFormat);
    }

    // write data
    QMap<QDate, IrregularCsd::AmountInfo> items = tableModel->getItems();
    QString amountString;
    for (auto it = items.cbegin(); it != items.cend(); ++it) {
        const QDate &date = it.key();
        const IrregularCsd::AmountInfo &info = it.value();
        //
        QString dateString = locale.toString(date, dateFormat );
        if (GbpController::getInstance().getExportTextAmountLocalized()) {
            // Localized
            amountString = CurrencyHelper::formatAmount(info.amount, currInfo, locale, false);
        } else {
            // not localized
            amountString = QString::number(info.amount,'f', currInfo.noOfDecimal);
        }
        s = QString("%1\t%2\t%3\n").arg(dateString,amountString, info.notes);
        file.write(s.toUtf8());
    }

    // Close file and exit
    file.close();
    LOG_INFO("    Export succeeded");
}

