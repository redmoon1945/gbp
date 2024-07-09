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

#include "editscenariodialog.h"
#include "ui_editscenariodialog.h"
#include "gbpcontroller.h"
#include "currencyhelper.h"
#include <QMessageBox>
#include <QFontDatabase>
#include <QCoreApplication>


EditScenarioDialog::EditScenarioDialog(QLocale locale, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditScenarioDialog)
{
    // Qt UI build-up
    ui->setupUi(this);

    // force description widget to be small (cant do it in Qt Designer...)
    QFontMetrics fm(ui->DescPlainTextEdit->font());
    ui->DescPlainTextEdit->setFixedHeight(fm.height()*4); // 4 lines

    // initial states of filter buttons (display everything except Expenses)
    // init filter hint string :  checked means filtering is OFF (items are shown)
    setFilterString();

    // reset min max of constant inflation spinbox programmatically (annual value !!)
    ui->inflationConstantDoubleSpinBox->setMinimum(Growth::MIN_GROWTH_DOUBLE);
    ui->inflationConstantDoubleSpinBox->setMaximum(Growth::MAX_GROWTH_DOUBLE);
    ui->inflationConstantDoubleSpinBox->setDecimals(Growth::NO_OF_DECIMALS);

    // init variables
    currentlyEditingExistingScenario = false;
    displayLocale = locale;
    tempVariableInflation = Growth::fromVariableDataAnnualBasisDecimal(QMap<QDate, qint64>());

    // tweak Item Table fonts
    uint oldFontSize;
    uint newFontSize;
    //
    QFont defaultTableFont = ui->itemsTableView->font();
    //
    QFont monoTableFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoTableFont.setPointSize(defaultTableFont.pointSize());
    //
    QFont strikeOutTableFont = ui->itemsTableView->font();
    strikeOutTableFont.setStrikeOut(true);
    strikeOutTableFont.setItalic(true);
    //
    QFont monoInactiveTableFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoInactiveTableFont.setPointSize(defaultTableFont.pointSize());
    monoInactiveTableFont.setStrikeOut(true);
    monoInactiveTableFont.setItalic(true);
    //
    QFont infoActiveTableFont = ui->itemsTableView->font();
    infoActiveTableFont.setItalic(true);
    oldFontSize = infoActiveTableFont.pointSize();
    newFontSize = Util::changeFontSize(false, true, oldFontSize);
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Edit Scenario - infoActiveTable - Font size set from %1 to %2").arg(oldFontSize).arg(newFontSize));
    infoActiveTableFont.setPointSize(newFontSize);
    //
    QFont infoInactiveTableFont = ui->itemsTableView->font();
    infoInactiveTableFont.setStrikeOut(true);
    infoInactiveTableFont.setItalic(true);
    oldFontSize = infoInactiveTableFont.pointSize();
    newFontSize = Util::changeFontSize(false, true, oldFontSize);
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Edit Scenario - infoInactiveTable - Font size set from %1 to %2").arg(oldFontSize).arg(newFontSize));
    infoInactiveTableFont.setPointSize(newFontSize);

    // set up list model for items ListView : filtering buttons must have been set according to model's default
    itemTableModel = new ScenarioFeTableModel(  locale, defaultTableFont, strikeOutTableFont, monoTableFont,
                                                monoInactiveTableFont, infoActiveTableFont, infoInactiveTableFont,
                                                GbpController::getInstance().getAllowDecorationColor());
    ui->itemsTableView->setModel(itemTableModel);

    // it appears this must be done AFTER setting the model (don't know why...)
    QFontMetrics fm2 = ui->itemsTableView->fontMetrics();
    ui->itemsTableView->setColumnWidth(0,fm2.averageCharWidth()*12);     // type
    ui->itemsTableView->setColumnWidth(1,fm2.averageCharWidth()*50);    // name
    ui->itemsTableView->setColumnWidth(2,fm2.averageCharWidth()*18);    // amount

    // use smaller font for description list
    QFont descFont = ui->DescPlainTextEdit->font();
    oldFontSize = descFont.pointSize();
    newFontSize = Util::changeFontSize(false,true, oldFontSize);
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Edit Scenario - Description - Font size set from %1 to %2").arg(oldFontSize).arg(newFontSize));
    descFont.setPointSize(newFontSize);
    ui->DescPlainTextEdit->setFont(descFont);

    // use smaller font for filter buttons
    QFont filterButtonFont = ui->periodicFilterPushButton->font();
    oldFontSize = filterButtonFont.pointSize();
    newFontSize =Util::changeFontSize(true,true, oldFontSize);
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Edit Scenario - Filter Buttons - Font size set from %1 to %2").arg(oldFontSize).arg(newFontSize));
    filterButtonFont.setPointSize(newFontSize);
    ui->periodicFilterPushButton->setFont(filterButtonFont);
    ui->irregularFilterPushButton->setFont(filterButtonFont);
    ui->activeFilterPushButton->setFont(filterButtonFont);
    ui->inactiveFilterPushButton->setFont(filterButtonFont);

    // Plain Text Edition Dialog
    editDescriptionDialog = new PlainTextEditionDialog(this);     // auto-destroyed by Qt because it is a child
    editDescriptionDialog->setModal(true);

    // Scenario Inflation edit dialog
    ecInflation = new EditVariableGrowthDialog(tr("Inflation"), locale, this);                // auto-destroyed by Qt because it is a child
    ecInflation->setModal(true);

    // Periodic Stream Def Edit dialog
    psStreamDefDialog = new EditPeriodicDialog(locale, this);     // auto-destroyed by Qt because it is a child
    psStreamDefDialog->setModal(true);

    // Irregular Stream Def Edit dialog
    irStreamDefDialog = new EditIrregularDialog(locale, this);     // auto-destroyed by Qt because it is a child
    irStreamDefDialog->setModal(true);

    // connect emitters & receivers for Dialogs : Variable Inflation Edition
    QObject::connect(this, &EditScenarioDialog::signalEditVariableInflationPrepareContent, ecInflation, &EditVariableGrowthDialog::slotPrepareContent);
    QObject::connect(ecInflation, &EditVariableGrowthDialog::signalEditVariableGrowthResult, this, &EditScenarioDialog::slotEditVariableInflationResult);
    QObject::connect(ecInflation, &EditVariableGrowthDialog::signalEditVariableGrowthCompleted, this, &EditScenarioDialog::slotEditVariableInflationCompleted);

    // connect emitters & receivers for Dialogs : Periodic Stream Def Edition
    QObject::connect(this, &EditScenarioDialog::signalEditPeriodicStreamDefPrepareContent, psStreamDefDialog, &EditPeriodicDialog::slotPrepareContent);
    QObject::connect(psStreamDefDialog, &EditPeriodicDialog::signalEditPeriodicStreamDefResult, this, &EditScenarioDialog::slotEditPeriodicStreamDefResult);
    QObject::connect(psStreamDefDialog, &EditPeriodicDialog::signalEditPeriodicStreamDefCompleted, this, &EditScenarioDialog::slotEditPeriodicStreamDefCompleted);

    // connect emitters & receivers for Dialogs : Irregular Stream Def Edition
    QObject::connect(this, &EditScenarioDialog::signalEditIrregularStreamDefPrepareContent, irStreamDefDialog, &EditIrregularDialog::slotPrepareContent);
    QObject::connect(irStreamDefDialog, &EditIrregularDialog::signalEditIrregularStreamDefResult, this, &EditScenarioDialog::slotEditIrregularStreamDefResult);
    QObject::connect(irStreamDefDialog, &EditIrregularDialog::signalEditIrregularStreamDefCompleted, this, &EditScenarioDialog::slotEditIrregularStreamDefCompleted);

    // connect emitters & receivers for Dialogs : Plain Text Edition
    QObject::connect(this, &EditScenarioDialog::signalPlainTextDialogPrepareContent, editDescriptionDialog, &PlainTextEditionDialog::slotPrepareContent);
    QObject::connect(editDescriptionDialog, &PlainTextEditionDialog::signalPlainTextEditionResult, this, &EditScenarioDialog::slotPlainTextEditionResult);
    QObject::connect(editDescriptionDialog, &PlainTextEditionDialog::signalPlainTextEditionCompleted, this, &EditScenarioDialog::slotPlainTextEditionCompleted);

}

