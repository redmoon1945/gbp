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

#include <QColorDialog>
#include <QTimer>
#include <QPalette>
#include <QFontDialog>
#include <QMessageBox>
#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include "gbpcontroller.h"
#include "gbplogger.h"
#include "util.h"
#include "uiutil.h"
#include "gbpqmessage.h"


OptionsDialog::OptionsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OptionsDialog)
{
    ui->setupUi(this);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);

    // "pack" the dialog to fit the font. This is required when there is no "expanding" widgets
    this->adjustSize();

    // init default system application font text
    QString s = GbpController::getInstance().getInitialSystemApplicationFont();
    QFont f;
    QString sysFontString="";
    bool ok = f.fromString(s);
    if ( ok ){
        sysFontString = fontLabel(f);
    }
    QString widgetString = QString(tr("System font : %1")).arg(sysFontString);
    ui->systemFontRadioButton->setText(widgetString);
    // make sure first tab is selected
    ui->tabWidget->setCurrentIndex(0);

    // Set default income and expense color
    setColorInfo(CI_INCOME_COLOR, Util::getOptimizedGreen());
    setColorInfo(CI_EXPENSE_COLOR, Util::getOptimizedRed());

}


OptionsDialog::~OptionsDialog()
{
    delete ui;
}


// Dialog is about to be displayed with new data
void OptionsDialog::slotPrepareContent()
{
    // Set chart theming
    switch(GbpController::getInstance().getChartTheming()){
        case GbpController::ChartTheming::FORCE_LIGHT:
            ui->chartThemingLightRadioButton->setChecked(true);
            break;
        case GbpController::ChartTheming::FORCE_DARK:
            ui->chartThemingDarkRadioButton->setChecked(true);
            break;
        case GbpController::ChartTheming::FOLLOW_DESKTOP_THEME:
            ui->chartThemingFollowRadioButton->setChecked(true);
            break;
        default: // should never happen
            return;
        }

    // chart point size
    ui->pointSizeSpinBox->setValue(GbpController::getInstance().getChartPointSize());

    // curve Dark Mode widgets
    darkModeCurveColor = GbpController::getInstance().getDarkModeCurveColor();
    darkModePointColor = GbpController::getInstance().getDarkModePointColor();
    darkModeSelectedPointColor = GbpController::getInstance().getDarkModeSelectedPointColor();
    setColorInfo(CI_CURVE_DT, darkModeCurveColor);
    setColorInfo(CI_POINT_DT, darkModePointColor);
    setColorInfo(CI_SELECTED_POINT_DT, darkModeSelectedPointColor);

    // curve Light Mode widgets
    lightModeCurveColor = GbpController::getInstance().getLightModeCurveColor();
    lightModePointColor = GbpController::getInstance().getLightModePointColor();
    lightModeSelectedPointColor = GbpController::getInstance().getLightModeSelectedPointColor();
    setColorInfo(CI_CURVE_LT, lightModeCurveColor);
    setColorInfo(CI_POINT_LT, lightModePointColor);
    setColorInfo(CI_SELECTED_POINT_LT, lightModeSelectedPointColor);

    // export text localization
    ui->exportTextAmountLocalizedCheckBox->setChecked(
        GbpController::getInstance().getExportTextNumberLocalized());
    ui->exportTextDateLocalizedCheckBox->setChecked(
        GbpController::getInstance().getExportTextDateLocalized());

    // main chart scaling
    ui->scalingMainChartSpinBox->setValue(
        GbpController::getInstance().getPercentageMainChartScaling());

    // Application Font
    newCustomFontString = "";
    if (GbpController::getInstance().getUseDefaultSystemFont()){
        ui->systemFontRadioButton->setChecked(true);
    } else {
        ui->customFontRadioButton->setChecked(true);
    }
    // we dont know if it is valid or not set
    QString s = GbpController::getInstance().getCustomApplicationFont();
    QString customFontString=tr("None defined");
    if (s.trimmed().length() != 0) {
        QFont f;
        bool ok = f.fromString(s);  // lets try if it is a valid QFont description
        if ( ok ){
            customFontString = fontLabel(f);
            newCustomFontString = s;
        }
    }
    setCustomFontlabel(customFontString);

    // disable Font choosing button if system font is selected
    if ( ui->systemFontRadioButton->isChecked()){
        ui->setCustomFontPushButton->setEnabled(false);
    } else {
        ui->setCustomFontPushButton->setEnabled(true);
    }

    // Today's Date
    ui->todayDateEdit->setDate(GbpController::getInstance().getTodayCustomDate());
    if (GbpController::getInstance().getTodayUseSystemDate()==true) {
        ui->todaySystemRadioButton->setChecked(true);
        ui->todayDateEdit->setEnabled(false);
    } else {
        ui->todaySpecificRadioButton->setChecked(true);
        ui->todayDateEdit->setEnabled(true);
    }

    // Allow Decoration (that is, Csd names) Colors
    if (GbpController::getInstance().getAllowDecorationColor()==true) {
        ui->allowDecorationColorCheckBox->setChecked(true);
    } else {
        ui->allowDecorationColorCheckBox->setChecked(false);
    }

    // use Present Value
    ui->pvDiscountRateDoubleSpinBox->setValue(GbpController::getInstance().getPvDiscountRate());
    if (GbpController::getInstance().getUsePresentValue()==true) {
        ui->usePresentValueCheckBox->setChecked(true);
        ui->discountRateLabel->setEnabled(true);
        ui->pvDiscountRateDoubleSpinBox->setEnabled(true);
    } else {
        ui->usePresentValueCheckBox->setChecked(false);
        ui->discountRateLabel->setEnabled(false);
        ui->pvDiscountRateDoubleSpinBox->setEnabled(false);
    }

    // wheel mouse zooming direction
    if (GbpController::getInstance().getWheelRotatedAwayZoomIn()==true) {
        ui->wheelZoomInRadioButton->setChecked(true);
    } else {
        ui->wheelZoomInRadioButton->setChecked(false);
    }

    // show Y=0 line
    if (GbpController::getInstance().getShowYzeroLine()==true) {
        ui->showYzeroLineCheckBox->setChecked(true);
    } else {
        ui->showYzeroLineCheckBox->setChecked(false);
    }

    // Y zero line color - dark mode
    yZeroLineDarkModeColor = GbpController::getInstance().getYZeroLineDarkModeColor();
    setColorInfo(CI_YZERO_LINE_DT, yZeroLineDarkModeColor);

    // Y zero line color - light mode
    yZeroLineLightModeColor = GbpController::getInstance().getYZeroLineLightModeColor();
    setColorInfo(CI_YZERO_LINE_LT, yZeroLineLightModeColor);

    // Gridlines color - dark mode
    gridlinesDarkModeColor = GbpController::getInstance().getGridlinesDarkModeColor();
    setColorInfo(CI_GRIDLINES_DT, gridlinesDarkModeColor);

    // Gridlines color - light mode
    gridlinesLightModeColor = GbpController::getInstance().getGridlinesLightModeColor();
    setColorInfo(CI_GRIDLINES_LT, gridlinesLightModeColor);

    // X-Axis Date Format
    switch (GbpController::getInstance().getXAxisDateFormat()) {
        case 0:
            ui->xAxisDateLocaleRadioButton->setChecked(true);
            break;
        case 1:
            ui->xAxisDateIsoRadioButton->setChecked(true);
            break;
        case 2:
            ui->xAxisDateIsoTwoDigitsRadioButton->setChecked(true);
            break;
        default:
            ui->xAxisDateLocaleRadioButton->setChecked(true); // should never happen
            break;
    }

    // Show Tooltips
    if (GbpController::getInstance().getShowTooltips()==true) {
        ui->tooltipsCheckBox->setChecked(true);
    } else {
        ui->tooltipsCheckBox->setChecked(false);
    }

    // Income colors
    incomeColor = GbpController::getInstance().getIncomeColor();
    setColorInfo(CI_INCOME_COLOR, incomeColor);

    // expense color
    expenseColor = GbpController::getInstance().getExpenseColor();
    setColorInfo(CI_EXPENSE_COLOR, expenseColor);

    // Reset focus the first item to work on (we dont want "space" to active Close or Save
    if ( ui->todaySystemRadioButton->isChecked()==true) {
        ui->todaySystemRadioButton->setFocus();
    } else {
        ui->todaySpecificRadioButton->setFocus();
    }
}


