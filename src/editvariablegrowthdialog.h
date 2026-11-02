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

#ifndef EDITVARIABLEGROWTHDIALOG_H
#define EDITVARIABLEGROWTHDIALOG_H
#include <QDialog>
#include <QSharedPointer>
#include "growth.h"
#include "editvariablegrowthmodel.h"
#include "editgrowthelementdialog.h"
#include "loadvariablegrowthtextfiledialog.h"


QT_BEGIN_NAMESPACE
namespace Ui { class EditVariableGrowthDialog; }
QT_END_NAMESPACE


/**
 * @brief Dialog for editing a variable growth schedule.
 *
 * Allows the user to view, add, edit, and delete dated growth rate entries that
 * make up a variable @c Growth object. Each entry associates a transition date
 * with an annual growth rate expressed as a percentage. The dialog also supports
 * importing entries from a text file via @c LoadVariableGrowthTextFileDialog.
 *
 * The growth name is a user-visible label identifying the financial attribute whose growth
 * rate is being edited (e.g. "Growth" for a periodic item). It is used in the window title,
 * the informational note, and is forwarded to child sub-dialogs for consistent labelling.
 * The name is fixed at construction time and cannot be changed afterwards.
 */
class EditVariableGrowthDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the dialog and its child sub-dialogs.
     * @param newGrowthName  Display name of the growth being edited.
     * @param locale         Locale used for number and date formatting.
     * @param parent         Optional parent widget.
     */
    explicit EditVariableGrowthDialog(QString newGrowthName, QLocale locale,
        QWidget *parent = nullptr);

    /** @brief Destroys the dialog, releasing the UI and table model. */
    ~EditVariableGrowthDialog();

signals:
    /**
     * @brief Requests the growth-element sub-dialog to prepare for add/edit.
     * @param newEditMode          @c true when editing an existing entry, @c false for add.
     * @param newExistingYears     Dates already present (used to prevent duplicates).
     * @param aDate                Date pre-filled in the sub-dialog.
     * @param growthInPercentage   Growth rate pre-filled in the sub-dialog (%).
     */
    void signalEditElementPrepareContent(bool newEditMode, QList<QDate> newExistingYears,
        QDate aDate, double growthInPercentage);

    /**
     * @brief Emitted when the user confirms the edit; carries the resulting growth.
     * @param growthOut  The updated @c Growth object.
     */
    void signalEditVariableGrowthResult(Growth growthOut);

    /** @brief Emitted after the dialog hides, signalling the editing session is over. */
    void signalEditVariableGrowthCompleted();

    /**
     * @brief Requests the import sub-dialog to prepare with the current growth name.
     * @param newGrowthName  Name forwarded to @c LoadVariableGrowthTextFileDialog.
     */
    void signalImportPrepareContent();


public slots:
    /**
     * @brief Initialises the dialog with an existing @c Growth before showing it.
     * @param newGrowth  Growth data to display and edit.
     */
    void slotPrepareContent(const Growth newGrowth);

    /**
     * @brief Receives the add/edit result from the growth-element sub-dialog.
     * @param isEdition          @c true when modifying an existing entry.
     * @param oldDate            Previous date (relevant only when @p isEdition is @c true).
     * @param newDate            New or confirmed date for the entry.
     * @param growthInPercentage New growth rate in percent.
     */
    void slotEditElementResult(bool isEdition, QDate oldDate, QDate newDate,
        double growthInPercentage);

    /** @brief Called when the growth-element sub-dialog has closed. */
    void slotEditElementCompleted();

    /**
     * @brief Receives imported growth factors from @c LoadVariableGrowthTextFileDialog.
     * @param factors  Map of transition dates to growth values (hundredths of a percent).
     */
    void slotImportResult(QMap<QDate,qint64> factors);

    /** @brief Called when the import sub-dialog has closed. */
    void slotImportCompleted();

private slots:

    /** @brief Emits the edited growth and hides the dialog. */
    void on_applyPushButton_clicked();

    /** @brief Opens the growth-element sub-dialog in add mode. */
    void on_addPushButton_clicked();

    /** @brief Deletes all currently selected growth entries. */
    void on_deletePushButton_clicked();

    /** @brief Hides the dialog (no change, or changes discarded). */
    void on_closePushButton_clicked();

    /** @brief Discards the changes not applied yet and hides the dialog. */
    void on_cancelPushButton_clicked();

    /** @brief Opens the growth-element sub-dialog in edit mode for the selected row. */
    void on_editPushButton_clicked();

    /** @brief Triggers editing on double-click, equivalent to the edit button. */
    void on_growthTableView_doubleClicked(const QModelIndex &index);

    /** @brief Handles dialog rejection (window close button) by delegating to close. */
    void on_EditVariableGrowthDialog_rejected();

    /** @brief Selects all rows in the growth table. */
    void on_SelectAllPushButton_clicked();

    /** @brief Clears the selection in the growth table. */
    void on_unselectAllPushButton_clicked();

    /** @brief Opens the import sub-dialog. */
    void on_importPushButton_clicked();

    /** @brief Exports the content of the growth table to a CSV file. */
    void on_exportPushButton_clicked();

protected:
    /** @brief Logs the initial dialog size after the first paint. */
    void showEvent(QShowEvent* event) override;

private:

    QLocale locale;

    /// Qt Designer-generated UI.
    Ui::EditVariableGrowthDialog *ui;

    /// Sub-dialog for adding or editing a single growth entry.
    EditGrowthElementDialog* ege;

    /// Sub-dialog for importing growth data from a text file.
    LoadVariableGrowthTextFileDialog* importDlg;

    /// Model backing the growth table view.
    EditVariableGrowthModel* tableModel;

    QString growthName;

    /**
     * @brief Returns the indices of all currently selected rows.
     * @return List of zero-based row indices.
     */
    QList<int> getSelectedRows();

    /**
     * @brief Record that the growth has been modified (or not), and update some UI hints :
     * warning sign, "*" in the window title, bottom buttons.
     * @details The only function allowed to write ContentChange::changed - enforced by
     * ContentChange's friend declaration. See ContentChange below.
     * @param v New value of the flag.
     */
    void contentHasChanged(bool v);

    /**
     * @brief Whether the growth in the table differs from originalGrowth.
     * @return True if there are changes not applied yet.
     */
    bool hasContentChanged() const;

    /**
     * @brief Detect if the growth in the table differs from originalGrowth, and update the
     * "content has changed" flag and the bottom buttons accordingly.
     * @details Compares the model's growth to originalGrowth (Growth::operator==). Called each
     * time the model is reset, which covers add, edit, delete and import. Does nothing while
     * slotPrepareContent() fills the table (fillingForm).
     */
    void updateContentChanged();

    /**
     * @brief Guards the "content has changed" flag so that only contentHasChanged() and
     * hasContentChanged() can read or write it (same as in EditScenarioDialog).
     */
    class ContentChange
    {
        bool changed = false;
        bool get() const { return changed; }
        void set(bool v)  { changed = v; }
        friend void EditVariableGrowthDialog::contentHasChanged(bool v);
        friend bool EditVariableGrowthDialog::hasContentChanged() const;
    };

    ContentChange contentChanged;

    /**
     * @brief Copy of the growth as it was when the dialog was prepared, kept as a reference for
     * detecting changes not applied yet.
     */
    Growth originalGrowth;

    /**
     * @brief True while slotPrepareContent() fills the table : change detection is suspended.
     */
    bool fillingForm = false;
};

#endif // EDITVARIABLEGROWTHDIALOG_H
