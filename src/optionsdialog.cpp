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

#include <QColorDialog>
#include <QPalette>
#include <QFontDialog>
#include <QMessageBox>
#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include "gbpcontroller.h"
#include "util.h"


OptionsDialog::OptionsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OptionsDialog)
{
    ui->setupUi(this);

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

}


OptionsDialog::~OptionsDialog()
{
    delete ui;
}


void OptionsDialog::slotPrepareContent()
{
    // load settings and init controls
    ui->chartDarkModeCheckBox->setChecked(GbpController::getInstance().getIsDarkModeSet());

    // chart point size
    ui->pointSizeSpinBox->setValue(GbpController::getInstance().getChartPointSize());

    // curve Dark Mode widgets
    darkModeCurveColor = GbpController::getInstance().getDarkModeCurveColor();
    darkModePointColor = GbpController::getInstance().getDarkModePointColor();
    darkModeSelectedPointColor = GbpController::getInstance().getDarkModeSelectedPointColor();
    setColorInfo(CT_DARK_MODE,CI_CURVE);
    setColorInfo(CT_DARK_MODE,CI_POINT);
    setColorInfo(CT_DARK_MODE,CI_SELECTED_POINT);

    // curve Light Mode widgets
    lightModeCurveColor = GbpController::getInstance().getLightModeCurveColor();
    lightModePointColor = GbpController::getInstance().getLightModePointColor();
    lightModeSelectedPointColor = GbpController::getInstance().getLightModeSelectedPointColor();
    setColorInfo(CT_LIGHT_MODE,CI_CURVE);
    setColorInfo(CT_LIGHT_MODE,CI_POINT);
    setColorInfo(CT_LIGHT_MODE,CI_SELECTED_POINT);

    // export text amount localization
    ui->exportTextAmountLocalizedCheckBox->setChecked(
        GbpController::getInstance().getExportTextAmountLocalized());

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
    if (GbpController::getInstance().getTodayUseSystemDate()==true) {
        ui->todaySystemRadioButton->setChecked(true);
        ui->todayDateEdit->setDate(QDate::currentDate());
        ui->todayDateEdit->setEnabled(false);
    } else {
        ui->todaySpecificRadioButton->setChecked(true);
        ui->todayDateEdit->setDate(GbpController::getInstance().getTodayCustomDate());
        ui->todayDateEdit->setEnabled(true);
    }

    // Allow Decoration Colors
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

}