// Apply the changes made to the options.
// It is this Dialog that determines the impacts of the settings changes on the application
void OptionsDialog::on_applyPushButton_clicked()
{
    LOG_INFO("Options Dialog : \"Save\" requested by user");

    // For the parameters for which the values are stored directly in widgets,
    // get the current values as currently entered in the form
    bool newUseSystemDateForToday = ui->todaySystemRadioButton->isChecked();
    QDate newCustomTodayDate = ui->todayDateEdit->date();
    bool newUseSystemFont = ui->systemFontRadioButton->isChecked();
    bool newAllowDecorationColor = ui->allowDecorationColorCheckBox->isChecked();
    bool newExportTextNumberLocalized = ui->exportTextAmountLocalizedCheckBox->isChecked();
    bool newExportTextDateLocalized = ui->exportTextDateLocalizedCheckBox->isChecked();
    bool newUsePv = ui->usePresentValueCheckBox->isChecked();
    double newDiscountrate = ui->pvDiscountRateDoubleSpinBox->value();
    bool newShowTooltips = ui->tooltipsCheckBox->isChecked();
    uint newChartPointSize = ui->pointSizeSpinBox->value();
    uint newMainChartScaling = ui->scalingMainChartSpinBox->value();
    bool newWheelZoomIn = ui->wheelZoomInRadioButton->isChecked();
    bool newShowYzeroLine = ui->showYzeroLineCheckBox->isChecked();
    uint newXAxisDateFormat = 0;
    if( ui->xAxisDateLocaleRadioButton->isChecked()==true){
        newXAxisDateFormat = 0;
    } else if (ui->xAxisDateIsoRadioButton->isChecked()==true){
        newXAxisDateFormat = 1;
    } else if (ui->xAxisDateIsoTwoDigitsRadioButton->isChecked()==true){
        newXAxisDateFormat = 2;
    }

    // get chart theming
    GbpController::ChartTheming newChartTheming;
    if (ui->chartThemingLightRadioButton->isChecked()==true) {
        newChartTheming = GbpController::ChartTheming::FORCE_LIGHT;
    } else if(ui->chartThemingDarkRadioButton->isChecked()==true) {
        newChartTheming = GbpController::ChartTheming::FORCE_DARK;
    } else {
        newChartTheming = GbpController::ChartTheming::FOLLOW_DESKTOP_THEME;
    }


    // *** DETERMINE IMPACTS OF CHOICES MADE ***

    // init the impact structure to "no impact"
    OptionsChangesImpact impact = {.data=DATA_UNCHANGED, .chart_scaling=CHART_SCALING_NONE,
        .decorationColorStreamDef=DECO_NONE, .mouseWheelZoom=WHEEL_ZOOM_NONE,
        .charts_theme=CHARTS_THEME_NONE, .yzeroLine = Y_ZERO_LINE_NONE,
        .xaxisDateFormat=XAXIS_DATE_FORMAT_NONE, .incomeExpenseColor=IECOLOR_UNCHANGED};

    // determine impact of options changes for data. All data have to be recalculated if PV changes
    if ( (newUsePv != GbpController::getInstance().getUsePresentValue()) ||
        ( (newUsePv==true) &&
        (newDiscountrate!=GbpController::getInstance().getPvDiscountRate()) )  ) {
        // All data need to be recalculated, charts completely rebuilt
        impact.data = DATA_RECALCULATE;
    }

    // determine impact of options changes on Cash Balance chart scaling (overscaling factor).
    if (newMainChartScaling !=
            GbpController::getInstance().getPercentageMainChartScaling()  ) {
        // Data stay the same but chart must be rescaled
        impact.chart_scaling = CHART_SCALING_RESCALE;
    }

    // determine impact of options changes on charts theme (overscaling factor).
    if  (
        ( newChartTheming != GbpController::getInstance().getChartTheming() ) ||
        ( darkModeCurveColor != GbpController::getInstance().getDarkModeCurveColor() ) ||
        ( lightModeCurveColor != GbpController::getInstance().getLightModeCurveColor() ) ||
        ( darkModePointColor != GbpController::getInstance().getDarkModePointColor() ) ||
        ( lightModePointColor != GbpController::getInstance().getLightModePointColor() ) ||
        ( darkModeSelectedPointColor !=
            GbpController::getInstance().getDarkModeSelectedPointColor() ) ||
        ( lightModeSelectedPointColor !=
            GbpController::getInstance().getLightModeSelectedPointColor() ) ||
        ( newChartPointSize != GbpController::getInstance().getChartPointSize() ) ||
        ( gridlinesDarkModeColor != GbpController::getInstance().getGridlinesDarkModeColor() ) ||
        ( gridlinesLightModeColor != GbpController::getInstance().getGridlinesLightModeColor() ) )
        {
        // Data and chart's scaling stay the same : just redraw charts with different
        // colors or background
        impact.charts_theme = CHARTS_THEME_REFRESH;
    }

    // determine impact for decorationsd names) change
    if (newAllowDecorationColor != GbpController::getInstance().getAllowDecorationColor()){
        impact.decorationColorStreamDef = DECO_REFRESH;
    }

    // determine impact for wheel mouse zooming behavior
    if (newWheelZoomIn != GbpController::getInstance().getWheelRotatedAwayZoomIn()){
        impact.mouseWheelZoom = WHEEL_ZOOM_REFRESH;
    }

    // determine impact for "show line at Y=0"
    if (newShowYzeroLine != GbpController::getInstance().getShowYzeroLine()){
        impact.yzeroLine = Y_ZERO_LINE_REFRESH;
    }

    // determine impact for "Y=0 line color"
    if (yZeroLineDarkModeColor != GbpController::getInstance().getYZeroLineDarkModeColor()){
        impact.yzeroLine = Y_ZERO_LINE_REFRESH;
    }
    if (yZeroLineLightModeColor != GbpController::getInstance().getYZeroLineLightModeColor()){
        impact.yzeroLine = Y_ZERO_LINE_REFRESH;
    }

    // determine impact for "x-Axis Date Format"
    if (newXAxisDateFormat != GbpController::getInstance().getXAxisDateFormat()){
        impact.xaxisDateFormat = XAXIS_DATE_FORMAT_REFRESH;
    }

    // determine impact for incomes / expenses colors
    if (incomeColor != GbpController::getInstance().getIncomeColor()){
        impact.incomeExpenseColor = IE_COLOR_REFRESH;
    }
    if (expenseColor != GbpController::getInstance().getExpenseColor()){
        impact.incomeExpenseColor = IE_COLOR_REFRESH;
    }

    // *** WARN USER SECTION ***

    // if custom font selected, a choice must have been made
    if ( newUseSystemFont==false) {
        if ( newCustomFontString.length() == 0 ){
            GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
                tr("You must choose a custom font if you don't use the default system font"), {tr("OK")}, 0, 0);
            LOG_DEBUG_INFO("Options Dialog : Aborted : Custom Font not selected");
            return;
        }
    }

    bool warnUserAppRestartRequired = false;
    QStringList changesRequiringStart;

    // if application font has changed, warn user the app has to be restarted
    if ( (GbpController::getInstance().getUseDefaultSystemFont() != newUseSystemFont) ||
        ( (newUseSystemFont==false) &&
        (GbpController::getInstance().getCustomApplicationFont() != newCustomFontString) ) ) {

        warnUserAppRestartRequired = true;
        changesRequiringStart.append(tr("Application font setting has changed."));
        LOG_INFO("    * Font Setting have changed, restart required");
    }

    // if today's date determination mechanism or custom date have changed,
    // warn user the app has to be restarted
    QString oldCustomDateString = GbpController::getInstance().getTodayCustomDate().toString(
        Qt::DateFormat::ISODate);
    QString newCustomDateString = newCustomTodayDate.toString(Qt::DateFormat::ISODate);
    if ( GbpController::getInstance().getTodayUseSystemDate() != newUseSystemDateForToday ) {
        warnUserAppRestartRequired = true;
        changesRequiringStart.append(tr("Today's date settings have changed."));
        LOG_INFO(
            QString("    * Today's date settings have changed, restart required : new value is : "
            " Use System date = %1").arg(newUseSystemDateForToday) );

    } else if ( (newUseSystemDateForToday==false) &&
        (GbpController::getInstance().getTodayCustomDate() != newCustomTodayDate) ){
        warnUserAppRestartRequired = true;
        changesRequiringStart.append(tr("Today's replacement date has changed."));
        LOG_INFO(
            QString("    * Today's custom date has been modified from %1 to %2, restart required")
            .arg(oldCustomDateString).arg(newCustomDateString));

    }

    // if income color has changed, warn user the app has to be restarted
    if ( GbpController::getInstance().getIncomeColor() != incomeColor  ) {
        warnUserAppRestartRequired = true;
        changesRequiringStart.append(tr("Income color has changed."));
        LOG_INFO( QString("    * Income color has changed to %1, restart required")
            .arg(incomeColor.name(QColor::HexRgb)));
    }

    // if expense color has changed, warn user the app has to be restarted
    if ( GbpController::getInstance().getExpenseColor() != expenseColor  ) {
        warnUserAppRestartRequired = true;
        changesRequiringStart.append(tr("Expense color has changed."));
        LOG_INFO( QString("    * Expense color has changed to %1, restart required")
            .arg(expenseColor.name(QColor::HexRgb)));
    }

    // check tooltips show status
    if ( GbpController::getInstance().getShowTooltips() != newShowTooltips ) {
        warnUserAppRestartRequired = true;
        changesRequiringStart.append(tr("Show tooltips setting has changed."));
        LOG_INFO("   * Show tooltips have been modified, restart required");
    }

    if (warnUserAppRestartRequired == true) {
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::WARNING, tr("Warning"),
            tr("Application must be restarted due to the following changes made :")
            + "<ul><li>" + changesRequiringStart.join("</li><li>") + "</li></ul>",
            {tr("OK")}, 0, 0);
    }


    // *** LOGGING THE NEW VALUES BEFORE SAVING, indicating what has changed or not ***

    LOG_INFO("    => All settings have been saved on disk as requested by user. New values are :");
    LOG_INFO(QString("    Impact : data=%1 chart_scaling=%2 deco=%3 charts theme=%4 y_zero_line=%5 "
        "x_axis_date_format=%6 income_expense_color=%7")
        .arg(impact.data).arg(impact.chart_scaling).arg(impact.decorationColorStreamDef)
        .arg(impact.charts_theme).arg(impact.yzeroLine).arg(impact.xaxisDateFormat)
        .arg(impact.incomeExpenseColor));

    LOG_INFO( QString("    ChartTheming = %1 %2")
        .arg(GbpController::chartThemingToString(newChartTheming))
        .arg( (newChartTheming!=GbpController::getInstance().getChartTheming())?(
            "(CHANGED)"):("")));

    LOG_INFO( QString("    ChartPointSize = %1 %2").arg(newChartPointSize)
        .arg( (newChartPointSize!=GbpController::getInstance()
        .getChartPointSize())?("(CHANGED)"):("")));

    LOG_INFO( QString("    DarkModeCurveColor = %1 %2")
        .arg(darkModeCurveColor.name(QColor::HexRgb))
        .arg((darkModeCurveColor!=GbpController::getInstance().getDarkModeCurveColor())?
            ("(CHANGED)"):("")));

    LOG_INFO( QString("    DarkModePointColor = %1 %2")
        .arg(darkModePointColor.name(QColor::HexRgb))
        .arg((darkModePointColor!=GbpController::getInstance().getDarkModePointColor())?
            ("(CHANGED)"):("")));

    LOG_INFO( QString("    DarkModeSelectedPointColor = %1 %2")
        .arg(darkModeSelectedPointColor.name(QColor::HexRgb))
        .arg((darkModeSelectedPointColor!=GbpController::getInstance()
            .getDarkModeSelectedPointColor())?("(CHANGED)"):("")));

    LOG_INFO( QString("    LightModeCurveColor = %1 %2")
        .arg(lightModeCurveColor.name(QColor::HexRgb))
        .arg((lightModeCurveColor!=GbpController::getInstance().getLightModeCurveColor())?
            ("(CHANGED)"):("")));

    LOG_INFO( QString("    LightModePointColor = %1 %2")
        .arg(lightModePointColor.name(QColor::HexRgb))
        .arg((lightModePointColor!=GbpController::getInstance().getLightModePointColor())?
            ("(CHANGED)"):("")));

    LOG_INFO( QString("    LightModeSelectedPointColor = %1 %2")
        .arg(lightModeSelectedPointColor.name(QColor::HexRgb))
        .arg((lightModeSelectedPointColor!=GbpController::getInstance()
            .getLightModeSelectedPointColor())?("(CHANGED)"):("")));

    LOG_INFO( QString("    ExportTextNumberLocalized = %1 %2")
        .arg(newExportTextNumberLocalized)
        .arg((newExportTextNumberLocalized!=GbpController::getInstance().
            getExportTextNumberLocalized())?("(CHANGED)"):("")));

    LOG_INFO( QString("    ExportTextDateLocalized = %1 %2")
        .arg(newExportTextDateLocalized)
        .arg((newExportTextDateLocalized!=GbpController::getInstance()
            .getExportTextDateLocalized())?("(CHANGED)"):("")));

    LOG_INFO( QString("    PercentageMainChartScaling = %1 %2")
        .arg(newMainChartScaling)
        .arg((newMainChartScaling!=GbpController::getInstance().getPercentageMainChartScaling())?
            ("(CHANGED)"):("")));

    LOG_INFO( QString("    UseDefaultSystemFont = %1 %2").arg(newUseSystemFont)
        .arg((newUseSystemFont != GbpController::getInstance().getUseDefaultSystemFont())?
            ("(CHANGED)"):("")));

    LOG_INFO( QString("    CustomApplicationFont = %1 %2")
        .arg(newCustomFontString)
        .arg((newCustomFontString!=GbpController::getInstance().getCustomApplicationFont())?
            ("(CHANGED)"):("")));

    LOG_INFO( QString("    UseSystemDateForToday = %1 %2")
        .arg(newUseSystemDateForToday)
        .arg((newUseSystemDateForToday!=GbpController::getInstance()
            .getTodayUseSystemDate())?("(CHANGED)"):("")));

    LOG_INFO( QString("    CustomTodayDate = %1 %2")
        .arg(newCustomDateString)
        .arg((oldCustomDateString!=newCustomDateString)?("(CHANGED)"):("")));

    LOG_INFO( QString("    AllowDecorationColor = %1 %2")
        .arg(newAllowDecorationColor)
        .arg((newAllowDecorationColor!=GbpController::getInstance().getAllowDecorationColor())?
            ("(CHANGED)"):("")));

    LOG_INFO( QString("    UsePresentValue = %1 %2")
        .arg(newUsePv)
        .arg((newUsePv!=GbpController::getInstance().getUsePresentValue())?("(CHANGED)"):("")));

    LOG_INFO( QString("    pvDiscountrate = %1 %2")
        .arg(newDiscountrate)
        .arg((newDiscountrate!=GbpController::getInstance().getPvDiscountRate())?
            ("(CHANGED)"):("")));

    LOG_INFO( QString("    ShowTooltips = %1 %2")
        .arg(newShowTooltips)
        .arg((newShowTooltips!=GbpController::getInstance().getShowTooltips())?("(CHANGED)"):("")));

    LOG_INFO( QString("    WheelRotatedAwayZoomIn = %1 %2")
        .arg(newWheelZoomIn)
        .arg((newWheelZoomIn!=GbpController::getInstance().getWheelRotatedAwayZoomIn())?
            ("(CHANGED)"):("")));

    LOG_INFO( QString("    ShowYzeroLine = %1 %2")
        .arg(newShowYzeroLine)
        .arg((newShowYzeroLine!=GbpController::getInstance().getShowYzeroLine())?
            ("(CHANGED)"):("")));

    LOG_INFO( QString("    yZeroLineDarkModeColor = %1 %2")
        .arg(yZeroLineDarkModeColor.name(QColor::HexRgb))
        .arg((yZeroLineDarkModeColor!=GbpController::getInstance().getYZeroLineDarkModeColor())?
            ("(CHANGED)"):("")));

    LOG_INFO( QString("    yZeroLineLightModeColor = %1 %2").arg(yZeroLineLightModeColor.name(QColor::HexRgb))
        .arg((yZeroLineLightModeColor!=GbpController::getInstance().getYZeroLineLightModeColor())?
        ("(CHANGED)"):("")));

    LOG_INFO( QString("    gridlinesDarkModeColor = %1 %2")
        .arg(gridlinesDarkModeColor.name(QColor::HexRgb))
        .arg((gridlinesDarkModeColor!=GbpController::getInstance().getGridlinesDarkModeColor())?
            ("(CHANGED)"):("")));

    LOG_INFO( QString("    gridlinesLightModeColor = %1 %2")
        .arg(gridlinesLightModeColor.name(QColor::HexRgb))
        .arg((gridlinesLightModeColor!=GbpController::getInstance().getGridlinesLightModeColor())?
            ("(CHANGED)"):("")));

    LOG_INFO( QString("    XAxis Date Format = %1 %2")
        .arg(newXAxisDateFormat)
        .arg((newXAxisDateFormat!=GbpController::getInstance().getXAxisDateFormat())?
            ("(CHANGED)"):("")));

    LOG_INFO( QString("    Income Color = %1 %2")
        .arg(incomeColor.name(QColor::HexRgb))
        .arg((incomeColor!=GbpController::getInstance().getIncomeColor())?
            ("(CHANGED)"):("")));

    LOG_INFO( QString("    Expense Color = %1 %2")
        .arg(expenseColor.name(QColor::HexRgb))
        .arg((expenseColor!=GbpController::getInstance().getExpenseColor())?
            ("(CHANGED)"):("")));

    // *** RECORD NEW VALUES IN THE SETTING ***
    // Update everything even if not changed

    GbpController::getInstance().setChartTheming(newChartTheming);
    GbpController::getInstance().setChartPointSize(newChartPointSize);
    GbpController::getInstance().setDarkModeCurveColor(darkModeCurveColor);
    GbpController::getInstance().setDarkModePointColor(darkModePointColor);
    GbpController::getInstance().setDarkModeSelectedPointColor(darkModeSelectedPointColor);
    GbpController::getInstance().setLightModeCurveColor(lightModeCurveColor);
    GbpController::getInstance().setLightModePointColor(lightModePointColor);
    GbpController::getInstance().setLightModeSelectedPointColor(lightModeSelectedPointColor);
    GbpController::getInstance().setExportTextNumberLocalized(newExportTextNumberLocalized);
    GbpController::getInstance().setExportTextDateLocalized(newExportTextDateLocalized);
    GbpController::getInstance().setPercentageMainChartScaling(newMainChartScaling);
    GbpController::getInstance().setUseDefaultSystemFont(newUseSystemFont);
    GbpController::getInstance().setCustomApplicationFont(newCustomFontString);
    GbpController::getInstance().setAllowDecorationColor(newAllowDecorationColor);
    GbpController::getInstance().setTodayUseSystemDate(newUseSystemDateForToday);
    GbpController::getInstance().setTodayCustomDate(newCustomTodayDate);
    GbpController::getInstance().setPvDiscountRate(newDiscountrate);
    GbpController::getInstance().setUsePresentValue(newUsePv);
    GbpController::getInstance().setWheelRotatedAwayZoomIn(newWheelZoomIn);
    GbpController::getInstance().setShowYzeroLine(newShowYzeroLine);
    GbpController::getInstance().setYZeroLineDarkModeColor(yZeroLineDarkModeColor);
    GbpController::getInstance().setYZeroLineLightModeColor(yZeroLineLightModeColor);
    GbpController::getInstance().setGridlinesDarkModeColor(gridlinesDarkModeColor);
    GbpController::getInstance().setGridlinesLightModeColor(gridlinesLightModeColor);
    GbpController::getInstance().setXAxisDateFormat(newXAxisDateFormat);
    GbpController::getInstance().setShowTooltips(newShowTooltips);
    GbpController::getInstance().setIncomeColor(incomeColor);
    GbpController::getInstance().setExpenseColor(expenseColor);

    GbpController::getInstance().saveSettings();
    LOG_INFO("Options Dialog : \"Save\" request completed successfully" );

    // Send to caller for action and hide
    emit signalOptionsResult(impact);
    emit signalOptionsCompleted();
    this->hide();
}


