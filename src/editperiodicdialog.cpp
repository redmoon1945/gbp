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
#include "gbplogger.h"
#include "util.h"
#include "periodiccsd.h"
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
    ui->periodComboBox->insertItem(0,PeriodicCsd::getPeriodName(PeriodicCsd::PeriodType::YEARLY,
        true,false), PeriodicCsd::periodTypeToInt(PeriodicCsd::PeriodType::YEARLY) );
    ui->periodComboBox->insertItem(0,PeriodicCsd::getPeriodName(
        PeriodicCsd::PeriodType::END_OF_MONTHLY,true,false),
        PeriodicCsd::periodTypeToInt(PeriodicCsd::PeriodType::END_OF_MONTHLY));
    ui->periodComboBox->insertItem(0,PeriodicCsd::getPeriodName(
        PeriodicCsd::PeriodType::MONTHLY,true,false),
        PeriodicCsd::periodTypeToInt(PeriodicCsd::PeriodType::MONTHLY));
    ui->periodComboBox->insertItem(0,PeriodicCsd::getPeriodName(
        PeriodicCsd::PeriodType::WEEKLY,true,false),
        PeriodicCsd::periodTypeToInt(PeriodicCsd::PeriodType::WEEKLY));
    ui->periodComboBox->insertItem(0,PeriodicCsd::getPeriodName(
        PeriodicCsd::PeriodType::DAILY,true,false),
        PeriodicCsd::periodTypeToInt(PeriodicCsd::PeriodType::DAILY));
    updatePeriodCombobox(PeriodicCsd::PeriodType::MONTHLY);

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
    QFont font = ui->descPlainTextEdit->font();
    Util::changeFontSize(font, Util::FontResizeIntensity::AVERAGE, true);
    ui->descPlainTextEdit->setFont(font);

    // use smaller font for tag list
    font = ui->tagsEdit->font();
    Util::changeFontSize(font, Util::FontResizeIntensity::AVERAGE, true);
    ui->tagsEdit->setFont(font);

    // make the description list not too high and fixed (must be done after font setting)
    QFontMetrics fm(ui->descPlainTextEdit->font());
    ui->descPlainTextEdit->setFixedHeight(fm.height()*3 * 1.4); // 3 lines

    // make the tag list not too high and fixed (must be done after font setting)
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
        Constants::DEFAULT_DURATION_FE_GENERATION));

    // set minimum date values
    ui->fromDateEdit->setMinimumDate(PeriodicCsd::MIN_START_DATE);
    ui->toDateEdit->setMinimumDate(PeriodicCsd::MIN_START_DATE.addDays(1));

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
    ui->nameLineEdit->setMaxLength(Csd::NAME_MAX_LEN);

    // Stream Def variable growth edit dialog
    editVariableGrowthDlg = new EditVariableGrowthDialog(tr("Growth"),locale,this);// auto-destroyed
    editVariableGrowthDlg->setModal(true);
    // Visualize occurrences  Dialog
    visualizeoccurrencesDialog = new VisualizeOccurrencesDialog(locale,this);// auto-destroyed by Qt
    visualizeoccurrencesDialog->setModal(true);
    // Plain Text Edition Dialog
    editDescriptionDialog = new PlainTextEditionDialog(this);  // auto-destroyed by Qt
    editDescriptionDialog->setModal(true);

    // Set the color of the StartWarning label to "optimized orange"
    ui->startWarningLabel->setText("\u26A0");
    ui->startWarningLabel->setStyleSheet("color: #FF9800;");

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


