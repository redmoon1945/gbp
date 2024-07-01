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

#ifndef LOADIRREGULARTEXTFILEDIALOG_H
#define LOADIRREGULARTEXTFILEDIALOG_H

#include <QDialog>
#include <QDate>
#include <QLocale>
#include "irregularfestreamdef.h"
#include "currencyhelper.h"

namespace Ui {
class LoadIrregularTextFileDialog;
}

class LoadIrregularTextFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoadIrregularTextFileDialog(QLocale aLocale, QWidget *parent = nullptr);
    ~LoadIrregularTextFileDialog();

public slots:
    // From client of LoadIrregularTextFileDialog : Prepare Dialog before edition
    void slotPrepareContent(CurrencyInfo currencyInfo);

signals:
    // For client of LoadIrregularTextFileDialog : Send results of import
    void signalImportResult(QMap<QDate,IrregularFeStreamDef::AmountInfo> amountSet);
    void signalImportCompleted();

private slots:
    void on_LoadIrregularTextFileDialog_rejected();

    void on_cancelPushButton_clicked();

    void on_importPushButton_clicked();

    void on_browsePushButton_clicked();

private:
    Ui::LoadIrregularTextFileDialog *ui;

    // misc variables
    QLocale theLocale;
    CurrencyInfo currInfo;
};

#endif // LOADIRREGULARTEXTFILEDIALOG_H
