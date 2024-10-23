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

#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QToolButton>

namespace Ui {
class OptionsDialog;
}

class OptionsDialog : public QDialog
{
    Q_OBJECT



public:
    // impact of options changes
    struct OptionsChangesImpact{
        // impact on the existing list of FE (which will impact the charts of course)
        quint8 data;
        // effet on charts scaling (OPTIONS_IMPACT_CHART_SCALING)
        quint8 chart_scaling;
        // effect on enabling StreamDef name colorization (OPTIONS_IMPACT_DECORATION_COLOR)
        quint8 decorationColorStreamDef;
        // effect on zooming with mouse wheel
        quint8 mouseWheelZoom;
        // effect on charts' theme (OPTIONS_IMPACT_CHARTS_THEME)
        quint8 charts_theme;
    };
    enum OPTIONS_IMPACT_DATA {DATA_UNCHANGED=0, DATA_RECALCULATE=1};
    enum OPTIONS_IMPACT_CHART_SCALING {CHART_SCALING_NONE=0, CHART_SCALING_RESCALE=1};
    enum OPTIONS_IMPACT_DECORATION_COLOR {DECO_NONE=0, DECO_REFRESH=1 };
    enum OPTIONS_IMPACT_CHARTS_THEME {CHARTS_THEME_NONE=0, CHARTS_THEME_REFRESH=1 };
    enum OPTIONS_IMPACT_WHEEL_ZOOM {WHEEL_ZOOM_NONE=0, WHEEL_ZOOM_REFRESH=1 };

    explicit OptionsDialog(QWidget *parent = nullptr);
    ~OptionsDialog();

public slots:
    // From client of optionsDialog : prepare to display content
    void slotPrepareContent();

signals:
    // For client of OptionsDialog : sending result and edition completion notification
    void signalOptionsResult(OptionsChangesImpact impact);
    void signalOptionsCompleted();

private slots:
    void on_applyPushButton_clicked();
    void on_cancelPushButton_clicked();
    void on_OptionsDialog_rejected();
    void on_setCustomFontPushButton_clicked();
    void on_systemFontRadioButton_toggled(bool checked);
    void on_todaySystemRadioButton_toggled(bool checked);
    void on_usePresentValueCheckBox_toggled(bool checked);
    void on_darkModeCurveColorPushButton_clicked();
    void on_darkModePointColorPushButton_clicked();
    void on_darkModeSelectedPointColorPushButton_clicked();
    void on_lightModeCurveColorPushButton_clicked();
    void on_lightModePointColorPushButton_clicked();
    void on_lightModeSelectedPointColorPushButton_clicked();

private:
    Ui::OptionsDialog *ui;

    enum ColorItem {CI_CURVE=1, CI_POINT=2, CI_SELECTED_POINT=3};
    enum ColorTheme {CT_DARK_MODE=1, CT_LIGHT_MODE=2};

    // variables
    QColor darkModeCurveColor;
    QColor darkModePointColor;
    QColor darkModeSelectedPointColor;
    QColor lightModeCurveColor;
    QColor lightModePointColor;
    QColor lightModeSelectedPointColor;
    QString newCustomFontString;

    // methods
    void setColorInfo(ColorTheme theme, ColorItem item);
    QString fontLabel(const QFont font) const;
    QString fontStyleToString(const QFont font) const;
    QString fontWeightToString(const QFont font) const;
    void setCustomFontlabel(QString fontLabel);
};

#endif // OPTIONSDIALOG_H
