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

#include "managetagschoosetagsdialog.h"
#include "ui_managetagschoosetagsdialog.h"


ManageTagsChooseTagsDialog::ManageTagsChooseTagsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ManageTagsChooseTagsDialog)
{
    ui->setupUi(this);
}


ManageTagsChooseTagsDialog::~ManageTagsChooseTagsDialog()
{
    delete ui;
}


// tags contains all the defined tags in the scenario. unlinkedTagIds contains the id of
// the tags that should be displayed.
void ManageTagsChooseTagsDialog::slotPrepareContent(Tags tags, QSet<QUuid> tagIdSet)
{
    this->tags = tags;
    updateList(tagIdSet);
}


void ManageTagsChooseTagsDialog::on_ManageTagsChooseTagsDialog_rejected()
{
    on_cancelPushButton_clicked();
}


void ManageTagsChooseTagsDialog::on_cancelPushButton_clicked()
{
    emit signalChooseTagsCompleted();
    hide();
}


void ManageTagsChooseTagsDialog::on_applyPushButton_clicked()
{
    // extract the QUuid of all the selected items
    QSet<QUuid> result;
    QList<QListWidgetItem *> selection = ui->listWidget->selectedItems();
    foreach (QListWidgetItem *item, selection) {
        CustomItem cItem = item->data(Qt::UserRole).value<CustomItem>();
        result.insert(cItem.id);
    }

    // notify the parent and quit
    emit signalChooseTagsResult(result);
    emit signalChooseTagsCompleted();
    hide();
}


// Update the content of the listbox
void ManageTagsChooseTagsDialog::updateList(QSet<QUuid> tagIdSet)
{
    ui->listWidget->clear();

    // fill the list
    CustomListItem *item;
    bool found;
    foreach (QUuid id, tagIdSet) {
        // get Tag
        if(tags.containsTagId(id)!=true){ // should always be true
            continue;
        }
        Tag tag = tags.getTag(id, found);   // will always be found

        // insert in the list to display
        QString displayText = QString("%1").arg(tag.getName());
        item = new CustomListItem(displayText,{.id=tag.getId(),.name=tag.getName()});
        ui->listWidget->addItem(item) ;  // list widget will take ownership of the item
    }
}


void ManageTagsChooseTagsDialog::on_selectAllPushButton_clicked()
{
    ui->listWidget->selectAll();
}


void ManageTagsChooseTagsDialog::on_unselectAllPushButton_clicked()
{
    ui->listWidget->clearSelection();
}