void OptionsDialog::on_cancelPushButton_clicked()
{
    this->hide();
    emit signalOptionsCompleted();
}


void OptionsDialog::on_OptionsDialog_rejected()
{
    on_cancelPushButton_clicked();
}


void OptionsDialog::setColorInfo(ColorItem item, QColor theColor)
{
    QString COLOR_STYLE("QPushButton { background-color : %1; border: none;}");
    QColor c;
    QLabel* label;
    QPushButton* pushButton;

    switch (item) {
        case CI_CURVE_DT:
            label = ui->darkModeCurveColorLabel;
            pushButton = ui->darkModeCurveColorPushButton;
            break;
        case CI_POINT_DT:
            label = ui->darkModePointColorLabel;
            pushButton = ui->darkModePointColorPushButton;
            break;
        case CI_SELECTED_POINT_DT:
            label = ui->darkModeSelectedPointColorLabel;
            pushButton = ui->darkModeSelectedPointColorPushButton;
            break;
        case CI_YZERO_LINE_DT:
            label = ui->darkModeYzeroLineColorLabel;
            pushButton = ui->darkModeYzeroLineColorPushButton;
            break;
        case CI_CURVE_LT:
            label = ui->lightModeCurveColorLabel;
            pushButton = ui->lightModeCurveColorPushButton;
            break;
        case CI_POINT_LT:
            label = ui->lightModePointColorLabel;
            pushButton = ui->lightModePointColorPushButton;
            break;
        case CI_SELECTED_POINT_LT:
            label = ui->lightModeSelectedPointColorLabel;
            pushButton = ui->lightModeSelectedPointColorPushButton;
            break;
        case CI_YZERO_LINE_LT:
            label = ui->lightModeYzeroLineColorLabel;
            pushButton = ui->lightModeYzeroLineColorPushButton;
            break;
        case CI_GRIDLINES_DT:
            label = ui->darkModeGridlinesColorLabel;
            pushButton = ui->darkModeGridlinesColorPushButton;
            break;
        case CI_GRIDLINES_LT:
            label = ui->lightModeGridlinesColorLabel;
            pushButton = ui->lightModeGridlinesColorPushButton;
            break;
        case CI_INCOME_COLOR:
            label = ui->incomeColorLabel;
            pushButton = ui->incomeColorPushButton;
            break;
        case CI_EXPENSE_COLOR:
            label = ui->expenseColorLabel;
            pushButton = ui->expenseColorPushButton;
            break;

        default:
            throw std::invalid_argument("Unknown color item");
            break;
    }

    pushButton->setStyleSheet(COLOR_STYLE.arg(theColor.name()));
    label->setText(Util::buildColorDisplayName(theColor));
}


