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

#include "editirregulardialog.h"
#include <QTimer>
#include "csvexporter.h"
#include "qcolordialog.h"
#include "ui_editirregulardialog.h"
#include "gbpcontroller.h"
#include "gbplogger.h"
#include "uiutil.h"
#include <QMessageBox>
#include <QFont>
#include <QCoreApplication>
#include <QFileDialog>
#include <qforeach.h>
#include "gbpqmessage.h"


EditIrregularDialog::EditIrregularDialog(QLocale aLocale, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EditIrregularDialog),
    locale(aLocale)
{
    ui->setupUi(this);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);

    QFont appFont = QApplication::font();

    // use smaller font for description list
    QFont descFont = appFont;
    Util::changeFontSize(descFont, Util::FontResizeIntensity::AVERAGE, true,
        "EditIrregularDialog - description");
    ui->descPlainTextEdit->setFont(descFont);

    // force description widget to be small (cant do it in Qt Designer...)
    QFontMetrics fm = ui->descPlainTextEdit->fontMetrics();
    ui->descPlainTextEdit->setFixedHeight(fm.height()*(2*1.2)); // 2 lines

    // use smaller font for tag list
    QFont tagFont = appFont;
    Util::changeFontSize(tagFont, Util::FontResizeIntensity::AVERAGE, true,
        "EditIrregularDialog - tag list");
    ui->tagsEdit->setFont(tagFont);

    // use smaller font for the "CRUD" buttons
    QFont crudButtonFont = appFont;
    Util::changeFontSize(crudButtonFont, Util::FontResizeIntensity::WEAK, true,
        "EditIrregularDialog - CRUD buttons");
    ui->addPushButton->setFont(crudButtonFont);
    ui->deletePushButton->setFont(crudButtonFont);
    ui->editPushButton->setFont(crudButtonFont);
    ui->selectAllPushButton->setFont(crudButtonFont);
    ui->unselectAllPushButton->setFont(crudButtonFont);

    // Make full description view button smaller
    QFont descFullViewFont = appFont;
    Util::changeFontSize(descFullViewFont, Util::FontResizeIntensity::WEAK, true,
        "EditIrregularDialog - desc full view button");
    ui->fullViewPushButton->setFont(descFullViewFont);

    // make the tag list not too high (must be done after font setting)
    QFontMetrics fm2(ui->tagsEdit->font());
    ui->tagsEdit->setFixedHeight(fm2.height()*(2 * 1.2)); // 2 lines

    // set the model (no internal data for now)
    tableModel = new EditIrregularModel(aLocale);
    ui->itemsTableView->setFont(appFont);
    ui->itemsTableView->setModel(tableModel);

    // Model : Date font
    QFont f = appFont;
    tableModel->setDateTableFont(f);
    f.setStrikeOut(true);
    tableModel->setDateDisabledTableFont(f);

    QFont amountFont = appFont;
    tableModel->setAmountTableFont(amountFont);
    amountFont.setStrikeOut(true);
    tableModel->setAmountDisabledTableFont(amountFont);

    // Model : Note font
    f = appFont;
    f.setItalic(true);
    tableModel->setNoteTableFont(f);
    f.setStrikeOut(true);
    tableModel->setNoteDisabledTableFont(f);

    fm = ui->itemsTableView->fontMetrics();
    ui->itemsTableView->horizontalHeader()->setFont(appFont);
    ui->itemsTableView->horizontalHeader()->setMinimumHeight(fm.height() + 10);
    ui->itemsTableView->setColumnWidth(0,fm.averageCharWidth()*30);  // date (long format)
    CurrencyInfo usCur;
    QFontMetrics fmAmount(tableModel->getAmountTableFont());
    ui->itemsTableView->setColumnWidth(1,CurrencyHelper::currencyAmountMaxPixelWidth(usCur,
        locale, fmAmount) + fmAmount.averageCharWidth()*2);  // longest amount + padding

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

    // Csd Breakdown Dialog
    csdBreakdownDialog = new CsdBreakdownDialog(locale,this);// auto-destroyed by Qt
    csdBreakdownDialog->setModal(true);

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
    // connect emitters & receivers for Dialogs : Csd Breakdown
    QObject::connect(this, &EditIrregularDialog::signalCsdBreakdownPrepareContent,
        csdBreakdownDialog, &CsdBreakdownDialog::slotPrepareContent);
    QObject::connect(csdBreakdownDialog,
        &CsdBreakdownDialog::signalCompleted, this,
        &EditIrregularDialog::slotCsdBreakdownCompleted);

}


EditIrregularDialog::~EditIrregularDialog()
{
    delete ui;
    delete tableModel; // dont forget, because we have not set "parent" !
}


