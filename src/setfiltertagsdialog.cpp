#include "setfiltertagsdialog.h"
#include "ui_setfiltertagsdialog.h"
#include <QMessageBox>


SetFilterTagsDialog::SetFilterTagsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SetFilterTagsDialog)
{
    ui->setupUi(this);
}


SetFilterTagsDialog::~SetFilterTagsDialog()
{
    delete ui;
}


void SetFilterTagsDialog::slotPrepareContent(Tags tags, QSet<QUuid> preSelectedTags,
    FilterTags::Mode mode)
{
    this->tags = tags;

    // display choosen mode
    if (mode==FilterTags::Mode::ALL) {
        ui->allRadioButton->setChecked(true);
    } else if (mode==FilterTags::Mode::ANY) {
        ui->anyRadioButton->setChecked(true);
    } else {
        ui->noneRadioButton->setChecked(true);
    }

    updateList(preSelectedTags);
}


void SetFilterTagsDialog::on_SetFilterTagsDialog_rejected()
{
    on_cancelPushButton_clicked();
}


// Update the content of the listbox
void SetFilterTagsDialog::updateList(QSet<QUuid> preSelectedTags)
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


void SetFilterTagsDialog::on_cancelPushButton_clicked()
{
    emit signalCompleted(true);
    hide();
}


// At least one tag must be selected, this is mandatory. It is expected by the caller of this
// Dialog.
void SetFilterTagsDialog::on_applyPushButton_clicked()
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

    // Get mode
    FilterTags::Mode mode;
    if (ui->allRadioButton->isChecked()==true) {
        mode = FilterTags::Mode::ALL;
    } else if (ui->anyRadioButton->isChecked()==true){
        mode = FilterTags::Mode::ANY;
    } else {
        mode = FilterTags::Mode::NONE;
    }

    // notify the parent and quit
    emit signalResult(mode, result);
    emit signalCompleted(false);
    hide();
}


void SetFilterTagsDialog::on_selectAllPushButton_clicked()
{
    ui->listWidget->selectAll();
}


void SetFilterTagsDialog::on_unselectAllPushButton_clicked()
{
    ui->listWidget->clearSelection();
}

