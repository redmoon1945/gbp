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

#ifndef EDITPERIODICDIALOG_H
#define EDITPERIODICDIALOG_H

#include <QDialog>
#include <QLocale>
#include "periodicfestreamdef.h"
#include "currencyhelper.h"
#include "editvariablegrowthdialog.h"
#include "plaintexteditiondialog.h"
#include "visualizeoccurrencesdialog.h"


QT_BEGIN_NAMESPACE
namespace Ui { class EditPeriodicDialog; }
QT_END_NAMESPACE



class EditPeriodicDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditPeriodicDialog(QLocale aLocale, QWidget *parent = nullptr);
    ~EditPeriodicDialog();

signals:
    // For client of EditPeriodicDialog : send result and edition completion notification
    void signalEditPeriodicStreamDefResult(bool isIncome, PeriodicFeStreamDef psStreamDef); // result of the edition
    void signalEditPeriodicStreamDefCompleted();
    // Edit variable growth : Prepare dialog before edition
    void signalEditVariableGrowthPrepareContent(Growth growthIn);
    // edition of description : prepare Dialog before edition
    void signalPlainTextDialogPrepareContent(QString title, QString content, bool readOnly);
    // show result : prepare Dialog before edition
    void signalShowResultPrepareContent(QString title, QString content, bool readOnly);
    // Visualize occurrences : prepare Dialog before edition
    void signalVisualizeOccurrencesPrepareContent(CurrencyInfo currInfo, Growth adjustedInflation, QDate maxDateScenarioFeGeneration, FeStreamDef *streamDef);

public slots:
    // From client of EditPeriodicDialog : Prepare edition
    void slotPrepareContent(bool isNewStreamDef, bool isIncome, PeriodicFeStreamDef psStreamDef, CurrencyInfo newCurrInfo, Growth inflation, QDate theMaxDateFeGeneration);  // call this before show()
    // Edit variable growth child Dialog : receive result and edition completion notification
    void slotEditVariableGrowthResult(Growth growthOut);
    void slotEditVariableGrowthCompleted();
    // PlainTextEdition child Dialog : receive result and edition completion notification
    void slotPlainTextEditionResult(QString result);
    void slotPlainTextEditionCompleted();
    // Show Result child Dialog : receive result and edition completion notification
    void slotShowResultResult(QString result);
    void slotShowResultCompleted();
    // Visualize occurrences child Dialog : receive completion notification
    void slotVisualizeOccurrencesCompleted();

private slots:
    void on_applyPushButton_clicked();
    void on_closePushButton_clicked();
    void on_EditPeriodicDialog_rejected();
    void on_growthVariablePushButton_clicked();
    void on_noGrowthRadioButton_clicked();
    void on_inflationRadioButton_clicked();
    void on_growthConstantRadioButton_clicked();
    void on_growthVariableRadioButton_clicked();
    void on_pushButton_clicked();
    void on_decorationColorPushButton_clicked();
    void on_decorationColorCheckBox_clicked();
    void on_visualizeOccurrencesPushButton_clicked();
    void on_toCustomRadioButton_toggled(bool checked);
    void on_toScenarioRadioButton_toggled(bool checked);

private:
    Ui::EditPeriodicDialog *ui;

    // variables
    QLocale locale;
    bool editingExistingStreamDef;
    CurrencyInfo currInfo;
    bool isIncome;
    Growth tempVariableGrowth;
    QUuid initialId;
    Growth scenarioInflation;
    QColor decorationColor;
    QDate maxDateFeGeneration;  // max date for Fe generation, come from scenario

    // children dialogs
    EditVariableGrowthDialog* editVariableGrowthDlg;
    PlainTextEditionDialog* editDescriptionDialog;
    VisualizeOccurrencesDialog* visualizeoccurrencesDialog;

    struct BuildFromFormDataResult{
        bool success;
        QString errorMessageUI;
        QString errorMessageLog;
        PeriodicFeStreamDef pStreamDef;
    };

    // methods
    void prepareDataToCreateANewStreamDef();
    void updateAuxCustomGrowthWidgetAccessibility();
    void buidlPeriodicFeStreamDefFromFormData(BuildFromFormDataResult &result);
    void updatePeriodCombobox(PeriodicFeStreamDef::PeriodType type);
    void setDecorationColorInfo();
};

#endif // EDITPERIODICDIALOG_H
