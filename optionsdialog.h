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
        quint32 chart;
        quint32 decorationColorStreamDef;
    };
    enum OPTIONS_IMPACT_CHART {CHART_NONE=0, CHART_FULL_RECALCULATION_REQUIRED=1, CHART_REPLOT=2, CHART_RESCALE_AND_REPLOT=3};
    enum OPTIONS_IMPACT_DECORATION_COLOR {DECO_NONE=0, DECO_REFRESH=1 };

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
    void on_curveDarkModeColorToolButton_clicked();
    void on_curveLightModeColorToolButton_clicked();
    void on_setCustomFontPushButton_clicked();
    void on_systemFontRadioButton_toggled(bool checked);
    void on_todaySystemRadioButton_toggled(bool checked);

private:
    Ui::OptionsDialog *ui;

    // variables
    QColor curveDarkModeColor;
    QColor curveLightModeColor;
    QString newCustomFontString;

    // methods
    void setDarkModeColorInfo();
    void setLightModeColorInfo();
    QString fontLabel(const QFont font) const;
    QString fontStyleToString(const QFont font) const;
    QString fontWeightToString(const QFont font) const;
    void setCustomFontlabel(QString fontLabel);
};

#endif // OPTIONSDIALOG_H
