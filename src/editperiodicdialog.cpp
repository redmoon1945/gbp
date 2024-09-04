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
    ui->periodComboBox->insertItem(0,Util::getPeriodName(Util::PeriodType::YEARLY,true,false),PeriodicFeStreamDef::PeriodType::YEARLY);
    ui->periodComboBox->insertItem(0,Util::getPeriodName(Util::PeriodType::END_OF_MONTHLY,true,false),PeriodicFeStreamDef::PeriodType::END_OF_MONTHLY);
    ui->periodComboBox->insertItem(0,Util::getPeriodName(Util::PeriodType::MONTHLY,true,false),PeriodicFeStreamDef::PeriodType::MONTHLY);
    ui->periodComboBox->insertItem(0,Util::getPeriodName(Util::PeriodType::WEEKLY,true,false),PeriodicFeStreamDef::PeriodType::WEEKLY);
    ui->periodComboBox->insertItem(0,Util::getPeriodName(Util::PeriodType::DAILY,true,false),PeriodicFeStreamDef::PeriodType::DAILY);
    updatePeriodCombobox(PeriodicFeStreamDef::PeriodType::MONTHLY);

    // use smaller font for description list
    QFont descFont = ui->descPlainTextEdit->font();
    uint oldFontSize = descFont.pointSize();
    uint newFontSize = Util::changeFontSize(false,true, oldFontSize);
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Edit Periodic Dialog - Description - Font size set from %1 to %2").arg(oldFontSize).arg(newFontSize));
    descFont.setPointSize(newFontSize);
    ui->descPlainTextEdit->setFont(descFont);

    // make DateWidget widget wide enough
    QFontMetrics fm = ui->fromDateEdit->fontMetrics();
    ui->fromDateEdit->setMinimumWidth(fm.averageCharWidth()*20);
    ui->toDateEdit->setMinimumWidth(fm.averageCharWidth()*20);

    // adjust parameters of growth edit box
    ui->growthDoubleSpinBox->setMinimum(Growth::MIN_GROWTH_DOUBLE);
    ui->growthDoubleSpinBox->setMaximum(Growth::MAX_GROWTH_DOUBLE);
    ui->growthDoubleSpinBox->setDecimals(Growth::NO_OF_DECIMALS);

    // Stream Def variable growth edit dialog
    editVariableGrowthDlg = new EditVariableGrowthDialog(tr("Growth"),locale,this);                // auto-destroyed by Qt because it is a child
    editVariableGrowthDlg->setModal(true);
    // Visualize occurrences  Dialog
    visualizeoccurrencesDialog = new VisualizeOccurrencesDialog(locale,this);     // auto-destroyed by Qt because it is a child
    visualizeoccurrencesDialog->setModal(true);
    // Plain Text Edition Dialog
    editDescriptionDialog = new PlainTextEditionDialog(this);     // auto-destroyed by Qt because it is a child
    editDescriptionDialog->setModal(true);
    // Show Result Dialog
    showResultDialog = new PlainTextEditionDialog(this);     // auto-destroyed by Qt because it is a child
    showResultDialog->setModal(true);
    // connect emitters & receivers for Dialogs : Variable Growth Edition
    QObject::connect(this, &EditPeriodicDialog::signalEditVariableGrowthPrepareContent, editVariableGrowthDlg, &EditVariableGrowthDialog::slotPrepareContent);
    QObject::connect(editVariableGrowthDlg, &EditVariableGrowthDialog::signalEditVariableGrowthResult, this, &EditPeriodicDialog::slotEditVariableGrowthResult);
    QObject::connect(editVariableGrowthDlg, &EditVariableGrowthDialog::signalEditVariableGrowthCompleted, this, &EditPeriodicDialog::slotEditVariableGrowthCompleted);
    // connect emitters & receivers for Dialogs : Description Edition
    QObject::connect(this, &EditPeriodicDialog::signalPlainTextDialogPrepareContent, editDescriptionDialog, &PlainTextEditionDialog::slotPrepareContent);
    QObject::connect(editDescriptionDialog, &PlainTextEditionDialog::signalPlainTextEditionResult, this, &EditPeriodicDialog::slotPlainTextEditionResult);
    QObject::connect(editDescriptionDialog, &PlainTextEditionDialog::signalPlainTextEditionCompleted, this, &EditPeriodicDialog::slotPlainTextEditionCompleted);
    // connect emitters & receivers for Dialogs : Show Result
    QObject::connect(this, &EditPeriodicDialog::signalShowResultPrepareContent, showResultDialog, &PlainTextEditionDialog::slotPrepareContent);
    QObject::connect(showResultDialog, &PlainTextEditionDialog::signalPlainTextEditionResult, this, &EditPeriodicDialog::slotShowResultResult);
    QObject::connect(showResultDialog, &PlainTextEditionDialog::signalPlainTextEditionCompleted, this, &EditPeriodicDialog::slotShowResultCompleted);
    // connect emitters & receivers for Dialogs : Visualize occurrences
    QObject::connect(this, &EditPeriodicDialog::signalVisualizeOccurrencesPrepareContent, visualizeoccurrencesDialog, &VisualizeOccurrencesDialog::slotPrepareContent);
    QObject::connect(visualizeoccurrencesDialog, &VisualizeOccurrencesDialog::signalCompleted, this, &EditPeriodicDialog::slotVisualizeOccurrencesCompleted);
}

