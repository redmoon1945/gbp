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

#ifndef EDITSCENARIODIALOG_H
#define EDITSCENARIODIALOG_H

#include <QDialog>
#include "plaintexteditiondialog.h"
#include "periodicfestreamdef.h"
#include "currencyhelper.h"
#include "editvariablegrowthdialog.h"
#include "editperiodicdialog.h"
#include "editirregulardialog.h"
#include "scenariocsdtablemodel.h"
#include "tagcsdrelationships.h"
#include "tags.h"
#include "managetagsdialog.h"
#include "setfiltertagsdialog.h"
#include "filtertags.h"


QT_BEGIN_NAMESPACE
namespace Ui { class EditScenarioDialog; }
QT_END_NAMESPACE


// Edit the content of an existing scenario
class EditScenarioDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditScenarioDialog(QLocale locale, QWidget *parent = nullptr);
    ~EditScenarioDialog();

    // methods
    void allowColoredCsdNames(bool value);

signals:
    // For client of EditScenarioDialog : sending result and edition completion notification
    void signalEditScenarioResult(bool regenerateData);
    void signalEditScenarioCompleted();

    // edition of description : prepare Dialog before edition
    void signalPlainTextDialogPrepareContent(QString title, QString content, bool readOnly);

    // edition of variable inflation : prepare Dialog before edition
    void signalEditVariableInflationPrepareContent(Growth growthIn);

    // edition of PeriodicFeStreamDef :  prepare Dialog before edition
    void signalEditPeriodicStreamDefPrepareContent(bool isNewStreamDef, bool isIncome,
        PeriodicFeStreamDef psStreamDef, CurrencyInfo currInfo, Growth scenarioInflation,
        QDate theMaxDateFeGeneration, QSet<QUuid> associatedTagIds, Tags availTags);

    // edition of IrregularFeStreamDef :  prepare Dialog before edition
    void signalEditIrregularStreamDefPrepareContent(bool isNewStreamDef, bool isIncome,
        IrregularFeStreamDef irStreamDef, CurrencyInfo currInfo, QDate theMaxDateFeGeneration,
        QSet<QUuid> associatedTagIds, Tags availTags);

    // Manage Tags : prepare Dialog before edition
    void signalManageTagsPrepareContent(Tags newTags, TagCsdRelationships newRelationships,
        QHash<QUuid, managetags::CsdItem> newFsdItems);

    // Set tags filters
    void signalSetFilterTagsPrepareContent(Tags tags, QSet<QUuid> preSelectedTags,
        FilterTags::Mode mode);

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

    // Manage Tags : result and completion
    void slotManageTagsResult(Tags newTags, TagCsdRelationships newRelationships);
    void slotManageTagsCompleted();

    // Edition of Filter Tags : result and completion
    void slotSetFilterTagsResult(FilterTags::Mode mode, QSet<QUuid> filterTagIdSet);
    void slotSetFilterTagsCompleted(bool canceled);

    // From client of EditScenarioDialog, to be called just before proceeding with
    // the scenario editing (show())
    void slotPrepareContent(QString countryCode, CurrencyInfo newCurrInfo);

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
    void on_setColorPushButton_clicked();
    void on_itemsTableView_doubleClicked(const QModelIndex &index);
    void on_maxDurationSpinBox_valueChanged(int arg1);
    void on_manageTagsPushButton_clicked();
    // filter change events
    void on_incomesRadioButton_toggled(bool checked);
    void on_filterPeriodicsCheckBox_checkStateChanged(const Qt::CheckState &arg1);
    void on_filterIrregularsCheckBox_checkStateChanged(const Qt::CheckState &arg1);
    void on_filterEnabledCheckBox_checkStateChanged(const Qt::CheckState &arg1);
    void on_filterDisabledCheckBox_checkStateChanged(const Qt::CheckState &arg1);
    void on_filterTagsCheckBox_checkStateChanged(const Qt::CheckState &arg1);
    void on_filterTagsPushButton_clicked();


private:
    Ui::EditScenarioDialog *ui;

    // used to display double amount with proper decimal and thousands separators
    QLocale displayLocale;

    // children dialogs
    PlainTextEditionDialog* editDescriptionDialog;
    EditVariableGrowthDialog* ecInflation;  // to edit variable inflation
    EditPeriodicDialog* psStreamDefDialog;  // to edit one particular Periodic Stream Def
    EditIrregularDialog* irStreamDefDialog; // to edit one particular irregular Stream Def
    ManageTagsDialog* manageTagsDlg;       // to edit scenario's tags and relationships
    SetFilterTagsDialog* setFilterTagsDlg; //to edit the set of tags used as filters for CSD display

    // --- Edited scenario elements that cannot be represented by UI elements ---

    QMap<QUuid,PeriodicFeStreamDef> incomesDefPeriodic;         // key is CSD ID
    QMap<QUuid,IrregularFeStreamDef> incomesDefIrregular;       // key is CSD ID
    QMap<QUuid,PeriodicFeStreamDef> expensesDefPeriodic;        // key is CSD ID
    QMap<QUuid,IrregularFeStreamDef> expensesDefIrregular;      // key is CSD ID
    Growth tempVariableInflation;           // to hold the inflation data for Variable type
    QString countryCode;
    CurrencyInfo currInfo;

    // Tags and relationships
    Tags tags;
    TagCsdRelationships tagCsdRelationships; // All tags links of this scenario

    // Set of Tag Id used for filtering, along with some related infos. This is not part of
    // a Scenario data.
    FilterTags filterTags;

    // -------------------------------------------------------------------------------

    // ListView model
    ScenarioCsdTableModel* itemTableModel;

    // misc methods
    QList<QUuid> getSelection();
    void setFilterString();
    Growth getInflationCurrentlyDefined();
    void selectRowsInTableView(QList<QUuid> idList);
    void updateNoItemsLabel();
    void updateNewButtonsText();
    bool checkAndAdjustFilterTags();
    void refreshCsdTableContent();
    QUuid duplicateCsd(QUuid id, bool &found);
    void removeCsds(QList<QUuid> toRemove);
    void enableDisableCsds(QList<QUuid> idList, bool enable);

    FeStreamDef::FeStreamType getCsdTypeFromId(QUuid id, bool &found);
    PeriodicFeStreamDef getPeriodicCsdFromId(QUuid id, bool &found);
    IrregularFeStreamDef getIrregularCsdFromId(QUuid id, bool &found);

};

#endif // EDITSCENARIODIALOG_H
