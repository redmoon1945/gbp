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

#include "managetagsdialog.h"
#include "gbpcontroller.h"
#include "ui_managetagsdialog.h"
#include <QMessageBox>
#include <QFileDialog>


//----- GENERAL ------------------------------------------------------------------------------------


ManageTagsDialog::ManageTagsDialog(QLocale aLocale,QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ManageTagsDialog), locale(aLocale)
{
    ui->setupUi(this);

    // *** TAB : Tags Definition ***

    // *** TAB : Tags Definition ***

    // *** TAB : Tags Definition ***

    // *** General ***

    // EditTag dialog
    editTagDlg = new EditTagDialog(aLocale, this);                // auto-destroyed by Qt because it is a child
    editTagDlg->setModal(true);

    // Add CSD dialog
    addCsdDlg = new ManageTagsChooseCsdDialog(this);
    addCsdDlg->setModal(true);

    // Add Tag Dialog
    addTagDlg = new ManageTagsChooseTagsDialog(this);
    addTagDlg->setModal(true);

    // Always Select the fist tab
    ui->tagsTabWidget->setCurrentIndex(0);

    // set up the list model for tags in the TagsDef table view
    QFont nameFont = ui->tagsDefTableView->font();
    QFont descFont = ui->tagsDefTableView->font();
    uint newDescFontSize = Util::changeFontSize(1, true, descFont.pointSize());
    descFont.setPointSize(newDescFontSize);
    descFont.setItalic(true);
    itemTableModel = new ManageTagsTagsDefModel(locale, nameFont, descFont);
    ui->tagsDefTableView->setModel(itemTableModel);

    // it appears this must be done AFTER setting the model (don't know why...)
    QFontMetrics fm2 = ui->tagsDefTableView->fontMetrics();
    ui->tagsDefTableView->setColumnWidth(0,fm2.averageCharWidth()*50);    // name

    // connect emitters & receivers for Dialogs : Tag Edition
    QObject::connect(this, &ManageTagsDialog::signalEditTagPrepareContent, editTagDlg,
        &EditTagDialog::slotPrepareContent);
    QObject::connect(editTagDlg, &EditTagDialog::signalEditTagResult, this,
        &ManageTagsDialog::slotEditTagResult);
    QObject::connect(editTagDlg, &EditTagDialog::signalEditTagCompleted, this,
        &ManageTagsDialog::slotEditTagCompleted);

    // connect emitters & receivers for Dialogs : Add Csd links
    QObject::connect(this, &ManageTagsDialog::signalAddCsdPrepareContent, addCsdDlg,
        &ManageTagsChooseCsdDialog::slotPrepareContent);
    QObject::connect(addCsdDlg, &ManageTagsChooseCsdDialog::signalChooseCsdsResult, this,
        &ManageTagsDialog::slotChooseCsdsResult);
    QObject::connect(addCsdDlg, &ManageTagsChooseCsdDialog::signalChooseCsdsCompleted, this,
        &ManageTagsDialog::slotChooseCsdsCompleted);

    // connect emitters & receivers for Dialogs : Add Tag links
    QObject::connect(this, &ManageTagsDialog::signalAddTagsPrepareContent, addTagDlg,
        &ManageTagsChooseTagsDialog::slotPrepareContent);
    QObject::connect(addTagDlg, &ManageTagsChooseTagsDialog::signalChooseTagsResult, this,
        &ManageTagsDialog::slotChooseTagsResult);
    QObject::connect(addTagDlg, &ManageTagsChooseTagsDialog::signalChooseTagsCompleted, this,
        &::ManageTagsDialog::slotChooseTagsCompleted);
}


ManageTagsDialog::~ManageTagsDialog()
{
    delete ui;
    delete itemTableModel;   // dont forget, because we have not set "parent" !

}


