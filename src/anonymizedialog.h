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

#ifndef ANONYMIZEDIALOG_H
#define ANONYMIZEDIALOG_H

#include <QDialog>

namespace Ui {
class AnonymizeDialog;
}

class AnonymizeDialog : public QDialog
{
    Q_OBJECT

public:

    struct AnonymizeOptions {
        bool anonymizeAmounts;
        int intensity; // 10–100, only relevant when anonymizeAmounts is true
    };

    explicit AnonymizeDialog(QWidget *parent = nullptr);
    ~AnonymizeDialog();

public slots:
    // From client of AnonymizeDialog : Prepare Dialog before edition
    void slotPrepareContent();

signals:
    // For client of AnonymizeDialog : Send results of edition and notify of edition completion
    void signalResult(AnonymizeDialog::AnonymizeOptions opts);
    void signalCompleted();

protected:
    /**
     * @brief Resizes the dialog height to fully display the info label, up to MAX_DIALOG_HEIGHT.
     * @details Calls QWidget::adjustSize() which triggers heightForWidth() on the QLabel
     * (possible because wordWrap is true). This is the Qt-idiomatic way to size a dialog
     * to its content and works correctly on all supported platforms. The width is preserved
     * explicitly before the call because adjustSize() may alter it.
     * @param event The show event passed by Qt.
     */
    void showEvent(QShowEvent *event) override;


private slots:
    void on_anonymizePushButton_clicked();
    void on_cancelPushButton_clicked();
    void on_anonymizeAmountsRadioButton_toggled(bool checked);
    void on_intensitySlider_valueChanged(int value);
    void on_AnonymizeDialog_rejected();


private:
    static constexpr int MAX_DIALOG_HEIGHT = 900;

    Ui::AnonymizeDialog *ui;
};

#endif // ANONYMIZEDIALOG_H
