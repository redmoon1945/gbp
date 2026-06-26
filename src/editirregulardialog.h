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

#ifndef EDITIRREGULARDIALOG_H
#define EDITIRREGULARDIALOG_H

#include <QDialog>
#include <QUuid>
#include <QLocale>
#include "csdbreakdowndialog.h"
#include "irregularcsd.h"
#include "currencyhelper.h"
#include "plaintexteditiondialog.h"
#include "editirregularmodel.h"
#include "editirregularelementdialog.h"
#include "loadirregulartextfiledialog.h"
#include "tags.h"
#include "visualizeoccurrencesdialog.h"


QT_BEGIN_NAMESPACE
namespace Ui {class EditIrregularDialog;}
QT_END_NAMESPACE


class EditIrregularDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditIrregularDialog(QLocale aLocale,  QWidget *parent = nullptr);
    ~EditIrregularDialog();

signals:
    // For client of EditIrregularDialog : send result and edition completion notification
    void signalEditIrregularCsdResult(bool isIncome, QSharedPointer<IrregularCsd> irCsd);
    void signalEditIrregularCsdCompleted();
    // For irregular element edition : Prepare dialog before edition
    void signalEditElementPrepareContent(bool isIncome, bool newEditMode, CurrencyInfo cInfo,
        QList<QDate> newExistingDates, QDate aDate, double amount, QString notes);
    // For irregular import : Prepare dialog before edition
    void signalImportPrepareContent(CurrencyInfo cInfo);
    // edition of description : prepare Dialog before edition
    void signalPlainTextDialogPrepareContent(QString title, QString content, bool readOnly);
    // show result : prepare Dialog before edition
    void signalShowResultPrepareContent(QString title, QString content, bool readOnly);
    // Visualize occurrences : prepare Dialog before edition
    void signalVisualizeOccurrencesPrepareContent(CurrencyInfo currInfo, QSharedPointer<FeStream>
        feStream, uint noOfSaturations, Growth scenarioInflation, FeMinMaxInfo minMax,
        QDate maxDateScenario);
    // Csd Breakdown : prepare Dialog before edition
    void signalCsdBreakdownPrepareContent(CurrencyInfo currInfo,  QSharedPointer<FeStream> feStream,
        QDate maxDateScenario);

public slots:

    /**
     * @brief From client of EditPeriodicDialog : Prepare edition.
     * @param isNewCsd True if we are going to create a new Csd.
     * @param isIncome True if it is editing for an income, false otherwise.
     * @param psCsd Weak pointer to the edited Csd. Null if we are creating a new Csd.
     * @param newCurrInfo Currency info.
     * @param maxDateScenarioFeGeneration Max date beyond which no finanical event will be created.
     * @param associatedTagIds  ALl the Tags already associated to this Csd. Empty if we are
     * going to create a new Csd.
     * @param availTags All the Tags defined in the scenario.
     */
    void slotPrepareContent(bool isNewCsd, bool isIncome, QWeakPointer<IrregularCsd> psCsd,
        const CurrencyInfo &newCurrInfo, QDate maxDateScenarioFeGeneration,
        const QSet<QUuid> &associatedTagIds, const Tags &availTags);

    // PlainTextEdition child Dialog : receive result and edition completion notification
    void slotPlainTextEditionResult(QString result);
    void slotPlainTextEditionCompleted();

    /**
     * @brief For irregular element edition : this is the result from edition of an irregular
     * date/amount pair. This can be for an edition of existing pair or the definition
     * of a new pair.
     * @param isEdition True if this was for editing an existing date/amount pair, false otherwise.
     * @param oldDate The previous date.
     * @param newDate The new date (can be the same as oldDate).
     * @param editedAmount The new edited amount. Always >= 0 even for expense.
     * @param editedNotes The new edited notes.
     */
    void slotEditElementResult(bool isEdition, QDate oldDate, QDate newDate, double editedAmount,
        QString editedNotes);// Edit element result

    void slotEditElementCompleted();    // Edit Element process is completed

    // For irregular import : getting result and completion notification
    void slotImportResult(const QMap<QDate,IrregularCsd::AmountInfo> &amountSet);
    void slotImportCompleted();

    // Visualize occurrences child Dialog : receive completion notification
    void slotVisualizeOccurrencesCompleted();

    // Csd Breakdown child Dialog : receive completion notification
    void slotCsdBreakdownCompleted();


