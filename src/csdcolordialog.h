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


#ifndef CSDCOLORDIALOG_H
#define CSDCOLORDIALOG_H

#include <optional>
#include <QColor>
#include <QDialog>

namespace Ui {
class CsdColorDialog;
}

/**
 * @brief Ask what to do with the color of the Cash Stream Definitions selected in Edit Scenario.
 * @details A click on a button closes the dialog at once : there is no Apply button. exec()
 * returns one of the values of Action.
 */
class CsdColorDialog : public QDialog
{
    Q_OBJECT

public:
    /// NONE must be QDialog::Rejected (0) : Cancel, ESC or closing the window
    enum Action { NONE = 0, COPY, PASTE, EDIT, SYSTEM_DEFAULT };

    /**
     * @param colorToCopy Color of the only selected CSD, if any : "Copy color" is available and
     * shows it in a small square. Nothing if several CSD are selected, or if the CSD uses the
     * system default.
     * @param canSetSystemDefault "System default" is available : false when the only selected CSD
     * already uses the system default.
     * @param clipboardColor Color copied, if any : "Paste color" is available and shows it in a
     * small square.
     */
    explicit CsdColorDialog(const std::optional<QColor>& colorToCopy, bool canSetSystemDefault,
        const std::optional<QColor>& clipboardColor, QWidget *parent = nullptr);
    ~CsdColorDialog();

private slots:
    void on_copyPushButton_clicked();
    void on_pastePushButton_clicked();
    void on_editPushButton_clicked();
    void on_systemDefaultPushButton_clicked();
    void on_cancelPushButton_clicked();

protected:
    void showEvent(QShowEvent* event) override;

private:
    Ui::CsdColorDialog *ui;
};

#endif // CSDCOLORDIALOG_H
