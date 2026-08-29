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

#include "editscenariodialog.h"
#include <QTimer>
#include "ui_editscenariodialog.h"
#include "gbpcontroller.h"
#include "gbplogger.h"
#include "currencyhelper.h"
#include <QMessageBox>
#include <QColorDialog>
#include <QCoreApplication>
#include "choosetagsdialog.h"
#include "gbpqmessage.h"
#include "constants.h"
#include "uiutil.h"


EditScenarioDialog::EditScenarioDialog(QLocale locale) :
    QDialog(NULL),  // By passing NULL, we make this window independant, but MainWindow must close
                    // it before exiting
    started(false),
    ui(new Ui::EditScenarioDialog)
{
    // Qt UI build-up
    ui->setupUi(this);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);

    connect(ui->addPeriodicPushButton, &QPushButton::clicked,
        this, &EditScenarioDialog::onAddMenuPeriodicClicked);
    connect(ui->addIrregularPushButton, &QPushButton::clicked,
        this, &EditScenarioDialog::onAddMenuIrregularClicked);

    auto *periodicAction = new QAction(tr("Periodic"), this);
    periodicAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
    connect(periodicAction, &QAction::triggered,
        this, &EditScenarioDialog::onAddMenuPeriodicClicked);
    auto *irregularAction = new QAction(tr("Irregular"), this);
    irregularAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I));
    connect(irregularAction, &QAction::triggered,
        this, &EditScenarioDialog::onAddMenuIrregularClicked);
    this->addAction(periodicAction);
    this->addAction(irregularAction);

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
    ui->maxDurationSpinBox->setMaximum(Constants::MAX_DURATION_FE_GENERATION);
    ui->maxDurationSpinBox->setMinimum(Constants::MIN_DURATION_FE_GENERATION);

    QFont appFont = QApplication::font();

    // tweak Item Table fonts
    //
    QFont defaultTableFont = appFont;
    //
    QFont amountTableFont = appFont;
    //
    QFont strikeOutTableFont = appFont;
    strikeOutTableFont.setStrikeOut(true);
    strikeOutTableFont.setItalic(true);
    //
    QFont amountInactiveTableFont = appFont;
    amountInactiveTableFont.setStrikeOut(true);
    amountInactiveTableFont.setItalic(true);
    //
    QFont infoActiveTableFont = appFont;
    infoActiveTableFont.setItalic(true);
    Util::changeFontSize(infoActiveTableFont, Util::FontResizeIntensity::WEAK, true,
        "EditScenarioDialog - infoActiveTableFont");
    //
    QFont infoInactiveTableFont = appFont;
    infoInactiveTableFont.setStrikeOut(true);
    infoInactiveTableFont.setItalic(true);
    Util::changeFontSize(infoInactiveTableFont, Util::FontResizeIntensity::WEAK, true,
        "EditScenarioDialog - infoInactiveTableFont");

    // set up the list model for items ListView : in UI form, filtering buttons must have been set
    // according to model's default
    itemTableModel = new ScenarioCsdTableModel(
        locale, defaultTableFont, strikeOutTableFont, amountTableFont, amountInactiveTableFont,
        infoActiveTableFont, infoInactiveTableFont,
        GbpController::getInstance().getAllowDecorationColor());
    ui->itemsTableView->setModel(itemTableModel);

    // Type/Name/Amount all show locale- and content-dependent text (translated type labels,
    // free-form CSD names, locale-formatted amounts) that a fixed averageCharWidth() guess
    // can under-size on some platform/font/locale combinations (see the Irregular dialog's
    // Date column for a concrete case of this). Re-fit all three to their actual content
    // every time the table's data changes, rather than guessing once at startup -
    // refresh() always goes through beginResetModel()/endResetModel(), so this single
    // connection covers every place the table gets (re)populated.
    connect(itemTableModel, &QAbstractItemModel::modelReset, this, [this]() {
        UiUtil::resizeTableViewColumns(ui->itemsTableView, {0, 1, 2});
    });
    // it appears this must be done AFTER setting the model (don't know why...)
    QFontMetrics fm2 = ui->itemsTableView->fontMetrics();
    UiUtil::resizeTableViewColumns(ui->itemsTableView, {0, 1, 2});  // type, name, amount
    // Horizontal header: minimum height derived from font so text is never clipped on Windows.
    ui->itemsTableView->horizontalHeader()->setFont(appFont);
    ui->itemsTableView->horizontalHeader()->setMinimumHeight(fm2.height() + 10);

    // use smaller font for description text
    QFont descFont = appFont;
    Util::changeFontSize(descFont, Util::FontResizeIntensity::AVERAGE, true,
        "EditScenarioDialog - description");
    ui->DescPlainTextEdit->setFont(descFont);

    // force description widget to be small (cant do it in Qt Designer...)
    QFontMetrics fm(ui->DescPlainTextEdit->font());
    ui->DescPlainTextEdit->setFixedHeight(fm.height()*(2*1.2)); // 2 lines

    // use smaller font for filter controls
    QFont filterButtonFont = appFont;
    Util::changeFontSize(filterButtonFont, Util::FontResizeIntensity::AVERAGE, true,
        "EditScenarioDialog - filter controls");
    ui->filterPeriodicsCheckBox->setFont(filterButtonFont);
    ui->filterIrregularsCheckBox->setFont(filterButtonFont);
    ui->filterEnabledCheckBox->setFont(filterButtonFont);
    ui->filterDisabledCheckBox->setFont(filterButtonFont);
    ui->filterLabel->setFont(filterButtonFont);
    ui->incomesRadioButton->setFont(filterButtonFont);
    ui->expensesRadioButton->setFont(filterButtonFont);
    ui->filterTagsCheckBox->setFont(filterButtonFont);
    ui->filterTagsPushButton->setVisible(false);

    // use much smaller font for tag filter button and combo
    QFont filterTagButtonFont = appFont;
    Util::changeFontSize(filterTagButtonFont, Util::FontResizeIntensity::AVERAGE, true,
        "EditScenarioDialog - tag filter buton and combo");
    ui->filterTagsPushButton->setFont(filterTagButtonFont);
    ui->filterTagsCombinationComboBox->setFont(filterTagButtonFont);

    // Set color of "incomes" and "expenses" filter
    ui->incomesRadioButton->setStyleSheet(Util::getStyleSheetStringForColor(
        GbpController::getInstance().getIncomeColor()));
    ui->expensesRadioButton->setStyleSheet(Util::getStyleSheetStringForColor(
        GbpController::getInstance().getExpenseColor()));


    // use smaller font for action buttons
    QFont actionButtonFont = appFont;
    Util::changeFontSize(actionButtonFont, Util::FontResizeIntensity::WEAK, true,
        "EditScenarioDialog - action buttons");
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

    // Make full description view button smaller
    QFont descFullViewFont = appFont;
    Util::changeFontSize(descFullViewFont, Util::FontResizeIntensity::WEAK, true,
        "EditScenarioDialog - desc full view button");
    ui->fullViewPushButton->setFont(descFullViewFont);

    // Make variable inflation edit button smaller
    QFont varInflationEditFont = appFont;
    Util::changeFontSize(varInflationEditFont, Util::FontResizeIntensity::WEAK, true,
        "EditScenarioDialog - variable inflation edit button");
    ui->editGrowthPushButton->setFont(varInflationEditFont);

    // Filter by tags is disabled
    filterTags.clear();
    ui->filterTagsCheckBox->setChecked(false);
    setFilterTagsWidgetsVisibility(false);

    // Color FilterTags widgets
    ui->filterTagsCheckBox->setStyleSheet(Util::getStyleSheetStringForColor(
        Util::getOptimizedBlue()));

    // fill FilterTags combination Combobox
    ui->filterTagsCombinationComboBox->insertItem(0,tr("All required"),FilterTags::Mode::ALL);
    ui->filterTagsCombinationComboBox->insertItem(0,tr("At least one"),FilterTags::Mode::ANY);
    ui->filterTagsCombinationComboBox->insertItem(0,
        tr("None of those"),FilterTags::Mode::NOT);
    ui->filterTagsCombinationComboBox->setCurrentIndex(1);
    filterTags.setMode(FilterTags::Mode::ANY);

    // set buttons titles
    ui->applyPushButton->setText(tr("Apply changes"));
    ui->cancelPushButton->setText(tr("Cancel"));

    // Plain Text Edition Dialog, auto-destroyed by Qt
    editDescriptionDialog = new PlainTextEditionDialog(this);
    editDescriptionDialog->setModal(true);

    // Scenario Inflation edit dialog, auto-destroyed by Qt
    ecInflation = new EditVariableGrowthDialog(tr("Inflation"), locale, this);
    ecInflation->setModal(true);

    // Periodic Stream Def Edit dialog, auto-destroyed by Qt
    psCsdDialog = new EditPeriodicDialog(locale, this);
    psCsdDialog->setModal(true);

    // Irregular Stream Def Edit dialog, auto-destroyed by Qt
    irCsdDialog = new EditIrregularDialog(locale, this);
    irCsdDialog->setModal(true);

    // Manage Tags dialog, auto-destroyed by Qt
    manageTagsDlg = new ManageTagsDialog(locale, this);
    manageTagsDlg->setModal(true);

    // Edit Filter Tags, auto-destroyed by Qt
    setFilterTagsDlg = new ChooseTagsDialog(this);
    setFilterTagsDlg->setModal(true);

    // connect emitters & receivers for Dialogs : Variable Inflation Edition
    QObject::connect(this, &EditScenarioDialog::signalEditVariableInflationPrepareContent,
        ecInflation, &EditVariableGrowthDialog::slotPrepareContent);
    QObject::connect(ecInflation, &EditVariableGrowthDialog::signalEditVariableGrowthResult,
        this, &EditScenarioDialog::slotEditVariableInflationResult);
    QObject::connect(ecInflation, &EditVariableGrowthDialog::signalEditVariableGrowthCompleted,
        this, &EditScenarioDialog::slotEditVariableInflationCompleted);

    // connect emitters & receivers for Dialogs : Periodic Stream Def Edition
    QObject::connect(this, &EditScenarioDialog::signalEditPeriodicCsdPrepareContent,
        psCsdDialog, &EditPeriodicDialog::slotPrepareContent);
    QObject::connect(psCsdDialog, &EditPeriodicDialog::signalEditPeriodicCsdResult,
        this, &EditScenarioDialog::slotEditPeriodicCsdResult);
    QObject::connect(psCsdDialog, &EditPeriodicDialog::signalEditPeriodicCsdCompleted,
        this, &EditScenarioDialog::slotEditPeriodicCsdCompleted);

    // connect emitters & receivers for Dialogs : Irregular Stream Def Edition
    QObject::connect(this, &EditScenarioDialog::signalEditIrregularCsdPrepareContent,
        irCsdDialog, &EditIrregularDialog::slotPrepareContent);
    QObject::connect(irCsdDialog, &EditIrregularDialog::signalEditIrregularCsdResult,
        this, &EditScenarioDialog::slotEditIrregularCsdResult);
    QObject::connect(irCsdDialog, &EditIrregularDialog::signalEditIrregularCsdCompleted,
        this, &EditScenarioDialog::slotEditIrregularCsdCompleted);

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
        setFilterTagsDlg, &ChooseTagsDialog::slotPrepareContent);
    QObject::connect(setFilterTagsDlg, &ChooseTagsDialog::signalResult,
        this, &EditScenarioDialog::slotSetFilterTagsResult);
    QObject::connect(setFilterTagsDlg, &ChooseTagsDialog::signalCompleted,
        this, &EditScenarioDialog::slotSetFilterTagsCompleted);

    // Constructor is completed
    started = true;

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


