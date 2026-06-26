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

#ifndef EDITPERIODICDIALOG_H
#define EDITPERIODICDIALOG_H

#include <QDialog>
#include <QLocale>
#include <QSet>
#include <QUuid>
#include "periodiccsd.h"
#include "currencyhelper.h"
#include "editvariablegrowthdialog.h"
#include "plaintexteditiondialog.h"
#include "csdbreakdowndialog.h"
#include "tags.h"
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
    void signalEditPeriodicCsdResult(bool isIncome, QSharedPointer<PeriodicCsd> pCsd);
    void signalEditPeriodicCsdCompleted();
    // Edit variable growth : Prepare dialog before edition
    void signalEditVariableGrowthPrepareContent(Growth growthIn);
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
     * @brief From client of EditPeriodicDialog : Prepare for creating a new Periodic Csd or
     * editing an existing one, before showing the Dialog.
     * @param isNewCsd True if we are creating a new Csd, false if we are editing an existing one.
     * @param isIncome True if we are creating/editing an income Csd, false if it an expense Csd.
     * @param psCsd QSharedPointer to an existing Csd (if editing), to null QSharedPointer
     * if creating.
     * @param newCurrInfo Currency info of the scenario.
     * @param inflation Scenario's inflation.
     * @param theMaxDateFeGeneration Max Fe generation date as defined in the scenario.
     * @param associatedTagIds For an exiting Csd, the set of tag IDs to which it is associated.
     * @param availTags Set of all the tags defined in the scenario.
     */
    void slotPrepareContent(bool isNewCsd, bool isIncome, QWeakPointer<PeriodicCsd> pCsd,
        const CurrencyInfo &newCurrInfo, const Growth &inflation, QDate theMaxDateFeGeneration,
        const QSet<QUuid> &associatedTagIds, const Tags &availTags);

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

    // Csd Breakdown child Dialog : receive completion notification
    void slotCsdBreakdownCompleted();


private slots:
    void on_applyPushButton_clicked();
    void on_closePushButton_clicked();
    void on_EditPeriodicDialog_rejected();
    void on_decorationColorPushButton_clicked();
    void on_decorationColorCheckBox_clicked();
    void on_visualizeOccurrencesPushButton_clicked();
    void on_toCustomRadioButton_toggled(bool checked);
    void on_toScenarioRadioButton_toggled(bool checked);
    void on_growthComboBox_currentIndexChanged(int index);
    void on_growthTypePushButton_clicked();
    void on_editDescriptionPushButton_clicked();
    void on_fromDateEdit_userDateChanged(const QDate &date);
    void on_toDateEdit_userDateChanged(const QDate &date);
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
    QSet<QUuid> tagIdSet;   // list of Tag ids this CSD is associated to
    Tags availableTags;     // set of ALL scenario tags available

    // children dialogs
    EditVariableGrowthDialog* editVariableGrowthDlg;
    PlainTextEditionDialog* editDescriptionDialog;
    VisualizeOccurrencesDialog* visualizeoccurrencesDialog;
    CsdBreakdownDialog* csdBreakdownDialog;

    /**
     * @struct BuildFromFormDataResult
     * @brief Contains the resulting PeriodicCsd built from the data in the
     * form and result of the build operation.
     */
    struct BuildFromFormDataResult{
        Util::ResultOfOperation result;
        QSharedPointer<PeriodicCsd> sharedPtrCsd;
        BuildFromFormDataResult();
        void init();
    };

    // To represent choices for Growth Combobox
    enum GrowthType {NONE=0, SCENARIO=1, CUSTOM_CONSTANT=2, CUSTOM_VARIABLE=3};

    // methods
    void prepareDataToCreateANewStreamDef(bool slotPrepare);
    void updateAuxCustomGrowthWidgetAccessibility();

    /**
     * @brief Use the data in the form to build a new Csd.
     * @param result The result of the operation.
     */
    void buildPeriodicCsdFromFormData(BuildFromFormDataResult &result);

    void updatePeriodCombobox(PeriodicCsd::PeriodType type);
    void setVisibilityComponentsGrowthType(GrowthType type);
    void updateGrowthTypeCombobox(GrowthType type);
    GrowthType getGrowthTypeSelected();
    void setDecorationColorInfo();
    QString convertTagIDSetToString();
    void updateTagListTextBox();

    /**
     * @brief Make the warning sign visible if Start date > scenario's End date
     */
    void setVisibilityStartDateWarningSign();

    /**
     * @brief Generate the financial events for that Csd with the current parameters
     * of the form. Return the no of saturations that occurred.
     * @param scenarioInflation Inflation for the scenario.
     * @param weakCsdPtr A QWeakPointer<Csd> reference to the Csd.
     * @param saturationCount Saturation count that occurred during the generation.
     * @param minMax Min/Max of the value generated.
     * @return The generated Fe Stream, wrapped with a QSharedPointer.
     */
    QSharedPointer<FeStream> generateFinancialEvents(const Growth &scenarioInflation,
        QWeakPointer<Csd> weakCsdPtr, uint& saturationCount, FeMinMaxInfo& minMax);

};

#endif // EDITPERIODICDIALOG_H
