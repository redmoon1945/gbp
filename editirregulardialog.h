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

#ifndef EDITIRREGULARDIALOG_H
#define EDITIRREGULARDIALOG_H

#include <QDialog>
#include <QLocale>
#include "irregularfestreamdef.h"
#include "currencyhelper.h"
#include "plaintexteditiondialog.h"
#include "editirregularmodel.h"
#include "editirregularelementdialog.h"
#include "loadirregulartextfiledialog.h"



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
    void signalEditIrregularStreamDefResult(bool isIncome, IrregularFeStreamDef irStreamDef); // result of the edition
    void signalEditIrregularStreamDefCompleted();

    // For irregular element edition : Prepare dialog before edition
    void signalEditElementPrepareContent(bool isIncome, bool newEditMode, CurrencyInfo cInfo, QList<QDate> newExistingDates, QDate aDate, double amount, QString notes);

    // For irregular import : Prepare dialog before edition
    void signalImportPrepareContent(CurrencyInfo cInfo);

    // edition of description : prepare Dialog before edition
    void signalPlainTextDialogPrepareContent(QString title, QString content, bool readOnly);

public slots:
    // From client of EditPeriodicDialog : Prepare edition
    void slotPrepareContent(bool isNewStreamDef, bool isIncome, IrregularFeStreamDef psStreamDef, CurrencyInfo newCurrInfo);  // call this before show()

    // PlainTextEdition child Dialog : receive result and edition completion notification
    void slotPlainTextEditionResult(QString result);
    void slotPlainTextEditionCompleted();

    // For irregular element edition : getting result and completion notification
    void slotEditElementResult(bool isEdition, QDate oldDate, QDate newDate, double editedAmount, QString editedNotes);// Edit element result
    void slotEditElementCompleted();    // Edit Element process is completed

    // For irregular import : getting result and completion notification
    void slotImportResult(QMap<QDate,IrregularFeStreamDef::AmountInfo> amountSet);
    void slotImportCompleted();


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

private:

    QLocale locale;
    CurrencyInfo currInfo;
    bool isIncome;
    bool editingExistingStreamDef;
    QUuid initialId;
    QColor decorationColor;

    // dialogs
    Ui::EditIrregularDialog *ui;
    PlainTextEditionDialog* editDescriptionDialog;
    EditIrregularElementDialog* eie;
    LoadIrregularTextFileDialog* importDlg;

    // table model
    EditIrregularModel* tableModel;

    // private methods
    QList<int> getSelectedRows();
    void cleanUpForNewStreamDef();
    void setDecorationColorInfo();

};

#endif // EDITIRREGULARDIALOG_H