void EditScenarioDialog::slotPrepareContent(CurrencyInfo newCurrInfo)
{
    // Get current scenario, if any. There must be one.
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario.isNull()){
        return; // if no scenario loaded (should not happen)
    }

    QDate maxDate = GbpController::getInstance().getToday()
        .addYears(Constants::DEFAULT_DURATION_FE_GENERATION);

    // *** Copy properties of the current scenario, so we can work on a copy instead of
    // *** the current one scenario. Some properties will go directly in UI components when
    // *** it is possible

    this->currencyIsoCode = newCurrInfo.isoCode;
    this->currInfo = newCurrInfo;
    tempVariableInflation = Growth::fromVariableDataAnnualBasisDecimal(QMap<QDate, qint64>());

    /*  The Csds of the scenario are DEEP copied, that is ID and name stay identical, but
        we get new (so independant) QSharedPointer to each CSD. We have to remember that they are
        referenced by data structures located elsewhere (e.g. main Cash Balance curve), so great
        care must be taken to preserve them if data must not be regenerated. These will be the
        working copies : they will be modified while editing the scenario data in the dialog.
        The current scenario stays untouched as long as "APPLY" button has not been pressed.
    */
    incomesPeriodicCsd = PeriodicCsd::deepCopyHashmap(scenario->getIncomePeriodicCsds());
    incomesIrregularCsd = IrregularCsd::deepCopyHashmap(scenario->getIncomeIrregularCsds());
    expensesPeriodicCsd = PeriodicCsd::deepCopyHashmap(scenario->getExpensePeriodicCsds());
    expensesIrregularCsd = IrregularCsd::deepCopyHashmap(scenario->getExpenseIrregularCsds());

    // copy current tags and relationships
    tags = scenario->getTags();
    tagCsdRelationships = scenario->getTagCsdRelationships();

    // fill fields from current scenario
    ui->scenarioNameLineEdit->setText(scenario->getName());
    ui->DescPlainTextEdit->setPlainText(scenario->getDescription());
    ui->maxDurationSpinBox->setValue(scenario->getFeGenerationDuration());

    // manage scenario inflation
    if (scenario->getInflation().getType()==Growth::Type::CONSTANT){
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

    // Filter Tags : we try to keep the previous filter tags, in case this is the same scenario
    // that we edited last time. checkAndAdjustFilterTags will take care to preserve the filter
    // tags if possible, removing what does not exist anymore. UI components are adjusted.
    checkAndAdjustFilterTags() ;

    // **********************************************************************************


    this->setWindowTitle(tr("Edit current scenario"));

    // refresh the table content
    refreshCsdTableContent();

    ui->scenarioNameLineEdit->setFocus();

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


void EditScenarioDialog::slotEditPeriodicCsdResult(bool isIncome,
    QSharedPointer<PeriodicCsd> pCsd)
{
    // add the new or edited csd
    if (pCsd->getIsIncome()==true) {
        incomesPeriodicCsd.insert(pCsd->getId(), pCsd);
    } else {
        expensesPeriodicCsd.insert(pCsd->getId(), pCsd);
    }

    // update table content
    refreshCsdTableContent();

    // select the new/edited item
    QList<QUuid> list = QList<QUuid>();
    list.append(pCsd->getId());
    selectRowsInTableView(list); // select if displayed

    // make sure it is visible in the viewport of the table
    bool found;
    int row = itemTableModel->getRow(pCsd->getId(),found);
    if (found){
        QModelIndex index = itemTableModel->index(row,0);
        // does not always work...
        ui->itemsTableView->scrollTo(index,QAbstractItemView::PositionAtCenter);
    }
}


// the edit/creation process of PeriodocSimpleCsd has completed
void EditScenarioDialog::slotEditPeriodicCsdCompleted()
{
}


void EditScenarioDialog::slotEditIrregularCsdResult(bool isIncome,
    QSharedPointer<IrregularCsd> irCsd)
{
    // add the new or edited csd
    if (irCsd->getIsIncome()==true) {
        incomesIrregularCsd.insert(irCsd->getId(), irCsd);
    } else {
        expensesIrregularCsd.insert(irCsd->getId(), irCsd);
    }

    // update table content
    refreshCsdTableContent();

    // select the edited/new item
    QList<QUuid> list = QList<QUuid>();
    list.append(irCsd->getId());
    selectRowsInTableView(list);

    // make sure it is visible in the viewport of the table
    bool found;
    int row = itemTableModel->getRow(irCsd->getId(),found);
    if (found){
        QModelIndex index = itemTableModel->index(row,0);
        // does not always work...
        ui->itemsTableView->scrollTo(index,QAbstractItemView::PositionAtCenter);
    }
}


void EditScenarioDialog::slotEditIrregularCsdCompleted()
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


/**
 * @brief Result of the tag selection Dialog. Combination is left unchanged, as it is not
 * handled by the Dialog.
 * @param filterTagIdSet The list of tags selected.
 */
void EditScenarioDialog::slotSetFilterTagsResult(QSet<QUuid> filterTagIdSet)
{
    filterTags.setFilterTagIdSet(filterTagIdSet);

    // update table's content
    refreshCsdTableContent();
}


/**
 * @brief Handle completion of the Tag Selection Dialog. Tags' combination mode is not touched
 * in any case.
 * @details If user canceled, filterTags could stay empty while "filterByTags" is enabled.
 * It does not make sense, so in that case, one disables by force "filterByTags".
 * @param canceled
 */
void EditScenarioDialog::slotSetFilterTagsCompleted(bool canceled)
{
    if ( (canceled==true) && (ui->filterTagsCheckBox->isChecked()==true) &&
        (filterTags.getFilterTagIdSet().size()==0) ) {
        filterTags.clear(); // clear obly the tags, let the combination mode untouched.
        ui->filterTagsCheckBox->setChecked(false);
        setFilterTagsWidgetsVisibility(false);
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


void EditScenarioDialog::onAddMenuPeriodicClicked()
{
    bool isIncome = ui->incomesRadioButton->isChecked();
    QDate maxDate = GbpController::getInstance().getToday().addYears(
        ui->maxDurationSpinBox->value());
    // launch the edit dialog to create a new Periodic FSD
    emit signalEditPeriodicCsdPrepareContent(true, isIncome, QWeakPointer<PeriodicCsd>(),
                                             currInfo, getInflationCurrentlyDefined(), maxDate,{},tags);
    psCsdDialog->show();
}


void EditScenarioDialog::onAddMenuIrregularClicked()
{
    // TODO
    bool isIncome = ui->incomesRadioButton->isChecked();
    QDate maxDate = GbpController::getInstance().getToday().addYears(
        ui->maxDurationSpinBox->value());
    emit signalEditIrregularCsdPrepareContent(true, isIncome, QWeakPointer<IrregularCsd>(),
                                              currInfo, maxDate,{},tags);
    irCsdDialog->show();
}


void EditScenarioDialog::on_cancelPushButton_clicked()
{
    this->hide();

    // clear useless variables that may use some space : they will be repopulated
    // at the next call to slotPrepareContent
    incomesPeriodicCsd.clear();
    incomesIrregularCsd.clear();
    expensesPeriodicCsd.clear();
    expensesIrregularCsd.clear();
    tags.clear();
    tagCsdRelationships.clear();

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
    // *** gather & validate some data in the form to create a new Scenario ***
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

    LOG_INFO(QString("Attempting to apply changes made to a scenario named %1").arg(REDACT(name)));

    // *** Create a completely new scenario from the edit dialog data.
    // *** Note that all Csd QSharedPointers will be different from the current scenario,
    // *** which creates a problem if data does not need to be regenerated, since they
    // *** are referenced elsewhere by WeakPointers. We will address this later.
    QSharedPointer<Scenario> newScenario;
    try {
        newScenario = QSharedPointer<Scenario>(new Scenario(
            Scenario::LATEST_VERSION, name, desc, maxDuration, inflation, currencyIsoCode,
            incomesPeriodicCsd, incomesIrregularCsd, expensesPeriodicCsd, expensesIrregularCsd,
            tags, tagCsdRelationships));
    } catch (const std::exception& e) {
        // we should not get any exception...but plan for the worst
        QString errorString = QString(tr("An unexpected error has occurred while creating a "
            "scenario.\n\nDetails : %1"))
            .arg(e.what());
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            errorString, {tr("OK")}, 0, 0);
        LOG_ERROR( QString("    Modification to this existing scenario failed, unexpected"
            " exception occurred : %1").arg(e.what()));
        LOG_INFO("Apply changes has completed");
        return;
    }

    // *** Evaluate the type of changes made to the scenario.  There are 2 options :
    // *** 1) Financial events list will be different (e.g. new/deleted/enabled/disabled Csd,
    //        max duration modified, amounts changed, etc) in which case data will have to be
    //        regenerated before chart is refreshed.
    // *** 2) Financial events list will be different : only cosmetic changes or no change
    bool regenerateData = true;
    // Get current scenario(if any loaded)
    QSharedPointer<Scenario> currScenario = GbpController::getInstance().getScenario()
        .toStrongRef();
    if(currScenario.isNull()==false){
        // compare
        QString diff;
        regenerateData = !(currScenario->evaluateIfSameFeStream(newScenario, diff));
        if(regenerateData==true){
            LOG_INFO(QString("    Changes made will trigger a full recalculation of data : %1")
                .arg(diff));
        } else {
            LOG_INFO("    Changes made will NOT trigger a full recalculation of data");
        }
    } else {
        LOG_ERROR("    There is no current scenario (should not happen)");
        LOG_INFO("Apply changes has completed");
        return ; // should not happen
    }

    // The tricky part...If data will not be re-generated, we want to keep the same
    // Csd QSharedpointers found in the current scenario, because they are referenced elsewhere in
    // the app. But set their contents with what/ has been changed during the edition (newScenario).
    if (regenerateData == false) {

        // --- Incomes, Periodic ---
        {
            auto curHash = currScenario->getIncomePeriodicCsds();
            auto newHash = newScenario->getIncomePeriodicCsds();
            for (auto it = curHash.begin(); it != curHash.end(); ++it) {
                const QUuid id = it.key();
                QSharedPointer<PeriodicCsd>& curCsd = it.value();// non-const ref editable in place
                QSharedPointer<PeriodicCsd> newCsd =
                    newHash.value(id,QSharedPointer<PeriodicCsd>());
                if (newCsd.isNull() || curCsd.isNull()) { // Should never happen
                    LOG_ERROR(QString("    CSD id=%1 : newCsd.isNull() || curCsd.isNull()")
                        .arg(id.toString()));
                    LOG_INFO("Apply changes has completed");
                    return;
                }
                *curCsd = *newCsd; // copy "new" into "current" csd, keep the same QShardPointer
            }
            newScenario->setIncomePeriodicCsds(curHash);// Swap the Hashmap
        }

        // --- Incomes, Irregular ---
        {
            auto curHash = currScenario->getIncomeIrregularCsds();
            auto newHash = newScenario->getIncomeIrregularCsds();
            for (auto it = curHash.begin(); it != curHash.end(); ++it) {
                const QUuid id = it.key();
                QSharedPointer<IrregularCsd>& curCsd = it.value();// non-const ref editable in place
                QSharedPointer<IrregularCsd>newCsd =
                    newHash.value(id,QSharedPointer<IrregularCsd>());
                if (newCsd.isNull() || curCsd.isNull()) { // Should never happen
                    LOG_ERROR(QString("    CSD id=%1 : newCsd.isNull() || curCsd.isNull()")
                        .arg(id.toString()));
                    LOG_INFO("Apply changes has completed");
                    return;
                }
                *curCsd = *newCsd; // copy "new" into "current" csd, keep the same QShardPointer
            }
            newScenario->setIncomeIrregularCsds(curHash);// Swap the Hashmap
        }

        // --- Expenses, Periodic ---
        {
            auto curHash = currScenario->getExpensePeriodicCsds();
            auto newHash = newScenario->getExpensePeriodicCsds();
            for (auto it = curHash.begin(); it != curHash.end(); ++it) {
                const QUuid id = it.key();
                QSharedPointer<PeriodicCsd>& curCsd = it.value();// non-const ref editable in place
                QSharedPointer<PeriodicCsd> newCsd =
                    newHash.value(id,QSharedPointer<PeriodicCsd>());
                if (newCsd.isNull() || curCsd.isNull()) { // Should never happen
                    LOG_ERROR(QString("    CSD id=%1 : newCsd.isNull() || curCsd.isNull()")
                        .arg(id.toString()));
                    LOG_INFO("Apply changes has completed");
                    return;
                }
                *curCsd = *newCsd; // copy "new" into "current" csd, keep the same QShardPointer
            }
            newScenario->setExpensePeriodicCsds(curHash);// Swap the Hashmap
        }

        // --- Expenses, Irregular ---
        {
            auto curHash = currScenario->getExpenseIrregularCsds();
            auto newHash = newScenario->getExpenseIrregularCsds();
            for (auto it = curHash.begin(); it != curHash.end(); ++it) {
                const QUuid id = it.key();
                QSharedPointer<IrregularCsd>& curCsd = it.value();// non-const ref editable in place
                QSharedPointer<IrregularCsd>newCsd =
                    newHash.value(id,QSharedPointer<IrregularCsd>());
                if (newCsd.isNull() || curCsd.isNull()) { // Should never happen
                    LOG_ERROR(QString("    CSD id=%1 : newCsd.isNull() || curCsd.isNull()")
                        .arg(id.toString()));
                    LOG_INFO("Apply changes has completed");
                    return;
                }
                *curCsd = *newCsd; // copy "new" into "current" csd, keep the same QShardPointer
            }
            newScenario->setExpenseIrregularCsds(curHash);// Swap the Hashmap
        }

    }

    // we dont need the current scenario Strong Reference
    currScenario.reset();

    // *** switch to the new/editted scenario
    GbpController::getInstance().setScenario(newScenario);

    // Make our own internal copies of Csd Hashes independant again. We have to do that
    // because of the QSharedPointer issue.
    currScenario = GbpController::getInstance().getScenario().toStrongRef();
    incomesPeriodicCsd = PeriodicCsd::deepCopyHashmap(currScenario->getIncomePeriodicCsds());
    incomesIrregularCsd = IrregularCsd::deepCopyHashmap(currScenario->getIncomeIrregularCsds());
    expensesPeriodicCsd = PeriodicCsd::deepCopyHashmap(currScenario->getExpensePeriodicCsds());
    expensesIrregularCsd = IrregularCsd::deepCopyHashmap(currScenario->getExpenseIrregularCsds());

    // *** log the changes
    LOG_INFO(  "    Modifications to the existing scenario have been applied "
        "(but not saved yet)");
    LOG_DEBUG_INFO( QString("      Name = %1")
        .arg(REDACT(currScenario->getName())));
    LOG_DEBUG_INFO( QString("      Currency ISO code = %1")
        .arg(currScenario->getCurrencyIsoCode()));
    LOG_DEBUG_INFO( QString("      Version = %1")
        .arg(currScenario->getVersion()));
    LOG_DEBUG_INFO( QString("      Fe Generation Duration = %1")
        .arg(currScenario->getFeGenerationDuration()));
    LOG_DEBUG_INFO( QString("      No of periodic incomes = %1")
        .arg(currScenario->getIncomePeriodicCsds().size()));
    LOG_DEBUG_INFO( QString("      No of irregular incomes = %1")
        .arg(currScenario->getIncomeIrregularCsds().size()));
    LOG_DEBUG_INFO( QString("      No of periodic expenses = %1")
        .arg(currScenario->getExpensePeriodicCsds().size()));
    LOG_DEBUG_INFO( QString("      No of irregular expenses = %1")
        .arg(currScenario->getExpenseIrregularCsds().size()));
    LOG_INFO("Apply changes has completed");

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
        s = QString(tr("Cash stream definitions (%1)").arg(displayLocale.toString(noItems)));
    }
    ui->groupBox->setTitle(s);
    //ui->noItemsLabel->setText(s);
}


bool EditScenarioDialog::checkAndAdjustFilterTags()
{
    bool changed = false;

    // In current filterTags, remove tags that do not exist anymore, whatever
    // if FilterTagging is ON or OFF.
    QSet<QUuid> fTags = filterTags.getFilterTagIdSet();
    QSet<QUuid> iterationCopySet = fTags;
    foreach(QUuid filterTagId, iterationCopySet){
        if (tags.containsTagId(filterTagId)==false) {
            fTags.remove(filterTagId);
            changed = true;
        }
    }
    if(changed==true){
        filterTags.setFilterTagIdSet(fTags); // update
    }

    // if FilterByTags is ON and there is no tag left in filtertags, then
    // we consider it is abnormal (why filter by tag when no tag is specified ?)
    // and reset&disable filterTags
    if ( (ui->filterTagsCheckBox->isChecked()==true) && (fTags.size()==0) ) {
        filterTags.clear();
        ui->filterTagsCheckBox->setChecked(false);
        setFilterTagsWidgetsVisibility(false);
        changed = true;
    }

    return changed;
}


void EditScenarioDialog::refreshCsdTableContent()
{
    itemTableModel->refresh(currInfo, incomesPeriodicCsd, incomesIrregularCsd,
        expensesPeriodicCsd, expensesIrregularCsd, tags, tagCsdRelationships,
        ui->incomesRadioButton->isChecked(), ui->expensesRadioButton->isChecked(),
        ui->filterPeriodicsCheckBox->isChecked(), ui->filterIrregularsCheckBox->isChecked(),
        ui->filterEnabledCheckBox->isChecked(), ui->filterDisabledCheckBox->isChecked(),
        filterTags, ui->filterTagsCheckBox->isChecked());

    // refresh indicator of the no of items in the table
    updateNoItemsLabel();
}



QUuid EditScenarioDialog::duplicateCsd(QUuid id, bool &found)
{
    found = false;
    QUuid newId;

    // find the Csd and duplicate
    if (incomesPeriodicCsd.contains(id)) {
        found = true;
        QSharedPointer<PeriodicCsd> p = incomesPeriodicCsd.value(id);
        QSharedPointer<PeriodicCsd> p2 = p->duplicate(false, false);
        incomesPeriodicCsd.insert(p2->getId(),p2);
        newId = p2->getId();
    } else if (incomesIrregularCsd.contains(id)) {
        found = true;
        QSharedPointer<IrregularCsd> p = incomesIrregularCsd.value(id);
        QSharedPointer<IrregularCsd> p2 = p->duplicate(false, false);
        incomesIrregularCsd.insert(p2->getId(),p2);
        newId = p2->getId();
    } else if (expensesPeriodicCsd.contains(id)) {
        found = true;
        QSharedPointer<PeriodicCsd> p = expensesPeriodicCsd.value(id);
        QSharedPointer<PeriodicCsd> p2 = p->duplicate(false, false);
        expensesPeriodicCsd.insert(p2->getId(),p2);
        newId = p2->getId();
    } else if (expensesIrregularCsd.contains(id)) {
        found = true;
        QSharedPointer<IrregularCsd> p = expensesIrregularCsd.value(id);
        QSharedPointer<IrregularCsd> p2 = p->duplicate(false, false);
        expensesIrregularCsd.insert(p2->getId(),p2);
        newId = p2->getId();
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
        if (incomesPeriodicCsd.contains(id)) {
            incomesPeriodicCsd.remove(id);
        }
        if (incomesIrregularCsd.contains(id)) {
            incomesIrregularCsd.remove(id);
        }
        if (expensesPeriodicCsd.contains(id)) {
            expensesPeriodicCsd.remove(id);
        }
        if (expensesIrregularCsd.contains(id)) {
            expensesIrregularCsd.remove(id);
        }
    }
}


void EditScenarioDialog::enableDisableCsds(QList<QUuid> idList, bool enable)
{
    foreach(QUuid id, idList){
        if (incomesPeriodicCsd.contains(id)) {
            QSharedPointer<PeriodicCsd> p = incomesPeriodicCsd.value(id);
            p->setActive(enable);
            continue;
        }
        if (incomesIrregularCsd.contains(id)) {
            QSharedPointer<IrregularCsd> p = incomesIrregularCsd.value(id);
            p->setActive(enable);
            continue;
        }
        if (expensesPeriodicCsd.contains(id)) {
            QSharedPointer<PeriodicCsd> p = expensesPeriodicCsd.value(id);
            p->setActive(enable);
            continue;
        }
        if (expensesIrregularCsd.contains(id)) {
            QSharedPointer<IrregularCsd> p = expensesIrregularCsd.value(id);
            p->setActive(enable);
            continue;
        }
    }
}


void EditScenarioDialog::setFilterTagsWidgetsVisibility(bool visible)
{
    if (visible == true) {
        ui->filterTagsPushButton->setVisible(true);
        ui->filterTagsCombinationComboBox->setVisible(true);
    } else {
        ui->filterTagsPushButton->setVisible(false);
        ui->filterTagsCombinationComboBox->setVisible(false);
    }
}


// Edition requested of the selected Csd
void EditScenarioDialog::on_editPushButton_clicked()
{
        bool found;

        // make sure exactly 1 row is selected and get the ID of the seleted Csd
        QList<QUuid> selectedIdList = getSelection();
        if (selectedIdList.size()!=1){
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
                tr("Select exactly one row"), {tr("OK")}, 0, 0);
            ui->itemsTableView->setFocus();    // fix the strange behavior
            return;
        }
        // get the associated Csd type, to redirect to the proper Edit Dialog
        Csd::CsdType type = getCsdTypeFromId(selectedIdList.at(0),found);
        if (found==false) {
            return; // should not happen
        }
        QDate maxDate = GbpController::getInstance().getToday().addYears(
            ui->maxDurationSpinBox->value());
        if (type == Csd::CsdType::PERIODIC){
            QSharedPointer<PeriodicCsd> ps = getPeriodicCsdFromId(selectedIdList.at(0),found);
            if (found==false) {
                return; // should not happen
            }
            // Gather the tag id set this csd is associated with
            QSet<QUuid> tSet = tagCsdRelationships.getRelationshipsForCsd(ps->getId());
            // Launch the edit dialog
            emit signalEditPeriodicCsdPrepareContent(false,
                ui->incomesRadioButton->isChecked(), ps.toWeakRef(), currInfo,
                    getInflationCurrentlyDefined(),
                maxDate, tSet, tags);
            psCsdDialog->show();
        } else {
            QSharedPointer<IrregularCsd> is =  getIrregularCsdFromId(selectedIdList.at(0),found);;
            if (found==false) {
                return; // should not happen
            }
            // Gather the tag id set this fsd is associated with
            QSet<QUuid> tSet = tagCsdRelationships.getRelationshipsForCsd(is->getId());
            // Launch the edit dialog
            emit signalEditIrregularCsdPrepareContent(false,
                ui->incomesRadioButton->isChecked(), is.toWeakRef(), currInfo, maxDate, tSet, tags);
            irCsdDialog->show();
        }

}