EditPeriodicDialog::~EditPeriodicDialog()
{
    delete ui;
}


void EditPeriodicDialog::slotPrepareContent(bool isNewStreamDef, bool isIncome, PeriodicFeStreamDef psStreamDef, CurrencyInfo newCurrInfo, Growth inflation)
{
    // remember some variables
    this->editingExistingStreamDef = !isNewStreamDef;
    this->currInfo = newCurrInfo;
    this->isIncome = isIncome;
    this->scenarioInflation = inflation;    // used only in the "show result" window

    // common settings
    ui->amountDoubleSpinBox->setDecimals(currInfo.noOfDecimal);
    ui->currencyIsoCodeLabel->setText(currInfo.isoCode);

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
        // editing an existing PeriodicFeStreamDef

        // remember the id
        initialId = psStreamDef.getId();
        // set some UI elements
        if(isIncome){
            this->setWindowTitle(tr("Editing Income of Category Type \"Periodic\""));
        } else {
            this->setWindowTitle(tr("Editing Expense of Category Type \"Periodic\""));
        }
        ui->applyPushButton->setText(tr("Apply"));
        ui->closePushButton->setText(tr("Cancel"));
        ui->nameLineEdit->setText(psStreamDef.getName());
        ui->descPlainTextEdit->setPlainText(psStreamDef.getDesc());
        int result;
        double amountDouble = CurrencyHelper::amountQint64ToDouble(psStreamDef.getAmount(),currInfo.noOfDecimal, result);
        if(result!=0){
            // should never happen
            amountDouble = 0;
        }
        ui->amountDoubleSpinBox->setValue(amountDouble);
        ui->fromDateEdit->setDate(psStreamDef.getValidityRange().getStart());
        ui->toDateEdit->setDate(psStreamDef.getValidityRange().getEnd());
        if (psStreamDef.getActive()){
            ui->activeYesRadioButton->setChecked(true);
        } else {
             ui->activeNoRadioButton->setChecked(true);
        }
        ui->gapSpinBox->setValue(psStreamDef.getGrowthApplicationPeriod());
        // Period type and multiplier
        updatePeriodCombobox(psStreamDef.getPeriod());
        ui->periodMultiplierSpinBox->setValue(psStreamDef.getPeriodMultiplier());

        // growth
        tempVariableGrowth = Growth::fromVariableDataAnnualBasisDecimal(QMap<QDate,qint64>());  // in any case, rebuild and reset to empty
        Growth ag = psStreamDef.getGrowth();
        PeriodicFeStreamDef::GrowthStrategy gs = psStreamDef.getGrowthStrategy();
        if(gs == PeriodicFeStreamDef::GrowthStrategy::NONE){
            ui->noGrowthRadioButton->setChecked(true);
        } else if ( gs == PeriodicFeStreamDef::GrowthStrategy::INFLATION){
            ui->inflationRadioButton->setChecked(true);
        } else {
            // CUSTOM GROWTH
            if (ag.getType()==Growth::CONSTANT){
                ui->growthConstantRadioButton->setChecked(true);
                qint64 d = ag.getAnnualConstantGrowth();
                double d2 = Growth::fromDecimalToDouble(d);
                ui->growthDoubleSpinBox->setValue(d2);
            }else{
                ui->growthVariableRadioButton->setChecked(true);
                tempVariableGrowth = ag;    // stock it
                ui->growthDoubleSpinBox->setValue(0);
            }
        }

        // inflation adjustment factor
        ui->inflationAdjustmentFactorDoubleSpinBox->setValue(psStreamDef.getInflationAdjustmentFactor());

        updateAuxCustomGrowthWidgetAccessibility();

    } else{

        // creating a new PeriodicFeStreamDef (value of psStreamDef then does not matter)

        initialId = QUuid::createUuid();
        // set some UI elements
        if(isIncome){
            this->setWindowTitle(tr("Creating Periodic Income"));
        } else {
            this->setWindowTitle(tr("Creating Periodic Expense"));
        }

        ui->applyPushButton->setText(tr("Create"));
        ui->closePushButton->setText(tr("Close"));
        prepareDataToCreateANewStreamDef();
        updateAuxCustomGrowthWidgetAccessibility();
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
}


