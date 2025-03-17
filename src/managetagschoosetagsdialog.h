/*
 *  Copyright (C) 2024-2025 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
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

#ifndef MANAGETAGSCHOOSETAGSDIALOG_H
#define MANAGETAGSCHOOSETAGSDIALOG_H

#include <QDialog>
#include <qlistwidget.h>
#include "tags.h"

namespace Ui {
class ManageTagsChooseTagsDialog;
}

class ManageTagsChooseTagsDialog : public QDialog
{
    Q_OBJECT

public:

    explicit ManageTagsChooseTagsDialog(QWidget *parent = nullptr);
    ~ManageTagsChooseTagsDialog();

signals:
    // For client of ManageTagsChooseTagsDialog : send result and edition completion notification
    void signalChooseTagsResult(QSet<QUuid>);
    void signalChooseTagsCompleted();

public slots:
    // From client of ManageTagsChooseTagsDialog: Prepare edition. Call this before show()
    void slotPrepareContent(Tags tags, QSet<QUuid> tagIdSet);

private slots:
    void on_ManageTagsChooseTagsDialog_rejected();
    void on_cancelPushButton_clicked();
    void on_applyPushButton_clicked();
    void on_selectAllPushButton_clicked();
    void on_unselectAllPushButton_clicked();

private:
    Ui::ManageTagsChooseTagsDialog *ui;

    // for lists
    struct CustomItem{
        QUuid id;
        QString name;   // original Name used in local sorting
    };
    class CustomListItem : public QListWidgetItem {
    public:
        CustomListItem(const QString& text, CustomItem cItem) : QListWidgetItem(text) {
            this->setData(Qt::UserRole, QVariant::fromValue(cItem));
        }
        bool operator<(const QListWidgetItem& other) const {
            QVariant var = this->data(Qt::UserRole);
            QString theName = var.value<CustomItem>().name;
            QVariant varOther = other.data(Qt::UserRole);
            QString otherName = varOther.value<CustomItem>().name;
            if ( QString::localeAwareCompare(theName,otherName) < 0 ){
                return true;
            } else {
                return false;
            }
        }
    };

    // variables
    Tags tags;

    // Methods
    void updateList(QSet<QUuid> tagIdSet);
};

#endif // MANAGETAGSCHOOSETAGSDIALOG_H