// Duplicate the selected Csds, along with all their respective tag relationships
void EditScenarioDialog::on_duplicatePushButton_clicked()
{
    bool found;

    // make sure exactly 1 row is selected
    QList<QUuid> selectedIdList = getSelection();
    if (selectedIdList.size()==0){
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            tr("Select at least 1 item"), {tr("OK")}, 0, 0);
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
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            tr("Select at least one item"), {tr("OK")}, 0, 0);
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
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            tr("Select at least one item"), {tr("OK")}, 0, 0);
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
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            tr("Select at least one item"), {tr("OK")}, 0, 0);
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
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            tr("Select at least one item"), {tr("OK")}, 0, 0);
        ui->itemsTableView->setFocus();    // fix the strange behavior
        return;
    }

    // This is the custom color that will be chosen, if custom selection is requested.
    QColor color;

    // Ask if we have to revert to default system color or rather custom one
    int choice = GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::QUESTION, tr("Question"),
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
        if (incomesPeriodicCsd.contains(id)==true) {
            QSharedPointer<PeriodicCsd> ps = incomesPeriodicCsd.value(id);
            ps->setDecorationColor(color);
            incomesPeriodicCsd.insert(id,ps);
        } else if (incomesIrregularCsd.contains(id)==true) {
            QSharedPointer<IrregularCsd> is = incomesIrregularCsd.value(id);
            is->setDecorationColor(color);
            incomesIrregularCsd.insert(id,is);
        } else if (expensesPeriodicCsd.contains(id)==true) {
            QSharedPointer<PeriodicCsd> ps = expensesPeriodicCsd.value(id);
            ps->setDecorationColor(color);
            expensesPeriodicCsd.insert(id,ps);
        } else if (expensesIrregularCsd.contains(id)==true) {
            QSharedPointer<IrregularCsd> is = expensesIrregularCsd.value(id);
            is->setDecorationColor(color);
            expensesIrregularCsd.insert(id,is);
        }
    }

    // update table
    refreshCsdTableContent();
}


