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

#include "editperiodicdialog.h"
#include "ui_editperiodicdialog.h"
#include "gbpcontroller.h"
#include "util.h"
#include "periodicfestreamdef.h"
#include <QMessageBox>
#include <QCoreApplication>
#include <QColorDialog>

EditPeriodicDialog::EditPeriodicDialog(QLocale aLocale, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditPeriodicDialog),
    locale(aLocale)
{
    ui->setupUi(this);


    // fill Period combobox
    ui->periodComboBox->insertItem(0,Util::getPeriodName(
        Util::PeriodType::YEARLY,true,false),PeriodicFeStreamDef::PeriodType::YEARLY);
    ui->periodComboBox->insertItem(0,Util::getPeriodName(
        Util::PeriodType::END_OF_MONTHLY,true,false),
        PeriodicFeStreamDef::PeriodType::END_OF_MONTHLY);
    ui->periodComboBox->insertItem(0,Util::getPeriodName(
        Util::PeriodType::MONTHLY,true,false),PeriodicFeStreamDef::PeriodType::MONTHLY);
    ui->periodComboBox->insertItem(0,Util::getPeriodName(
        Util::PeriodType::WEEKLY,true,false),PeriodicFeStreamDef::PeriodType::WEEKLY);
    ui->periodComboBox->insertItem(0,Util::getPeriodName(
        Util::PeriodType::DAILY,true,false),PeriodicFeStreamDef::PeriodType::DAILY);
    updatePeriodCombobox(PeriodicFeStreamDef::PeriodType::MONTHLY);

    // Fill Growth Combobox
    ui->growthComboBox->insertItem(0,tr("No growth"),
        QVariant::fromValue(GrowthType::NONE));
    ui->growthComboBox->insertItem(0,tr("Follow scenario's inflation"),
        QVariant::fromValue(GrowthType::SCENARIO));
    ui->growthComboBox->insertItem(0,tr("Custom - Constant"),
        QVariant::fromValue(GrowthType::CUSTOM_CONSTANT));
    ui->growthComboBox->insertItem(0,tr("Custom - Variable"),
        QVariant::fromValue(GrowthType::CUSTOM_VARIABLE));

    // Init value of Growth Combobox
    updateGrowthTypeCombobox(GrowthType::SCENARIO);

    // set some Labels programatically (otherwise, the minimum window size is bit big)
    ui->growthTypePreLabel->setText(tr("Multiplier : "));
    ui->growthTypePostLabel->setText(tr(" on annual basis"));

    // use smaller font for description list
    QFont descFont = ui->descPlainTextEdit->font();
    uint oldFontSize = descFont.pointSize();
    uint newFontSize = Util::changeFontSize(2,true, oldFontSize);
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        QString("Edit periodic dialog - Description - Font size set from %1 to %2").
        arg(oldFontSize).arg(newFontSize));
    descFont.setPointSize(newFontSize);
    ui->descPlainTextEdit->setFont(descFont);

    // use smaller font for tag list
    QFont tagFont = ui->tagsEdit->font();
    uint oldTagFontSize = tagFont.pointSize();
    uint newTagFontSize = Util::changeFontSize(2, true, oldTagFontSize);
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        QString("Edit periodic dialog - Tags - Font size set from %1 to %2").
        arg(oldTagFontSize).arg(newTagFontSize));
    tagFont.setPointSize(newTagFontSize);
    ui->tagsEdit->setFont(tagFont);

    // make the description list not too high (must be done after font setting)
    QFontMetrics fm(ui->descPlainTextEdit->font());
    ui->descPlainTextEdit->setFixedHeight(fm.height()*3 * 1.4); // 3 lines

    // make the tag list not too high (must be done after font setting)
    QFontMetrics fm2(ui->tagsEdit->font());
    ui->tagsEdit->setFixedHeight(fm2.height()*(2 * 1.4)); // 2 lines

    // make DateWidgets widget wide enough
    QFontMetrics fm3 = ui->fromDateEdit->fontMetrics();
    ui->fromDateEdit->setMinimumWidth(fm3.averageCharWidth()*20);
    ui->toDateEdit->setMinimumWidth(fm3.averageCharWidth()*20);

    // Set Date widgets to display date in Locale short format
    ui->fromDateEdit->setDisplayFormat(locale.dateFormat(QLocale::ShortFormat));
    ui->toDateEdit->setDisplayFormat(locale.dateFormat(QLocale::ShortFormat));

    // set "from" / "to" date default
    ui->toScenarioRadioButton->setChecked(true);
    ui->toDateEdit->setEnabled(false);
    ui->fromDateEdit->setDate(GbpController::getInstance().getTomorrow());
    ui->toDateEdit->setDate(GbpController::getInstance().getToday().addYears(
        Scenario::DEFAULT_DURATION_FE_GENERATION));

    // Set format of Growth Type "Scenario Inflation" ComboBox
    ui->growthTypeScenaroInflationDoubleSpinBox->setMinimum(0);
    ui->growthTypeScenaroInflationDoubleSpinBox->setMaximum(100);
    ui->growthTypeScenaroInflationDoubleSpinBox->setDecimals(5);

    // Set format of Growth Type "Custom Constant" ComboBox
    ui->growthTypeCustomConstantDoubleSpinBox->setMinimum(-100);
    ui->growthTypeCustomConstantDoubleSpinBox->setMaximum(10000);
    ui->growthTypeCustomConstantDoubleSpinBox->setDecimals(Growth::NO_OF_DECIMALS);
    ui->growthTypeCustomConstantDoubleSpinBox->setSuffix("%");

    // force max len for name (not possible for Description)
    ui->nameLineEdit->setMaxLength(FeStreamDef::NAME_MAX_LEN);

    // Stream Def variable growth edit dialog
    editVariableGrowthDlg = new EditVariableGrowthDialog(tr("Growth"),locale,this);// auto-destroyed
    editVariableGrowthDlg->setModal(true);
    // Visualize occurrences  Dialog
    visualizeoccurrencesDialog = new VisualizeOccurrencesDialog(locale,this);// auto-destroyed by Qt
    visualizeoccurrencesDialog->setModal(true);
    // Plain Text Edition Dialog
    editDescriptionDialog = new PlainTextEditionDialog(this);  // auto-destroyed by Qt
    editDescriptionDialog->setModal(true);


    // "pack" the dialog to fit the font. This is required when there is no "expanding" widgets
    this->adjustSize();

    // connect emitters & receivers for Dialogs : Variable Growth Edition
    QObject::connect(this, &EditPeriodicDialog::signalEditVariableGrowthPrepareContent,
        editVariableGrowthDlg, &EditVariableGrowthDialog::slotPrepareContent);
    QObject::connect(editVariableGrowthDlg,
        &EditVariableGrowthDialog::signalEditVariableGrowthResult, this,
        &EditPeriodicDialog::slotEditVariableGrowthResult);
    QObject::connect(editVariableGrowthDlg,
        &EditVariableGrowthDialog::signalEditVariableGrowthCompleted, this,
        &EditPeriodicDialog::slotEditVariableGrowthCompleted);
    // connect emitters & receivers for Dialogs : Description Edition
    QObject::connect(this, &EditPeriodicDialog::signalPlainTextDialogPrepareContent,
        editDescriptionDialog, &PlainTextEditionDialog::slotPrepareContent);
    QObject::connect(editDescriptionDialog, &PlainTextEditionDialog::signalPlainTextEditionResult,
        this, &EditPeriodicDialog::slotPlainTextEditionResult);
    QObject::connect(editDescriptionDialog,
        &PlainTextEditionDialog::signalPlainTextEditionCompleted, this,
        &EditPeriodicDialog::slotPlainTextEditionCompleted);
    // connect emitters & receivers for Dialogs : Visualize occurrences
    QObject::connect(this, &EditPeriodicDialog::signalVisualizeOccurrencesPrepareContent,
        visualizeoccurrencesDialog, &VisualizeOccurrencesDialog::slotPrepareContent);
    QObject::connect(visualizeoccurrencesDialog,
        &VisualizeOccurrencesDialog::signalCompleted, this,
        &EditPeriodicDialog::slotVisualizeOccurrencesCompleted);

}