EditScenarioDialog::~EditScenarioDialog()
{
    delete ui;
    delete itemTableModel;   // dont forget, because we have not set "parent" !
}


void EditScenarioDialog::allowDecorationColor(bool value)
{
    itemTableModel->setAllowDecorationColor(value);
}


// We are about to start a new editing session of the current existing scenario or create a new one.
// if isNewScenario==true, it indicates we are going to edit a new scenario even if there could be one already loaded
void EditScenarioDialog::slotPrepareContent(bool isNewScenario,QString countryCode, CurrencyInfo newCurrInfo)
{
    // remember some variables
    this->countryCode = countryCode;
    this->currInfo = newCurrInfo;
    this->currentlyEditingExistingScenario = !isNewScenario;
    tempVariableInflation = Growth::fromVariableDataAnnualBasisDecimal(QMap<QDate, qint64>()); // empty

    // fill Currency section
    ui->currencyInfoLabel->setText(QString("%1 (%2)").arg(currInfo.name,currInfo.isoCode));

    // // set contents of some components
    if ( !currentlyEditingExistingScenario ){
        // *** NEW SCENARIO ***

        // init ListView model
        itemTableModel->newScenario(currInfo, QMap<QUuid,PeriodicFeStreamDef>(),QMap<QUuid,IrregularFeStreamDef>(),
                                   QMap<QUuid,PeriodicFeStreamDef>(),QMap<QUuid,IrregularFeStreamDef>());

        // fill fields with empty stuff
        ui->scenarioNameLineEdit->setText(tr("Unnamed"));
        ui->DescPlainTextEdit->setPlainText("");

        ui->inflationConstantRadioButton->setChecked(true);
        ui->inflationConstantDoubleSpinBox->setValue(0);
        ui->inflationConstantDoubleSpinBox->setEnabled(true);

        ui->applyPushButton->setText(tr("Create Scenario"));
        ui->cancelPushButton->setText(tr("Cancel"));
        ui->editGrowthPushButton->setEnabled(false);
        this->setWindowTitle(tr("Create a new Scenario"));
    } else {
        // ***EDITING EXISTING SCENARIO ***

        // fill fields from current scenario
        QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario() ;
        ui->scenarioNameLineEdit->setText(scenario->getName());
        ui->DescPlainTextEdit->setPlainText(scenario->getDescription());
        // manage scenario inflation
        if (scenario->getInflation().getType()==Growth::CONSTANT){
            ui->inflationConstantRadioButton->setChecked(true);
            qint64 intInf = scenario->getInflation().getAnnualConstantGrowth();
            double inf = Growth::fromDecimalToDouble(intInf);
            ui->inflationConstantDoubleSpinBox->setValue(inf);
            ui->editGrowthPushButton->setEnabled(false);
            ui->inflationConstantDoubleSpinBox->setEnabled(true);
        } else{
            ui->inflationVariableRadioButton->setChecked(true);
            tempVariableInflation = scenario->getInflation();
            ui->inflationConstantDoubleSpinBox->setValue(0);
            ui->inflationConstantDoubleSpinBox->setEnabled(false);
            ui->editGrowthPushButton->setEnabled(true);
        }
        // init ListView model
        itemTableModel->newScenario(currInfo, scenario->getIncomesDefPeriodic(), scenario->getIncomesDefIrregular(),
                                    scenario->getExpensesDefPeriodic(), scenario->getExpensesDefIrregular());

        // set some buttons and titles
        ui->applyPushButton->setText(tr("Apply Changes"));
        ui->cancelPushButton->setText(tr("Close"));
        this->setWindowTitle(tr("Edit Current Scenario"));
    }

    updateNoItemsLabel();
}

