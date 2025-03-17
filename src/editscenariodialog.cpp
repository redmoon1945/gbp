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

#include "editscenariodialog.h"
#include "ui_editscenariodialog.h"
#include "gbpcontroller.h"
#include "currencyhelper.h"
#include <QMessageBox>
#include <QFontDatabase>
#include <QColorDialog>
#include <QCoreApplication>
#include "setfiltertagsdialog.h"


EditScenarioDialog::EditScenarioDialog(QLocale locale, QWidget *parent) :
//    QDialog(parent),
    QDialog(NULL),  // By passing NULL, we make this window independant, but MainWindow must close
                    // it before exiting
    ui(new Ui::EditScenarioDialog)
{
    // Qt UI build-up
    ui->setupUi(this);

    // reset min max of constant inflation spinbox programmatically (annual value !!)
    ui->inflationConstantDoubleSpinBox->setMinimum(Growth::MIN_GROWTH_DOUBLE);
    ui->inflationConstantDoubleSpinBox->setMaximum(Growth::MAX_GROWTH_DOUBLE);
    ui->inflationConstantDoubleSpinBox->setDecimals(Growth::NO_OF_DECIMALS);

    // init variables
    displayLocale = locale;
    tempVariableInflation = Growth::fromVariableDataAnnualBasisDecimal(QMap<QDate, qint64>());

    // set max size for name (description lack s a max value)
    ui->scenarioNameLineEdit->setMaxLength(Scenario::NAME_MAX_LEN);

    // set constraints for end date calculation
    ui->maxDurationSpinBox->setMaximum(Scenario::MAX_DURATION_FE_GENERATION);
    ui->maxDurationSpinBox->setMinimum(Scenario::MIN_DURATION_FE_GENERATION);

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
    newFontSize = Util::changeFontSize(1, true, oldFontSize);
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        QString("Edit scenario - infoActiveTable - Font size set from %1 to %2")
        .arg(oldFontSize).arg(newFontSize));
    infoActiveTableFont.setPointSize(newFontSize);
    //
    QFont infoInactiveTableFont = ui->itemsTableView->font();
    infoInactiveTableFont.setStrikeOut(true);
    infoInactiveTableFont.setItalic(true);
    oldFontSize = infoInactiveTableFont.pointSize();
    newFontSize = Util::changeFontSize(1, true, oldFontSize);
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        QString("Edit scenario - infoInactiveTable - Font size set from %1 to %2")
        .arg(oldFontSize).arg(newFontSize));
    infoInactiveTableFont.setPointSize(newFontSize);

    // set up the list model for items ListView : in UI form, filtering buttons must have been set
    // according to model's default
    itemTableModel = new ScenarioCsdTableModel(
        locale, defaultTableFont, strikeOutTableFont, monoTableFont, monoInactiveTableFont,
        infoActiveTableFont, infoInactiveTableFont,
        GbpController::getInstance().getAllowDecorationColor());
    ui->itemsTableView->setModel(itemTableModel);

    // it appears this must be done AFTER setting the model (don't know why...)
    QFontMetrics fm2 = ui->itemsTableView->fontMetrics();
    ui->itemsTableView->setColumnWidth(0,fm2.averageCharWidth()*12);    // type
    ui->itemsTableView->setColumnWidth(1,fm2.averageCharWidth()*40);    // name
    ui->itemsTableView->setColumnWidth(2,fm2.averageCharWidth()*18);    // amount

    // use smaller font for description text
    QFont descFont = ui->DescPlainTextEdit->font();
    oldFontSize = descFont.pointSize();
    newFontSize = Util::changeFontSize(2,true, oldFontSize);
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        QString("Edit scenario - Description - Font size set from %1 to %2")
        .arg(oldFontSize).arg(newFontSize));
    descFont.setPointSize(newFontSize);
    ui->DescPlainTextEdit->setFont(descFont);

    // force description widget to be small (cant do it in Qt Designer...)
    QFontMetrics fm(ui->DescPlainTextEdit->font());
    ui->DescPlainTextEdit->setFixedHeight(fm.height()*(3*1.2)); // 3 lines

    // use much smaller font for filter controls
    QFont filterButtonFont = ui->filterPeriodicsCheckBox->font();
    oldFontSize = filterButtonFont.pointSize();
    newFontSize =Util::changeFontSize(2,true, oldFontSize);
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        QString("Edit scenario - Filter buttons - Font size set from %1 to %2")
        .arg(oldFontSize).arg(newFontSize));
    filterButtonFont.setPointSize(newFontSize);
    ui->filterPeriodicsCheckBox->setFont(filterButtonFont);
    ui->filterIrregularsCheckBox->setFont(filterButtonFont);
    ui->filterEnabledCheckBox->setFont(filterButtonFont);
    ui->filterDisabledCheckBox->setFont(filterButtonFont);
    ui->filterLabel->setFont(filterButtonFont);
    ui->incomesRadioButton->setFont(filterButtonFont);
    ui->expensesRadioButton->setFont(filterButtonFont);
    ui->filterTagsCheckBox->setFont(filterButtonFont);
    ui->filterTagsPushButton->setFont(filterButtonFont);
    ui->filterTagsPushButton->setVisible(false);

    // Set color of "incomes" and "expenses" filter
    ui->incomesRadioButton->setStyleSheet("color: rgb(0,255,0);");
    ui->expensesRadioButton->setStyleSheet("color: rgb(255,0,0);");

    // Update text for "new" buttons
    updateNewButtonsText();

    // use smaller font for action buttons
    QFont actionButtonFont = ui->addPeriodicPushButton->font();
    oldFontSize = actionButtonFont.pointSize();
    newFontSize =Util::changeFontSize(1,true, oldFontSize);
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        QString("Edit scenario - Action buttons - Font size set from %1 to %2")
        .arg(oldFontSize).arg(newFontSize));
    actionButtonFont.setPointSize(newFontSize);
    ui->addPeriodicPushButton->setFont(actionButtonFont);
    ui->addIrregularPushButton->setFont(actionButtonFont);
    ui->deletePushButton->setFont(actionButtonFont);
    ui->editPushButton->setFont(actionButtonFont);
    ui->duplicatePushButton->setFont(actionButtonFont);
    ui->setColorPushButton->setFont(actionButtonFont);
    ui->enablePushButton->setFont(actionButtonFont);
    ui->disablePushButton->setFont(actionButtonFont);
    ui->selectAllPushButton->setFont(actionButtonFont);
    ui->unselectAllPushButton->setFont(actionButtonFont);

    // Filter tags are disabled
    filterTags.clear();
    ui->filterTagsCheckBox->setChecked(false);
    ui->filterTagsPushButton->setVisible(false);

    // Plain Text Edition Dialog, auto-destroyed by Qt
    editDescriptionDialog = new PlainTextEditionDialog(this);
    editDescriptionDialog->setModal(true);

    // Scenario Inflation edit dialog, auto-destroyed by Qt
    ecInflation = new EditVariableGrowthDialog(tr("Inflation"), locale, this);
    ecInflation->setModal(true);

    // Periodic Stream Def Edit dialog, auto-destroyed by Qt
    psStreamDefDialog = new EditPeriodicDialog(locale, this);
    psStreamDefDialog->setModal(true);

    // Irregular Stream Def Edit dialog, auto-destroyed by Qt
    irStreamDefDialog = new EditIrregularDialog(locale, this);
    irStreamDefDialog->setModal(true);

    // Manage Tags dialog, auto-destroyed by Qt
    manageTagsDlg = new ManageTagsDialog(locale, this);
    manageTagsDlg->setModal(true);

    // Edit Filter Tags, auto-destroyed by Qt
    setFilterTagsDlg = new SetFilterTagsDialog(this);
    setFilterTagsDlg->setModal(true);

    // connect emitters & receivers for Dialogs : Variable Inflation Edition
    QObject::connect(this, &EditScenarioDialog::signalEditVariableInflationPrepareContent,
        ecInflation, &EditVariableGrowthDialog::slotPrepareContent);
    QObject::connect(ecInflation, &EditVariableGrowthDialog::signalEditVariableGrowthResult,
        this, &EditScenarioDialog::slotEditVariableInflationResult);
    QObject::connect(ecInflation, &EditVariableGrowthDialog::signalEditVariableGrowthCompleted,
        this, &EditScenarioDialog::slotEditVariableInflationCompleted);

    // connect emitters & receivers for Dialogs : Periodic Stream Def Edition
    QObject::connect(this, &EditScenarioDialog::signalEditPeriodicStreamDefPrepareContent,
        psStreamDefDialog, &EditPeriodicDialog::slotPrepareContent);
    QObject::connect(psStreamDefDialog, &EditPeriodicDialog::signalEditPeriodicStreamDefResult,
        this, &EditScenarioDialog::slotEditPeriodicStreamDefResult);
    QObject::connect(psStreamDefDialog, &EditPeriodicDialog::signalEditPeriodicStreamDefCompleted,
        this, &EditScenarioDialog::slotEditPeriodicStreamDefCompleted);

    // connect emitters & receivers for Dialogs : Irregular Stream Def Edition
    QObject::connect(this, &EditScenarioDialog::signalEditIrregularStreamDefPrepareContent,
        irStreamDefDialog, &EditIrregularDialog::slotPrepareContent);
    QObject::connect(irStreamDefDialog, &EditIrregularDialog::signalEditIrregularStreamDefResult,
        this, &EditScenarioDialog::slotEditIrregularStreamDefResult);
    QObject::connect(irStreamDefDialog, &EditIrregularDialog::signalEditIrregularStreamDefCompleted,
        this, &EditScenarioDialog::slotEditIrregularStreamDefCompleted);

    // connect emitters & receivers for Dialogs : Plain Text Edition
    QObject::connect(this, &EditScenarioDialog::signalPlainTextDialogPrepareContent,
        editDescriptionDialog, &PlainTextEditionDialog::slotPrepareContent);
    QObject::connect(editDescriptionDialog, &PlainTextEditionDialog::signalPlainTextEditionResult,
       this, &EditScenarioDialog::slotPlainTextEditionResult);
    QObject::connect(editDescriptionDialog,
        &PlainTextEditionDialog::signalPlainTextEditionCompleted,
        this, &EditScenarioDialog::slotPlainTextEditionCompleted);

    // connect emitters & receivers for Dialogs : ManageTags Edition
    QObject::connect(this, &EditScenarioDialog::signalManageTagsPrepareContent,
        manageTagsDlg, &ManageTagsDialog::slotPrepareContent);
    QObject::connect(manageTagsDlg, &ManageTagsDialog::signalManageTagsResult,
        this, &EditScenarioDialog::slotManageTagsResult);
    QObject::connect(manageTagsDlg, &ManageTagsDialog::signalManageTagsCompleted,
        this, &EditScenarioDialog::slotManageTagsCompleted);

    // connect emitters & receivers for Dialogs : SetFilterTags Edition
    QObject::connect(this, &EditScenarioDialog::signalSetFilterTagsPrepareContent,
        setFilterTagsDlg, &SetFilterTagsDialog::slotPrepareContent);
    QObject::connect(setFilterTagsDlg, &SetFilterTagsDialog::signalResult,
        this, &EditScenarioDialog::slotSetFilterTagsResult);
    QObject::connect(setFilterTagsDlg, &SetFilterTagsDialog::signalCompleted,
        this, &EditScenarioDialog::slotSetFilterTagsCompleted);

}

