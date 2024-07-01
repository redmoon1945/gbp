/*
 *  Copyright (C) 2024 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
 *  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
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

#ifndef EDITVARIABLEGROWTHDIALOG_H
#define EDITVARIABLEGROWTHDIALOG_H
#include <QDialog>
#include <QSharedPointer>
#include "growth.h"
#include "editvariablegrowthmodel.h"
#include "editgrowthelementdialog.h"


QT_BEGIN_NAMESPACE
namespace Ui { class EditVariableGrowthDialog; }
QT_END_NAMESPACE


class EditVariableGrowthDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditVariableGrowthDialog(QString newGrowthName,QLocale locale,QWidget *parent = nullptr);
    ~EditVariableGrowthDialog();

signals:
    // For growth element edition : Prepare dialog before edition
    void signalEditElementPrepareContent(bool newEditMode, QList<QDate> newExistingYears, QDate aDate, double growthInPercentage);  // call this before show()

    // for client of EditVariableGrowthDialog : send result od edition and notify of edition completion
    void signalEditVariableGrowthResult(Growth growthOut); // result of the edition
    void signalEditVariableGrowthCompleted(); // edit process is completed


public slots:
    // from client of EditVariableGrowthDialog : Prepare Dialog before edition
    void slotPrepareContent(Growth newGrowth);

    // For growth element edition : getting result and completion notification
    void slotEditElementResult(bool isEdition, QDate oldDate, QDate newDate, double growthInPercentage);// Edit element result
    void slotEditElementCompleted();    // Edit Element process is completed

private slots:

    void on_applyPushButton_clicked();
    void on_addPushButton_clicked();
    void on_deletePushButton_clicked();
    void on_cancelPushButton_clicked();
    void on_editPushButton_clicked();
    void on_growthTableView_doubleClicked(const QModelIndex &index);
    void on_EditVariableGrowthDialog_rejected();
    void on_SelectAllPushButton_clicked();
    void on_unselectAllPushButton_clicked();

private:

    QLocale locale;

    // dialogs
    Ui::EditVariableGrowthDialog *ui;
    EditGrowthElementDialog* ege;

    // table model
    EditVariableGrowthModel* tableModel;

    // private methods
    QList<int> getSelectedRows();
};

#endif // EDITVARIABLEGROWTHDIALOG_H