void EditIrregularDialog::slotPrepareContent(bool isNewCsd, bool isIncome,
    QWeakPointer<IrregularCsd> iCsd, const CurrencyInfo &newCurrInfo,
    QDate maxDateScenarioFeGeneration, const QSet<QUuid> &associatedTagIds,
    const Tags &availTags)
{
    // If we are editing a existing Csd, convert csd reference to strong pointer
    QSharedPointer<const IrregularCsd> irCsd;
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


void EditIrregularDialog::slotImportResult(const QMap<QDate, IrregularCsd::AmountInfo> &items)
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

void EditIrregularDialog::slotCsdBreakdownCompleted()
{
    // Log the operation
    LOG_INFO("\"Csd breakdown\" dialog completed");
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
        LOG_INFO( QString("Attempting to apply changes to an irregular csd \"%1\" ...")
            .arg(REDACT(ui->nameLineEdit->text())));
    } else {
        LOG_INFO(QString("Creating a new Irregular csd \"%1\" ...")
            .arg(REDACT(ui->nameLineEdit->text())));
    }

    // From the form data, build a temporary Periodic CSD (found in r.sharedPtrCsd) that
    // will stay alive only for the duration of this method.
    BuildFromFormDataResult r;
    buildIrregularCsdFromFormData(r);
    if (r.result.status==Util::ResultOfOperationStatus::ERROR){
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            r.result.userErrorMessage, {tr("OK")}, 0, 0);
        return ;
    }

    emit signalEditIrregularCsdResult(ui->activeYesRadioButton->isChecked(), r.sharedPtrCsd);

    // if editing an existing Csd, then this is the end of it. For create,
    // then stay right there to facilitate the rapid creation of multiple Csd
    if (editingExistingCsd) {
        hide();
        emit signalEditIrregularCsdCompleted();
        LOG_INFO("Modifications applied to the irregular csd");
    } else {
        cleanUpForNewCsd();
        ui->nameLineEdit->setFocus();
        LOG_INFO("New irregular csd created");
    }
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
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            tr("Select exactly one row"), {tr("OK")}, 0, 0);
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
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            tr("Select at least one row"), {tr("OK")}, 0, 0);
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


void EditIrregularDialog::buildIrregularCsdFromFormData(BuildFromFormDataResult &result)
{
    // Reset result to ERROR
    result.init();

    // Build a new IrregularCsd using data in the fields. Data should always be valid.
    QMap<QDate, IrregularCsd::AmountInfo> items = tableModel->getItems();
    try {
        result.sharedPtrCsd = QSharedPointer<IrregularCsd>(
            new IrregularCsd(items, initialId, ui->nameLineEdit->text(),
                ui->descPlainTextEdit->toPlainText(),
                ui->activeYesRadioButton->isChecked(), isIncome, decorationColor));
    } catch(const std::exception& e){
        // should never happen
        result.result.userErrorMessage = QString(tr("An unexpected error has occurred."
            " Details : %1")).arg(e.what());
        result.result.logErrorMessage = QString("An unexpected error has occurred."
            " Details : %1").arg(e.what());
        return;
    } catch (...) {
        // should never happen
        result.result.userErrorMessage = QString(tr("An unexpected error has occurred"));
        result.result.logErrorMessage = QString("An unexpected error has occurred");
        return;
    }

    result.result.status = Util::ResultOfOperationStatus::SUCCESS;
    return;
}


QSharedPointer<FeStream> EditIrregularDialog::generateFinancialEvents(QWeakPointer<Csd> weakCsdPtr,
    uint &saturationCount, FeMinMaxInfo &minMax)
{
    QDate tomorrow = GbpController::getInstance().getTomorrow();
    DateRange fromto = DateRange(tomorrow, maxDateFeGeneration);
    int maxNoOfdays = fromto.getNoOfDays();
    QSharedPointer<FeStream> result = QSharedPointer<FeStream>::create(maxNoOfdays, weakCsdPtr,
        tomorrow);

    // info on PV conversion
    bool usePvConversion = GbpController::getInstance().getUsePresentValue();
    double pvAnnualDiscountRate = GbpController::getInstance().getPvDiscountRate();

    // Generate financial events build for the maximum range set by scenario
    QSharedPointer<IrregularCsd> iPtr = qSharedPointerDynamicCast<IrregularCsd>(weakCsdPtr);
    if (iPtr != nullptr){
        iPtr->generateEventStream(*result, tomorrow, fromto, maxDateFeGeneration,
            (usePvConversion)?(pvAnnualDiscountRate):(0), tomorrow, saturationCount, minMax);
    } else {
        return result; // should never happen
    }

    return result;
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
    // From the form data, build a temporary Periodic CSD (found in r.sharedPtrCsd) that
    // will stay alive only for the duration of this method.
    BuildFromFormDataResult r;
    buildIrregularCsdFromFormData(r);
    if (r.result.status==Util::ResultOfOperationStatus::ERROR){
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            r.result.userErrorMessage, {tr("OK")}, 0, 0);
        return ;
    }

    // Generate the FeStream for this Csd
    uint saturationCount;
    FeMinMaxInfo minMax;
    QSharedPointer<FeStream> feStream = generateFinancialEvents( r.sharedPtrCsd.toWeakRef(),
        saturationCount, minMax);

    // Log the operation
    LOG_INFO( QString("About to visualize occurrences for irregular item name = %1")
        .arg(REDACT(ui->nameLineEdit->text())));

    // Prepare display of Visualization. The slotPrepareContent must do all the calculation because
    // both CSD and FeStream are own by this method on_visualizeOccurrencesPushButton_clicked
    // and will disappear when it completes.
    emit signalVisualizeOccurrencesPrepareContent(currInfo, feStream, saturationCount,
        Growth(), minMax, maxDateFeGeneration);
    visualizeOccurrencesDialog->show();
}


