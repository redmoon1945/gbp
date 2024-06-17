/*
 *  Copyright (C) 2024 Claude Dumas <claudedumas63@protonmail.com>
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

#include <QColorDialog>
#include <QPalette>
#include <QFontDialog>
#include <QMessageBox>
#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include "gbpcontroller.h"


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
    QString widgetString = QString(tr("System Font : %1")).arg(sysFontString);
    ui->systemFontRadioButton->setText(widgetString);

}


OptionsDialog::~OptionsDialog()
{
    delete ui;
}


void OptionsDialog::slotPrepareContent()
{
    // load settings and init controls
    qint16 years = GbpController::getInstance().getScenarioMaxYearsSpan();
    ui->scenarioYearsSpinBox->setValue(years);
    ui->chartDarkModeCheckBox->setChecked(GbpController::getInstance().getChartDarkMode());
    // curve Dark Mode widgets
    curveDarkModeColor = GbpController::getInstance().getCurveDarkModeColor();
    setDarkModeColorInfo();
    // curve Light Mode widgets
    curveLightModeColor = GbpController::getInstance().getCurveLightModeColor();
    setLightModeColorInfo();
    // export text amount localization
    ui->exportTextAmountLocalizedCheckBox->setChecked(GbpController::getInstance().getExportTextAmountLocalized());
    // main chart scaling
    ui->scalingMainChartSpinBox->setValue( GbpController::getInstance().getPercentageMainChartScaling());
    // chart exported image type
    if (GbpController::getInstance().getChartExportImageType()=="JPG"){
        ui->jpgRadioButton->setChecked(true);
    } else {
        ui->pngRadioButton->setChecked(true);
    }
    // chart exported image quality
    ui->qualitySpinBox->setValue(GbpController::getInstance().getChartExportImageQuality());
    // Application Font
    newCustomFontString = "";
    if (GbpController::getInstance().getUseDefaultSystemFont()){
        ui->systemFontRadioButton->setChecked(true);
    } else {
        ui->customFontRadioButton->setChecked(true);
    }
    QString s = GbpController::getInstance().getCustomApplicationFont();    // we dont know if it is valid or not set
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

}


void OptionsDialog::on_applyPushButton_clicked()
{
    qint16 years = ui->scenarioYearsSpinBox->value();
    uint newMainChartScaling = ui->scalingMainChartSpinBox->value();
    bool chartDarkMode = ui->chartDarkModeCheckBox->isChecked();
    QString chartExportType = (ui->pngRadioButton->isChecked() ? ("PNG") : ("JPG"));

    OptionsChangesImpact impact = NONE;

    // determine impact of options changes for the charts : from worst to less worst
    if ( years !=  GbpController::getInstance().getScenarioMaxYearsSpan()) {
        impact= FULL_RECALCULATION_REQUIRED;    // all data need to be recalculate, rescale and replot also
    } else if (newMainChartScaling != GbpController::getInstance().getPercentageMainChartScaling()  ) {
        impact= RESCALE_AND_REPLOT;             // data stay the same, but rescale is required
    } else if (
        ( chartDarkMode != GbpController::getInstance().getChartDarkMode() ) ||
        ( curveDarkModeColor != GbpController::getInstance().getCurveDarkModeColor() ) ||
        ( curveLightModeColor != GbpController::getInstance().getCurveLightModeColor() )
        ) {
        impact = REPLOT;                        // just redraw with different colors or background
    } else {
        impact = NONE;
    }

    // if custom font selected, a choice must have been made
    if ( ui->customFontRadioButton->isChecked() == true) {
        if ( newCustomFontString.length() == 0 ){
            QMessageBox::warning(nullptr,tr("Font Unselected"),tr("You must choose a custom font if you don't use the default system font"));
            return;
        }
    }

    // if application font has changed, warn user the app has to be restarted
    if ( (GbpController::getInstance().getUseDefaultSystemFont() != ui->systemFontRadioButton->isChecked()) ||
        (GbpController::getInstance().getCustomApplicationFont() != newCustomFontString) ) {
        QMessageBox::warning(nullptr,tr("Font Changed"),tr("Application must be restarted for font changes to take effect"));
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "Font Setting changed");
    }

    // if today's date determination mechanism or custom date have changed, warn user the app has to be restarted
    QString oldCustomDateString = GbpController::getInstance().getTodayCustomDate().toString(Qt::DateFormat::ISODate);
    QString newCustomDateString = ui->todayDateEdit->date().toString(Qt::DateFormat::ISODate);
    if ( GbpController::getInstance().getTodayUseSystemDate() != ui->todaySystemRadioButton->isChecked() ) {
        QMessageBox::warning(nullptr,tr("Today's Determination Changed"),
            tr("Today's date determination mechanism changed. Application must be restarted for changes to take effect"));
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
            QString("Today's date determination mechanism changed : new settings -> System Date=%1").arg(ui->todaySystemRadioButton->isChecked()) );
    } else if ( (ui->todaySpecificRadioButton->isChecked()) &&
               (GbpController::getInstance().getTodayCustomDate() != ui->todayDateEdit->date()) ){
        QMessageBox::warning(nullptr,tr("Today's Custom Date Changed"),
            tr("Today's replacement date has changed from %1 to %2. Application must be restarted for changes to take effect").arg(oldCustomDateString).arg(newCustomDateString));
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info,
            QString("Today's custom date has changed from %1 to %2").arg(oldCustomDateString).arg(newCustomDateString));
    }

    // set settings new values
    GbpController::getInstance().setScenarioMaxYearsSpan(years);
    GbpController::getInstance().setChartDarkMode(chartDarkMode);
    GbpController::getInstance().setCurveDarkModeColor(curveDarkModeColor);
    GbpController::getInstance().setCurveLightModeColor(curveLightModeColor);
    GbpController::getInstance().setExportTextAmountLocalized(ui->exportTextAmountLocalizedCheckBox->isChecked());
    GbpController::getInstance().setPercentageMainChartScaling(newMainChartScaling);
    GbpController::getInstance().setChartExportImageQuality(ui->qualitySpinBox->value());
    GbpController::getInstance().setChartExportImageType(chartExportType);
    GbpController::getInstance().setUseDefaultSystemFont(ui->systemFontRadioButton->isChecked());
    GbpController::getInstance().setCustomApplicationFont(newCustomFontString);
    GbpController::getInstance().setTodayUseSystemDate(ui->todaySystemRadioButton->isChecked());
    if (ui->todaySpecificRadioButton->isChecked()) {
        GbpController::getInstance().setTodayCustomDate(ui->todayDateEdit->date());
    } else {
        GbpController::getInstance().setTodayCustomDate(QDate());
    }

    GbpController::getInstance().saveSettings();

    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("Options have been changed"));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("    Impact = %1").arg(impact));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("    ScenarioMaxYearsSpan = %1").arg(years));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("    ChartDarkMode = %1").arg(chartDarkMode));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("    CurveDarkModeColor = %1").arg(curveDarkModeColor.rgba()));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("    CurveDarkLightColor = %1").arg(curveLightModeColor.rgba()));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("    ExportTextAmountLocalized = %1").arg(ui->exportTextAmountLocalizedCheckBox->isChecked()));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("    PercentageMainChartScaling = %1").arg(newMainChartScaling));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("    ChartExportType = %1").arg(chartExportType));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("    ChartExportQuality = %1").arg(ui->qualitySpinBox->value()));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("    UseDefaultSystemFont = %1").arg(ui->systemFontRadioButton->isChecked()));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("    CustomApplicationFont = %1").arg(newCustomFontString));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("    UseSystemDateForToday = %1").arg(ui->todaySystemRadioButton->isChecked()));
    GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, QString("    CustomTodayDate = %1").arg(
        GbpController::getInstance().getTodayCustomDate().toString(Qt::DateFormat::ISODate)));

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


void OptionsDialog::setDarkModeColorInfo()
{
    QString COLOR_STYLE("QPushButton { background-color : %1; }");
    ui->curveDarkModeColorToolButton->setStyleSheet(COLOR_STYLE.arg(curveDarkModeColor.name()));

    QColor c = curveDarkModeColor.name(QColor::HexRgb);
    ui->curveDMcolorLabel->setText(buildColorDisplayName(c));
}


void OptionsDialog::setLightModeColorInfo()
{
    QString COLOR_STYLE("QPushButton { background-color : %1; }");
    ui->curveLightModeColorToolButton->setStyleSheet(COLOR_STYLE.arg(curveLightModeColor.name()));

    QColor c = curveLightModeColor.name(QColor::HexRgb);
    ui->curveLMcolorLabel->setText(buildColorDisplayName(c));
}


QString OptionsDialog::fontLabel(const QFont font) const
{
    QFont::Style style = font.style();
    QFont::Weight w = font.weight();

    // the info we report depends on the style and weight
    QString customFontString;
    if ( (style != QFont::Style::StyleNormal) && (w != QFont::Weight::Normal) ){
        customFontString = QString("%1 %2 %3 %4").arg(font.family()).arg(fontWeightToString(font)).arg(fontStyleToString(font)).arg(font.pointSize());
    } else if ( (style == QFont::Style::StyleNormal) && (w != QFont::Weight::Normal) ) {
        customFontString = QString("%1 %2 %3").arg(font.family()).arg(fontWeightToString(font)).arg(font.pointSize());
    } else if ( (style != QFont::Style::StyleNormal) && (w == QFont::Weight::Normal) ) {
        customFontString = QString("%1 %2 %3").arg(font.family()).arg(fontStyleToString(font)).arg(font.pointSize());
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


QString OptionsDialog::buildColorDisplayName(QColor color)
{
    QString s = QString(tr("R:%1 G:%2 B:%3")).arg(color.red()).arg(color.green()).arg(color.blue());
    bool found;
    QString smartName = Util::getColorSmartName(color,found);
    if(found){
        s = s.append(QString(" (%1)").arg(smartName));
    }
    return s;
}


void OptionsDialog::on_curveDarkModeColorToolButton_clicked()
{
    QColorDialog::ColorDialogOptions opt = QColorDialog::DontUseNativeDialog;
    QColor color = QColorDialog::getColor(curveDarkModeColor, this, "Color Chooser",opt);
    if (color.isValid()==false) {
        return;
    } else {
        curveDarkModeColor = color;
        setDarkModeColorInfo();
    }
}


void OptionsDialog::on_curveLightModeColorToolButton_clicked()
{
    QColorDialog::ColorDialogOptions opt = QColorDialog::DontUseNativeDialog;
    QColor color = QColorDialog::getColor(curveLightModeColor,this, "Color Chooser", opt);
    if (color.isValid()==false) {
        return;
    } else {
        curveLightModeColor = color;
        setLightModeColorInfo();
    }
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
            // GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "Passing custom font to FontDialog : " + cFont.toString());
            // f = QFontDialog::getFont(&ok, cFont);

            // FontDialog seems to have a bug some tim ewith the font passed...
            f = QFontDialog::getFont(&ok);
        } else {
            GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Warning, "Cannot create custom font : " +newCustomFontString);
            f = QFontDialog::getFont(&ok );
        }
    } else {
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "No custom font available to pass to FontDialog");
        f = QFontDialog::getFont(&ok );
    }

    if (ok){
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Info, "Font returned by FontDialog : " + f.toString());
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