void EditScenarioDialog::slotPlainTextEditionResult(QString result)
{
    ui->DescPlainTextEdit->setPlainText(result);
}



void EditScenarioDialog::slotPlainTextEditionCompleted()
{
    //ui->incExpTableView->setFocus();    // fix strange behavior
}



void EditScenarioDialog::slotEditVariableInflationResult(Growth growthOut)
{
    tempVariableInflation = growthOut;
}



void EditScenarioDialog::slotEditVariableInflationCompleted()
{
    //ui->incExpTableView->setFocus();    // fix strange behavior
}


void EditScenarioDialog::slotEditPeriodicStreamDefResult(bool isIncome, PeriodicFeStreamDef psStreamDef)
{
    itemTableModel->addModifyPeriodicItem(psStreamDef);
    QList<QUuid> list = QList<QUuid>();
    list.append(psStreamDef.getId());
    selectRowsInTableView(list); // select if displayed
    updateNoItemsLabel();
}


// the edit/creation process of PeriodocSimpleStreamDef has completed
void EditScenarioDialog::slotEditPeriodicStreamDefCompleted()
{
    //ui->incExpTableView->setFocus();    // fix a strange bug or behavior (selected row stay highlighted "in reverse")
}


void EditScenarioDialog::slotEditIrregularStreamDefResult(bool isIncome, IrregularFeStreamDef irStreamDef)
{
    itemTableModel->addModifyIrregularItem(irStreamDef);
    QList<QUuid> list = QList<QUuid>();
    list.append(irStreamDef.getId());
    selectRowsInTableView(list);  // select if displayed
    updateNoItemsLabel();
}