// It is this Dialog that determines the impacts of the settings changes on the application
void OptionsDialog::on_applyPushButton_clicked()
{
    uint newMainChartScaling = ui->scalingMainChartSpinBox->value();
    bool chartDarkMode = ui->chartDarkModeCheckBox->isChecked();
    bool usePV = ui->usePresentValueCheckBox->isChecked();
    double discountrate = ui->pvDiscountRateDoubleSpinBox->value();
    uint chartPointSize = ui->pointSizeSpinBox->value();

    OptionsChangesImpact impact = {.data=DATA_UNCHANGED, .chart_scaling=CHART_SCALING_NONE,
        .decorationColorStreamDef=DECO_NONE, .mouseWheelZoom=WHEEL_ZOOM_NONE,.charts_theme=CHARTS_THEME_NONE};

    // determine impact of options changes for data. All data have to be recalculated if PV changes
    if ( (usePV != GbpController::getInstance().getUsePresentValue()) ||
        ( (usePV) && (discountrate!=GbpController::getInstance().getPvDiscountRate()) )  ) {
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
        ( chartDarkMode != GbpController::getInstance().getIsDarkModeSet() ) ||
        ( darkModeCurveColor != GbpController::getInstance().getDarkModeCurveColor() ) ||
        ( lightModeCurveColor != GbpController::getInstance().getLightModeCurveColor() ) ||
        ( darkModePointColor != GbpController::getInstance().getDarkModePointColor() ) ||
        ( lightModePointColor != GbpController::getInstance().getLightModePointColor() ) ||
        ( darkModeSelectedPointColor !=
            GbpController::getInstance().getDarkModeSelectedPointColor() ) ||
        ( lightModeSelectedPointColor !=
            GbpController::getInstance().getLightModeSelectedPointColor() ) ||
        ( chartPointSize != GbpController::getInstance().getChartPointSize() ) )
        {
        // Data and chart's scaling stay the same : just redraw charts with different
        // colors or background
        impact.charts_theme = CHARTS_THEME_REFRESH;
    }

    // determine impact for decoration change
    if (ui->allowDecorationColorCheckBox->isChecked() !=
        GbpController::getInstance().getAllowDecorationColor()){
        impact.decorationColorStreamDef = DECO_REFRESH;
    }

    // determine impact for wheel mouse zooming behavior
    if (ui->wheelZoomInRadioButton->isChecked() !=
        GbpController::getInstance().getWheelRotatedAwayZoomIn()){
        impact.mouseWheelZoom = WHEEL_ZOOM_REFRESH;
    }

    // if custom font selected, a choice must have been made
    if ( ui->customFontRadioButton->isChecked() == true) {
        if ( newCustomFontString.length() == 0 ){
            QMessageBox::warning(nullptr,tr("Font Unselected"),
                tr("You must choose a custom font if you don't use the default system font"));
            return;
        }
    }

    // if application font has changed, warn user the app has to be restarted
    if ( (GbpController::getInstance().getUseDefaultSystemFont() !=
         ui->systemFontRadioButton->isChecked()) ||
        (GbpController::getInstance().getCustomApplicationFont() != newCustomFontString) ) {
        QMessageBox::warning(nullptr,tr("Font Changed"),
            tr("Application must be restarted for font changes to take effect"));
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
            "Font Setting changed");
    }

    // if today's date determination mechanism or custom date have changed,
    // warn user the app has to be restarted
    QString oldCustomDateString = GbpController::getInstance().getTodayCustomDate().toString(
        Qt::DateFormat::ISODate);
    QString newCustomDateString = ui->todayDateEdit->date().toString(Qt::DateFormat::ISODate);
    if ( GbpController::getInstance().getTodayUseSystemDate() !=
        ui->todaySystemRadioButton->isChecked() ) {
        QMessageBox::warning(nullptr,tr("Today's Determination Changed"),
            tr("Today's date determination mechanism changed. Application must be restarted for changes to take effect"));
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
            QString("Today's date determination mechanism changed : new settings -> System Date=%1").arg(ui->todaySystemRadioButton->isChecked()) );
    } else if ( (ui->todaySpecificRadioButton->isChecked()) &&
               (GbpController::getInstance().getTodayCustomDate() != ui->todayDateEdit->date()) ){
        QMessageBox::warning(nullptr,tr("Today's Custom Date Changed"),
            tr("Today's replacement date has changed from %1 to %2. Application must be restarted for changes to take effect").arg(oldCustomDateString).arg(newCustomDateString));
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
            QString("Today's custom date has changed from %1 to %2").arg(oldCustomDateString).arg
                (newCustomDateString));
    }

    // set settings new values

    GbpController::getInstance().setIsDarkModeSet(chartDarkMode);
    GbpController::getInstance().setChartPointSize(chartPointSize);
    GbpController::getInstance().setDarkModeCurveColor(darkModeCurveColor);
    GbpController::getInstance().setDarkModePointColor(darkModePointColor);
    GbpController::getInstance().setDarkModeSelectedPointColor(darkModeSelectedPointColor);
    GbpController::getInstance().setLightModeCurveColor(lightModeCurveColor);
    GbpController::getInstance().setLightModePointColor(lightModePointColor);
    GbpController::getInstance().setLightModeSelectedPointColor(lightModeSelectedPointColor);
    GbpController::getInstance().setExportTextAmountLocalized(
        ui->exportTextAmountLocalizedCheckBox->isChecked());
    GbpController::getInstance().setPercentageMainChartScaling(newMainChartScaling);
    GbpController::getInstance().setUseDefaultSystemFont(ui->systemFontRadioButton->isChecked());
    GbpController::getInstance().setCustomApplicationFont(newCustomFontString);
    GbpController::getInstance().setTodayUseSystemDate(ui->todaySystemRadioButton->isChecked());
    GbpController::getInstance().setAllowDecorationColor(
        ui->allowDecorationColorCheckBox->isChecked());
    if (ui->todaySpecificRadioButton->isChecked()) {
        GbpController::getInstance().setTodayCustomDate(ui->todayDateEdit->date());
    } else {
        GbpController::getInstance().setTodayCustomDate(QDate());
    }
    GbpController::getInstance().setPvDiscountRate(ui->pvDiscountRateDoubleSpinBox->value());
    GbpController::getInstance().setUsePresentValue(ui->usePresentValueCheckBox->isChecked());
    GbpController::getInstance().setWheelRotatedAwayZoomIn(
        ui->wheelZoomInRadioButton->isChecked());


    GbpController::getInstance().saveSettings();

    // Log the new values
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString(
        "Options have been changed"));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString(
        "    Impact : data=%1 chart_scaling=%2 deco=%3 charts theme=%4").arg(impact.data).
        arg(impact.chart_scaling).arg(impact.decorationColorStreamDef).arg(impact.charts_theme));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString(
        "    ChartDarkMode = %1").arg(chartDarkMode));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString(
        "    DarkModeCurveColor = %1").arg(darkModeCurveColor.rgba()));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString(
        "    DarkModePointColor = %1").arg(darkModePointColor.rgba()));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString(
        "    DarkModeSelectedPointColor = %1").arg(darkModeSelectedPointColor.rgba()));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString(
        "    LightModeCurveColor = %1").arg(lightModeCurveColor.rgba()));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString(
        "    LightModePointColor = %1").arg(lightModePointColor.rgba()));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString(
        "    LightModeSelectedPointColor = %1").arg(lightModeSelectedPointColor.rgba()));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString(
        "    ExportTextAmountLocalized = %1").arg(
        ui->exportTextAmountLocalizedCheckBox->isChecked()));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString(
        "    PercentageMainChartScaling = %1").arg(newMainChartScaling));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString(
        "    UseDefaultSystemFont = %1").arg(ui->systemFontRadioButton->isChecked()));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString(
        "    CustomApplicationFont = %1").arg(newCustomFontString));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString(
        "    UseSystemDateForToday = %1").arg(ui->todaySystemRadioButton->isChecked()));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString(
        "    CustomTodayDate = %1").arg(
        GbpController::getInstance().getTodayCustomDate().toString(Qt::DateFormat::ISODate)));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString(
        "    AllowDecorationColor = %1").arg(ui->allowDecorationColorCheckBox->isChecked()));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString(
        "    UsePresentValue = %1").arg(ui->usePresentValueCheckBox->isChecked()));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString(
        "    pvDiscountrate = %1").arg(ui->pvDiscountRateDoubleSpinBox->value()));

    emit signalOptionsResult(impact);
    this->hide();
    emit signalOptionsCompleted();
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


