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

#ifndef EDITGROWTHELEMENTDIALOG_H
#define EDITGROWTHELEMENTDIALOG_H

#include <QDialog>
#include <QLocale>
#include <QDate>


QT_BEGIN_NAMESPACE
namespace Ui {class EditGrowthElementDialog;}
QT_END_NAMESPACE


class EditGrowthElementDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditGrowthElementDialog(QString newGrowthName,QLocale aLocale, QWidget *parent = nullptr);
    ~EditGrowthElementDialog();

public slots:
    // From client of EditGrowthElementDialog : Prepare Dialog before edition
    void slotPrepareContent(bool newEditMode, QList<QDate> newExistingDates, QDate currentDate, double growthInPercentage);

signals:
    // For client of EditGrowthElementDialog : Send results of edition and notify of edition completion
    void signalEditElementResult(bool isEdition, QDate oldDate, QDate newDate, double growthInPercentage);
    void signalEditElementCompleted();

private slots:
    void on_closePushButton_clicked();
    void on_applyPushButton_clicked();
    void on_EditGrowthElementDialog_rejected();
    void on_growthDoubleSpinBox_valueChanged(double arg1);

private:
    Ui::EditGrowthElementDialog *ui;
    bool editMode;  // edit an existing element (true) or add new element (false)
    QString growthName;
    QList<QDate> existingDates;
    QLocale locale;
    QDate latestOldDate;    // to remember the old date when, during an edit, the date is changed.

    void fillMonthComboBox();
};

#endif // EDITGROWTHELEMENTDIALOG_H