EditPeriodicDialog::~EditPeriodicDialog()
{
    delete ui;
}


// Prepare for creating a new Periodic Stream or editing an existing one, before showing the Dialog
void EditPeriodicDialog::slotPrepareContent(bool isNewStreamDef, bool isIncome,
    PeriodicFeStreamDef psStreamDef, CurrencyInfo newCurrInfo, Growth inflation,
    QDate theMaxDateFeGeneration, QSet<QUuid> associatedTagIds, Tags availTags)
{
    // check input values
    if ( theMaxDateFeGeneration.isValid()==false ) {
        throw std::invalid_argument("Invalid max date for FeGenerationDuration");
    }
    if ( (isNewStreamDef==false) && ((psStreamDef.getStartDate().isValid()==false) ||
        psStreamDef.getEndDate().isValid()==false)) {
        throw std::invalid_argument("Invalid Validity Range value for this existing "
            "Periodic Cash Stream Definition");
    }

    // remember some variables
    this->editingExistingStreamDef = !isNewStreamDef;
    this->currInfo = newCurrInfo;
    this->isIncome = isIncome;
    this->scenarioInflation = inflation;
    maxDateFeGeneration = theMaxDateFeGeneration;
    tagIdSet = associatedTagIds;    // ids of the Tag this FSD is associated with
    availableTags = availTags; // All the tags defined in the scenario

    // some more settings
    ui->amountDoubleSpinBox->setDecimals(currInfo.noOfDecimal);
    ui->currencyIsoCodeLabel->setText(currInfo.isoCode);

    // set name of Label for end date for Scenario case
    ui->toScenarioRadioButton->setText(tr("Defined by the scenario (%1)").
        arg(locale.toString(maxDateFeGeneration,locale.dateFormat(QLocale::ShortFormat))));

    // Name colorization
    if (isNewStreamDef) {
        decorationColor = QColor(); // use normal color for new Stream Def
    } else {
        decorationColor = psStreamDef.getDecorationColor(); // can be normal or custom
    }
    if (decorationColor.isValid()==false) {
        // use normal color
        ui->decorationColorCheckBox->setChecked(false);
        ui->decorationColorCustomTextLabel->setEnabled(false);
        ui->decorationColorPushButton->setVisible(false);
    } else {
        // Use custom color for text
        ui->decorationColorCheckBox->setChecked(true);
        ui->decorationColorCustomTextLabel->setEnabled(true);
        ui->decorationColorPushButton->setVisible(true);
    }
    setDecorationColorInfo();

    if(editingExistingStreamDef){
        // *** editing an existing PeriodicFeStreamDef ***

        // remember the id
        initialId = psStreamDef.getId();

        // Set title
        if(isIncome){
            this->setWindowTitle(tr("Editing periodic income"));
        } else {
            this->setWindowTitle(tr("Editing periodic expense"));
        }
        ui->applyPushButton->setText(tr("Apply"));
        ui->closePushButton->setText(tr("Cancel"));
        ui->nameLineEdit->setText(psStreamDef.getName());
        ui->descPlainTextEdit->setPlainText(psStreamDef.getDesc());
        int result;
        double amountDouble = CurrencyHelper::amountQint64ToDouble(psStreamDef.getAmount(),
            currInfo.noOfDecimal, result);
        if(result!=0){
            // should never happen
            amountDouble = 0;
        }
        ui->amountDoubleSpinBox->setValue(amountDouble);
        ui->fromDateEdit->setDate(psStreamDef.getStartDate());

        // set "to" date : do not clear value (handy)
        if (psStreamDef.getUseScenarioForEndDate()==true) {
            // used scenario value for end date
            ui->toScenarioRadioButton->setChecked(true);
            ui->toDateEdit->setEnabled(false);
        } else {
            // use custom value for end date
            ui->toDateEdit->setDate(psStreamDef.getEndDate());
            ui->toCustomRadioButton->setChecked(true);
            ui->toDateEdit->setEnabled(true);
        }

        if (psStreamDef.getActive()){
            ui->activeYesRadioButton->setChecked(true);
        } else {
             ui->activeNoRadioButton->setChecked(true);
        }
        ui->gapSpinBox->setValue(psStreamDef.getGrowthApplicationPeriod());

        // Period type and multiplier
        updatePeriodCombobox(psStreamDef.getPeriod());
        ui->periodMultiplierSpinBox->setValue(psStreamDef.getPeriodMultiplier());

        // --- growth ---

        // in any case, rebuild and reset to empty
        tempVariableGrowth = Growth::fromVariableDataAnnualBasisDecimal(QMap<QDate,qint64>());

        Growth ag = psStreamDef.getGrowth();
        PeriodicFeStreamDef::GrowthStrategy gs = psStreamDef.getGrowthStrategy();

        if(gs == PeriodicFeStreamDef::GrowthStrategy::NONE){
            ui->growthTypeScenaroInflationDoubleSpinBox->setValue(1);
            ui->growthTypeCustomConstantDoubleSpinBox->setValue(0);
            updateGrowthTypeCombobox(GrowthType::NONE);

        } else if ( gs == PeriodicFeStreamDef::GrowthStrategy::INFLATION){
            ui->growthTypeScenaroInflationDoubleSpinBox->setValue(
                psStreamDef.getInflationAdjustmentFactor());
            ui->growthTypeCustomConstantDoubleSpinBox->setValue(0);
            updateGrowthTypeCombobox(GrowthType::SCENARIO);
        } else {
            // CUSTOM GROWTH
            if (ag.getType()==Growth::CONSTANT){
                qint64 d = ag.getAnnualConstantGrowth();
                double d2 = Growth::fromDecimalToDouble(d);
                ui->growthTypeScenaroInflationDoubleSpinBox->setValue(1);
                ui->growthTypeCustomConstantDoubleSpinBox->setValue(d2);
                updateGrowthTypeCombobox(GrowthType::CUSTOM_CONSTANT);

            }else{
                tempVariableGrowth = ag;    // stock it
                ui->growthTypeScenaroInflationDoubleSpinBox->setValue(1);
                ui->growthTypeCustomConstantDoubleSpinBox->setValue(0);
                updateGrowthTypeCombobox(GrowthType::CUSTOM_VARIABLE);
            }
        }
        // -----------------------------

        // Tags
        updateTagListTextBox();

    } else{

        // *** creating a new PeriodicFeStreamDef (value of psStreamDef then does not matter) ***

        initialId = QUuid::createUuid();

        // set title
        if(isIncome){
            this->setWindowTitle(tr("Creating periodic income"));
        } else {
            this->setWindowTitle(tr("Creating periodic expense"));
        }

        ui->applyPushButton->setText(tr("Create"));
        ui->closePushButton->setText(tr("Close"));

        // Init fields for a new Csd
        prepareDataToCreateANewStreamDef(true);
    }

    ui->nameLineEdit->setFocus();
}


