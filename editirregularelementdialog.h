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

#ifndef EDITIRREGULARELEMENTDIALOG_H
#define EDITIRREGULARELEMENTDIALOG_H

#include <QDialog>
#include <QLocale>
#include "currencyhelper.h"
#include "qdatetime.h"

namespace Ui {
class EditIrregularElementDialog;
}

class EditIrregularElementDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditIrregularElementDialog(QLocale aLocale, QWidget *parent = nullptr);
    ~EditIrregularElementDialog();

public slots:
    // From client of EditIrregularElementDialog : Prepare Dialog before edition
    void slotPrepareContent(bool isIncome, bool newEditMode, CurrencyInfo cInfo, QList<QDate> newExistingDates, QDate currentDate, double amount, QString notes);

signals:
    // For client of EditIrregularElementDialog : Send results of edition and notify of edition completion
    void signalEditElementResult(bool isEdition, QDate oldDate, QDate newDate, double amount, QString notes);
    void signalEditElementCompleted();

private slots:
    void on_applyPushButton_clicked();
    void on_closePushButton_clicked();
    void on_EditIrregularElementDialog_rejected();


private:
    Ui::EditIrregularElementDialog *ui;

    bool editMode;  // edit an existing element (true) or add new element (false)
    QLocale locale;
    QDate latestOldDate;    // to remember the old date when, during an edit, the date is changed.
    QList<QDate> existingDates;
    CurrencyInfo currInfo;
};

#endif // EDITIRREGULARELEMENTDIALOG_H