void EditScenarioDialog::slotEditIrregularStreamDefCompleted()
{

}


void EditScenarioDialog::on_editGrowthPushButton_clicked()
{
    // edit only if this is a variable inflation
    if ( !(ui->inflationVariableRadioButton->isChecked())){
        return;
    }

    // fill table of EditVariableGrowth with current values for variable inflation
    emit signalEditVariableInflationPrepareContent(tempVariableInflation);
    ecInflation->show();
}


void EditScenarioDialog::on_addPeriodicPushButton_clicked()
{
    bool isIncome = ui->incomesRadioButton->isChecked();
    PeriodicFeStreamDef psStreamDef;  // useless
    emit signalEditPeriodicStreamDefPrepareContent(true, isIncome, psStreamDef, currInfo, getInflationCurrentlyDefined());
    psStreamDefDialog->show();
}


void EditScenarioDialog::on_addIrregularPushButton_clicked()
{
    bool isIncome = ui->incomesRadioButton->isChecked();
    IrregularFeStreamDef irStreamDef;  // useless
    emit signalEditIrregularStreamDefPrepareContent(true, isIncome, irStreamDef, currInfo);
    irStreamDefDialog->show();
}


// void EditScenarioDialog::on_applyPushButton_clicked()
// {



// }


void EditScenarioDialog::on_cancelPushButton_clicked()
{
    this->hide();
    emit signalEditScenarioCompleted();
}



void EditScenarioDialog::on_inflationVariableRadioButton_clicked()
{
    ui->editGrowthPushButton->setEnabled(true);
    ui->inflationConstantDoubleSpinBox->setEnabled(false);
}


void EditScenarioDialog::on_inflationConstantRadioButton_clicked()
{
    ui->editGrowthPushButton->setEnabled(false);
    ui->inflationConstantDoubleSpinBox->setEnabled(true);
}


// Get the list of IDs for the items currently selected
QList<QUuid> EditScenarioDialog::getSelection()
{
    QList<QUuid> result ={};
    // get the indexes of selected rows
    QItemSelectionModel* selectionModel = ui->itemsTableView->selectionModel();
    QModelIndexList selectedRows = selectionModel->selectedRows();
    // convert them all to UUID
    bool found;
    foreach (const QModelIndex &index, selectedRows) {
        QUuid id = itemTableModel->getId(index.row(),found);
        if (found){
            result.append(id);
        }
    }
    return result;
}


// we dont display this anymore, keep it around in case we change idea
void EditScenarioDialog::setFilterString()
{
    // QStringList buf;
    // bool atLeastOne = false;
    // buf.append(tr("Filters : "));
    // if (true == ui->periodicFilterPushButton->isChecked()){
    //     buf.append(tr("<Periodics Hidden>"));
    //     atLeastOne = true;
    // }
    // if (true == ui->irregularFilterPushButton->isChecked()){
    //     buf.append(tr("<Irregulars Hidden>"));
    //     atLeastOne = true;
    // }
    // if (true == ui->activeFilterPushButton->isChecked()){
    //     buf.append(tr("<Enabled Hidden>"));
    //     atLeastOne = true;
    // }
    // if (true == ui->inactiveFilterPushButton->isChecked()){
    //     buf.append(tr("<Disabled Hidden>"));
    //     atLeastOne = true;
    // }

    // if (atLeastOne==false){
    //     buf.append(tr("None"));
    // }
    //ui->filterLabel->setText(buf.join(" "));
}