QString OptionsDialog::fontLabel(const QFont font) const
{
    QFont::Style style = font.style();
    QFont::Weight w = font.weight();

    // the info we report depends on the style and weight
    QString customFontString;
    if ( (style != QFont::Style::StyleNormal) && (w != QFont::Weight::Normal) ){
        customFontString = QString("%1 %2 %3 %4").arg(font.family()).arg(
            fontWeightToString(font)).arg(fontStyleToString(font)).arg(font.pointSize());
    } else if ( (style == QFont::Style::StyleNormal) && (w != QFont::Weight::Normal) ) {
        customFontString = QString("%1 %2 %3").arg(font.family()).arg(fontWeightToString(font)).arg(
            font.pointSize());
    } else if ( (style != QFont::Style::StyleNormal) && (w == QFont::Weight::Normal) ) {
        customFontString = QString("%1 %2 %3").arg(font.family()).arg(fontStyleToString(font)).arg(
            font.pointSize());
    } else if ( (style == QFont::Style::StyleNormal) && (w == QFont::Weight::Normal) ) {
        customFontString = QString("%1 %2").arg(font.family()).arg(font.pointSize());
    }

    return customFontString;
}


QString OptionsDialog::fontStyleToString(const QFont font) const
{
    QFont::Style style = font.style();
    switch(style){
        case QFont::StyleNormal:
            return "Normal";
            break;
        case QFont::StyleItalic:
            return "Italic";
            break;
        case QFont::StyleOblique:
            return "Oblique";
            break;
        default:
            return "";
    }
}


