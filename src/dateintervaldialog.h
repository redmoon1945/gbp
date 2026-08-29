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

#ifndef DATEINTERVALDIALOG_H
#define DATEINTERVALDIALOG_H

#include <QDialog>

namespace Ui {
class DateIntervalDialog;
}

class DateIntervalDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DateIntervalDialog(QWidget *parent = nullptr);
    ~DateIntervalDialog();

public slots:

    /**
     * @brief From client of DateIntervalDialog : to be called just before showing the Dialog.
     * @param from Minimum date allowed for both date edits.
     * @param to Maximum date allowed for both date edits.
     * @details "from"/"to" only bound the allowed [min,max] range for the "From"/"To" date
     * edits, they are not values to display. The previously selected values are preserved
     * across invocations of this Dialog, and only adjusted (clamped) if they no longer fit
     * within that range.
     */
    void slotPrepareContent(QDate from, QDate to);

signals:
    // For client of DateIntervalDialog : Send results of edition and notify of edition completion
    void signalDateIntervalResult(QDate from, QDate to);
    void signalDateIntervalCompleted();

private slots:
    void on_applyPushButton_clicked();
    void on_cancelPushButton_clicked();
    void on_DateIntervalDialog_rejected();
    void on_setTomorrowPushButton_clicked();
    void on_setEoyPushButton_clicked();

protected:
    void showEvent(QShowEvent* event) override;

private:
    Ui::DateIntervalDialog *ui;
};

#endif // DATEINTERVALDIALOG_H