void EditScenarioDialog::on_incomesRadioButton_toggled(bool checked)
{
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
    foreach (QSharedPointer<PeriodicCsd> f, incomesPeriodicCsd) {
        newCsdItems.insert(f->getId(),{.id=f->getId(),.name=f->getName(),
            .isIncome=f->getIsIncome(),.color=f->getDecorationColor()});
    }
    foreach (QSharedPointer<IrregularCsd> f, incomesIrregularCsd) {
        newCsdItems.insert(f->getId(), {.id=f->getId(),.name=f->getName(),
            .isIncome=f->getIsIncome(),.color=f->getDecorationColor()});
    }
    foreach (QSharedPointer<PeriodicCsd> f, expensesPeriodicCsd) {
        newCsdItems.insert(f->getId(),{.id=f->getId(),.name=f->getName(),
            .isIncome=f->getIsIncome(), .color=f->getDecorationColor()});
    }
    foreach (QSharedPointer<IrregularCsd> f, expensesIrregularCsd) {
        newCsdItems.insert(f->getId(),{.id=f->getId(),.name=f->getName(),
            .isIncome=f->getIsIncome(),.color=f->getDecorationColor()});
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
        setFilterTagsWidgetsVisibility(false);
        // Refresh table
        refreshCsdTableContent();
    } else if (arg1==Qt::CheckState::Checked) {
        // *** FILTER BY TAGS IS NOW ENABLED ***
        // *** Current filterTag is re-used ***
        // *** There must be at least one tag defined ***
        if (tags.size()==0) {
            GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Warning"),
                tr("There are no tag defined for this scenario, so you cannot "
                    "use Tag-based filtering."), {tr("OK")}, 0, 0);
            // revert the state of the checkbox, a new state signal will be emitted and received
            // by this function. Use setCheckState because setChecked does not work as expected
            ui->filterTagsCheckBox->setCheckState(Qt::CheckState::Unchecked);

            return;
        }

        setFilterTagsWidgetsVisibility(true);
        if ( (filterTags.getFilterTagIdSet().size()==0) ) {
            // if no tag have been added to the filter set, so send user to the
            // tag selection dialog.
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
    emit signalSetFilterTagsPrepareContent(tags, filterTags.getFilterTagIdSet());
    setFilterTagsDlg->show();
}


// Return the type of Csd having "id" as ID. Check value of "found" upon return to be sure
// return value is meaningful.
Csd::CsdType EditScenarioDialog::getCsdTypeFromId(QUuid id, bool &found)
{
    found = false;
    if (incomesPeriodicCsd.contains(id)) {
        found = true;
        return Csd::CsdType::PERIODIC;
    } else if (incomesIrregularCsd.contains(id)) {
        found = true;
        return Csd::CsdType::IRREGULAR;
    } else if (expensesPeriodicCsd.contains(id)) {
        found = true;
        return Csd::CsdType::PERIODIC;
    } else if (expensesIrregularCsd.contains(id)) {
        found = true;
        return Csd::CsdType::IRREGULAR;
    }
    return Csd::CsdType::PERIODIC; // dummy value since not found
}



QSharedPointer<PeriodicCsd> EditScenarioDialog::getPeriodicCsdFromId(QUuid id, bool &found)
{
    found = false;

    QSharedPointer<PeriodicCsd> p = incomesPeriodicCsd.value(id, QSharedPointer<PeriodicCsd>());
    if (p.isNull()==false) {
        found = true;
        return p;
    }

    QSharedPointer<PeriodicCsd> p2 = expensesPeriodicCsd.value(id, QSharedPointer<PeriodicCsd>());
    if (p2.isNull()==false) {
        found = true;
        return p2;
    }

    return QSharedPointer<PeriodicCsd>(); // dummy value since not found
}


// Return an Irregular Csd (income or expense) having "id" as ID. Check value of "found" upon
// return to be sure return value is meaningful.
QSharedPointer<IrregularCsd> EditScenarioDialog::getIrregularCsdFromId(QUuid id, bool &found)
{
    found = false;

    QSharedPointer<IrregularCsd> p = incomesIrregularCsd.value(id, QSharedPointer<IrregularCsd>());
    if (p.isNull()==false) {
        found = true;
        return p;
    }

    QSharedPointer<IrregularCsd> p2 = expensesIrregularCsd.value(id, QSharedPointer<IrregularCsd>());
    if (p2.isNull()==false) {
        found = true;
        return p2;
    }


    return QSharedPointer<IrregularCsd>(); // dummy value since not found
}


/**
 * @brief Called when filter tag combination mode selection is changed. WARNING : this is
 * also called when we insert item in the combobox at init time
 * @param index Rank of the new selected item
 */
void EditScenarioDialog::on_filterTagsCombinationComboBox_currentIndexChanged(int index)
{
    if (started==false){
        return;
    }
    // insert the selected combination mode into filterTags
    QVariant qvar = ui->filterTagsCombinationComboBox->currentData();
    FilterTags::Mode m = qvar.value<FilterTags::Mode>();
    filterTags.setMode(m);

    // update accordingly the CSD list
    refreshCsdTableContent();
}


void EditScenarioDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    // QTimer::singleShot(0) defers execution until after all show-related events have been
    // processed, including the platform style's focusInEvent which calls selectAll() on the
    // focused QLineEdit. Calling deselect() directly in slotPrepareContent() has no effect
    // because that slot runs before the dialog is shown.
    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("EditScenarioDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
        ui->scenarioNameLineEdit->deselect();
    });
}