Growth EditScenarioDialog::getInflationCurrentlyDefined()
{
    if (ui->inflationConstantRadioButton->isChecked()) {
        double annualInflationDouble = ui->inflationConstantDoubleSpinBox->value(); // we assume the value is constrained, thus always valid
        return Growth::fromConstantAnnualPercentageDouble(annualInflationDouble);
    } else {
        return tempVariableInflation;
    }
}


void EditScenarioDialog::on_periodicFilterPushButton_toggled(bool checked)
{
    if(checked){
        ui->periodicFilterPushButton->setText(tr("Show Periodics"));
    } else {
        ui->periodicFilterPushButton->setText(tr("Hide Periodics"));
    }
    setFilterString();
    itemTableModel->filtersChanged(ui->incomesRadioButton->isChecked(), !ui->periodicFilterPushButton->isChecked(),
                                   !ui->irregularFilterPushButton->isChecked(), !ui->activeFilterPushButton->isChecked(), !ui->inactiveFilterPushButton->isChecked());
}


void EditScenarioDialog::on_irregularFilterPushButton_toggled(bool checked)
{
    if(checked){
        ui->irregularFilterPushButton->setText(tr("Show Irregulars"));
    } else {
        ui->irregularFilterPushButton->setText(tr("Hide Irregulars"));
    }
    setFilterString();
    itemTableModel->filtersChanged(ui->incomesRadioButton->isChecked(), !ui->periodicFilterPushButton->isChecked(),
                                   !ui->irregularFilterPushButton->isChecked(), !ui->activeFilterPushButton->isChecked(), !ui->inactiveFilterPushButton->isChecked());
}


void EditScenarioDialog::on_activeFilterPushButton_toggled(bool checked)
{
    if(checked){
        ui->activeFilterPushButton->setText(tr("Show Enabled"));
    } else {
        ui->activeFilterPushButton->setText(tr("Hide Enabled"));
    }
    setFilterString();
    itemTableModel->filtersChanged(ui->incomesRadioButton->isChecked(), !ui->periodicFilterPushButton->isChecked(),
                                   !ui->irregularFilterPushButton->isChecked(), !ui->activeFilterPushButton->isChecked(), !ui->inactiveFilterPushButton->isChecked());
}


void EditScenarioDialog::on_inactiveFilterPushButton_toggled(bool checked)
{
    if(checked){
        ui->inactiveFilterPushButton->setText(tr("Show Disabled"));
    } else {
        ui->inactiveFilterPushButton->setText(tr("Hide Disabled"));
    }
    setFilterString();
    itemTableModel->filtersChanged(ui->incomesRadioButton->isChecked(), !ui->periodicFilterPushButton->isChecked(),
                                   !ui->irregularFilterPushButton->isChecked(), !ui->activeFilterPushButton->isChecked(), !ui->inactiveFilterPushButton->isChecked());
}


void EditScenarioDialog::on_fullViewPushButton_clicked()
{
    emit signalPlainTextDialogPrepareContent(tr("Edit Description"), ui->DescPlainTextEdit->toPlainText(), false);
    editDescriptionDialog->show();
}