EditScenarioDialog::~EditScenarioDialog()
{
    delete ui;
    delete itemTableModel;   // dont forget, because we have not set "parent" !
}


void EditScenarioDialog::allowColoredCsdNames(bool value)
{
    itemTableModel->setAllowColoredCsdNames(value); // table will update itself
}


// We are about to start the process of editing the content of the current scenario
void EditScenarioDialog::slotPrepareContent(QString countryCode,
    CurrencyInfo newCurrInfo)
{
    // remember/init some variables
    this->countryCode = countryCode;
    this->currInfo = newCurrInfo;
    tempVariableInflation = Growth::fromVariableDataAnnualBasisDecimal(QMap<QDate, qint64>());

    QDate maxDate = GbpController::getInstance().getToday()
        .addYears(Scenario::DEFAULT_DURATION_FE_GENERATION);

    // fill fields from current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario() ;
    ui->scenarioNameLineEdit->setText(scenario->getName());
    ui->DescPlainTextEdit->setPlainText(scenario->getDescription());
    ui->maxDurationSpinBox->setValue(scenario->getFeGenerationDuration());

    // Set Csds
    incomesDefPeriodic = scenario->getIncomesDefPeriodic();
    incomesDefIrregular = scenario->getIncomesDefIrregular();
    expensesDefPeriodic = scenario->getExpensesDefPeriodic();
    expensesDefIrregular = scenario->getExpensesDefIrregular();

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

    // copy current tags and relationships
    tags = scenario->getTags();
    tagCsdRelationships = scenario->getTagCsdRelationships();

    // Filter Tags : we try to keep the previous tag filters, in case this is the same scenario
    // that we edited last time. checkAndAdjustFilterTags will take care to preserve the filter
    // tags if possible, removing what does not exist anymore. UI components are adjusted.
    checkAndAdjustFilterTags() ;

    // set some buttons and titles
    ui->applyPushButton->setText(tr("Apply changes"));
    ui->cancelPushButton->setText(tr("Hide"));
    this->setWindowTitle(tr("Edit current scenario"));

    // refresh the table content
    refreshCsdTableContent();
}