// Didn't find any transalation of these terms in French...
QString OptionsDialog::fontWeightToString(const QFont font) const
{
    QFont::Weight w = font.weight();
    switch(w){
        case QFont::Weight::Black:
            return "Black";
            break;
        case QFont::Thin:
            return "Thin";
            break;
        case QFont::ExtraLight:
            return "ExtraLight";
            break;
        case QFont::Light:
            return "Light";
            break;
        case QFont::Normal:
            return "Normal";
            break;
        case QFont::Medium:
            return "Medium";
            break;
        case QFont::DemiBold:
            return "DemiBold";
            break;
        case QFont::Bold:
            return "Bold";
            break;
        case QFont::ExtraBold:
            return "ExtraBold";
            break;
        default:
            return "";
        }
}


void OptionsDialog::setCustomFontlabel(QString fontLabel)
{
    QString widgetString = QString(tr("Custom : %1")).arg(fontLabel);
    ui->customFontRadioButton->setText(widgetString);
}


void OptionsDialog::on_setCustomFontPushButton_clicked()
{
    bool ok;
    QFont f;

    // try to pass the current custom font as selected font, if there is any
    if (newCustomFontString.length() != 0) {
        QFont cFont;
        bool fOK = cFont.fromString(newCustomFontString);
        if (fOK){
            // FontDialog seems to have a bug some time with the font passed...
            f = QFontDialog::getFont(&ok);
        } else {
            LOG_ERROR("Cannot create custom font : " +newCustomFontString);
            f = QFontDialog::getFont(&ok );
        }
    } else {
        LOG_INFO("No custom font available to pass to FontDialog");
        f = QFontDialog::getFont(&ok );
    }

    if (ok){
        LOG_INFO( "Font returned by FontDialog : " + f.toString());
        // record the new font
        newCustomFontString = f.toString();
        // update the label of custom string
        setCustomFontlabel(fontLabel(f));
    }
}