void EditPeriodicDialog::on_applyPushButton_clicked()
{
    if(editingExistingStreamDef){
        GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Info, QString("Attempting to modify Periodic item \"%1\" ...").arg(ui->nameLineEdit->text()));
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Info, QString("Attempting to create new Periodic item \"%1\" ...").arg(ui->nameLineEdit->text()));
    }

    BuildFromFormDataResult result;
    buidlPeriodicFeStreamDefFromFormData(result);
    if (result.success==false){
        QMessageBox::critical(nullptr,tr("Invalid Data Entered"),result.errorMessageUI);
        GbpController::getInstance().log(GbpController::LogLevel::Debug,GbpController::Info, QString("    Changes not applied : Invalid Data Entered (%1)").arg(result.errorMessageLog));
        return;
    }

    // send back
    emit signalEditPeriodicStreamDefResult (isIncome, result.pStreamDef);

    // if editing an existing Stream Def, then this is the end of it. For create, then stay right there to
    // facilitate the rapid creation of multiple Stream Def
    if(editingExistingStreamDef){
        hide();
        emit signalEditPeriodicStreamDefCompleted();
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("    Modifications applied to Periodic item"));
    } else{
        // reset some parameters so we are ready to create yet another Stream Def
        prepareDataToCreateANewStreamDef();
        updateAuxCustomGrowthWidgetAccessibility();

        ui->nameLineEdit->setFocus();
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("    New Periodic item created"));
    }
}


void EditPeriodicDialog::prepareDataToCreateANewStreamDef()
{
    initialId = QUuid::createUuid();
    ui->nameLineEdit->setText("");
    ui->descPlainTextEdit->setPlainText("");
    ui->amountDoubleSpinBox->setValue(0);
    ui->fromDateEdit->setDate(GbpController::getInstance().getTomorrow());
    ui->toDateEdit->setDate(GbpController::getInstance().getTomorrow().addYears(GbpController::getInstance().getScenarioMaxYearsSpan()));
    ui->activeYesRadioButton->setChecked(true);
    ui->gapSpinBox->setValue(1);
    ui->periodMultiplierSpinBox->setValue(1);
    ui->inflationAdjustmentFactorDoubleSpinBox->setValue(1);
    // growth is constant & 0 by default
    ui->noGrowthRadioButton->setChecked(true);
    ui->growthDoubleSpinBox->setValue(0);
    // but initialize an empty Variable Growth if ever user decide to switch to that type
    tempVariableGrowth = Growth::fromVariableDataAnnualBasisDecimal(QMap<QDate, qint64>());
    // decoration color
    ui->decorationColorCheckBox->setChecked(false);
    on_decorationColorCheckBox_clicked();
}


void EditPeriodicDialog::updateAuxCustomGrowthWidgetAccessibility()
{
    if (ui->noGrowthRadioButton->isChecked() ){
        ui->inflationAdjustmentFactorDoubleSpinBox->setEnabled(false);
        ui->growthDoubleSpinBox->setEnabled(false);
        ui->growthVariablePushButton->setEnabled(false);
    } else if (ui->inflationRadioButton->isChecked() ){
        ui->inflationAdjustmentFactorDoubleSpinBox->setEnabled(true);
        ui->growthDoubleSpinBox->setEnabled(false);
        ui->growthVariablePushButton->setEnabled(false);
    } else if (ui->growthConstantRadioButton->isChecked()){
        ui->inflationAdjustmentFactorDoubleSpinBox->setEnabled(false);
        ui->growthDoubleSpinBox->setEnabled(true);
        ui->growthVariablePushButton->setEnabled(false);
    } else if(ui->growthVariableRadioButton->isChecked()){
        ui->inflationAdjustmentFactorDoubleSpinBox->setEnabled(false);
        ui->growthDoubleSpinBox->setEnabled(false);
        ui->growthVariablePushButton->setEnabled(true);
    }

}

