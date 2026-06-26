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

#ifndef LOADVARIABLEGROWTHTEXTFILEDIALOG_H
#define LOADVARIABLEGROWTHTEXTFILEDIALOG_H

#include <QDate>
#include <QDialog>
#include <QLocale>
#include <QMap>

namespace Ui {
class LoadVariableGrowthTextFileDialog;
}

/**
 * @brief Dialog for importing a variable growth schedule from a text file.
 * @details Lets the user browse for a TAB-separated text file where each line holds an
 * ISO date and an annual growth rate (in percent). On success the parsed entries
 * are emitted via @c signalImportResult() as a map of transition dates to growth
 * values expressed in hundredths of a percent.
 * The growth name identifies the financial attribute being imported (e.g. "growth").
 * It is fixed at construction time, stored in lower-case form, and used in the
 * window title and log/error messages for context.
 */
class LoadVariableGrowthTextFileDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the dialog.
     * @param newGrowthName  Label of the growth attribute being imported; its first
     *                       character is lowercased for display purposes.
     * @param aLocale        Locale used for number formatting.
     * @param parent         Optional parent widget.
     */
    explicit LoadVariableGrowthTextFileDialog( const QString newGrowthName, QLocale aLocale,
        QWidget *parent = nullptr);

    /** @brief Destroys the dialog, releasing the UI. */
    ~LoadVariableGrowthTextFileDialog();

public slots:
    /** @brief Prepares the dialog for a new import session before it is shown. */
    void slotPrepareContent();

signals:
    /**
     * @brief Emitted when the file has been parsed successfully.
     * @param factors  Map of transition dates to growth values (hundredths of a percent).
     */
    void signalImportResult(QMap<QDate,qint64> factors);

    /** @brief Emitted after the dialog hides, signalling the import session is over. */
    void signalImportCompleted();

private slots:
    /** @brief Handles dialog rejection (window close button) by delegating to cancel. */
    void on_LoadVariableGrowthTextFileDialog_rejected();

    /** @brief Hides the dialog without importing. */
    void on_cancelPushButton_clicked();

    /** @brief Parses the selected file and emits @c signalImportResult() on success. */
    void on_importPushButton_clicked();

    /** @brief Opens a file-chooser dialog and populates the file-name field. */
    void on_browsePushButton_clicked();

protected:
    /** @brief Logs the initial dialog size after the first paint. */
    void showEvent(QShowEvent* event) override;

private:
    /// Qt Designer-generated UI.
    Ui::LoadVariableGrowthTextFileDialog *ui;

    /// Locale used for number formatting in error messages.
    QLocale theLocale;

    /// Lower-cased growth name used in the window title and log/error messages.
    QString growthName;
};

#endif // LOADVARIABLEGROWTHTEXTFILEDIALOG_H