void OptionsDialog::setColorInfo(ColorTheme theme, ColorItem item)
{
    QString COLOR_STYLE("QPushButton { background-color : %1; border: none;}");
    QColor c;
    QLabel* label;
    QPushButton* pushButton;
    QColor color;
    if(theme==CT_DARK_MODE){
        if(item==CI_CURVE){
            label = ui->darkModeCurveColorLabel;
            pushButton = ui->darkModeCurveColorPushButton;
            color = darkModeCurveColor;
        } else if (item==CI_POINT){
            label = ui->darkModePointColorLabel;
            pushButton = ui->darkModePointColorPushButton;
            color = darkModePointColor;
        } else if (item==CI_SELECTED_POINT){
            label = ui->darkModeSelectedPointColorLabel;
            pushButton = ui->darkModeSelectedPointColorPushButton;
            color = darkModeSelectedPointColor;        }
        else {
            throw std::invalid_argument("Unknown color item");
        }
    } else if (theme==CT_LIGHT_MODE){
        if(item==CI_CURVE){
            label = ui->lightModeCurveColorLabel;
            pushButton = ui->lightModeCurveColorPushButton;
            color = lightModeCurveColor;
        } else if (item==CI_POINT){
            label = ui->lightModePointColorLabel;
            pushButton = ui->lightModePointColorPushButton;
            color = lightModePointColor;
        } else if (item==CI_SELECTED_POINT){
            label = ui->lightModeSelectedPointColorLabel;
            pushButton = ui->lightModeSelectedPointColorPushButton;
            color = lightModeSelectedPointColor;
        } else {
            throw std::invalid_argument("Unknown color item");
        }
    } else {
        throw std::invalid_argument("Unknown color theme");
    }

    pushButton->setStyleSheet(COLOR_STYLE.arg( color.name()));
    label->setText(Util::buildColorDisplayName(color));
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
            // FontDialog seems to have a bug some tim ewith the font passed...
            f = QFontDialog::getFont(&ok);
        } else {
            GbpController::getInstance().log(GbpController::LogLevel::Minimal,
                GbpController::Warning, "Cannot create custom font : " +newCustomFontString);
            f = QFontDialog::getFont(&ok );
        }
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal,
            GbpController::Info, "No custom font available to pass to FontDialog");
        f = QFontDialog::getFont(&ok );
    }

    if (ok){
        GbpController::getInstance().log(GbpController::LogLevel::Minimal,
            GbpController::Info, "Font returned by FontDialog : " + f.toString());
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
    QColor color = QColorDialog::getColor(darkModeCurveColor, this, tr("Color Chooser"));
    if (color.isValid()==false) {
        return;
    } else {
        darkModeCurveColor = color;
        setColorInfo(CT_DARK_MODE, CI_CURVE);
    }
}