void OptionsDialog::on_systemFontRadioButton_toggled(bool checked)
{
    if ( ui->systemFontRadioButton->isChecked()){
        ui->setCustomFontPushButton->setEnabled(false);
    } else {
        ui->setCustomFontPushButton->setEnabled(true);
    }
}


void OptionsDialog::on_todaySystemRadioButton_toggled(bool checked)
{
    if (ui->todaySystemRadioButton->isChecked()) {
        ui->todayDateEdit->setEnabled(false);
    } else {
        ui->todayDateEdit->setEnabled(true);
    }
}


void OptionsDialog::on_usePresentValueCheckBox_toggled(bool checked)
{
    if( ui->usePresentValueCheckBox->isChecked()){
        ui->discountRateLabel->setEnabled(true);
        ui->pvDiscountRateDoubleSpinBox->setEnabled(true);
    } else {
        ui->discountRateLabel->setEnabled(false);
        ui->pvDiscountRateDoubleSpinBox->setEnabled(false);
    }

}


void OptionsDialog::on_darkModeCurveColorPushButton_clicked()
{
    //QColorDialog::ColorDialogOptions opt = QColorDialog::ColorDialogOptions();
    QColor color = QColorDialog::getColor(darkModeCurveColor, this, tr("Color chooser"));
    if (color.isValid()==false) {
        return;
    } else {
        darkModeCurveColor = color;
        setColorInfo(CI_CURVE_DT, darkModeCurveColor);
    }
}


