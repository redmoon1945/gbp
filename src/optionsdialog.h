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
        // effect on Y=0 line (visibility or color changed)
        quint8 yzeroLine;
        // effect on X axis Date Format
        quint8 xaxisDateFormat;
        // effect for some "incomes / expenses" colors throughtout the application
        quint8 incomeExpenseColor;
    };
    enum OPTIONS_IMPACT_DATA {DATA_UNCHANGED=0, DATA_RECALCULATE=1};
    enum OPTIONS_IMPACT_CHART_SCALING {CHART_SCALING_NONE=0, CHART_SCALING_RESCALE=1};
    enum OPTIONS_IMPACT_DECORATION_COLOR {DECO_NONE=0, DECO_REFRESH=1 };
    enum OPTIONS_IMPACT_CHARTS_THEME {CHARTS_THEME_NONE=0, CHARTS_THEME_REFRESH=1 };
    enum OPTIONS_IMPACT_WHEEL_ZOOM {WHEEL_ZOOM_NONE=0, WHEEL_ZOOM_REFRESH=1 };
    enum OPTIONS_IMPACT_Y_ZERO_LINE {Y_ZERO_LINE_NONE=0, Y_ZERO_LINE_REFRESH=1 };
    enum OPTIONS_IMPACT_XAXIS_DATE_FORMAT {XAXIS_DATE_FORMAT_NONE=0, XAXIS_DATE_FORMAT_REFRESH=1 };
    enum OPTIONS_IMPACT_IECOLOR{IECOLOR_UNCHANGED=0, IE_COLOR_REFRESH=1};

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
    void on_darkModeYzeroLineColorPushButton_clicked();
    void on_lightModeYzeroLineColorPushButton_clicked();
    void on_darkModeGridlinesColorPushButton_clicked();
    void on_lightModeGridlinesColorPushButton_clicked();
    void on_lightModeCurveColorResetPushButton_clicked();
    void on_lightModePointColorResetPushButton_clicked();
    void on_lightModeSelectedPointColorResetPushButton_clicked();
    void on_lightModeYzeroLineColorResetPushButton_clicked();
    void on_lightModeGridlinesColorResetPushButton_clicked();
    void on_darkModeCurveColorResetPushButton_clicked();
    void on_darkModePointColorResetPushButton_clicked();
    void on_darkModeSelectedPointColorResetPushButton_clicked();
    void on_darkModeYzeroLineColorResetPushButton_clicked();
    void on_darkModeGridlinesColorResetPushButton_clicked();
    void on_incomeColorPushButton_clicked();
    void on_expenseColorPushButton_clicked();
    void on_incomeColorResetPushButton_clicked();
    void on_expenseColorResetPushButton_clicked();
    void on_resetPushButton_clicked();

protected:
    void showEvent(QShowEvent* event) override;

private:
    Ui::OptionsDialog *ui;

    /**
     * @brief Color indicator (identify a push button and an associated label)
     */
    enum ColorItem {CI_CURVE_DT, CI_CURVE_LT, CI_POINT_DT, CI_POINT_LT, CI_SELECTED_POINT_DT,
        CI_SELECTED_POINT_LT, CI_YZERO_LINE_DT, CI_YZERO_LINE_LT,
        CI_GRIDLINES_DT, CI_GRIDLINES_LT,
        CI_INCOME_COLOR, CI_EXPENSE_COLOR};

    // *** VARIABLES ***

    // Custom Colors set by the user in the form. Their values are kept in explicit variables
    // and not in widgets. It is simpler than extracting the color from the color pushbutton.
    QColor darkModeCurveColor;
    QColor darkModePointColor;
    QColor darkModeSelectedPointColor;
    QColor lightModeCurveColor;
    QColor lightModePointColor;
    QColor lightModeSelectedPointColor;
    QColor yZeroLineDarkModeColor;
    QColor yZeroLineLightModeColor;
    QColor gridlinesDarkModeColor;
    QColor gridlinesLightModeColor;
    QColor incomeColor;
    QColor expenseColor;

    // Custom font selected by the user. Its value is kept in an explicit variable
    // and not in widgets. It is simpler than extracting the string from the widget.
    // If empty, it means it has not been set yet (unitialized).
    QString newCustomFontString;


    // *** METHODS ***

    /**
     * @brief Set the color of a particular "color indicator", that is a push button and a label.
     * @param item The color indicator identifier.
     * @param color The color to set.
     */
    void setColorInfo(ColorItem item, QColor theColor);

    QString fontLabel(const QFont font) const;
    QString fontStyleToString(const QFont font) const;
    QString fontWeightToString(const QFont font) const;
    void setCustomFontlabel(QString fontLabel);
};

#endif // OPTIONSDIALOG_H