// Do not change the active tab
void ManageTagsDialog::slotPrepareContent(Tags newTags, TagCsdRelationships newRelationships,
    QHash<QUuid, managetags::CsdItem> newCsdItems)
{
    tags = newTags;
    relationships = newRelationships;
    allCsds = newCsdItems;

    // First, update atgs definitions
    tagsDef_UpdateTagList();

    // Csds could have changed since last time we enter this dialog : UI has to be updated.
    tagsView_Refresh();
    csdsView_Refresh();
}


void ManageTagsDialog::on_ManageTagsDialog_rejected()
{
    on_cancelPushButton_clicked();
}

void ManageTagsDialog::on_cancelPushButton_clicked()
{
    emit signalManageTagsCompleted();
    hide();
}


void ManageTagsDialog::on_applyPushButton_clicked()
{
    emit signalManageTagsResult(tags, relationships);
    emit signalManageTagsCompleted();
    hide();
}


// Entering a new tab
void ManageTagsDialog::on_tagsTabWidget_currentChanged(int index)
{
    if(index==1){
        tagsView_Refresh();
    } else if (index==2){
        csdsView_Refresh();
    }
}



//----- TAB : Tags Definitions ---------------------------------------------------------------------


// A new Tag has been created or an existing one has been updated.
void ManageTagsDialog::slotEditTagResult(Tag newtag)
{
    // this can be a new or existing tag. If we have reached the max no of tags,
    // do not insert if this is a new tag
    if (tags.size() >= Tags::MAX_NO_TAGS) {
        if (tags.containsTagId(newtag.getId())==false){
            return;
        }
    }

    // insert the tag. Existing edited one will just replace the old one.
    tags.insert(newtag);

    // update tag list
    tagsDef_UpdateTagList();

    // make visible the new or edited tag
    tagsDef_SelectTags({newtag.getId()});

    // make visible the new or edited tag
    makeVisibleTag(newtag.getId());
}


void ManageTagsDialog::slotEditTagCompleted()
{
}


// Tags Definition Tab : clear the tag list and rebuild from data.
// Remove any selection already present
void ManageTagsDialog::tagsDef_UpdateTagList()
{
    itemTableModel->setModelData(tags);

    //ui->tagsDefListWidget->clear();

    // fill tags list. Display name is name + description
    // CustomListItem *item;
    // bool found;
    // QSet<Tag> tagSet = tags.getTags();
    // foreach (Tag tag, tagSet) {
    //     QString displayName = tag.getName();
    //     if (tag.getDescription().length() !=0 ) {
    //         // limit description to 50 char
    //         displayName =  QString("%1  (%2)").arg(tag.getName())
    //             .arg(Util::elideText(tag.getDescription(),50,true));
    //     }
    //     item = new CustomListItem(displayName,{.id=tag.getId(),.name=tag.getName()});
    //     ui->tagsDefListWidget->addItem(item) ;  // list widget will take ownership of the item
    // }

    // write no of elements in the Tag Label
    tagsDef_UpdateTagListLabel();
}


void ManageTagsDialog::tagsDef_UpdateTagListLabel()
{
    QString s = tr("Tags defined");
    if (tags.size()!=0) {
        s = QString("%1 (%2)").arg(s).arg(tags.size());
    }
    ui->tagsDefListTitleLabel->setText(s);
}


// TAB : Tags Definition
// Get ids of selected tags
QList<QUuid> ManageTagsDialog::tagsDef_GetSelectedTagsIds(){

    QList<QUuid> result ={};

    // get the indexes of selected rows
    QItemSelectionModel* selectionModel = ui->tagsDefTableView->selectionModel();
    QModelIndexList selectedRows = selectionModel->selectedRows();

    // convert them all to UUID
    bool found;
    foreach (const QModelIndex &index, selectedRows) {
        QUuid id = itemTableModel->getId(index.row(),found);
        if(found==true){
            result.append(id);
        }
    }

    return result;
}