void EditScenarioDialog::slotPlainTextEditionResult(QString result)
{
    ui->DescPlainTextEdit->setPlainText(result);
}



void EditScenarioDialog::slotPlainTextEditionCompleted()
{
}



void EditScenarioDialog::slotEditVariableInflationResult(Growth growthOut)
{
    tempVariableInflation = growthOut;
}



void EditScenarioDialog::slotEditVariableInflationCompleted()
{
}


void EditScenarioDialog::slotEditPeriodicStreamDefResult(bool isIncome,
    PeriodicFeStreamDef psStreamDef)
{
    // add the new or edited csd
    if (psStreamDef.getIsIncome()==true) {
        incomesDefPeriodic.insert(psStreamDef.getId(), psStreamDef);
    } else {
        expensesDefPeriodic.insert(psStreamDef.getId(), psStreamDef);
    }

    // update table content
    refreshCsdTableContent();

    // select the new/edited item
    QList<QUuid> list = QList<QUuid>();
    list.append(psStreamDef.getId());
    selectRowsInTableView(list); // select if displayed

    // make sure it is visible in the viewport of the table
    bool found;
    int row = itemTableModel->getRow(psStreamDef.getId(),found);
    if (found){
        QModelIndex index = itemTableModel->index(row,0);
        // does not always work...
        ui->itemsTableView->scrollTo(index,QAbstractItemView::PositionAtCenter);
    }

    // Log the operation
    if (GbpController::getInstance().getLogLevel()==GbpController::Debug) {
        GbpController::getInstance().log(GbpController::LogLevel::Debug,
            GbpController::Info, QString("A periodic csd has been edited "
            ". Income=%1 , name=%2")
            .arg(isIncome).arg(psStreamDef.getName()));
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal,
            GbpController::Info, QString("A periodic csd has been edited "
            ". Income=%1").arg(isIncome) );
    }

}


// the edit/creation process of PeriodocSimpleStreamDef has completed
void EditScenarioDialog::slotEditPeriodicStreamDefCompleted()
{
}


