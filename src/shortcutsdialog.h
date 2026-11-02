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


#ifndef SHORTCUTSDIALOG_H
#define SHORTCUTSDIALOG_H

#include <QAction>
#include <QDialog>
#include <QList>
#include <QStringList>

namespace Ui {
class ShortcutsDialog;
}

class ShortcutsDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Dialog listing the keyboard and mouse shortcuts of the application.
     * @param menuActions Main window actions having a shortcut. Their key sequence and their
     * description are read from the action, so that the table cannot drift from the menus.
     */
    explicit ShortcutsDialog(const QList<QAction*>& menuActions, QWidget *parent = nullptr);
    ~ShortcutsDialog();

private slots:
    void on_exportHtmlPushButton_clicked();
    void on_closePushButton_clicked();

protected:
    void showEvent(QShowEvent* event) override;

private:
    Ui::ShortcutsDialog *ui;
    QList<QAction*> menuActions;
    int keysColumnWidth;
    int textColumnWidth;
    int sectionWidth;
    QString htmlRows;   // Rows of the table, in the form used for the HTML export

    void buildTable();
    void addSection(const QString& title);
    void addRow(const QStringList& keys, const QString& description);
    void adjustSizeToTable();
};

#endif // SHORTCUTSDIALOG_H