void EditScenarioDialog::on_applyPushButton_clicked()
{
    // gather & validate data to create a new Scenario

    QString name = ui->scenarioNameLineEdit->text();
    QString desc = ui->DescPlainTextEdit->toPlainText();
    // inflation
    Growth inflation;
    if (ui->inflationConstantRadioButton->isChecked()){
        double annualBasisInfDouble = ui->inflationConstantDoubleSpinBox->value();
        inflation = Growth::fromConstantAnnualPercentageDouble(annualBasisInfDouble);
    } else{
        inflation = tempVariableInflation;
    }
    QMap<QUuid,PeriodicFeStreamDef> incomesDefPeriodic = itemTableModel->getIncomesDefPeriodic();
    QMap<QUuid,IrregularFeStreamDef> incomesDefIrregular = itemTableModel->getIncomesDefIrregular();
    QMap<QUuid,PeriodicFeStreamDef> expensesDefPeriodic = itemTableModel->getExpensesDefPeriodic();
    QMap<QUuid,IrregularFeStreamDef> expensesDefIrregular = itemTableModel->getExpensesDefIrregular();
    // Create it
    QSharedPointer<Scenario> scenario;
    try {
        scenario = QSharedPointer<Scenario>(new Scenario(
            Scenario::LatestVersion, name, desc, inflation, countryCode, incomesDefPeriodic, incomesDefIrregular, expensesDefPeriodic, expensesDefIrregular));
    } catch (...) {
        // we should not get any exception...but plan for the worst
        std::exception_ptr p = std::current_exception();
        QString errorString = QString(tr("An unexpected error has occured.\n\nDetails : %1")).arg((p ? p.__cxa_exception_type()->name() : "null"));
        if (currentlyEditingExistingScenario) {
            QMessageBox::critical(nullptr,tr("Error modifying an existing scenario"), errorString);
            GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Warning, QString("Modifying an existing scenario failed, unexpected exception occured : %1").arg((p ? p.__cxa_exception_type()->name() : "null")));
        } else {
            QMessageBox::critical(nullptr,tr("Error creating scenario"), errorString);
            GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Warning, QString("Creating a new scenario failed, unexpected exception occured : %1").arg((p ? p.__cxa_exception_type()->name() : "null")));
        }

        return;
    }

    // switch current scenario to this one
    GbpController::getInstance().setScenario(scenario);
    if (currentlyEditingExistingScenario){
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Modifications to the existing scenario have been applied (but not saved yet"));
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    Name = %1").arg(scenario->getName()));
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    Country ISO code = %1").arg(scenario->getCountryCode()));
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    Version = %1").arg(scenario->getVersion()));
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    No of periodic incomes = %1").arg(scenario->getIncomesDefPeriodic().size()));
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    No of irregular incomes = %1").arg(scenario->getIncomesDefIrregular().size()));
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    No of periodic expenses = %1").arg(scenario->getExpensesDefPeriodic().size()));
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    No of irregular expenses = %1").arg(scenario->getExpensesDefIrregular().size()));
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Creation of a new scenario have succeeded (but not saved yet"));
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    Name = %1").arg(scenario->getName()));
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    Country ISO code = %1").arg(scenario->getCountryCode()));
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    Version = %1").arg(scenario->getVersion()));
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    No of periodic incomes = %1").arg(scenario->getIncomesDefPeriodic().size()));
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    No of irregular incomes = %1").arg(scenario->getIncomesDefIrregular().size()));
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    No of periodic expenses = %1").arg(scenario->getExpensesDefPeriodic().size()));
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info, QString("    No of irregular expenses = %1").arg(scenario->getExpensesDefIrregular().size()));
    }

    // update scenario data displayed
    emit signalEditScenarioResult(!currentlyEditingExistingScenario);

    // If it was a new scenario that we were editing, close the window.
    // For existing scenario being edited, keep the window opened for convenience.
    if (!currentlyEditingExistingScenario){
        GbpController::getInstance().setFullFileName("");  // new scenario has not been saved yet
        this->hide();
        emit signalEditScenarioCompleted();
    }
}


void EditScenarioDialog::on_EditScenarioDialog_rejected()
{
    on_cancelPushButton_clicked();
}


// This is not trivial : Qt is complicated for non adjencent multirow selection
void EditScenarioDialog::selectRowsInTableView(QList<QUuid> idList) {
    // first clear current selection
    ui->itemsTableView->clearSelection();
    // then select one by one item that can be found
    bool found;
    QItemSelection selection;

    foreach(QUuid id, idList){
        int row = itemTableModel->getRow(id, found);
        if (found==false){
            continue; // cannot select anything since not found in what is displayed
        }
        QModelIndex leftIndex  = ui->itemsTableView->model()->index(row, 0);
        QModelIndex rightIndex = ui->itemsTableView->model()->index(row, 3);

        QItemSelection rowSelection(leftIndex, rightIndex);
        selection.merge(rowSelection, QItemSelectionModel::Select);
    }
    ui->itemsTableView->selectionModel()->select(selection, QItemSelectionModel::Select);
}


void EditScenarioDialog::updateNoItemsLabel()
{
    int noItems = itemTableModel->getNoItems();
    QString s;
    if (noItems==0){
        s = QString(tr("No items"));
    } else if (noItems==1){
        s = QString(tr("1 item"));
    } else {
        s = QString(tr("%1 items")).arg(noItems);
    }
    ui->noItemsLlabel->setText(s);
}