void EditScenarioDialog::slotEditIrregularStreamDefResult(bool isIncome,
    IrregularFeStreamDef irStreamDef)
{
    // add the new or edited csd
    if (irStreamDef.getIsIncome()==true) {
        incomesDefIrregular.insert(irStreamDef.getId(), irStreamDef);
    } else {
        expensesDefIrregular.insert(irStreamDef.getId(), irStreamDef);
    }

    // update table content
    refreshCsdTableContent();

    // select the edited/new item
    QList<QUuid> list = QList<QUuid>();
    list.append(irStreamDef.getId());
    selectRowsInTableView(list);

    // make sure it is visible in the viewport of the table
    bool found;
    int row = itemTableModel->getRow(irStreamDef.getId(),found);
    if (found){
        QModelIndex index = itemTableModel->index(row,0);
        // does not always work...
        ui->itemsTableView->scrollTo(index,QAbstractItemView::PositionAtCenter);
    }

    // Log the operation
    if (GbpController::getInstance().getLogLevel()==GbpController::Debug) {
        GbpController::getInstance().log(GbpController::LogLevel::Debug,
            GbpController::Info, QString("An irregular csd has been edited. "
            "Income = %1 , name = %2").arg(isIncome).
            arg(irStreamDef.getName()));
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
            QString("An irregular csd has been edited. Income = %1").arg(isIncome));
    }
}


void EditScenarioDialog::slotEditIrregularStreamDefCompleted()
{
}


void EditScenarioDialog::slotManageTagsResult(Tags newTags, TagCsdRelationships newRelationships)
{
    // first, update the tags & relationships
    tags = newTags;
    tagCsdRelationships = newRelationships;

    // Some tags may have been deleted : remove them from filter tags, adjust UI components
    checkAndAdjustFilterTags();

    // In any case, links may have been changed and there is no simple way to know it.
    // Refresh the table's content.
    refreshCsdTableContent();
}


void EditScenarioDialog::slotManageTagsCompleted()
{
}


void EditScenarioDialog::slotSetFilterTagsResult(FilterTags::Mode mode, QSet<QUuid> filterTagIdSet)
{
    // we dont touch the AnyTags (which should be anyway set to false)
    filterTags.setMode(mode);
    filterTags.setFilterTagIdSet(filterTagIdSet);

    // update table's content
    refreshCsdTableContent();
}


void EditScenarioDialog::slotSetFilterTagsCompleted(bool canceled)
{
    // If user canceled, filterTags may stay empty while "EnableFilterTag" is ON.
    // This is not a normal situation, so in that case force EnableFilterTag to OFF
    if ( (canceled==true) && (filterTags.getEnableFilterByTags()==true) &&
        (filterTags.getFilterTagIdSet().size()==0) ) {
        filterTags.clear();
        ui->filterTagsCheckBox->setChecked(false);
        ui->filterTagsPushButton->setVisible(false);
        // update table's content
        refreshCsdTableContent();
    }
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


// Create a new Period FSD
void EditScenarioDialog::on_addPeriodicPushButton_clicked()
{
    bool isIncome = ui->incomesRadioButton->isChecked();
    PeriodicFeStreamDef psStreamDef;  // useless
    QDate maxDate = GbpController::getInstance().getToday().addYears(
        ui->maxDurationSpinBox->value());
    // launch the edit dialog to create a new Periodic FSD
    emit signalEditPeriodicStreamDefPrepareContent(true, isIncome, psStreamDef, currInfo,
        getInflationCurrentlyDefined(), maxDate,{},tags);
    psStreamDefDialog->show();
}


void EditScenarioDialog::on_addIrregularPushButton_clicked()
{
    bool isIncome = ui->incomesRadioButton->isChecked();
    IrregularFeStreamDef irStreamDef;  // useless
    QDate maxDate = GbpController::getInstance().getToday().addYears(
        ui->maxDurationSpinBox->value());
    emit signalEditIrregularStreamDefPrepareContent(true, isIncome, irStreamDef, currInfo,
        maxDate,{},tags);
    irStreamDefDialog->show();
}


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


Growth EditScenarioDialog::getInflationCurrentlyDefined()
{
    if (ui->inflationConstantRadioButton->isChecked()) {
        // we assume the value is constrained, thus always valid
        double annualInflationDouble = ui->inflationConstantDoubleSpinBox->value();
        return Growth::fromConstantAnnualPercentageDouble(annualInflationDouble);
    } else {
        return tempVariableInflation;
    }
}


void EditScenarioDialog::on_fullViewPushButton_clicked()
{
    emit signalPlainTextDialogPrepareContent(tr("Edit description"),
        ui->DescPlainTextEdit->toPlainText(), false);
    editDescriptionDialog->show();
}


void EditScenarioDialog::on_applyPushButton_clicked()
{
    // *** gather & validate some data to create a new Scenario ***

    QString name = ui->scenarioNameLineEdit->text().trimmed();
    QString desc = ui->DescPlainTextEdit->toPlainText();
    quint16 maxDuration = ui->maxDurationSpinBox->value();
    // inflation
    Growth inflation;
    if (ui->inflationConstantRadioButton->isChecked()){
        double annualBasisInfDouble = ui->inflationConstantDoubleSpinBox->value();
        inflation = Growth::fromConstantAnnualPercentageDouble(annualBasisInfDouble);
    } else{
        inflation = tempVariableInflation;
    }

    // *** Create a new scenario from the edit dialog data.
    QSharedPointer<Scenario> scenario;
    try {
        scenario = QSharedPointer<Scenario>(new Scenario(
            Scenario::LATEST_VERSION, name, desc, maxDuration, inflation, countryCode,
            incomesDefPeriodic, incomesDefIrregular, expensesDefPeriodic, expensesDefIrregular,
            tags, tagCsdRelationships));
    } catch (const std::exception& e) {
        // we should not get any exception...but plan for the worst
        QString errorString = QString(tr("An unexpected error has occured while creating a "
            "scenario.\n\nDetails : %1"))
            .arg(e.what());
        QMessageBox::critical(nullptr,tr("Error"), errorString);
        GbpController::getInstance().log(GbpController::LogLevel::Minimal,
            GbpController::Warning, QString(
            "Modifying an existing scenario failed, unexpected exception occured : %1")
            .arg(e.what()));
        return;
    }

    // *** Evaluate the type of changes made to the scenario. There are 2 options :
    // *** 1) Scenario flow data will be different (e.g. new/deleted streamDef, max duration
    //        modified) in which case data will have to be regenerated before chart is refreshed.
    // *** 2) Cosmetic or no change
    bool regenerateData = true;
    if(GbpController::getInstance().isScenarioLoaded()==true){
        regenerateData = !(GbpController::getInstance().getScenario()->evaluateIfSameFlowData(
            scenario));
    }

    // *** switch to the new/editted scenario
    GbpController::getInstance().setScenario(scenario);

    // *** log the changes
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        QString("Modifications to the existing scenario have been applied (but not saved yet"));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info,
        QString("    Name = %1").arg(scenario->getName()));
    GbpController::getInstance().log(GbpController::LogLevel::Debug,
        GbpController::Info, QString("    Country ISO code = %1")
        .arg(scenario->getCountryCode()));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info,
        QString("    Version = %1").arg(scenario->getVersion()));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info,
        QString("    Fe Generation Duration = %1").arg(scenario->getFeGenerationDuration()));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info,
        QString("    No of periodic incomes = %1").arg(scenario->getIncomesDefPeriodic()
        .size()));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info,
        QString("    No of irregular incomes = %1").arg(scenario->getIncomesDefIrregular()
        .size()));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info,
        QString("    No of periodic expenses = %1").arg(scenario->getExpensesDefPeriodic()
        .size()));
    GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info,
        QString("    No of irregular expenses = %1").arg(scenario->getExpensesDefIrregular()
        .size()));

    // *** Provide the editing result to caller
    // Retrieve New/edited scenario using GbpController::getInstance()
    emit signalEditScenarioResult(regenerateData);

    // remove focus from apply button (it is also an indication that processing is completed)
    ui->itemsTableView->setFocus();
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
        s = QString(tr("Cash stream definitions"));
    } else {
        s = QString(tr("Cash stream definitions (%1)").arg(noItems));
    }
    ui->groupBox->setTitle(s);
    //ui->noItemsLabel->setText(s);
}