void EditPeriodicDialog::slotEditVariableGrowthResult(Growth growthOut)
{
    tempVariableGrowth = growthOut;
}


void EditPeriodicDialog::slotEditVariableGrowthCompleted()
{
}


void EditPeriodicDialog::slotPlainTextEditionResult(QString result)
{
    ui->descPlainTextEdit->setPlainText(result);
}


void EditPeriodicDialog::slotPlainTextEditionCompleted()
{
}


// will never be called
void EditPeriodicDialog::slotShowResultResult(QString result)
{
}


void EditPeriodicDialog::slotShowResultCompleted()
{
}


void EditPeriodicDialog::slotVisualizeOccurrencesCompleted()
{
    // Log the operation
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
        QString("Visualize occurrences completed"));
}


void EditPeriodicDialog::on_applyPushButton_clicked()
{
    if(editingExistingStreamDef){
        GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Info,
            QString("Attempting to modify periodic item \"%1\" ...")
            .arg(ui->nameLineEdit->text()));
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Info,
            QString("Attempting to create new periodic item \"%1\" ...")
            .arg(ui->nameLineEdit->text()));
    }

    BuildFromFormDataResult result;
    buidlPeriodicFeStreamDefFromFormData(result);
    if (result.success==false){
        QMessageBox::critical(nullptr,tr("Error"),result.errorMessageUI);
        GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Info,
            QString("    Changes not applied : Invalid data entered (%1)")
            .arg(result.errorMessageLog));
        return;
    }

    // send back result
    emit signalEditPeriodicStreamDefResult (isIncome, result.pStreamDef);

    // if editing an existing Stream Def, then this is the end of it. For create,
    // then stay right there to facilitate the rapid creation of multiple Stream Def
    if(editingExistingStreamDef){
        hide();
        emit signalEditPeriodicStreamDefCompleted();
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
            QString("    Modifications applied to periodic item"));
    } else{
        // reset some parameters so we are ready to create yet another Stream Def
        prepareDataToCreateANewStreamDef(false);
        ui->nameLineEdit->setFocus();
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
            QString("    New periodic item created"));
    }
}

