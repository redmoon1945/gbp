#include "choosetagsdialog.h"
#include "ui_choosetagsdialog.h"
#include <QMessageBox>


ChooseTagsDialog::ChooseTagsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ChooseTagsDialog)
{
    ui->setupUi(this);
}


ChooseTagsDialog::~ChooseTagsDialog()
{
    delete ui;
}


// Prepare the dialog before it is displayed
// Input parameters :
//  tags : the set of all tags available for selection
//  preSelectedTags : list of Tags that must be pre-selected when the Dialog is displayed
void ChooseTagsDialog::slotPrepareContent(Tags tags, QSet<QUuid> preSelectedTags)
{
    this->tags = tags;

    updateList(preSelectedTags);

    // Set focus on Apply
    ui->applyPushButton->setFocus();
}


void ChooseTagsDialog::on_ChooseTagsDialog_rejected()
{
    on_cancelPushButton_clicked();
}


// Update the content of the listbox
void ChooseTagsDialog::updateList(QSet<QUuid> preSelectedTags)
{
    ui->listWidget->clear();

    // fill the list with all known tags
    CustomListItem *item;
    bool found;
    QSet<Tag> tagsSet = tags.getTags();
    foreach (Tag tag, tagsSet) {
        // insert in the list to display
        QString displayText = QString("%1").arg(tag.getName());
        item = new CustomListItem(displayText,{.id=tag.getId(),.name=tag.getName()});
        ui->listWidget->addItem(item) ;  // list widget will take ownership of the item
    }

    // pre-select from preSelectedTags
    int noRows = ui->listWidget->count();
    for(int i=0;i<noRows;i++){
        QListWidgetItem *item = ui->listWidget->item(i);
        CustomItem cItem = item->data(Qt::UserRole).value<CustomItem>();
        if( true == preSelectedTags.contains(cItem.id) ){
            item->setSelected(true);
        }
    }
}


void ChooseTagsDialog::on_cancelPushButton_clicked()
{
    emit signalCompleted(true);
    hide();
}


// At least one tag must be selected, this is mandatory. It is expected by the caller of this
// Dialog.
void ChooseTagsDialog::on_applyPushButton_clicked()
{
    // Get selected items and make sure at least one tag is selected
    QList<QListWidgetItem *> selection = ui->listWidget->selectedItems();
    if (selection.size()==0) {
        QMessageBox::critical(nullptr,tr("Error"),
            tr("You must select at leat one tag."));
        return;
    }

    // extract the QUuid of all the selected items
    QSet<QUuid> result;
    foreach (QListWidgetItem *item, selection) {
        CustomItem cItem = item->data(Qt::UserRole).value<CustomItem>();
        result.insert(cItem.id);
    }

    // notify the parent and quit
    emit signalResult(result);
    emit signalCompleted(false);
    hide();
}


void ChooseTagsDialog::on_selectAllPushButton_clicked()
{
    ui->listWidget->selectAll();
}


void ChooseTagsDialog::on_unselectAllPushButton_clicked()
{
    ui->listWidget->clearSelection();
}