void EditPeriodicDialog::buidlPeriodicFeStreamDefFromFormData(BuildFromFormDataResult &result)
{
    result.success = false;
    result.errorMessageUI = "";
    result.errorMessageLog = "";

    // check if Validity ranges are valid
    QDate from = ui->fromDateEdit->date();
    QDate to = ui->toDateEdit->date();
    if (to<from){
        result.errorMessageUI = tr("For Validity Range, the \"to\" date must not occur before the \"from\" date");
        result.errorMessageLog = "For Validity Range, the \"to\" date must not occur before the \"from\" date";
        return;
    }
    // gather and transform data to create a Periodic Stream Def
    QVariant selectedData = ui->periodComboBox->itemData(ui->periodComboBox->currentIndex());
    PeriodicFeStreamDef::PeriodType periodicType = static_cast<PeriodicFeStreamDef::PeriodType>(selectedData.toInt());
    int resultConv;
    qint64 amount = CurrencyHelper::amountDoubleToQint64(ui->amountDoubleSpinBox->value(), currInfo.noOfDecimal, resultConv);
    if (resultConv!=0){
        if (resultConv==-1){
            result.errorMessageUI = QString(tr("The amount cannot be bigger than %1")).arg(CurrencyHelper::maxValueAllowedForAmountInDouble(currInfo.noOfDecimal));
            result.errorMessageLog = QString("The amount cannot be bigger than %1").arg(CurrencyHelper::maxValueAllowedForAmountInDouble(currInfo.noOfDecimal));
            return;
        } else {
            // should never happen
            result.errorMessageUI = QString(tr("An error occured while processing the amount : code=%1")).arg(resultConv);
            result.errorMessageLog = QString("An error occured while processing the amount : code=%1").arg(resultConv);
            return;
        }
    }
    PeriodicFeStreamDef::GrowthStrategy gs;
    if (ui->noGrowthRadioButton->isChecked()){
        gs = PeriodicFeStreamDef::NONE;
    } else if (ui->inflationRadioButton->isChecked()){
        gs = PeriodicFeStreamDef::INFLATION;
    } else {
        gs = PeriodicFeStreamDef::CUSTOM;
    }
    Growth growth;
    if (ui->growthConstantRadioButton->isChecked() ){
        double d = ui->growthDoubleSpinBox->value();
        growth = Growth::fromConstantAnnualPercentageDouble(d);
    } else if (ui->growthVariableRadioButton->isChecked()){
        growth = tempVariableGrowth;
    }
    DateRange validityRange(ui->fromDateEdit->date(), ui->toDateEdit->date());
    qint16 periodMultiplier = static_cast<quint16>(ui->periodMultiplierSpinBox->value());
    quint16 gap = static_cast<quint16>(ui->gapSpinBox->value());

    // inflationModifFactor
    double inflationModifFactor = ui->inflationAdjustmentFactorDoubleSpinBox->value();

    // build item
    try {
        result.pStreamDef = PeriodicFeStreamDef(periodicType, periodMultiplier, amount, growth, gs,
                                                gap, initialId, ui->nameLineEdit->text(), ui->descPlainTextEdit->toPlainText(),
                                                ui->activeYesRadioButton->isChecked(), isIncome, decorationColor, validityRange, inflationModifFactor);
    } catch (...) {
        // unexpected error, should never happen
        std::exception_ptr p = std::current_exception();
        result.errorMessageUI = QString(tr("An unexpected error has occured.\n\nDetails : %1")).arg((p ? p.__cxa_exception_type()->name() : "null"));
        result.errorMessageLog = QString("An unexpected error has occured.\n\nDetails : %1").arg((p ? p.__cxa_exception_type()->name() : "null"));
        return;
    }

    result.success = true;
    return;
}


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


void EditPeriodicDialog::on_growthVariablePushButton_clicked()
{
    // edit only if this is a variable growth
    if ( !(ui->growthVariableRadioButton->isChecked())){
        return;
    }
    emit signalEditVariableGrowthPrepareContent(tempVariableGrowth);
    editVariableGrowthDlg->show();
}


void EditPeriodicDialog::on_noGrowthRadioButton_clicked()
{
    updateAuxCustomGrowthWidgetAccessibility();
}


void EditPeriodicDialog::on_inflationRadioButton_clicked()
{
    updateAuxCustomGrowthWidgetAccessibility();
}


void EditPeriodicDialog::on_growthConstantRadioButton_clicked()
{
    updateAuxCustomGrowthWidgetAccessibility();
}


void EditPeriodicDialog::on_growthVariableRadioButton_clicked()
{
    updateAuxCustomGrowthWidgetAccessibility();
}


void EditPeriodicDialog::on_pushButton_clicked()
{
    emit signalPlainTextDialogPrepareContent(tr("Edit Description"),ui->descPlainTextEdit->toPlainText(), false);
    editDescriptionDialog->show();
}




void EditPeriodicDialog::on_decorationColorPushButton_clicked()
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


void EditPeriodicDialog::on_decorationColorCheckBox_clicked()
{
    if (ui->decorationColorCheckBox->isChecked()){
        // normal to custom color
        ui->decorationColorCustomTextLabel->setEnabled(true);
        ui->decorationColorPushButton->setVisible(true);
        decorationColor = QColor::fromRgb(128,128,128); // default custom color (we dont remember the last one used)
        setDecorationColorInfo();   // take note of it
        on_decorationColorPushButton_clicked(); // user must select a color now (cancelling is allowed)
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
        QMessageBox::critical(nullptr,tr("Invalid Data Entered"),result.errorMessageUI);
        return;
    }

    // send for display
    emit signalVisualizeOccurrencesPrepareContent(currInfo, scenarioInflation, &(result.pStreamDef));
    visualizeoccurrencesDialog->show();
}