// Set form's widgets contents for creation of a new Stream Def
// If slotPrepare == true, some more field are initialized
void EditPeriodicDialog::prepareDataToCreateANewStreamDef(bool slotPrepare)
{
    initialId = QUuid::createUuid();
    ui->nameLineEdit->setText("");
    ui->descPlainTextEdit->setPlainText("");
    ui->amountDoubleSpinBox->setValue(0);

    // set from date
    ui->fromDateEdit->setDate(GbpController::getInstance().getTomorrow());

    // set "to" Date : do not touch the to date
    ui->toScenarioRadioButton->setChecked(true);
    ui->toDateEdit->setEnabled(false);

    ui->activeYesRadioButton->setChecked(true);
    ui->gapSpinBox->setValue(1);
    ui->periodMultiplierSpinBox->setValue(1);

    // inflation adjustment factor
    ui->growthTypeScenaroInflationDoubleSpinBox->setValue(1);

    // growth is constant & 0 by default if slotPrepare = true
    // Otherwise, dot touch it.
    if (slotPrepare==true) {
        ui->growthTypeScenaroInflationDoubleSpinBox->setValue(1);
        ui->growthTypeCustomConstantDoubleSpinBox->setValue(0);
        updateGrowthTypeCombobox(GrowthType::SCENARIO);
    }

    // but initialize an empty Variable Growth if ever user decide to switch to that type
    tempVariableGrowth = Growth::fromVariableDataAnnualBasisDecimal(QMap<QDate, qint64>());

    // decoration color
    ui->decorationColorCheckBox->setChecked(false);
    on_decorationColorCheckBox_clicked();

    // tags
    ui->tagsEdit->clear();
    tagIdSet.clear();   // new fsd will have no association with tags when created
}