// This is not trivial : Qt is complicated for non adjencent multirow selection
void ManageTagsDialog::tagsDef_SelectTags(QSet<QUuid> tagIdList)
{
    // first clear current selection
    ui->tagsDefTableView->clearSelection();

    // then select one by one item that can be found
    bool found;
    QItemSelection selection;

    foreach(QUuid id, tagIdList){
        int row = itemTableModel->getRow(id, found);
        if (found==false){
            continue; // cannot select anything since not found in what is displayed
        }
        QModelIndex leftIndex  = ui->tagsDefTableView->model()->index(row, 0);
        QModelIndex rightIndex = ui->tagsDefTableView->model()->index(row, 1);

        QItemSelection rowSelection(leftIndex, rightIndex);
        selection.merge(rowSelection, QItemSelectionModel::Select);
    }
    ui->tagsDefTableView->selectionModel()->select(selection, QItemSelectionModel::Select);
}


void ManageTagsDialog::makeVisibleTag(QUuid tagId)
{
    bool found;
    int row = itemTableModel->getRow(tagId, found);
    if (found){
        QModelIndex index = itemTableModel->index(row,0);
        // does not always work...
        ui->tagsDefTableView->scrollTo(index,QAbstractItemView::PositionAtCenter);
    }
}


// Add a new tag in TAB : Tags Definition.
void ManageTagsDialog::on_tagsDefAddPushButton_clicked()
{
    // Check if max no of tags has been reached
    if (tags.size() >= Tags::MAX_NO_TAGS) {
        QString errorString = QString(tr("The maximum no of tags has been reached (%1).")
            .arg(Tags::MAX_NO_TAGS));
        QMessageBox::critical(nullptr,tr("Error"), errorString);
        return;
    }

    // send for display
    emit signalEditTagPrepareContent(tags, Tag(), true);
    editTagDlg->show();
}


void ManageTagsDialog::on_tagsDefDeletePushButton_clicked()
{
    // get selected tag ids if any
    QList<QUuid> selectedTagsIds = tagsDef_GetSelectedTagsIds();
    if (selectedTagsIds.count()==0) {
        // no tag selected
        QString errorString = QString(tr("At least 1 tag must be selected."));
        QMessageBox::critical(nullptr,tr("Error"), errorString);
        return;
    }

    // delete the tags and related relationships with Fsds
    foreach (QUuid tagId, selectedTagsIds) {
        tags.remove(tagId);
        relationships.deleteRelationshipsForTag(tagId);
    }
    // refresh the list
    tagsDef_UpdateTagList();
}


void ManageTagsDialog::on_tagsDefEditPushButton_clicked()
{
    // get selected tag id if any
    QList<QUuid> selection = tagsDef_GetSelectedTagsIds();
    if (selection.count()!=1) {
        // no tag selected
        QString errorString = QString(tr("Exactly 1 tag must be selected."));
        QMessageBox::critical(nullptr,tr("Error"), errorString);
        return;
    }
    // edit the tag
    bool found;
    Tag toBeEdited = tags.getTag(selection[0],found);
    if (found==false) {
        // should never happen
        QString errorString = QString(tr("Unknown tag id : %1.").arg(selection[0].toString()));
        QMessageBox::critical(nullptr,tr("Error"), errorString);
        return;
    }
    emit signalEditTagPrepareContent(tags, toBeEdited, false);
    editTagDlg->show();
}


