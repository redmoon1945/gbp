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


#include "managetagschoosecsddialog.h"
#include "ui_managetagschoosecsddialog.h"
#include "gbpcontroller.h"

ManageTagsChooseCsdDialog::ManageTagsChooseCsdDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ManageTagsChooseCsdDialog)
{
    ui->setupUi(this);
}


ManageTagsChooseCsdDialog::~ManageTagsChooseCsdDialog()
{
    delete ui;
}


void ManageTagsChooseCsdDialog::slotPrepareContent(QSet<managetags::CsdItem> newCsdSet)
{
    csdSet = newCsdSet;
    updateList();
}


void ManageTagsChooseCsdDialog::on_applyPushButton_clicked()
{
    // extract the QUuid of all the selected items
    QSet<QUuid> result;
    QList<QListWidgetItem *> selection = ui->csdListWidget->selectedItems();
    foreach (QListWidgetItem *item, selection) {
        CustomItem cItem = item->data(Qt::UserRole).value<CustomItem>();
        result.insert(cItem.id);
    }

    // notify the parent and quit
    emit signalChooseCsdsResult(result);
    emit signalChooseCsdsCompleted();
    hide();
}


void ManageTagsChooseCsdDialog::on_cancelPushButton_clicked()
{
    emit signalChooseCsdsCompleted();
    hide();
}


void ManageTagsChooseCsdDialog::on_ManageTagsChooseCsdDialog_rejected()
{
    on_cancelPushButton_clicked();
}


void ManageTagsChooseCsdDialog::on_selectAllPushButton_clicked()
{
    ui->csdListWidget->selectAll();
}


void ManageTagsChooseCsdDialog::on_unselectAllPushButton_clicked()
{
    ui->csdListWidget->clearSelection();
}


void ManageTagsChooseCsdDialog::updateList()
{
    ui->csdListWidget->clear();

    // fill the list
    CustomListItem *item;
    foreach (managetags::CsdItem fsdItem, csdSet) {
        QString displayText = QString("%1").arg(fsdItem.name);
        item = new CustomListItem(displayText,{.id=fsdItem.id,.name=displayText});
        if ( (GbpController::getInstance().getAllowDecorationColor()==true) &&
            (fsdItem.color.isValid()==true) ) {
            item->setForeground(fsdItem.color);
        }
        // take into account type of FSD requested
        if ( ui->allRadioButton->isChecked() == true ){
            ui->csdListWidget->addItem(item) ;  // list widget will take ownership of the item
        } else if ( (true == ui->incomesRadioButton->isChecked()) && (fsdItem.isIncome==true) ){
            ui->csdListWidget->addItem(item) ;  // list widget will take ownership of the item
        }else if( (true == ui->expensesRadioButton->isChecked()) && (fsdItem.isIncome==false) ){
            ui->csdListWidget->addItem(item) ;  // list widget will take ownership of the item
        }
    }
}


void ManageTagsChooseCsdDialog::on_allRadioButton_toggled(bool checked)
{
    updateList();
}


void ManageTagsChooseCsdDialog::on_incomesRadioButton_toggled(bool checked)
{
    updateList();
}


void ManageTagsChooseCsdDialog::on_expensesRadioButton_toggled(bool checked)
{
    updateList();
}