// When Incomes or Expenses filters change, we want the "New Periodic" and "New Irregular"
// buttons have their text change accordingly
void EditScenarioDialog::updateNewButtonsText()
{
    if(ui->incomesRadioButton->isChecked()==true){
        ui->addPeriodicPushButton->setText(tr("New periodic income..."));
        ui->addIrregularPushButton->setText(tr("New irregular income..."));
    } else {
        ui->addPeriodicPushButton->setText(tr("New periodic expense..."));
        ui->addIrregularPushButton->setText(tr("New irregular expense..."));
    }
}


// Check if each tag in Filter Tags set is still exist in the current Tags set.
// If not, remove it.  Return true if a change has been made, false otherwise.
bool EditScenarioDialog::checkAndAdjustFilterTags()
{
    bool changed = false;

    // In filterTags, remove tags that do not exist anymore, whatever
    // if EnableFilterTagging is true or not.
    QSet<QUuid> fTags = filterTags.getFilterTagIdSet();
    QSet<QUuid> iterationCopySet = fTags;
    foreach(QUuid filterTagId, iterationCopySet){
        if (tags.containsTagId(filterTagId)==false) {
            fTags.remove(filterTagId);
            changed = true;
        }
    }
    if(changed==true){
        filterTags.setFilterTagIdSet(fTags); // refresh
    }

    // if EnableFilterByTags is true and there is no tag left in filtertags, then
    // we consider it is abnormal (why filter by tag when no tag is specified ?)
    // and reset filterTags
    if ( (filterTags.getEnableFilterByTags()==true) && (fTags.size()==0) ) {
        filterTags.clear();
        ui->filterTagsCheckBox->setChecked(false);
        ui->filterTagsPushButton->setVisible(false);
        changed = true;
    }

    return changed;
}


void EditScenarioDialog::refreshCsdTableContent()
{
    itemTableModel->refresh(currInfo, incomesDefPeriodic, incomesDefIrregular,
        expensesDefPeriodic, expensesDefIrregular, tags, tagCsdRelationships,
        ui->incomesRadioButton->isChecked(), ui->expensesRadioButton->isChecked(),
        ui->filterPeriodicsCheckBox->isChecked(), ui->filterIrregularsCheckBox->isChecked(),
        ui->filterEnabledCheckBox->isChecked(), ui->filterDisabledCheckBox->isChecked(),
        filterTags);

    // refresh indicator of the no of items in the table
    updateNoItemsLabel();
}


