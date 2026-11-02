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

#ifndef MANAGETAGSDIALOG_H
#define MANAGETAGSDIALOG_H

#include <QDialog>
#include <QLocale>
#include <QHash>
#include <qlistwidget.h>
#include "choosetagsdialog.h"
#include "edittagdialog.h"
#include "managetagschoosecsddialog.h"
#include "tags.h"
#include "tagcsdrelationships.h"
#include "edittagdialog.h"
#include "managetagscsdinfo.h"
#include "managetagstagsdefmodel.h"

namespace Ui {
class ManageTagsDialog;
}

class ManageTagsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ManageTagsDialog(QLocale aLocale, QWidget *parent = nullptr);
    ~ManageTagsDialog();

signals:
    // For client of ManageTagsDialog : send result and edition completion notification
    void signalManageTagsResult(Tags newTags, TagCsdRelationships newRelationships);
    void signalManageTagsCompleted();

    // EditTag : prepare before edition
    void signalEditTagPrepareContent(Tags currentTags, Tag tagIdToEdit, bool isForNewTag);

    // Add Csd to link : prepare before edition
    void signalAddCsdPrepareContent(QSet<managetags::CsdItem> csdSet);

    // Add Tag to link : prepare before edition
    void signalAddTagsPrepareContent(Tags tags, QSet<QUuid> preSelectedTags);

public slots:
    // From client of ManageTagsDialog : Prepare edition. Call this before show()
    void slotPrepareContent(Tags newTags, TagCsdRelationships newRelationships,
        QHash<QUuid, managetags::CsdItem> newCsdItems);

    // Result and completion for tag creation/edition
    void slotEditTagResult(Tag newtag);
    void slotEditTagCompleted();

    // Result and completion signal from Add Csd (TAB Tag View)
    void slotChooseCsdsResult(QSet<QUuid> newCsdsToLink);
    void slotChooseCsdsCompleted();

    // Result and completion signal from Add Tag (TAB Csd View)
    void slotChooseTagsResult(QSet<QUuid> selectedTagsIds);
    void slotChooseTagsCompleted(bool canceled);

private slots:
    // General
    void on_cancelPushButton_clicked();
    void on_applyPushButton_clicked();
    void on_closePushButton_clicked();
    void on_ManageTagsDialog_rejected();
    void on_tagsTabWidget_currentChanged(int index);

    // TAB : Tags Definition
    void on_tagsDefAddPushButton_clicked();
    void on_tagsDefDeletePushButton_clicked();
    void on_tagsDefEditPushButton_clicked();
    void on_tagsDefDuplicatePushButton_clicked();
    void on_tagsDefSelectAllPushButton_clicked();
    void on_tagsDefUnselectAllPushButton_clicked();
    void on_tagsDefImportPushButton_clicked();
    void on_tagsDefTableView_doubleClicked(const QModelIndex &index);

    // TAB : Tags View
    void on_tagsViewAddLinkPushButton_clicked();
    void on_tagsViewDeleteLinksPushButton_clicked();
    void on_tagsViewSelectAllLinksPushButton_clicked();
    void on_tagsViewUnselectAllLinksPushButton_clicked();
    void on_tagsViewTagsListWidget_itemSelectionChanged();

    // TAB : CSD View
    void on_csdsViewUnselectPushButton_clicked();
    void on_csdsViewSelectAllPushButton_clicked();
    void on_csdsViewUnlinkPushButton_clicked();
    void on_csdsViewLinkPushButton_clicked();
    void on_csdsViewCsdsListWidget_itemSelectionChanged();

protected:
    void showEvent(QShowEvent* event) override;

private:
    Ui::ManageTagsDialog *ui;

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
    QLocale locale;
    Tags tags;  // tags being edited
    QHash<QUuid, managetags::CsdItem> allCsds; // Complete set of all the Csds of the scenario
    TagCsdRelationships relationships; // tag-fsd relationships being edited

    // ListView model
    ManageTagsTagsDefModel* itemTableModel;

    // children dialogs
    EditTagDialog* editTagDlg;
    ManageTagsChooseCsdDialog* addCsdDlg;
    ChooseTagsDialog* addTagDlg;

    // *** Methods ***

    // TAB : Tags Definitions
    void tagsDef_UpdateTagList();
    void tagsDef_UpdateTagListLabel();
    QList<QUuid> tagsDef_GetSelectedTagsIds();
    void tagsDef_SelectTags(QSet<QUuid> tagIdSet);
    void makeVisibleTag(QUuid tagId);

    // TAB : Tags View
    void tagsView_Refresh();
    void tagsView_UpdateTagList();
    void tagsView_UpdateCsdList();
    QList<QUuid> tagsView_GetSelectedTagsIds();
    QList<QUuid> tagsView_GetSelectedCsdsIds();
    void tagsView_SelectTag(QUuid tagId);
    void tagsView_UpdateCsdListLabel();

    // TAB : CSD View
    void csdsView_Refresh();
    void csdsView_UpdateCsdList();
    void csdsView_UpdateTagList();
    QList<QUuid> csdsView_GetSelectedTagsIds();
    QList<QUuid> csdsView_GetSelectedCsdsIds();
    void csdsView_SelectCsd(QUuid csdId);
    void csdsView_UpdateTagListLabel();

    // Change tracking
    /**
     * @brief Record that the tags and/or relationships have been modified (or not), and update
     * some UI hints : warning sign, "*" in the window title, bottom buttons.
     * @details The only function allowed to write ContentChange::changed - enforced by
     * ContentChange's friend declaration. See ContentChange below.
     * @param v New value of the flag.
     */
    void contentHasChanged(bool v);

    /**
     * @brief Whether the tags or relationships differ from originalTags / originalRelationships.
     * @return True if there are changes not applied yet.
     */
    bool hasContentChanged() const;

    /**
     * @brief Detect if the tags or relationships differ from what was received in
     * slotPrepareContent(), and update the "content has changed" flag and the bottom buttons
     * accordingly.
     * @details Compares with Tags::operator== and TagCsdRelationships::operator==. Must be called
     * after anything that modifies tags or relationships, in any of the 3 tabs.
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
        friend void ManageTagsDialog::contentHasChanged(bool v);
        friend bool ManageTagsDialog::hasContentChanged() const;
    };

    ContentChange contentChanged;

    // Copies of the tags and relationships as they were when the dialog was prepared, kept as a
    // reference for detecting changes not applied yet.
    Tags originalTags;
    TagCsdRelationships originalRelationships;

};

#endif // MANAGETAGSDIALOG_H