void EditPeriodicDialog::buidlPeriodicFeStreamDefFromFormData(BuildFromFormDataResult &result)
{
    result.success = false;
    result.errorMessageUI = "";
    result.errorMessageLog = "";

    // check if Validity range data is valid, then build it
    QDate from = ui->fromDateEdit->date();
    if( !from.isValid()){
        result.errorMessageUI = tr("Start date is invalid");
        result.errorMessageLog = "Start date is invalid";
        return;
    }
    if (ui->toScenarioRadioButton->isChecked()==true) {
        if(maxDateFeGeneration<from){
            result.errorMessageUI = tr("End date as defined at the scenario level must not occur "
                "before the Start date");
            result.errorMessageLog = "End date as defined at the scenario level must not occur "
                "before the Start date";
            return;
        }
    }
    QDate to = ui->toDateEdit->date(); // to date is allowed to go over maxDate from the scenario
    if ( ui->toCustomRadioButton->isChecked()==true){
        if( !to.isValid()){
            result.errorMessageUI = tr("End date is invalid");
            result.errorMessageLog = "End date is invalid";
            return;
        }
        if(to<from){
            result.errorMessageUI = tr("End date must not occur before the Start date");
            result.errorMessageLog = "End date must not occur before the Start date";
            return;
        }
    } else{
        // just make sure the unused to date is still valid
        if( !to.isValid()){
            to = from;
        }
    }

    //*** gather and transform data to create a Periodic Stream Def ***

    QVariant selectedData = ui->periodComboBox->itemData(ui->periodComboBox->currentIndex());
    PeriodicFeStreamDef::PeriodType periodicType = static_cast<PeriodicFeStreamDef::PeriodType>(
        selectedData.toInt());
    int resultConv;
    qint64 amount = CurrencyHelper::amountDoubleToQint64(ui->amountDoubleSpinBox->value(),
        currInfo.noOfDecimal, resultConv);
    if (resultConv!=0){
        if (resultConv==-1){
            result.errorMessageUI = QString(tr("The amount cannot be bigger than %1"))
                .arg(CurrencyHelper::maxValueAllowedForAmountInDouble(currInfo.noOfDecimal));
            result.errorMessageLog = QString("The amount cannot be bigger than %1")
                .arg(CurrencyHelper::maxValueAllowedForAmountInDouble(currInfo.noOfDecimal));
            return;
        } else {
            // should never happen
            result.errorMessageUI = QString(tr("An error occured while processing the "
                "amount : code=%1")).arg(resultConv);
            result.errorMessageLog = QString("An error occured while processing the "
                "amount : code=%1").arg(resultConv);
            return;
        }
    }

    PeriodicFeStreamDef::GrowthStrategy gs;
    GrowthType gt = getGrowthTypeSelected();
    Growth growth; // type "NONE"

    // Set Growth Strategy
    switch (gt) {
        case GrowthType::NONE:
            gs = PeriodicFeStreamDef::GrowthStrategy::NONE;
            break;
        case GrowthType::SCENARIO:
            gs = PeriodicFeStreamDef::GrowthStrategy::INFLATION;
            break;
        case GrowthType::CUSTOM_CONSTANT:
            gs = PeriodicFeStreamDef::GrowthStrategy::CUSTOM;
            break;
        case GrowthType::CUSTOM_VARIABLE:
            gs = PeriodicFeStreamDef::GrowthStrategy::CUSTOM;
            break;
        default:
            break;
    }

    // Set Growth Object
    switch (gt) {
        case GrowthType::CUSTOM_CONSTANT:
            growth = Growth::fromConstantAnnualPercentageDouble(
                ui->growthTypeCustomConstantDoubleSpinBox->value());
            break;
        case GrowthType::CUSTOM_VARIABLE:
            growth = tempVariableGrowth;
            break;
        default:
            break;
    }

    qint16 periodMultiplier = static_cast<quint16>(ui->periodMultiplierSpinBox->value());
    quint16 gap = static_cast<quint16>(ui->gapSpinBox->value());

    // inflationModifFactor
    double inflationModifFactor = ui->growthTypeScenaroInflationDoubleSpinBox->value();

    // ********************************************

    // build item
    try {
        result.pStreamDef = PeriodicFeStreamDef(periodicType, periodMultiplier, amount, growth, gs,
            gap, initialId, (ui->nameLineEdit->text().trimmed()).left(FeStreamDef::NAME_MAX_LEN),
            ui->descPlainTextEdit->toPlainText().left(FeStreamDef::DESC_MAX_LEN),
            ui->activeYesRadioButton->isChecked(), isIncome, decorationColor, from, to,
            ui->toScenarioRadioButton->isChecked(),inflationModifFactor);
    } catch (const std::exception& e) {
        // unexpected error, should never happen
        result.errorMessageUI = QString(tr("An unexpected error has occured.\n\nDetails : %1"))
            .arg(e.what());
        result.errorMessageLog = QString("An unexpected error has occured.\n\nDetails : %1")
            .arg(e.what());
        return;
    }

    result.success = true;
    return;
}