void OptionsDialog::on_darkModePointColorPushButton_clicked()
{
    //QColorDialog::ColorDialogOptions opt = QColorDialog::ColorDialogOptions();
    QColor color = QColorDialog::getColor(darkModePointColor, this, tr("Color Chooser"));
    if (color.isValid()==false) {
        return;
    } else {
        darkModePointColor = color;
        setColorInfo(CT_DARK_MODE, CI_POINT);
    }
}


void OptionsDialog::on_darkModeSelectedPointColorPushButton_clicked()
{
    //QColorDialog::ColorDialogOptions opt = QColorDialog::ColorDialogOptions();
    QColor color = QColorDialog::getColor(darkModeSelectedPointColor, this, tr("Color Chooser"));
    if (color.isValid()==false) {
        return;
    } else {
        darkModeSelectedPointColor = color;
        setColorInfo(CT_DARK_MODE, CI_SELECTED_POINT);
    }
}


void OptionsDialog::on_lightModeCurveColorPushButton_clicked()
{
    //QColorDialog::ColorDialogOptions opt = QColorDialog::ColorDialogOptions();
    QColor color = QColorDialog::getColor(lightModeCurveColor, this, tr("Color Chooser"));
    if (color.isValid()==false) {
        return;
    } else {
        lightModeCurveColor = color;
        setColorInfo(CT_LIGHT_MODE, CI_CURVE);
    }
}


void OptionsDialog::on_lightModePointColorPushButton_clicked()
{
    //QColorDialog::ColorDialogOptions opt = QColorDialog::ColorDialogOptions();
    QColor color = QColorDialog::getColor(lightModePointColor, this, tr("Color Chooser"));
    if (color.isValid()==false) {
        return;
    } else {
        lightModePointColor = color;
        setColorInfo(CT_LIGHT_MODE, CI_POINT);
    }
}


void OptionsDialog::on_lightModeSelectedPointColorPushButton_clicked()
{
    //QColorDialog::ColorDialogOptions opt = QColorDialog::ColorDialogOptions();
    QColor color = QColorDialog::getColor(lightModeSelectedPointColor, this, tr("Color Chooser"));
    if (color.isValid()==false) {
        return;
    } else {
        lightModeSelectedPointColor = color;
        setColorInfo(CT_LIGHT_MODE, CI_SELECTED_POINT);
    }
}