// Duplicate all the selected tags, including all their relationships. Name of a duplicate
// is prefixed with "Copy of"
void ManageTagsDialog::on_tagsDefDuplicatePushButton_clicked()
{
    // get selected tag id if any
    bool found;
    QList<QUuid> selection = tagsDef_GetSelectedTagsIds();
    if (selection.count()==0) {
        // no tag selected
        QString errorString = QString(tr("At least 1 tag must be selected."));
        QMessageBox::critical(nullptr,tr("Error"), errorString);
        return;
    }

    // Check if max no of tags would be exceeded
    if ( (tags.size() + selection.count()) > Tags::MAX_NO_TAGS) {
        QString errorString = QString(tr("The maximum no of tags would be exceeded (%1).")
            .arg(Tags::MAX_NO_TAGS));
        QMessageBox::critical(nullptr,tr("Error"), errorString);
        return;
    }

    // Proceed to duplication. Remember the first created, because this will be the one
    // made visible in the list viewport.
    QSet<QUuid> newDuplicatedItems;
    bool firstDuplicateCreated = false;
    QUuid firstDuplicate;
    foreach (QUuid selectedTagId, selection) {
        // Get the corresponding Tag
        Tag selectedTag = tags.getTag(selectedTagId,found);
        if(found==false){
            return; // should never occur
        }

        // Create the new Tag, make sure to have a unique name, not too long
        QString newName = tr("Copy of ")+selectedTag.getName();
        newName = newName.left(Tag::MAX_NAME_LEN); // chop...
        if (true == tags.containsTagName(newName)) {
            // use a random name then...
            newName = QUuid::createUuid().toString(QUuid::WithoutBraces).left(Tag::MAX_NAME_LEN);
        }
        Tag newTag = Tag(newName);
        newTag.setDescription(selectedTag.getDescription());
        tags.insert(newTag);
        newDuplicatedItems.insert(newTag.getId());

        // remember first duplicate
        if(firstDuplicateCreated==false){
            firstDuplicateCreated = true;
            firstDuplicate  =newTag.getId();
        }

        // clone the CSD relationships (if any)
        relationships.cloneCsdRelationshipsForTag(selectedTagId, newTag.getId());
    }

    // Update the displayed tag list
    tagsDef_UpdateTagList();

    // select the duplicate tags
    tagsDef_SelectTags(newDuplicatedItems);

    // make visible the first one (does not mean the first sorted one...)
    makeVisibleTag(firstDuplicate);
}



void ManageTagsDialog::on_tagsDefSelectAllPushButton_clicked()
{
    ui->tagsDefTableView->selectAll();
}


void ManageTagsDialog::on_tagsDefUnselectAllPushButton_clicked()
{
    ui->tagsDefTableView->clearSelection();
}


// void ManageTagsDialog::on_tagsDefListWidget_itemDoubleClicked(QListWidgetItem *item)
// {
//     on_tagsDefEditPushButton_clicked();
// }



void ManageTagsDialog::on_tagsDefImportPushButton_clicked()
{
    // Choose the scenario and load it
    QSharedPointer<Scenario> scenario;
    QString dir = GbpController::getInstance().getLastDir();
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open an Existing Scenario"),dir);
    if (fileName != ""){
        // load the scenario file
        Scenario::FileResult fr ;
        fr = Scenario::loadFromFile(fileName);
        if (fr.code != Scenario::SUCCESS){
            QMessageBox::critical(nullptr,tr("Error"), fr.errorStringUI);
            return;
        }
        // extract tags
        Tags importedTags = fr.scenarioPtr->getTags();

        // Import in current tags. Tags with identical name are dropped. UUID are changed.
        foreach(Tag t, importedTags.getTags()){
            if (tags.containsTagName(t.getName()) == false) {
                Tag newTag(t.getName(),t.getDescription());
                tags.insert(newTag);
            }
        }

        // Refresh tags lsit
        tagsDef_UpdateTagList();

    } else {
        return; // user canceled
    }

    // check for max no of tags


}



//----- TAB : Tags View ----------------------------------------------------------------------------


// We are entering this tab. Tag set and/or Csd set may be different. Update the full Tab.
// Strategy is to refresh the tag list and then reselect the previous selected tag (if still
// existing) and display the relevant linked Csds
void ManageTagsDialog::tagsView_Refresh()
{
    // first, remember the tag selected (if any)
    QList<QUuid> selection = tagsView_GetSelectedTagsIds();

    // then clear everything and rebuild lists
    tagsView_UpdateTagList();

    // Finally try to reselect the same tag if it still exists
    if(selection.count()==1){
        if( tags.containsTagId(selection[0])==true ){
            tagsView_SelectTag(selection[0]); // Csds will be displayed
        }
    }
}