void EditIrregularDialog::on_breakdownPushButton_clicked()
{
    // From the form data, build a temporary Periodic CSD (found in r.sharedPtrCsd) that
    // will stay alive only for the duration of this method.
    BuildFromFormDataResult r;
    buildIrregularCsdFromFormData(r);
    if (r.result.status==Util::ResultOfOperationStatus::ERROR){
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            r.result.userErrorMessage, {tr("OK")}, 0, 0);
        return ;
    }

    // Generate the FeStream for this Csd
    uint saturationCount;
    FeMinMaxInfo minMax;
    QSharedPointer<FeStream> feStream = generateFinancialEvents( r.sharedPtrCsd.toWeakRef(),
        saturationCount, minMax);

    // Log the operation
    LOG_INFO( QString("About to view Csd breakdown for irregular item name = %1")
        .arg(REDACT(ui->nameLineEdit->text())));

    // Prepare display of Breakdown view. The slotPrepareContent must do all the calculation
    // because both CSD and FeStream are own by this method on_breakdownPushButton_clicked
    // and will disappear when it completes.
    emit signalCsdBreakdownPrepareContent(currInfo, feStream, maxDateFeGeneration);
    csdBreakdownDialog->show();
}

void EditIrregularDialog::on_exportPushButton_clicked()
{

    // *** STEP 1 : Build the columns definitions ***
    QList<CsvColumnDescriptor> columns;
    columns.append({tr("Date"), CsvColumnType::date(CsvDateFormat::YearMonthDay,
        GbpController::getInstance().getExportTextDateLocalized())});
    columns.append({tr("Amount"), CsvColumnType::currency(
        GbpController::getInstance().getExportTextNumberLocalized())});
    columns.append({tr("Notes"), CsvColumnType::string()});

    // *** STEP 2 : Populate rows ***
    QList<QList<QVariant>> data;
    QMap<QDate, IrregularCsd::AmountInfo> items = tableModel->getItems();
    QString amountString;
    for (auto it = items.cbegin(); it != items.cend(); ++it) {
        const QDate &date = it.key();
        const IrregularCsd::AmountInfo &info = it.value();

        int res;
        double amount = CurrencyHelper::amountQint64ToDouble(info.amount,currInfo.noOfDecimal,res);
        if(res==0){
            data.append({date,amount,info.notes});
        }
    }

    // *** STEP 3 : Call exportToCsv() and handle the result ***
    CsvExportResult result = CsvExporter::exportToCsv(
        "Irregular csd",  columns, data, locale, currInfo, '\t' );
    switch (result.status) {
        case CsvExportResult::Status::Success:
            break;
        case CsvExportResult::Status::Canceled:
            return; // user closed the dialog — nothing to do
        case CsvExportResult::Status::FileOpenError:
            GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
                tr("Export process failed. Cannot open the "
                "file for saving"), {tr("OK")}, 0, 0);
            return;
        case CsvExportResult::Status::WriteError:
            GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
                tr("Export process failed. Write error."), {tr("OK")}, 0, 0);
            return;
        case CsvExportResult::Status::DataError:
            GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
                tr("Export process failed. Data error."), {tr("OK")}, 0, 0);
            return;
    }

}



EditIrregularDialog::BuildFromFormDataResult::BuildFromFormDataResult()
{
    init();
}


void EditIrregularDialog::BuildFromFormDataResult::init()
{
    result.init();
    sharedPtrCsd = QSharedPointer<IrregularCsd>(); // null
}


void EditIrregularDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    // QTimer::singleShot(0) defers execution until after all show-related events have been
    // processed, including the platform style's focusInEvent which calls selectAll() on the
    // focused QLineEdit. Calling deselect() directly in slotPrepareContent() has no effect
    // because that slot runs before the dialog is shown.
    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("EditIrregularDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
        ui->nameLineEdit->deselect();
    });
}