void OptionsDialog::on_darkModePointColorPushButton_clicked()
{
    //QColorDialog::ColorDialogOptions opt = QColorDialog::ColorDialogOptions();
    QColor color = QColorDialog::getColor(darkModePointColor, this, tr("Color chooser"));
    if (color.isValid()==false) {
        return;
    } else {
        darkModePointColor = color;
        setColorInfo(CI_POINT_DT, darkModePointColor);
    }
}


void OptionsDialog::on_darkModeSelectedPointColorPushButton_clicked()
{
    //QColorDialog::ColorDialogOptions opt = QColorDialog::ColorDialogOptions();
    QColor color = QColorDialog::getColor(darkModeSelectedPointColor, this, tr("Color chooser"));
    if (color.isValid()==false) {
        return;
    } else {
        darkModeSelectedPointColor = color;
        setColorInfo(CI_SELECTED_POINT_DT, darkModeSelectedPointColor);
    }
}


void OptionsDialog::on_lightModeCurveColorPushButton_clicked()
{
    //QColorDialog::ColorDialogOptions opt = QColorDialog::ColorDialogOptions();
    QColor color = QColorDialog::getColor(lightModeCurveColor, this, tr("Color chooser"));
    if (color.isValid()==false) {
        return;
    } else {
        lightModeCurveColor = color;
        setColorInfo(CI_CURVE_LT, lightModeCurveColor);
    }
}


void OptionsDialog::on_lightModePointColorPushButton_clicked()
{
    //QColorDialog::ColorDialogOptions opt = QColorDialog::ColorDialogOptions();
    QColor color = QColorDialog::getColor(lightModePointColor, this, tr("Color chooser"));
    if (color.isValid()==false) {
        return;
    } else {
        lightModePointColor = color;
        setColorInfo(CI_POINT_LT, lightModePointColor);
    }
}


void OptionsDialog::on_lightModeSelectedPointColorPushButton_clicked()
{
    //QColorDialog::ColorDialogOptions opt = QColorDialog::ColorDialogOptions();
    QColor color = QColorDialog::getColor(lightModeSelectedPointColor, this, tr("Color chooser"));
    if (color.isValid()==false) {
        return;
    } else {
        lightModeSelectedPointColor = color;
        setColorInfo(CI_SELECTED_POINT_LT, lightModeSelectedPointColor);
    }
}


void OptionsDialog::on_darkModeYzeroLineColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor(yZeroLineDarkModeColor, this, tr("Color chooser"));
    if (color.isValid()==false) {
        return;
    } else {
        yZeroLineDarkModeColor = color;
        setColorInfo(CI_YZERO_LINE_DT, yZeroLineDarkModeColor);
    }
}


void OptionsDialog::on_lightModeYzeroLineColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor(yZeroLineLightModeColor, this, tr("Color chooser"));
    if (color.isValid()==false) {
        return;
    } else {
        yZeroLineLightModeColor = color;
        setColorInfo(CI_YZERO_LINE_LT, yZeroLineLightModeColor);
    }
}


void OptionsDialog::on_darkModeGridlinesColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor(gridlinesDarkModeColor, this, tr("Color chooser"));
    if (color.isValid()==false) {
        return;
    } else {
        gridlinesDarkModeColor = color;
        setColorInfo(CI_GRIDLINES_DT, gridlinesDarkModeColor);
    }
}


void OptionsDialog::on_lightModeGridlinesColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor(gridlinesLightModeColor, this, tr("Color chooser"));
    if (color.isValid()==false) {
        return;
    } else {
        gridlinesLightModeColor = color;
        setColorInfo(CI_GRIDLINES_LT, gridlinesLightModeColor);
    }
}


void OptionsDialog::on_lightModeCurveColorResetPushButton_clicked()
{
    lightModeCurveColor = GbpController::getInstance().getFactorySettings().lightModeCurveColor;
    setColorInfo(CI_CURVE_LT, lightModeCurveColor);
}


void OptionsDialog::on_lightModePointColorResetPushButton_clicked()
{
    lightModePointColor = GbpController::getInstance().getFactorySettings().lightModePointColor;
    setColorInfo(CI_POINT_LT, lightModePointColor);
}


void OptionsDialog::on_lightModeSelectedPointColorResetPushButton_clicked()
{
    lightModeSelectedPointColor =
        GbpController::getInstance().getFactorySettings().lightModeSelectedPointColor;
    setColorInfo(CI_SELECTED_POINT_LT, lightModeSelectedPointColor);
}


void OptionsDialog::on_lightModeYzeroLineColorResetPushButton_clicked()
{
    yZeroLineLightModeColor =
        GbpController::getInstance().getFactorySettings().yZeroLineLightModeColor;
    setColorInfo(CI_YZERO_LINE_LT, yZeroLineLightModeColor);
}


void OptionsDialog::on_lightModeGridlinesColorResetPushButton_clicked()
{
    gridlinesLightModeColor =
        GbpController::getInstance().getFactorySettings().gridlinesLightModeColor;
    setColorInfo(CI_GRIDLINES_LT, gridlinesLightModeColor);
}


void OptionsDialog::on_darkModeCurveColorResetPushButton_clicked()
{
    darkModeCurveColor = GbpController::getInstance().getFactorySettings().darkModeCurveColor;
    setColorInfo(CI_CURVE_DT, darkModeCurveColor);
}


void OptionsDialog::on_darkModePointColorResetPushButton_clicked()
{
    darkModePointColor = GbpController::getInstance().getFactorySettings().darkModePointColor;
    setColorInfo(CI_POINT_DT, darkModePointColor);
}


