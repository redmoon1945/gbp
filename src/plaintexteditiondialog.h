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

#ifndef PLAINTEXTEDITIONDIALOG_H
#define PLAINTEXTEDITIONDIALOG_H

#include <QDialog>

namespace Ui {
class PlainTextEditionDialog;
}

class PlainTextEditionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PlainTextEditionDialog(QWidget *parent = nullptr);
    ~PlainTextEditionDialog();

public slots:
    void slotPrepareContent(QString title, QString content, bool isItReadOnly);

signals:
    // For client of EditScenarioDialog : sending result and edition completion notification
    void signalPlainTextEditionResult(QString result);
    void signalPlainTextEditionCompleted();

private slots:

    void on_applyChangesPushButton_clicked();

    void on_cancelPushButton_clicked();

    void on_PlainTextEditionDialog_rejected();

private:
    Ui::PlainTextEditionDialog *ui;
    bool readOnly;
};

#endif // PLAINTEXTEDITIONDIALOG_H