// Tags View : clear the tag list and rebuild from data.
// Remove any selection already present. Csd list is also cleared since tag selection is gone.
void ManageTagsDialog::tagsView_UpdateTagList()
{
    ui->tagsViewTagsListWidget->clear();

    // fill tags list. Display name is name
    CustomListItem *item;
    bool found;
    QSet<Tag> tagSet = tags.getTags();
    foreach (Tag tag, tagSet) {
        QString displayName = tag.getName();
        item = new CustomListItem(displayName,{.id=tag.getId(),.name=tag.getName()});
        ui->tagsViewTagsListWidget->addItem(item) ;  // list widget will take ownership of items
    }

    // Csd list must also be cleared.
    ui->tagsViewCsdListWidget->clear();
}


void ManageTagsDialog::tagsView_UpdateCsdList()
{
    ui->tagsViewCsdListWidget->clear();

    // Get the id of the selected tag
    QList<QUuid> tagIds = tagsView_GetSelectedTagsIds();

    // fill the list
    if (tagIds.count()!=0) {
        QSet<QUuid> csdSet = relationships.getRelationshipsForTag(tagIds[0]);
        CustomListItem *item;
        foreach (QUuid csdId, csdSet) {
            if(allCsds.contains(csdId)==true){ // should always be true in priniple
                managetags::CsdItem csdItem = allCsds.value(csdId);
                QString displayText = QString("%2").arg(csdItem.name);
                item = new CustomListItem(displayText,{.id=csdItem.id,.name=displayText});
                if ( (GbpController::getInstance().getAllowDecorationColor()==true) &&
                    (csdItem.color.isValid()==true)) {
                    item->setForeground(csdItem.color);
                }
                ui->tagsViewCsdListWidget->addItem(item) ;// list widget will take ownership of item
            }
        }
    }

    // update list title
    tagsView_UpdateCsdListLabel();
}


QList<QUuid> ManageTagsDialog::tagsView_GetSelectedTagsIds()
{
    QList<QUuid> result;

    QList<QListWidgetItem *> selection = ui->tagsViewTagsListWidget->selectedItems();
    for (int var = 0; var < selection.count(); ++var) {
        CustomItem selectedItem = selection[var]->data(Qt::UserRole).value<CustomItem>();
        result.append(selectedItem.id);
    }
    return result;
}


void ManageTagsDialog::on_tagsViewTagsListWidget_itemSelectionChanged()
{
    tagsView_UpdateCsdList();
}


void ManageTagsDialog::tagsView_UpdateCsdListLabel()
{
    QString s = tr("Linked cash stream definitions");
    if (ui->tagsViewCsdListWidget->count()!=0) {
        s = QString("%1 (%2)").arg(s).arg(ui->tagsViewCsdListWidget->count());
    }
    ui->tagsViewCsdListLabel->setText(s);
}


QList<QUuid> ManageTagsDialog::tagsView_GetSelectedCsdsIds()
{
    QList<QUuid> result;

    QList<QListWidgetItem *> selection = ui->tagsViewCsdListWidget->selectedItems();
    for (int var = 0; var < selection.count(); ++var) {
        CustomItem selectedItem = selection[var]->data(Qt::UserRole).value<CustomItem>();
        result.append(selectedItem.id);
    }
    return result;
}


// Select a single item in tagsView Tags list. if the list does not contain the tagId,
// no selection is left on the list
void ManageTagsDialog::tagsView_SelectTag(QUuid tagId)
{
    ui->tagsViewTagsListWidget->clearSelection();
    int noRows = ui->tagsViewTagsListWidget->count();
    for(int i=0;i<noRows;i++){
        QListWidgetItem *item = ui->tagsViewTagsListWidget->item(i);
        CustomItem cItem = item->data(Qt::UserRole).value<CustomItem>();
        if(tagId == cItem.id){
            item->setSelected(true);
            return;
        }
    }
}