void EditPeriodicDialog::slotPrepareContent(bool isNewCsd, bool isIncome,
    QWeakPointer<PeriodicCsd> pCsd, CurrencyInfo newCurrInfo, Growth inflation,
    QDate theMaxDateFeGeneration, QSet<QUuid> associatedTagIds, Tags availTags)
{
    // Convert csd to strong pointer if editing an existing csd
    QSharedPointer<PeriodicCsd> psCsd;
    if (isNewCsd==false) {
        psCsd = pCsd.toStrongRef();
        if(psCsd.isNull()){
            return; // should never happen
        }
    }

    // check input values
    if ( theMaxDateFeGeneration.isValid()==false ) {
        throw std::invalid_argument("Invalid max date for FeGenerationDuration");
    }

    // remember some variables
    this->editingExistingStreamDef = !isNewCsd;
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
    if (isNewCsd) {
        decorationColor = QColor(); // use normal color for new Csd
    } else {
        decorationColor = psCsd->getDecorationColor(); // can be normal or custom
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
        // *** editing an existing PeriodicCsd ***

        // remember the id
        initialId = psCsd->getId();

        // Set title
        if(isIncome){
            this->setWindowTitle(tr("Editing periodic income"));
        } else {
            this->setWindowTitle(tr("Editing periodic expense"));
        }
        ui->applyPushButton->setText(tr("Apply"));
        ui->closePushButton->setText(tr("Cancel"));
        ui->nameLineEdit->setText(psCsd->getName());
        ui->descPlainTextEdit->setPlainText(psCsd->getDesc());
        int result;
        double amountDouble = CurrencyHelper::amountQint64ToDouble(psCsd->getAmount(),
            currInfo.noOfDecimal, result);
        if(result!=0){
            // should never happen
            amountDouble = 0;
        }
        ui->amountDoubleSpinBox->setValue(amountDouble);
        ui->fromDateEdit->setDate(psCsd->getStartDate());

        // set "to" date : do not clear value (handy)
        if (psCsd->getUseScenarioForEndDate()==true) {
            // used scenario value for end date
            ui->toScenarioRadioButton->setChecked(true);
            ui->toDateEdit->setEnabled(false);
        } else {
            // use custom value for end date
            ui->toDateEdit->setDate(psCsd->getEndDate());
            ui->toCustomRadioButton->setChecked(true);
            ui->toDateEdit->setEnabled(true);
        }

        if (psCsd->getActive()){
            ui->activeYesRadioButton->setChecked(true);
        } else {
             ui->activeNoRadioButton->setChecked(true);
        }
        ui->gapSpinBox->setValue(psCsd->getGrowthApplicationPeriod());

        // Period type and multiplier
        updatePeriodCombobox(psCsd->getPeriod());
        ui->periodMultiplierSpinBox->setValue(psCsd->getPeriodMultiplier());

        // --- growth ---

        // in any case, rebuild and reset to empty
        tempVariableGrowth = Growth::fromVariableDataAnnualBasisDecimal(QMap<QDate,qint64>());

        Growth ag = psCsd->getGrowth();
        PeriodicCsd::GrowthStrategy gs = psCsd->getGrowthStrategy();

        if(gs == PeriodicCsd::GrowthStrategy::NONE){
            ui->growthTypeScenaroInflationDoubleSpinBox->setValue(1);
            ui->growthTypeCustomConstantDoubleSpinBox->setValue(0);
            updateGrowthTypeCombobox(GrowthType::NONE);

        } else if ( gs == PeriodicCsd::GrowthStrategy::INFLATION){
            ui->growthTypeScenaroInflationDoubleSpinBox->setValue(
                psCsd->getInflationAdjustmentFactor());
            ui->growthTypeCustomConstantDoubleSpinBox->setValue(0);
            updateGrowthTypeCombobox(GrowthType::SCENARIO);
        } else {
            // CUSTOM GROWTH
            if (ag.getType()==Growth::Type::CONSTANT){
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

        // *** creating a new PeriodicCsd (value of psCsd then does not matter) ***

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

    // set visibility of the Start Date warning. From date must have been filled.
    setVisibilityStartDateWarningSign();

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
    LOG_INFO("Visualize occurrences completed");
}


void EditPeriodicDialog::on_applyPushButton_clicked()
{
    if(editingExistingStreamDef){
        LOG_INFO(QString("Attempting to apply changes made to a periodic csd \"%1\" ...")
            .arg(REDACT(ui->nameLineEdit->text())));
    } else {
        LOG_INFO(QString("Creating new periodic csd \"%1\" ...")
            .arg(REDACT(ui->nameLineEdit->text())));
    }

    BuildFromFormDataResult result;
    buildPeriodicCsdFromFormData(result);
    if (result.result.status==Util::ResultOfOperationStatus::ERROR){
        QMessageBox::critical(nullptr,tr("Error"),result.result.userErrorMessage);
        LOG_WARNING( QString("    Changes not applied : Invalid data entered (%1)")
            .arg(result.result.logErrorMessage));
        LOG_INFO("End of edition");
        return;
    }

    // send back result
    emit signalEditPeriodicCsdResult (isIncome, result.pCsd);

    // if editing an existing Stream Def, then this is the end of it. For create,
    // then stay right there to facilitate the rapid creation of multiple Stream Def
    if(editingExistingStreamDef){
        hide();
        emit signalEditPeriodicCsdCompleted();
        LOG_INFO("    Modifications applied to periodic csd");
    } else{
        // reset some parameters so we are ready to create yet another Stream Def
        prepareDataToCreateANewStreamDef(false);
        ui->nameLineEdit->setFocus();
        LOG_INFO("    New periodic csd created");
    }
    LOG_INFO("End of edition");
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


void EditPeriodicDialog::buildPeriodicCsdFromFormData(BuildFromFormDataResult &result)
{
    // Reset result to ERROR
    result.init();

    // check if Validity range data is valid, then build it
    QDate from = ui->fromDateEdit->date();
    if( !from.isValid()){
        result.result.userErrorMessage = tr("Start date is invalid");
        result.result.logErrorMessage = "Start date is invalid";
        return;
    }
    if( from < PeriodicCsd::MIN_START_DATE){
        result.result.userErrorMessage = tr("Start date must not occur before %1")
            .arg(locale.toString(PeriodicCsd::MIN_START_DATE, QLocale::LongFormat));
        result.result.logErrorMessage = "Start date is invalid";
        return;
    }
    if (ui->toScenarioRadioButton->isChecked()==true) {
        if(maxDateFeGeneration<from){

        // Do nothing, as it is a valid case that can happen. No FE will be generated though.

        //     result.result.userErrorMessage = tr("End date as defined at the scenario "
        //         "level must not occur before the Start date");
        //     result.result.logErrorMessage = "End date as defined at the scenario level"
        //         " must not occur before the Start date";
        //     return;
        }
    }
    QDate to = ui->toDateEdit->date(); // to date is allowed to go over maxDate from the scenario
    if ( ui->toCustomRadioButton->isChecked()==true){
        if( !to.isValid()){
            result.result.userErrorMessage = tr("End date is invalid");
            result.result.logErrorMessage = "End date is invalid";
            return;
        }
        if(to<from){
            result.result.userErrorMessage = tr("End date must not occur before the Start date");
            result.result.logErrorMessage = "End date must not occur before the Start date";
            return;
        }
        if( to < PeriodicCsd::MIN_START_DATE.addDays(1)){
            result.result.userErrorMessage = tr("End date must not occur before %1")
            .arg(locale.toString(PeriodicCsd::MIN_START_DATE.addDays(1), QLocale::LongFormat));
            result.result.logErrorMessage = "End date is invalid";
            return;
        }
    } else{
        // just make sure the unused to date is still valid
        if( !to.isValid()){
            to = from;
        }
    }

    //*** gather and transform data to create a Periodic Csd ***

    QVariant selectedData = ui->periodComboBox->itemData(ui->periodComboBox->currentIndex());
    PeriodicCsd::PeriodType periodicType;
    if ( false == PeriodicCsd::intToPeriodType( selectedData.toInt(), periodicType ) ){
        // should never happen
        throw std::invalid_argument("Unknown Periodic Type value");
    }
    int resultConv;
    qint64 amount = CurrencyHelper::amountDoubleToQint64(ui->amountDoubleSpinBox->value(),
        currInfo.noOfDecimal, resultConv);
    if (resultConv!=0){
        if (resultConv==-1){
            result.result.userErrorMessage = QString(tr("The amount cannot be bigger than %1"))
                .arg(CurrencyHelper::maxValueAllowedForAmountInDouble(currInfo.noOfDecimal));
            result.result.logErrorMessage = QString("The amount cannot be bigger than %1")
                .arg(CurrencyHelper::maxValueAllowedForAmountInDouble(currInfo.noOfDecimal));
            return;
        } else {
            // should never happen
            result.result.userErrorMessage = QString(tr("An error occurred while processing the "
                "amount : code=%1")).arg(resultConv);
            result.result.logErrorMessage = QString("An error occurred while processing the "
                "amount : code=%1").arg(resultConv);
            return;
        }
    }

    PeriodicCsd::GrowthStrategy gs;
    GrowthType gt = getGrowthTypeSelected();
    Growth growth; // type "NONE"

    // Set Growth Strategy
    switch (gt) {
        case GrowthType::NONE:
            gs = PeriodicCsd::GrowthStrategy::NONE;
            break;
        case GrowthType::SCENARIO:
            gs = PeriodicCsd::GrowthStrategy::INFLATION;
            break;
        case GrowthType::CUSTOM_CONSTANT:
            gs = PeriodicCsd::GrowthStrategy::CUSTOM;
            break;
        case GrowthType::CUSTOM_VARIABLE:
            gs = PeriodicCsd::GrowthStrategy::CUSTOM;
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
        result.pCsd = QSharedPointer<PeriodicCsd>( new PeriodicCsd(periodicType, periodMultiplier, amount, growth, gs,
            gap, initialId, (ui->nameLineEdit->text().trimmed()).left(Csd::NAME_MAX_LEN),
            ui->descPlainTextEdit->toPlainText().left(Csd::DESC_MAX_LEN),
            ui->activeYesRadioButton->isChecked(), isIncome, decorationColor, from, to,
            ui->toScenarioRadioButton->isChecked(),inflationModifFactor));
    } catch (const std::exception& e) {
        // unexpected error, should never happen
        result.result.userErrorMessage = QString(tr("An unexpected error has occurred.\n\nDetails : %1"))
            .arg(e.what());
        result.result.logErrorMessage = QString("An unexpected error has occurred.\n\nDetails : %1")
            .arg(e.what());
        return;
    }

    result.result.status = Util::ResultOfOperationStatus::SUCCESS;
    return;
}


// Set the selected item for Period Type combobox
void EditPeriodicDialog::updatePeriodCombobox(PeriodicCsd::PeriodType type)
{
    // highly DEPENDANT on order to insertion in Dialog init !
    if ( type == PeriodicCsd::PeriodType::DAILY){
        ui->periodComboBox->setCurrentIndex(0) ;
    } else if (type == PeriodicCsd::PeriodType::WEEKLY){
        ui->periodComboBox->setCurrentIndex(1) ;
    } else if (type == PeriodicCsd::PeriodType::MONTHLY){
        ui->periodComboBox->setCurrentIndex(2) ;
    } else if (type == PeriodicCsd::PeriodType::END_OF_MONTHLY){
        ui->periodComboBox->setCurrentIndex(3) ;
    } else if (type == PeriodicCsd::PeriodType::YEARLY){
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


void EditPeriodicDialog::setVisibilityStartDateWarningSign()
{
    QDate sDate = ui->fromDateEdit->date();
    if (sDate.isValid()==false) {
        return;
    }

    if( ui->toCustomRadioButton->isChecked()==false){
        if (sDate > maxDateFeGeneration) {
            ui->startWarningLabel->setVisible(true);
            // ui->fromDateEdit->setStyleSheet(
            //     "QDateEdit {"
            //     "  border: 2px solid #FF6B6B;"  // Red border
            //     "  border-radius: 4px;"
            //     "}"
            //     );

        } else {
            ui->startWarningLabel->setVisible(false);
            //ui->fromDateEdit->setStyleSheet("");
        }
    } else {
        QDate eDate = ui->toDateEdit->date();
        if (eDate.isValid()==false) {
            return;
        }
        if (eDate < sDate) {
            ui->startWarningLabel->setVisible(true);
            // ui->fromDateEdit->setStyleSheet(
            //     "QDateEdit {"
            //     "  border: 2px solid #FF6B6B;"  // Red border
            //     "  border-radius: 4px;"
            //     "}"
            //     );
        } else {
            ui->startWarningLabel->setVisible(false);
            //ui->fromDateEdit->setStyleSheet("");
        }
    }


}


void EditPeriodicDialog::on_closePushButton_clicked()
{
    hide();
    emit signalEditPeriodicCsdCompleted();
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
    buildPeriodicCsdFromFormData(result);
    if (result.result.status==Util::ResultOfOperationStatus::ERROR){
        QMessageBox::critical(nullptr,tr("Error"),result.result.userErrorMessage);
        return;
    }

    // Log the operation
    LOG_INFO( QString("About to visualize occurrences for Periodic item name=%1")
        .arg(REDACT(ui->nameLineEdit->text())));


    // send for display
    emit signalVisualizeOccurrencesPrepareContent(currInfo, scenarioInflation, maxDateFeGeneration,
        result.pCsd.toWeakRef());
    visualizeoccurrencesDialog->show();
}


void EditPeriodicDialog::on_toCustomRadioButton_toggled(bool checked)
{
    setVisibilityStartDateWarningSign();
    if (ui->toCustomRadioButton->isChecked()==true) {
        ui->toDateEdit->setEnabled(true);
    } else {
        ui->toDateEdit->setEnabled(false);
    }
}


void EditPeriodicDialog::on_toScenarioRadioButton_toggled(bool checked)
{
    setVisibilityStartDateWarningSign();
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


EditPeriodicDialog::BuildFromFormDataResult::BuildFromFormDataResult()
{
    init();
}


void EditPeriodicDialog::BuildFromFormDataResult::init()
{
    result.init();
    pCsd = QSharedPointer<PeriodicCsd>(); // null
}

void EditPeriodicDialog::on_fromDateEdit_userDateChanged(const QDate &date)
{
    setVisibilityStartDateWarningSign();
}


void EditPeriodicDialog::on_toDateEdit_userDateChanged(const QDate &date)
{
    setVisibilityStartDateWarningSign();
}