private slots:
    void on_loadPushButton_clicked();
    void on_cancelPushButton_clicked();
    void on_applyPushButton_clicked();
    void on_addPushButton_clicked();
    void on_editPushButton_clicked();
    void on_deletePushButton_clicked();
    void on_fullViewPushButton_clicked();
    void on_EditIrregularDialog_rejected();
    void on_itemsTableView_doubleClicked(const QModelIndex &index);
    void on_selectAllPushButton_clicked();
    void on_unselectAllPushButton_clicked();
    void on_decorationColorPushButton_clicked();
    void on_decorationColorCheckBox_clicked();
    void on_visualizeOccurrencesPushButton_clicked();
    void on_exportPushButton_clicked();
    void on_breakdownPushButton_clicked();

protected:
    /**
     * @brief Defers deselection of nameLineEdit until after the platform style has finished
     * processing focus-in events.
     * @details On many platforms (e.g. KDE Breeze), the style calls selectAll() inside
     * focusInEvent when a QLineEdit gains focus, which fires during the show sequence.
     * A direct deselect() in slotPrepareContent() is overridden by that call. Using
     * QTimer::singleShot(0) posts the deselect to the end of the event queue, after all
     * show-related events have completed.
     */
    void showEvent(QShowEvent* event) override;

private:

    // *** PRIVATE STRCUT AND ENUM ***

    /**
     * @struct BuildFromFormDataResult
     * @brief Contains the resulting IrregularCsd built from the data in the
     * form and result of the build operation.
     */
    struct BuildFromFormDataResult{
        Util::ResultOfOperation result;
        QSharedPointer<IrregularCsd> sharedPtrCsd;
        BuildFromFormDataResult();
        void init();
    };


    // *** PRIVATE VARIABLES ***

    Ui::EditIrregularDialog *ui;

    QLocale locale;
    CurrencyInfo currInfo;
    bool isIncome;
    bool editingExistingCsd;
    QUuid initialId;
    QColor decorationColor;
    QDate maxDateFeGeneration;  // max date for Fe generation, come from scenario
    QSet<QUuid> tagIdSet;   // list of Tag ids this CSD is associated to
    Tags availableTags;     // set of ALL scenario tags available

    // children dialogs
    PlainTextEditionDialog* editDescriptionDialog;
    EditIrregularElementDialog* eie;
    LoadIrregularTextFileDialog* importDlg;
    VisualizeOccurrencesDialog* visualizeOccurrencesDialog;
    CsdBreakdownDialog* csdBreakdownDialog;

    // table model
    EditIrregularModel* tableModel;


    // *** PRIVATE METHODS ***

    QList<int> getSelectedRows();
    void cleanUpForNewCsd();
    void setDecorationColorInfo();
    QString convertTagIDSetToString();
    void updateTagListTextBox();

    /**
     * @brief Use the data in the form to build a new Csd.
     * @param result The result of the operation.
     */
    void buildIrregularCsdFromFormData(BuildFromFormDataResult &result);

    /**
     * @brief Generate the financial events for that Csd with the current parameters
     * of the form. Return the no of saturations that occurred.
     * @param weakCsdPtr A QWeakPointer<Csd> reference to the Csd.
     * @param saturationCount Saturation count that occurred during the generation.
     * @param minMax Min/Max of the value generated.
     * @return The generated Fe Stream, wrapped by a QSharedPointer.
     */
    QSharedPointer<FeStream> generateFinancialEvents(QWeakPointer<Csd> weakCsdPtr,
        uint& saturationCount, FeMinMaxInfo& minMax);
};

#endif // EDITIRREGULARDIALOG_H