void EditScenarioDialog::on_editPushButton_clicked()
{
        bool found;

        // make sure exactly 1 row is selected
        QList<QUuid> selectedIdList = getSelection();
        if (selectedIdList.size()!=1){
            QMessageBox::critical(this,tr("Invalid Selection"),tr("Select exactly one row"));
            ui->itemsTableView->setFocus();    // fix the strange behavior
            return;
        }
        // get type of StreamDef
        FeStreamDef::FeStreamType type = itemTableModel->getTypeOfFeStreamDef(selectedIdList.at(0),found);
        if (found==false) {
            // should not happen
            return;
        }
        if (type == FeStreamDef::PERIODIC){
            PeriodicFeStreamDef ps = itemTableModel->getPeriodicFeStreamDef(selectedIdList.at(0),found) ;
            if (found==false) {
                return;  // should never happen
            }
            emit signalEditPeriodicStreamDefPrepareContent(false, ui->incomesRadioButton->isChecked(), ps, currInfo, getInflationCurrentlyDefined());
            psStreamDefDialog->show();
        } else {
            IrregularFeStreamDef is = itemTableModel->getIrregularFeStreamDef(selectedIdList.at(0),found) ;
            if (found==false) {
                return;  // should never happen
            }
            emit signalEditIrregularStreamDefPrepareContent(false, ui->incomesRadioButton->isChecked(), is, currInfo);
            irStreamDefDialog->show();
        }

}


void EditScenarioDialog::on_duplicatePushButton_clicked()
{
    bool found;

    // make sure exactly 1 row is selected
    QList<QUuid> selectedIdList = getSelection();
    if (selectedIdList.size()==0){
        QMessageBox::critical(this,tr("Invalid Selection"),tr("Select at least 1 item"));
        ui->itemsTableView->setFocus();    // fix the strange behavior
        return;
    }

    QList<QUuid> list =  QList<QUuid>();
    foreach(QUuid id, selectedIdList){
        QUuid newId = itemTableModel->duplicateItem(id,found);
        if (found==false) {
            // should not happen
            return;
        }
        list.append(newId);
    }
    selectRowsInTableView(list);
    updateNoItemsLabel();
}


void EditScenarioDialog::on_deletePushButton_clicked()
{
    // make sure at least 1 row is selected
    QList<QUuid> selectedIdList = getSelection();
    if (selectedIdList.size()==0){
        QMessageBox::critical(this,tr("Invalid Selection"),tr("Select at least one item"));
        ui->itemsTableView->setFocus();    // fix the strange behavior
        return;
    }

    itemTableModel->removeItems(selectedIdList);
    updateNoItemsLabel();
}


void EditScenarioDialog::on_selectAllPushButton_clicked()
{
    ui->itemsTableView->selectAll();
}


void EditScenarioDialog::on_unselectAllPushButton_clicked()
{
    ui->itemsTableView->clearSelection();
}


void EditScenarioDialog::on_enablePushButton_clicked()
{
    // make sure at least 1 row is selected
    QList<QUuid> selectedIdList = getSelection();
    if (selectedIdList.size()==0){
        QMessageBox::critical(this,tr("Invalid Selection"),tr("Select at least one item"));
        ui->itemsTableView->setFocus();    // fix the strange behavior
        return;
    }

    itemTableModel->changeActiveStatusItems(selectedIdList, true);
    // reselect the items
    selectRowsInTableView(selectedIdList);
}


void EditScenarioDialog::on_disablePushButton_clicked()
{
    // make sure at least 1 row is selected
    QList<QUuid> selectedIdList = getSelection();
    if (selectedIdList.size()==0){
        QMessageBox::critical(this,tr("Invalid Selection"),tr("Select at least one item"));
        ui->itemsTableView->setFocus();    // fix the strange behavior
        return;
    }

    itemTableModel->changeActiveStatusItems(selectedIdList, false);
    // reselect the items
    selectRowsInTableView(selectedIdList);
}


void EditScenarioDialog::on_incomesRadioButton_toggled(bool checked)
{
    itemTableModel->filtersChanged(ui->incomesRadioButton->isChecked(), !ui->periodicFilterPushButton->isChecked(),
                                   !ui->irregularFilterPushButton->isChecked(), !ui->activeFilterPushButton->isChecked(), !ui->inactiveFilterPushButton->isChecked());
    updateNoItemsLabel();

}


void EditScenarioDialog::on_itemsTableView_doubleClicked(const QModelIndex &index)
{
    on_editPushButton_clicked();
}