// Set the selected item for Period Type combobox
void EditPeriodicDialog::updatePeriodCombobox(PeriodicFeStreamDef::PeriodType type)
{
    // highly DEPENDANT on order to insertion in Dialog init !
    if ( type == PeriodicFeStreamDef::PeriodType::DAILY){
        ui->periodComboBox->setCurrentIndex(0) ;
    } else if (type == PeriodicFeStreamDef::PeriodType::WEEKLY){
        ui->periodComboBox->setCurrentIndex(1) ;
    } else if (type == PeriodicFeStreamDef::PeriodType::MONTHLY){
        ui->periodComboBox->setCurrentIndex(2) ;
    } else if (type == PeriodicFeStreamDef::PeriodType::END_OF_MONTHLY){
        ui->periodComboBox->setCurrentIndex(3) ;
    } else if (type == PeriodicFeStreamDef::PeriodType::YEARLY){
        ui->periodComboBox->setCurrentIndex(4) ;
    }
}


// Change the visibility of components for Growth type, so that we see only
// what is relevant to the type. Properties are adjusted if required.
// Component's data are NOT filled with the proper value.
void EditPeriodicDialog::setVisibilityComponentsGrowthType(GrowthType type)
{
    switch (type) {
        case GrowthType::NONE:
            // visibility
            ui->growthTypePreLabel->setVisible(false);
            ui->growthTypePostLabel->setVisible(false);
            ui->growthTypeScenaroInflationDoubleSpinBox->setVisible(false);
            ui->growthTypeCustomConstantDoubleSpinBox->setVisible(false);
            ui->growthTypePushButton->setVisible(false);
            break;
        case GrowthType::SCENARIO:
            // visibility
            ui->growthTypePreLabel->setVisible(true);
            ui->growthTypePostLabel->setVisible(false);
            ui->growthTypeScenaroInflationDoubleSpinBox->setVisible(true);
            ui->growthTypeCustomConstantDoubleSpinBox->setVisible(false);
            ui->growthTypePushButton->setVisible(false);
            break;
        case GrowthType::CUSTOM_CONSTANT:
            // visibility
            ui->growthTypePreLabel->setVisible(false);
            ui->growthTypePostLabel->setVisible(true);
            ui->growthTypeScenaroInflationDoubleSpinBox->setVisible(false);
            ui->growthTypeCustomConstantDoubleSpinBox->setVisible(true);
            ui->growthTypePushButton->setVisible(false);
            break;
        case GrowthType::CUSTOM_VARIABLE:
            // visibility
            ui->growthTypePreLabel->setVisible(false);
            ui->growthTypePostLabel->setVisible(false);
            ui->growthTypeScenaroInflationDoubleSpinBox->setVisible(false);
            ui->growthTypeCustomConstantDoubleSpinBox->setVisible(false);
            ui->growthTypePushButton->setVisible(true);
            break;
        default:
            break;
    }
}