// Duplicate the selected Csd, add it to the proper set and return its ID.
// If the csd "id" cannot be found, set "found" to false, otherwise set it to true.
QUuid EditScenarioDialog::duplicateCsd(QUuid id, bool &found)
{
    found = false;
    QUuid newId;

    // find the Csd and duplicate
    if (incomesDefPeriodic.contains(id)) {
        found = true;
        PeriodicFeStreamDef p = incomesDefPeriodic.value(id);
        PeriodicFeStreamDef p2 = p.duplicate();
        incomesDefPeriodic.insert(p2.getId(),p2);
        newId = p2.getId();
    } else if (incomesDefIrregular.contains(id)) {
        found = true;
        IrregularFeStreamDef p = incomesDefIrregular.value(id);
        IrregularFeStreamDef p2 = p.duplicate();
        incomesDefIrregular.insert(p2.getId(),p2);
        newId = p2.getId();
    } else if (expensesDefPeriodic.contains(id)) {
        found = true;
        PeriodicFeStreamDef p = expensesDefPeriodic.value(id);
        PeriodicFeStreamDef p2 = p.duplicate();
        expensesDefPeriodic.insert(p2.getId(),p2);
        newId = p2.getId();
    } else if (expensesDefIrregular.contains(id)) {
        found = true;
        IrregularFeStreamDef p = expensesDefIrregular.value(id);
        IrregularFeStreamDef p2 = p.duplicate();
        expensesDefIrregular.insert(p2.getId(),p2);
        newId = p2.getId();
    } else{
        // not found !
        return id;
    }

    return newId;
}


// Remove a list of Csds from the global list of Csds
void EditScenarioDialog::removeCsds(QList<QUuid> toRemove)
{
    foreach(QUuid id, toRemove){
        if (incomesDefPeriodic.contains(id)) {
            incomesDefPeriodic.remove(id);
        }
        if (incomesDefIrregular.contains(id)) {
            incomesDefIrregular.remove(id);
        }
        if (expensesDefPeriodic.contains(id)) {
            expensesDefPeriodic.remove(id);
        }
        if (expensesDefIrregular.contains(id)) {
            expensesDefIrregular.remove(id);
        }
    }
}


// Change the "enabled" status of a list of Csds.
void EditScenarioDialog::enableDisableCsds(QList<QUuid> idList, bool enable)
{
    foreach(QUuid id, idList){
        if (incomesDefPeriodic.contains(id)) {
            PeriodicFeStreamDef p = incomesDefPeriodic.value(id);
            p.setActive(enable);
            incomesDefPeriodic.insert(p.getId(),p);
        }
        if (incomesDefIrregular.contains(id)) {
            IrregularFeStreamDef p = incomesDefIrregular.value(id);
            p.setActive(enable);
            incomesDefIrregular.insert(p.getId(),p);
        }
        if (expensesDefPeriodic.contains(id)) {
            PeriodicFeStreamDef p = expensesDefPeriodic.value(id);
            p.setActive(enable);
            expensesDefPeriodic.insert(p.getId(),p);
        }
        if (expensesDefIrregular.contains(id)) {
            IrregularFeStreamDef p = expensesDefIrregular.value(id);
            p.setActive(enable);
            expensesDefIrregular.insert(p.getId(),p);
        }
    }
}


// Edition requested of the selected Csd
void EditScenarioDialog::on_editPushButton_clicked()
{
        bool found;

        // make sure exactly 1 row is selected and get the ID of the seleted Csd
        QList<QUuid> selectedIdList = getSelection();
        if (selectedIdList.size()!=1){
            QMessageBox::critical(this,tr("Error"),tr("Select exactly one row"));
            ui->itemsTableView->setFocus();    // fix the strange behavior
            return;
        }
        // get the associated Csd type
        FeStreamDef::FeStreamType type = getCsdTypeFromId(selectedIdList.at(0),found);
        if (found==false) {
            return; // should not happen
        }
        QDate maxDate = GbpController::getInstance().getToday().addYears(
            ui->maxDurationSpinBox->value());
        if (type == FeStreamDef::FeStreamType::PERIODIC){
            PeriodicFeStreamDef ps = getPeriodicCsdFromId(selectedIdList.at(0),found);
            if (found==false) {
                return; // should not happen
            }
            // Gather the tag id set this csd is associated with
            QSet<QUuid> tSet = tagCsdRelationships.getRelationshipsForCsd(ps.getId());
            // Launch the edit dialog
            emit signalEditPeriodicStreamDefPrepareContent(false,
                ui->incomesRadioButton->isChecked(), ps, currInfo, getInflationCurrentlyDefined(),
                maxDate, tSet, tags);
            psStreamDefDialog->show();
        } else {
            IrregularFeStreamDef is =  getIrregularCsdFromId(selectedIdList.at(0),found);;
            if (found==false) {
                return; // should not happen
            }
            // Gather the tag id set this fsd is associated with
            QSet<QUuid> tSet = tagCsdRelationships.getRelationshipsForCsd(is.getId());
            // Launch the edit dialog
            emit signalEditIrregularStreamDefPrepareContent(false,
                ui->incomesRadioButton->isChecked(), is, currInfo, maxDate, tSet, tags);
            irStreamDefDialog->show();
        }

}