void ManageTagsDialog::on_tagsViewSelectAllLinksPushButton_clicked()
{
    ui->tagsViewCsdListWidget->selectAll();
}


void ManageTagsDialog::on_tagsViewUnselectAllLinksPushButton_clicked()
{
    ui->tagsViewCsdListWidget->clearSelection();
}


void ManageTagsDialog::on_tagsViewAddLinkPushButton_clicked()
{
    // get selected tag id
    QList<QUuid> selectionTags = tagsView_GetSelectedTagsIds();
    if (selectionTags.count() != 1) {
        QString errorString = QString(tr("A tag must be selected."));
        QMessageBox::critical(nullptr,tr("Error"), errorString);
        return; // should never happen
    }

    if (allCsds.size()==0) {
        // no Csds defined yet in the scenario
        QString errorString = QString(tr("No cash stream definition in the scenario. "
            "Cannot link to Tag"));
        QMessageBox::critical(nullptr,tr("Error"), errorString);
        return;
    }

    // Get all the CSD ids involved in a relationship with that tag
    QSet<QUuid> csdIdInRelationsship = relationships.getRelationshipsForTag(selectionTags[0]);

    // Build list of FSD not included in relationships with selected tag
    QSet<managetags::CsdItem> csdSet;
    foreach (QUuid fsdId, allCsds.keys()) {
        if (false == csdIdInRelationsship.contains(fsdId)) {
            managetags::CsdItem item = allCsds.value(fsdId);
            csdSet.insert(item);
        }
    }

    if (csdSet.size()==0) {
        // all Fsds are already in relationship with the selected tag
        QString errorString = QString(tr("No more cash stream definition to link. All the existing "
            "ones defined the scenario have already been linked to this tag."));
        QMessageBox::warning(nullptr,tr("Warning"), errorString);
        return;
    }

    emit signalAddCsdPrepareContent(csdSet);
    addCsdDlg->show();
}


void ManageTagsDialog::on_tagsViewDeleteLinksPushButton_clicked()
{
    // get the selected CSD ids
    QList<QUuid> selectionCsds = tagsView_GetSelectedCsdsIds();
    if (selectionCsds.size() == 0) {
        // no selection
        QString errorString = QString(tr("Select at least 1 cash stream definition."));
        QMessageBox::critical(nullptr,tr("Error"), errorString);
        return;
    }

    // get selected tag id (there should be one, since there are Csd items)
    QList<QUuid> selectionTags = tagsView_GetSelectedTagsIds();
    if (selectionTags.count() != 1) {
        return; // should never happen
    }

    // delete the relationships if any to delete
    foreach (QUuid csdId, selectionCsds) {
        relationships.deleteRelationship(selectionTags[0], csdId);
    }

    // update list of FSD
    tagsView_UpdateCsdList();
}


// New Csds have been chosen and must be added
void ManageTagsDialog::slotChooseCsdsResult(QSet<QUuid> newCsdsToLink)
{
    // get selected tag id (there should be one, since there are Csd items)
    QList<QUuid> selectionTags = tagsView_GetSelectedTagsIds();
    if (selectionTags.count() != 1) {
        return; // should never happen
    }

    // create new relationships
    foreach (QUuid csdId, newCsdsToLink) {
        relationships.addRelationship(selectionTags[0], csdId);
    }

    // update csd list
    tagsView_UpdateCsdList();
}


void ManageTagsDialog::slotChooseCsdsCompleted()
{
}


