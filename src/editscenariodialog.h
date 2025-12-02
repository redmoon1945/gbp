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
#include "periodiccsd.h"
#include "currencyhelper.h"
#include "editvariablegrowthdialog.h"
#include "editperiodicdialog.h"
#include "editirregulardialog.h"
#include "scenariocsdtablemodel.h"
#include "tagcsdrelationships.h"
#include "tags.h"
#include "managetagsdialog.h"
#include "choosetagsdialog.h"
#include "filtertags.h"


QT_BEGIN_NAMESPACE
namespace Ui { class EditScenarioDialog; }
QT_END_NAMESPACE


/**
 * @brief Dialog to edit the content of an existing scenario
 */
class EditScenarioDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditScenarioDialog(QLocale locale);
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

    // edition of PeriodicCsd :  prepare Dialog before edition
    void signalEditPeriodicCsdPrepareContent(bool isNewCsd, bool isIncome,
        QWeakPointer<PeriodicCsd> psCsd, CurrencyInfo currInfo, Growth scenarioInflation,
        QDate theMaxDateFeGeneration, QSet<QUuid> associatedTagIds, Tags availTags);

    // edition of IrregularCsd :  prepare Dialog before edition
    void signalEditIrregularCsdPrepareContent(bool isNewCsd, bool isIncome,
        QWeakPointer<IrregularCsd> irCsd, CurrencyInfo currInfo, QDate theMaxDateFeGeneration,
        QSet<QUuid> associatedTagIds, Tags availTags);

    // Manage Tags : prepare Dialog before edition
    void signalManageTagsPrepareContent(Tags newTags, TagCsdRelationships newRelationships,
        QHash<QUuid, managetags::CsdItem> newFsdItems);

    // Set tags filters
    void signalSetFilterTagsPrepareContent(Tags tags, QSet<QUuid> preSelectedTags);


public slots:
    // response from child PlainTextEdition Dialog
    void slotPlainTextEditionResult(QString result);
    void slotPlainTextEditionCompleted();

    // edition of variable inflation : result and completion
    void slotEditVariableInflationResult(Growth growthOut);
    void slotEditVariableInflationCompleted();

    // edition of Periodic Csd : result and completion
    void slotEditPeriodicCsdResult(bool isIncome, QSharedPointer<PeriodicCsd> pCsd);
    void slotEditPeriodicCsdCompleted();

    // edition of Irregular Csd : result and completion
    void slotEditIrregularCsdResult(bool isIncome, QSharedPointer<IrregularCsd> irCsd);
    void slotEditIrregularCsdCompleted();

    // Manage Tags : result and completion
    void slotManageTagsResult(Tags newTags, TagCsdRelationships newRelationships);
    void slotManageTagsCompleted();

    // Edition of Filter Tags : result and completion
    void slotSetFilterTagsResult(QSet<QUuid> filterTagIdSet);
    void slotSetFilterTagsCompleted(bool canceled);

    /**
     * @brief From client of EditScenarioDialog, to be called just before proceeding with the
     * scenario editing (show()) The result will be a brand new scenario.
     * @details This is actually creatig a copy of the current scenario parameters.
     * There is always a current scenario when this is called, even in the case
     * of a new scenario.
     * @param countryCode ISO code of the country for the scenario to edit.
     * @param newCurrInfo Currency info of the country for the scenario to edit.
     */
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
    void on_filterTagsCombinationComboBox_currentIndexChanged(int index);

private:
    Ui::EditScenarioDialog *ui;

    bool started; ///< True if this object has completed constructor phase

    // used to display double amount with proper decimal and thousands separators
    QLocale displayLocale;

    // children dialogs
    PlainTextEditionDialog* editDescriptionDialog;
    EditVariableGrowthDialog* ecInflation;  // to edit variable inflation
    EditPeriodicDialog* psCsdDialog;  // to edit one particular Periodic Stream Def
    EditIrregularDialog* irCsdDialog; // to edit one particular irregular Stream Def
    ManageTagsDialog* manageTagsDlg;       // to edit scenario's tags and relationships
    ChooseTagsDialog* setFilterTagsDlg; //to edit the set of tags used as filters for CSD display

    // --- Edited scenario elements that cannot be represented by UI elements ---

    /**
     * @brief List of WORKING COPY of periodic income Csds. This is a DEEP copy, initially
     * set from the current scenario. Key is CSD ID.
     */
    QHash<QUuid,QSharedPointer<PeriodicCsd>> incomesPeriodicCsd;

    /**
     * @brief List of WORKING COPY of irregular income Csds. This is a DEEP copy, initially
     * set from the current scenario. Key is CSD ID.
     */
    QHash<QUuid,QSharedPointer<IrregularCsd>> incomesIrregularCsd;

    /**
     * @brief List of WORKING COPY of periodic expense Csds. This is a DEEP copy, initially
     * set from the current scenario. Key is CSD ID.
     */
    QHash<QUuid,QSharedPointer<PeriodicCsd>> expensesPeriodicCsd;

    /**
     * @brief List of WORKING COPY of expense irregular Csds. This is a DEEP copy, initially
     * set from the current scenario. Key is CSD ID.
     */
    QHash<QUuid,QSharedPointer<IrregularCsd>> expensesIrregularCsd;

    Growth tempVariableInflation;           // to hold the inflation data for Variable type
    QString countryCode;
    CurrencyInfo currInfo;

    // Tags and relationships
    Tags tags;
    TagCsdRelationships tagCsdRelationships; // All tags links of this scenario

    // Set of Tag Id used for CDS filtering. This is NOT part of a Scenario data.
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

    /**
     * @brief Refresh the whole Csd table fron the current content of the working Csd lists.
     */
    void refreshCsdTableContent();

    /**
     * @brief Duplicate the selected Csd, add it to the proper set and return its ID.
     * The duplicate has a new ID and new name.
     * @param id ID of the Csd to duplicate.
     * @param found Idicate if the Csd to duplicate exists.
     * @return
     */
    QUuid duplicateCsd(QUuid id, bool &found);

    void removeCsds(QList<QUuid> toRemove);

    /**
     * @brief Change the "enabled" status of a list of Csds.
     * @param idList The list of Csd IDs.
     * @param enable New state.
     */
    void enableDisableCsds(QList<QUuid> idList, bool enable);

    void setFilterTagsWidgetsVisibility(bool visible);

    Csd::CsdType getCsdTypeFromId(QUuid id, bool &found);

    /**
     * @brief Return a Periodic Csd (income or expense) having "id" as ID. Check value of "found"
     * upon return to be sure return value is meaningful.
     * @param id Id of the Csd to get.
     * @param found True if the Csd exists, false otherwise.
     * @return The Csd, wrapped in a QSharedPointer.
     */
    QSharedPointer<PeriodicCsd> getPeriodicCsdFromId(QUuid id, bool &found);

    /**
     * @brief Return an Irregualr Csd (income or expense) having "id" as ID. Check value of "found"
     * upon return to be sure return value is meaningful.
     * @param id Id of the Csd to get.
     * @param found True if the Csd exists, false otherwise.
     * @return The Csd, wrapped in a QSharedPointer.
     */
    QSharedPointer<IrregularCsd> getIrregularCsdFromId(QUuid id, bool &found);

};

#endif // EDITSCENARIODIALOG_H
