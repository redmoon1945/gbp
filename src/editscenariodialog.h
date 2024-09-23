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

#ifndef EDITSCENARIODIALOG_H
#define EDITSCENARIODIALOG_H

#include <QDialog>
#include "plaintexteditiondialog.h"
#include "periodicfestreamdef.h"
#include "currencyhelper.h"
#include "editvariablegrowthdialog.h"
#include "editperiodicdialog.h"
#include "editirregulardialog.h"
#include "scenariofetablemodel.h"


QT_BEGIN_NAMESPACE
namespace Ui { class EditScenarioDialog; }
QT_END_NAMESPACE


class EditScenarioDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditScenarioDialog(QLocale locale, QWidget *parent = nullptr);
    ~EditScenarioDialog();

    // methods
    void allowDecorationColor(bool value);

signals:
    // For client of EditScenarioDialog : sending result and edition completion notification
    void signalEditScenarioResult(bool currentlyEditingNewScenario, bool feGenerationDurationHasChanged);
    void signalEditScenarioCompleted();

    // edition of description : prepare Dialog before edition
    void signalPlainTextDialogPrepareContent(QString title, QString content, bool readOnly);

    // edition of variable inflation : prepare Dialog before edition
    void signalEditVariableInflationPrepareContent(Growth growthIn);

    // edition of PeriodicFeStreamDef :  prepare Dialog before edition
    void signalEditPeriodicStreamDefPrepareContent(bool isNewStreamDef, bool isIncome, PeriodicFeStreamDef psStreamDef, CurrencyInfo currInfo, Growth scenarioInflation, QDate theMaxDateFeGeneration);

    // edition of IrregularFeStreamDef :  prepare Dialog before edition
    void signalEditIrregularStreamDefPrepareContent(bool isNewStreamDef, bool isIncome, IrregularFeStreamDef irStreamDef, CurrencyInfo currInfo, QDate theMaxDateFeGeneration);


public slots:
    // response from child PlainTextEdition Dialog
    void slotPlainTextEditionResult(QString result);
    void slotPlainTextEditionCompleted();

    // edition of variable inflation : result and completion
    void slotEditVariableInflationResult(Growth growthOut);
    void slotEditVariableInflationCompleted();

    // edition of PeriodicStreamDef : result and completion
    void slotEditPeriodicStreamDefResult(bool isIncome, PeriodicFeStreamDef psStreamDef);
    void slotEditPeriodicStreamDefCompleted();

    // edition of IrregularStreamDef : result and completion
    void slotEditIrregularStreamDefResult(bool isIncome, IrregularFeStreamDef irStreamDef);
    void slotEditIrregularStreamDefCompleted();

    // From client of EditScenarioDialog, to be called just before proceeding with the scenario editing (show())
    void slotPrepareContent(bool isNewScenario, QString countryCode, CurrencyInfo newCurrInfo);


private slots:

    void on_inflationVariableRadioButton_clicked();
    void on_inflationConstantRadioButton_clicked();
    void on_fullViewPushButton_clicked();
    void on_cancelPushButton_clicked();
    void on_editGrowthPushButton_clicked();
    void on_addPeriodicPushButton_clicked();
    void on_addIrregularPushButton_clicked();
    void on_applyPushButton_clicked();
    void on_EditScenarioDialog_rejected();
    void on_editPushButton_clicked();
    void on_duplicatePushButton_clicked();
    void on_deletePushButton_clicked();
    void on_selectAllPushButton_clicked();
    void on_unselectAllPushButton_clicked();
    void on_enablePushButton_clicked();
    void on_disablePushButton_clicked();
    void on_itemsTableView_doubleClicked(const QModelIndex &index);
    void on_incomesRadioButton_toggled(bool checked);
    void on_periodicFilterPushButton_toggled(bool checked);
    void on_irregularFilterPushButton_toggled(bool checked);
    void on_activeFilterPushButton_toggled(bool checked);
    void on_inactiveFilterPushButton_toggled(bool checked);

    void on_maxDurationSpinBox_valueChanged(int arg1);

private:
    Ui::EditScenarioDialog *ui;

    bool currentlyEditingExistingScenario;  // true if we are editing an existing scenario, false if it is a new scenario (not loaded from disk)
    QLocale displayLocale;  // used to display double

    // // children dialogs
    PlainTextEditionDialog* editDescriptionDialog;
    EditVariableGrowthDialog* ecInflation;           // to edit variable inflation
    EditPeriodicDialog* psStreamDefDialog;           // to edit one particular Periodic Stream Def
    EditIrregularDialog* irStreamDefDialog;          // to edit one particular irregular Stream Def

    // buffer variables for some edited scenario elements
    Growth tempVariableInflation;           // to hold the inflation data for Variable type
    QString countryCode;
    CurrencyInfo currInfo;

    // ListView model
    ScenarioFeTableModel* itemTableModel;

    // misc methods
    QList<QUuid> getSelection();
    void setFilterString();
    Growth getInflationCurrentlyDefined();
    void selectRowsInTableView(QList<QUuid> idList);
    void updateNoItemsLabel();

};

#endif // EDITSCENARIODIALOG_H