// Duplicate the selected Csds, along with all their respective tag relationships
void EditScenarioDialog::on_duplicatePushButton_clicked()
{
    bool found;

    // make sure exactly 1 row is selected
    QList<QUuid> selectedIdList = getSelection();
    if (selectedIdList.size()==0){
        QMessageBox::critical(this,tr("Error"),tr("Select at least 1 item"));
        ui->itemsTableView->setFocus();    // fix the strange behavior
        return;
    }

    // duplicate the selected Csds
    QList<QUuid> list = QList<QUuid>();
    QUuid newId ;
    foreach(QUuid id, selectedIdList){
        newId = duplicateCsd(id,found);
        if (found==false) {
            // should not happen
            return;
        }
        list.append(newId);
        // add the same tag relationships for the cloned elements
        tagCsdRelationships.cloneTagRelationshipsForCsd(id,newId);
    }

    // update table content
    refreshCsdTableContent();

    // select the new duplicated Csds
    selectRowsInTableView(list);

    // make sure the last selection is visible in the viewport of the table
    int row = itemTableModel->getRow(newId,found);
    if (found){
        QModelIndex index = itemTableModel->index(row,0);
        // does not always work...
        ui->itemsTableView->scrollTo(index,QAbstractItemView::PositionAtCenter);
    }
}


void EditScenarioDialog::on_deletePushButton_clicked()
{
    // make sure at least 1 row is selected
    QList<QUuid> selectedIdList = getSelection();
    if (selectedIdList.size()==0){
        QMessageBox::critical(this,tr("Error"),tr("Select at least one item"));
        ui->itemsTableView->setFocus();    // fix the strange behavior
        return;
    }

    // remove all associated tags with the deleted Csds
    foreach (QUuid id, selectedIdList) {
        tagCsdRelationships.deleteRelationshipsForCsd(id);
    }

    // Remove the Csds
    removeCsds(selectedIdList);

    // Last thing to do : update the table
    refreshCsdTableContent();
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
        QMessageBox::critical(this,tr("Error"),tr("Select at least one item"));
        ui->itemsTableView->setFocus();    // fix the strange behavior
        return;
    }

    // change status of selected items
    enableDisableCsds(selectedIdList, true);


    // update table's content
    refreshCsdTableContent();

    // reselect the items
    selectRowsInTableView(selectedIdList);
}


void EditScenarioDialog::on_disablePushButton_clicked()
{
    // make sure at least 1 row is selected
    QList<QUuid> selectedIdList = getSelection();
    if (selectedIdList.size()==0){
        QMessageBox::critical(this,tr("Error"),tr("Select at least one item"));
        ui->itemsTableView->setFocus();    // fix the strange behavior
        return;
    }

    // change status of selected items
    enableDisableCsds(selectedIdList, false);

    // update table's content
    refreshCsdTableContent();

    // reselect the items
    selectRowsInTableView(selectedIdList);
}


// Set the color of the names of the selected Csds
void EditScenarioDialog::on_setColorPushButton_clicked()
{
    // make sure at least 1 row is selected
    QList<QUuid> selectedIdList = getSelection();
    if (selectedIdList.size()==0){
        QMessageBox::critical(this,tr("Error"),tr("Select at least one item"));
        ui->itemsTableView->setFocus();    // fix the strange behavior
        return;
    }

    // This is the custom color that will be choosen, if custom selection is requested.
    QColor color;

    // Ask if we have to revert to default system color or rather custom one
    int choice = Util::messageBoxQuestion(this, tr("Question"),
        tr("Do you want to revert back to the default system color or choose a custom one ?"),
        {tr("Cancel"),tr("System's default"),tr("Custom color")},2,0 );
    if(choice==-1){
        // ESC pressed
        return ;
    } else if (choice==0){
        // Cancel button pressed
        return ;
    } else if (choice==1){
        // revert back to system default : set color to "invalid"
        color = QColor();
    } else {
        // Pick a custom color
        color = QColorDialog::getColor(Qt::gray ,this, tr("Color chooser"));
        if (color.isValid()==false) {
            return; // user cancelled
        }
    }

    // Apply to all selected Csds
    foreach (QUuid id, selectedIdList) {
        if (incomesDefPeriodic.contains(id)==true) {
            PeriodicFeStreamDef ps = incomesDefPeriodic.value(id);
            ps.setDecorationColor(color);
            incomesDefPeriodic.insert(id,ps);
        } else if (incomesDefIrregular.contains(id)==true) {
            IrregularFeStreamDef is = incomesDefIrregular.value(id);
            is.setDecorationColor(color);
            incomesDefIrregular.insert(id,is);
        } else if (expensesDefPeriodic.contains(id)==true) {
            PeriodicFeStreamDef ps = expensesDefPeriodic.value(id);
            ps.setDecorationColor(color);
            expensesDefPeriodic.insert(id,ps);
        } else if (expensesDefIrregular.contains(id)==true) {
            IrregularFeStreamDef is = expensesDefIrregular.value(id);
            is.setDecorationColor(color);
            expensesDefIrregular.insert(id,is);
        }
    }

    // update table
    refreshCsdTableContent();
}


void EditScenarioDialog::on_incomesRadioButton_toggled(bool checked)
{
    updateNewButtonsText();
    refreshCsdTableContent();
}


void EditScenarioDialog::on_itemsTableView_doubleClicked(const QModelIndex &index)
{
    on_editPushButton_clicked();
}



void EditScenarioDialog::on_maxDurationSpinBox_valueChanged(int arg1)
{
}