void ManageTagsDialog::slotChooseTagsResult(QSet<QUuid> newTagsToLink)
{
    // get selected csd id (there should be one, since there are Tags items)
    QList<QUuid> selectionCsds = csdsView_GetSelectedCsdsIds();
    if (selectionCsds.count() != 1) {
        return; // should never happen
    }

    // create new relationships
    foreach (QUuid tagId, newTagsToLink) {
        relationships.addRelationship(tagId, selectionCsds[0]);
    }

    // update tag list
    csdsView_UpdateTagList();
}


void ManageTagsDialog::slotChooseTagsCompleted()
{

}

//----- TAB : CSD View -----------------------------------------------------------------------------

// We are entering this tab. Tag set and/or Csd set may be different. Update the full Tab.
// Strategy is to refresh the csd list and then reselect the previous selected csd (if still
// existing) and display the relevant linked Tags
void ManageTagsDialog::csdsView_Refresh()
{
    // first, remember the csd selected (if any)
    QList<QUuid> selection = csdsView_GetSelectedCsdsIds();

    // then clear everything and rebuild lists
    csdsView_UpdateCsdList();

    // Finally try to reselect the same csd if it still exists
    if(selection.count()==1){
        if( allCsds.contains(selection[0])==true ){
            csdsView_SelectCsd(selection[0]); // Linked Tags will be displayed
        }
    }
}


void ManageTagsDialog::csdsView_UpdateTagList()
{
    ui->csdsViewTagsListWidget->clear();

    // Get the id of the selected csd
    QList<QUuid> csdIds = csdsView_GetSelectedCsdsIds();

    // fill the list
    if (csdIds.count()!=0) {
        QSet<QUuid> tagSet = relationships.getRelationshipsForCsd(csdIds[0]);
        CustomListItem *item;
        foreach (QUuid tagId, tagSet) {
            bool found;
            Tag tag = tags.getTag(tagId, found);
            if (found==false){  // should never happen
                continue;
            }
            QString displayName = tag.getName();
            item = new CustomListItem(displayName,{.id=tag.getId(),.name=tag.getName()});
            ui->csdsViewTagsListWidget->addItem(item) ;  // list widget will take ownership of items
        }
    }

    // update tag list title
    csdsView_UpdateTagListLabel();

}


// Csds View : clear the csds list and rebuild from data.
// Remove any selection already present. Tag list is also cleared since csd selection is gone.
void ManageTagsDialog::csdsView_UpdateCsdList()
{
    ui->csdsViewCsdsListWidget->clear();

    CustomListItem *item;
    foreach (managetags::CsdItem csdItem, allCsds) {
        QString displayText = QString("%2").arg(csdItem.name);
        item = new CustomListItem(displayText,{.id=csdItem.id,.name=displayText});
        if ( (GbpController::getInstance().getAllowDecorationColor()==true) &&
            (csdItem.color.isValid()==true)) {
            item->setForeground(csdItem.color);
        }
        ui->csdsViewCsdsListWidget->addItem(item) ;// list widget will take ownership of item
    }

    // Tag list must also be cleared.
    ui->csdsViewTagsListWidget->clear();
}


QList<QUuid> ManageTagsDialog::csdsView_GetSelectedTagsIds()
{
    QList<QUuid> result;

    QList<QListWidgetItem *> selection = ui->csdsViewTagsListWidget->selectedItems();
    for (int var = 0; var < selection.count(); ++var) {
        CustomItem selectedItem = selection[var]->data(Qt::UserRole).value<CustomItem>();
        result.append(selectedItem.id);
    }
    return result;
}


QList<QUuid> ManageTagsDialog::csdsView_GetSelectedCsdsIds()
{
    QList<QUuid> result;

    QList<QListWidgetItem *> selection = ui->csdsViewCsdsListWidget->selectedItems();
    for (int var = 0; var < selection.count(); ++var) {
        CustomItem selectedItem = selection[var]->data(Qt::UserRole).value<CustomItem>();
        result.append(selectedItem.id);
    }
    return result;
}