void OptionsDialog::on_darkModeSelectedPointColorResetPushButton_clicked()
{
    darkModeSelectedPointColor =
        GbpController::getInstance().getFactorySettings().darkModeSelectedPointColor;
    setColorInfo(CI_SELECTED_POINT_DT, darkModeSelectedPointColor);
}


void OptionsDialog::on_darkModeYzeroLineColorResetPushButton_clicked()
{
    yZeroLineDarkModeColor =
        GbpController::getInstance().getFactorySettings().yZeroLineDarkModeColor;
    setColorInfo(CI_YZERO_LINE_DT, yZeroLineDarkModeColor);
}


void OptionsDialog::on_darkModeGridlinesColorResetPushButton_clicked()
{
    gridlinesDarkModeColor =
        GbpController::getInstance().getFactorySettings().gridlinesDarkModeColor;
    setColorInfo(CI_GRIDLINES_DT, gridlinesDarkModeColor);
}


void OptionsDialog::on_incomeColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor(incomeColor, this, tr("Color chooser"));
    if (color.isValid()==false) {
        return;
    } else {
        incomeColor = color;
        setColorInfo(CI_INCOME_COLOR, incomeColor);
    }
}


void OptionsDialog::on_expenseColorPushButton_clicked()
{
    QColor color = QColorDialog::getColor(expenseColor, this, tr("Color chooser"));
    if (color.isValid()==false) {
        return;
    } else {
        expenseColor = color;
        setColorInfo(CI_EXPENSE_COLOR, expenseColor);
    }
}


void OptionsDialog::on_incomeColorResetPushButton_clicked()
{
    incomeColor = Util::getOptimizedGreen();
    setColorInfo(CI_INCOME_COLOR, incomeColor);
}


void OptionsDialog::on_expenseColorResetPushButton_clicked()
{
    expenseColor = Util::getOptimizedRed();
    setColorInfo(CI_EXPENSE_COLOR, expenseColor);
}


void OptionsDialog::on_resetPushButton_clicked()
{
    // Ask confirmation before resetting all the settings
    int choice = GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::WARNING, tr("Warning"),
        tr("All settings will be reset to their factory defaults once the \"Save\" button "
        "is pressed. Are you certain you wish to continue?"), {tr("No"),tr("Yes")},0,0);
    switch(choice){
        case -1:
            return;
            break;
        case 0:
            return;
            break;
    }

    // Lets reset the settings, but all changes are NOT saved (user must press Save)
    GbpController::FactorySettings fs = GbpController::getInstance().getFactorySettings();

    ui->todaySystemRadioButton->setChecked(fs.todayUseSystemDate);
    ui->systemFontRadioButton->setChecked(fs.useDefaultSystemFont);
    newCustomFontString = fs.customApplicationFont; // ""
    setCustomFontlabel(newCustomFontString);
    ui->allowDecorationColorCheckBox->setChecked(fs.allowDecorationColor);
    ui->exportTextAmountLocalizedCheckBox->setChecked(fs.exportTextNumberLocalized);
    ui->exportTextDateLocalizedCheckBox->setChecked(fs.exportTextDateLocalized);
    ui->usePresentValueCheckBox->setChecked(fs.usePresentValue);
    ui->tooltipsCheckBox->setChecked(fs.showTooltips);
    incomeColor = fs.incomeColor;
    setColorInfo(CI_INCOME_COLOR, incomeColor);
    expenseColor = fs.expenseColor;
    setColorInfo(CI_EXPENSE_COLOR, expenseColor);

    ui->pointSizeSpinBox->setValue(fs.chartPointSize);
    ui->scalingMainChartSpinBox->setValue(fs.percentageMainChartScaling);
    ui->wheelZoomInRadioButton->setChecked(fs.wheelRotatedAwayZoomIn);
    ui->showYzeroLineCheckBox->setChecked(fs.showYzeroLine);
    switch(fs.xAxisDateFormat){
        case 0:
            ui->xAxisDateLocaleRadioButton->setChecked(true);
            break;
        case 1:
            ui->xAxisDateIsoRadioButton->setChecked(true);
            break;
        case 2:
            ui->xAxisDateIsoTwoDigitsRadioButton->setChecked(true);
            break;
    }

    switch(fs.chartTheming){
        case GbpController::ChartTheming::FORCE_LIGHT:
            ui->chartThemingLightRadioButton->setChecked(true);
            break;
        case GbpController::ChartTheming::FORCE_DARK:
            ui->chartThemingDarkRadioButton->setChecked(true);
            break;
        case GbpController::ChartTheming::FOLLOW_DESKTOP_THEME:
            ui->chartThemingFollowRadioButton->setChecked(true);
            break;
        }

    lightModeCurveColor = fs.lightModeCurveColor;
    setColorInfo(CI_CURVE_LT, lightModeCurveColor);
    lightModePointColor = fs.lightModePointColor;
    setColorInfo(CI_POINT_LT, lightModePointColor);
    lightModeSelectedPointColor = fs.lightModeSelectedPointColor;
    setColorInfo(CI_SELECTED_POINT_LT, lightModeSelectedPointColor);
    darkModeCurveColor = fs.darkModeCurveColor;
    setColorInfo(CI_CURVE_DT, darkModeCurveColor);
    darkModePointColor = fs.darkModePointColor;
    setColorInfo(CI_POINT_DT, darkModePointColor);
    darkModeSelectedPointColor = fs.darkModeSelectedPointColor;
    setColorInfo(CI_SELECTED_POINT_DT, darkModeSelectedPointColor);
    yZeroLineLightModeColor = fs.yZeroLineLightModeColor;
    setColorInfo(CI_YZERO_LINE_LT, yZeroLineLightModeColor);
    yZeroLineDarkModeColor = fs.yZeroLineDarkModeColor;
    setColorInfo(CI_YZERO_LINE_DT, fs.yZeroLineDarkModeColor);
}


void OptionsDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("OptionsDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
    });
}