// Set the selected item for Growth Type combobox.
// This will trigger an event for change in current item
void EditPeriodicDialog::updateGrowthTypeCombobox(GrowthType type)
{
    // independant of insertion order of items
    for (int i = 0; i < ui->growthComboBox->count(); ++i) {
        QVariant qvar = ui->growthComboBox->itemData(i);
        GrowthType gt = qvar.value<GrowthType>();
        if(gt==type){
            ui->growthComboBox->setCurrentIndex(i);
            return;
        }
    }
}


EditPeriodicDialog::GrowthType EditPeriodicDialog::getGrowthTypeSelected()
{
    QVariant qvar = ui->growthComboBox->currentData();
    GrowthType gt = qvar.value<GrowthType>();
    return gt;
}


// use the decorationColor variable
void EditPeriodicDialog::setDecorationColorInfo()
{
    QString COLOR_STYLE("QPushButton { background-color : %1; border: none;}");

    if (decorationColor.isValid()) {
        ui->decorationColorPushButton->setStyleSheet(COLOR_STYLE.arg(decorationColor.name()));
        QColor c = decorationColor.name(QColor::HexRgb);
        ui->decorationColorCustomTextLabel->setText(Util::buildColorDisplayName(c));
    } else {
        ui->decorationColorCustomTextLabel->setText("");
        ui->decorationColorPushButton->setStyleSheet(""); // reset
    }
}


