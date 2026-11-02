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
    void on_closePushButton_clicked();
    void on_PlainTextEditionDialog_rejected();
    void on_MonoFontCheckBox_toggled(bool checked);

protected:
    void showEvent(QShowEvent* event) override;

private:
    Ui::PlainTextEditionDialog *ui;
    bool readOnly;

    // Mono font choice, shared by all instances for the app session (off at launch)
    static bool useMonoFont;

    void applyFont();

    /**
     * @brief Text as it was when the dialog was prepared, read back from the editor (so that
     * the editor's own normalization does not look like a change). Reference for detecting
     * changes not applied yet.
     */
    QString originalText;

    /**
     * @brief True while the editor content is set by the dialog itself : change detection is
     * suspended.
     */
    bool fillingForm = false;

    /**
     * @brief Record that the text has been modified (or not), and update some UI hints :
     * warning sign, "*" in the window title, bottom buttons.
     * @details The only function allowed to write ContentChange::changed - enforced by
     * ContentChange's friend declaration. See ContentChange below.
     * @param v New value of the flag.
     */
    void contentHasChanged(bool v);

    /**
     * @brief Whether the text in the editor differs from originalText.
     * @return True if there are changes not applied yet.
     */
    bool hasContentChanged() const;

    /**
     * @brief Detect if the text in the editor differs from originalText, and update the
     * "content has changed" flag and the bottom buttons accordingly.
     * @details Called on each textChanged of the editor. Does nothing while fillingForm is true.
     * In read-only mode, the text cannot change : only "Close" is shown.
     */
    void updateContentChanged();

    /**
     * @brief Guards the "content has changed" flag so that only contentHasChanged() and
     * hasContentChanged() can read or write it (same as in EditScenarioDialog).
     */
    class ContentChange
    {
        bool changed = false;
        bool get() const { return changed; }
        void set(bool v)  { changed = v; }
        friend void PlainTextEditionDialog::contentHasChanged(bool v);
        friend bool PlainTextEditionDialog::hasContentChanged() const;
    };

    ContentChange contentChanged;
};

#endif // PLAINTEXTEDITIONDIALOG_H