void ManageTagsDialog::csdsView_SelectCsd(QUuid csdId)
{
    ui->csdsViewCsdsListWidget->clearSelection();
    int noRows = ui->csdsViewCsdsListWidget->count();
    for(int i=0;i<noRows;i++){
        QListWidgetItem *item = ui->csdsViewCsdsListWidget->item(i);
        CustomItem cItem = item->data(Qt::UserRole).value<CustomItem>();
        if(csdId == cItem.id){
            item->setSelected(true);
            return;
        }
    }
}


void ManageTagsDialog::csdsView_UpdateTagListLabel()
{
    QString s = tr("Linked Tags");
    if (ui->csdsViewTagsListWidget->count()!=0) {
        s = QString("%1 (%2)").arg(s).arg(ui->csdsViewTagsListWidget->count());
    }
    ui->csdsViewTagsListLabel->setText(s);

}


void ManageTagsDialog::on_csdsViewUnselectPushButton_clicked()
{
    ui->csdsViewTagsListWidget->clearSelection();
}


void ManageTagsDialog::on_csdsViewSelectAllPushButton_clicked()
{
    ui->csdsViewTagsListWidget->selectAll();
}


void ManageTagsDialog::on_csdsViewUnlinkPushButton_clicked()
{
    // get the selected Tags ids
    QList<QUuid> selectionTags = csdsView_GetSelectedTagsIds();
    if (selectionTags.size() == 0) {
        // no selection
        QString errorString = QString(tr("Select at least 1 tag."));
        QMessageBox::critical(nullptr,tr("Error"), errorString);
        return;
    }

    // get selected csds id (there should be one, since there are Tags items)
    QList<QUuid> selectionCsds = csdsView_GetSelectedCsdsIds();
    if (selectionCsds.count() != 1) {
        return; // should never happen
    }

    // delete the relationships if any to delete
    bool removed;
    foreach (QUuid tagId, selectionTags) {
        removed = relationships.deleteRelationship(tagId, selectionCsds[0]);
    }

    // update list of Tags
    csdsView_UpdateTagList();
}


void ManageTagsDialog::on_csdsViewLinkPushButton_clicked()
{
    // get selected csd id
    QList<QUuid> selectionCsds= csdsView_GetSelectedCsdsIds();
    if (selectionCsds.count() != 1) {
        QString errorString = QString(tr("A cash stream definition must be selected."));
        QMessageBox::critical(nullptr,tr("Error"), errorString);
        return;
    }

    if (tags.size()==0) {
        // no Tag defined yet in the scenario
        QString errorString = QString(tr("No tag defined in the scenario. "
            "Cannot link to cash stream definition"));
        QMessageBox::critical(nullptr,tr("Error"), errorString);
        return;
    }

    // Get all the Tag ids involved in a relationship with that csd
    QSet<QUuid> tagIdsInRelationship = relationships.getRelationshipsForCsd(selectionCsds[0]);

    // Build list of Tag ids not included in relationships with selected csd
    QSet<QUuid> tagUnlinkedSet;
    QSet<Tag> allTagsSet = tags.getTags();
    foreach (Tag tag, allTagsSet) {
        QUuid id = tag.getId();
        if (false == tagIdsInRelationship.contains(id)) {
            tagUnlinkedSet.insert(id);
        }
    }

    if (tagUnlinkedSet.size()==0) {
        // all Tags are already in relationship with the selected csd
        QString errorString = QString(tr("No more tags to link. All the existing "
            "ones defined the scenario have already been linked to this cash stream definition."));
        QMessageBox::information(nullptr,tr("Information"), errorString);
        return;
    }

    emit signalAddTagsPrepareContent(tags, tagUnlinkedSet);
    addTagDlg->show();
}


void ManageTagsDialog::on_csdsViewCsdsListWidget_itemSelectionChanged()
{
    csdsView_UpdateTagList();
}


void ManageTagsDialog::on_tagsDefTableView_doubleClicked(const QModelIndex &index)
{
    on_tagsDefEditPushButton_clicked();
}