QString EditPeriodicDialog::convertTagIDSetToString()
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


void EditPeriodicDialog::updateTagListTextBox()
{
    QString tagsString = convertTagIDSetToString();
    ui->tagsEdit->setPlainText(tagsString);
}


void EditPeriodicDialog::on_closePushButton_clicked()
{
    hide();
    emit signalEditPeriodicStreamDefCompleted();
}


// user has manually closed the dialog
void EditPeriodicDialog::on_EditPeriodicDialog_rejected()
{
    on_closePushButton_clicked();
}


void EditPeriodicDialog::on_decorationColorPushButton_clicked()
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


void EditPeriodicDialog::on_decorationColorCheckBox_clicked()
{
    if (ui->decorationColorCheckBox->isChecked()){
        // normal to custom color
        ui->decorationColorCustomTextLabel->setEnabled(true);
        ui->decorationColorPushButton->setVisible(true);
        // default custom color (we dont remember the last one used)
        decorationColor = QColor::fromRgb(128,128,128);
        // take note of it
        setDecorationColorInfo();
        // user must select a color now (cancelling is allowed)
        on_decorationColorPushButton_clicked();
    } else{
        // custom to normal color
        ui->decorationColorCustomTextLabel->setEnabled(false);
        ui->decorationColorPushButton->setVisible(false);
        decorationColor = QColor();
        setDecorationColorInfo();
    }
}


void EditPeriodicDialog::on_visualizeOccurrencesPushButton_clicked()
{
    QString amountString;

    // build the Stream Def from the data entered in the form (can be valid or not)
    BuildFromFormDataResult result;
    buidlPeriodicFeStreamDefFromFormData(result);
    if (result.success==false){
        QMessageBox::critical(nullptr,tr("Error"),result.errorMessageUI);
        return;
    }

    // Log the operation
    if (GbpController::getInstance().getLogLevel()==GbpController::Debug) {
        GbpController::getInstance().log(GbpController::LogLevel::Debug, GbpController::Info,
            QString("About to visualize occurrences for Periodic item name=%1")
            .arg(ui->nameLineEdit->text()));
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
            QString("About to visualize occurrences for Periodic item"));
    }

    // send for display
    emit signalVisualizeOccurrencesPrepareContent(currInfo, scenarioInflation, maxDateFeGeneration,
        &(result.pStreamDef));
    visualizeoccurrencesDialog->show();
}


void EditPeriodicDialog::on_toCustomRadioButton_toggled(bool checked)
{
    if (ui->toCustomRadioButton->isChecked()==true) {
        ui->toDateEdit->setEnabled(true);
    } else {
        ui->toDateEdit->setEnabled(false);
    }
}


void EditPeriodicDialog::on_toScenarioRadioButton_toggled(bool checked)
{
    if (ui->toCustomRadioButton->isChecked()==true) {
        ui->toDateEdit->setEnabled(true);
    } else {
        ui->toDateEdit->setEnabled(false);
    }
}


// Current item has changed for Combobox Growth Type. When an item is fist inserted, it becomes
// the current item (!)
void EditPeriodicDialog::on_growthComboBox_currentIndexChanged(int index)
{
    QVariant qvar = ui->growthComboBox->itemData(index);
    GrowthType gt = qvar.value<GrowthType>();
    setVisibilityComponentsGrowthType(gt);
}


void EditPeriodicDialog::on_growthTypePushButton_clicked()
{
    GrowthType gt = getGrowthTypeSelected();

    // edit only if this is a variable growth
    if ( gt != GrowthType::CUSTOM_VARIABLE ){
        return;
    }
    emit signalEditVariableGrowthPrepareContent(tempVariableGrowth);
    editVariableGrowthDlg->show();
}


void EditPeriodicDialog::on_editDescriptionPushButton_clicked()
{
    emit signalPlainTextDialogPrepareContent(tr("Edit description"),
        ui->descPlainTextEdit->toPlainText(), false);
    editDescriptionDialog->show();
}