void EditScenarioDialog::on_manageTagsPushButton_clicked()
{
    // Build the info structure for all currently defined Csds of the scenario
    QHash<QUuid, managetags::CsdItem> newCsdItems;
    foreach (PeriodicFeStreamDef f, incomesDefPeriodic) {
        newCsdItems.insert(f.getId(),{.id=f.getId(),.name=f.getName(),.isIncome=f.getIsIncome(),
            .color=f.getDecorationColor()});
    }
    foreach (IrregularFeStreamDef f, incomesDefIrregular) {
        newCsdItems.insert(f.getId(), {.id=f.getId(),.name=f.getName(),.isIncome=f.getIsIncome(),
            .color=f.getDecorationColor()});
    }
    foreach (PeriodicFeStreamDef f, expensesDefPeriodic) {
        newCsdItems.insert(f.getId(),{.id=f.getId(),.name=f.getName(),.isIncome=f.getIsIncome(),
            .color=f.getDecorationColor()});
    }
    foreach (IrregularFeStreamDef f, expensesDefIrregular) {
        newCsdItems.insert(f.getId(),{.id=f.getId(),.name=f.getName(),.isIncome=f.getIsIncome(),
            .color=f.getDecorationColor()});
    }

    // send for display
    emit signalManageTagsPrepareContent(tags, tagCsdRelationships, newCsdItems);
    manageTagsDlg->show();
}


void EditScenarioDialog::on_filterPeriodicsCheckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    refreshCsdTableContent();
}


void EditScenarioDialog::on_filterIrregularsCheckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    refreshCsdTableContent();
}


void EditScenarioDialog::on_filterEnabledCheckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    refreshCsdTableContent();
}


void EditScenarioDialog::on_filterDisabledCheckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    refreshCsdTableContent();

}


void EditScenarioDialog::on_filterTagsCheckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1==Qt::CheckState::Unchecked){
        // *** FILTER BY TAGS IS NOW DISABLED ***
        filterTags.setEnableFilterByTags(false);
        ui->filterTagsPushButton->setVisible(false);
        // Refresh table
        refreshCsdTableContent();
    } else if (arg1==Qt::CheckState::Checked) {
        // *** FILTER BY TAGS IS NOW ENABLED ***
        // *** Current filterTag is re-used ***
        // *** There must be at least one tag defined ***
        if (tags.size()==0) {
            QMessageBox::critical(nullptr,tr("Warning"),
                tr("There are no tag defined for this scenario, so you cannot "
                    "use Tag-based filtering."));
            // revert the state of the checkbox, a new state signal will be emitted and received
            // by this function. Use setCheckState because setChecked does not work as expected
            ui->filterTagsCheckBox->setCheckState(Qt::CheckState::Unchecked);

            return;
        }

        filterTags.setEnableFilterByTags(true);
        ui->filterTagsPushButton->setVisible(true);
        if ( (filterTags.getFilterTagIdSet().size()==0) ) {
            // if no tag have been added to the filter set, this is "legal", but weird.
            // So help user by forwarding him to the tag selection dialog.
            on_filterTagsPushButton_clicked();
            return;
        } else {
            // Refresh table
            refreshCsdTableContent();
        }
    }

}


void EditScenarioDialog::on_filterTagsPushButton_clicked()
{
    emit signalSetFilterTagsPrepareContent(tags, filterTags.getFilterTagIdSet(),
        filterTags.getMode());
    setFilterTagsDlg->show();
}


// Return the type of Csd having "id" as ID. Check value of "found" upon return to be sure
// return value is meaningful.
FeStreamDef::FeStreamType EditScenarioDialog::getCsdTypeFromId(QUuid id, bool &found)
{
    found = false;
    if (incomesDefPeriodic.contains(id)) {
        found = true;
        return FeStreamDef::FeStreamType::PERIODIC;
    } else if (incomesDefIrregular.contains(id)) {
        found = true;
        return FeStreamDef::FeStreamType::IRREGULAR;
    } else if (expensesDefPeriodic.contains(id)) {
        found = true;
        return FeStreamDef::FeStreamType::PERIODIC;
    } else if (expensesDefIrregular.contains(id)) {
        found = true;
        return FeStreamDef::FeStreamType::IRREGULAR;
    }
    return FeStreamDef::FeStreamType::PERIODIC; // dummy value since not found
}


// Return a Periodic Csd (income or expense) having "id" as ID. Check value of "found"
// upon return to be sure return value is meaningful.
PeriodicFeStreamDef EditScenarioDialog::getPeriodicCsdFromId(QUuid id, bool &found)
{
    found = false;
    if (incomesDefPeriodic.contains(id)) {
        found = true;
        PeriodicFeStreamDef p = incomesDefPeriodic.value(id);
        return p;
    } else if (expensesDefPeriodic.contains(id)) {
        found = true;
        PeriodicFeStreamDef p = expensesDefPeriodic.value(id);
        return p;
    }
    return PeriodicFeStreamDef(); // dummy value since not found
}


// Return an Irregular Csd (income or expense) having "id" as ID. Check value of "found" upon
// return to be sure return value is meaningful.
IrregularFeStreamDef EditScenarioDialog::getIrregularCsdFromId(QUuid id, bool &found)
{
    found = false;
    if (incomesDefIrregular.contains(id)) {
        found = true;
        IrregularFeStreamDef i = incomesDefIrregular.value(id);
        return i;
    } else if (expensesDefIrregular.contains(id)) {
        found = true;
        IrregularFeStreamDef i = expensesDefIrregular.value(id);
        return i;
    }
    return IrregularFeStreamDef(); // dummy value since not found
}


