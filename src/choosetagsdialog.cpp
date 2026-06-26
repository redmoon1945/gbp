#include "choosetagsdialog.h"
#include "gbplogger.h"
#include <QTimer>
#include "ui_choosetagsdialog.h"
#include "uiutil.h"
#include <QMessageBox>
#include "gbpqmessage.h"


ChooseTagsDialog::ChooseTagsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ChooseTagsDialog)
{
    ui->setupUi(this);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);

    // Set smaller font for action buttons
    QFont appFont = QApplication::font();
    QFont font = appFont;
    Util::changeFontSize(font, Util::FontResizeIntensity::WEAK, true,
        "ChooseTagsDialog - action buttons");
    ui->listWidget->setFont(appFont);
    ui->selectAllPushButton->setFont(font);
    ui->unselectAllPushButton->setFont(font);
}


ChooseTagsDialog::~ChooseTagsDialog()
{
    delete ui;
}


// Prepare the dialog before it is displayed
// Input parameters :
//  tags : the set of all tags available for selection
//  preSelectedTags : list of Tags that must be pre-selected when the Dialog is displayed
void ChooseTagsDialog::slotPrepareContent(const Tags &tags, const QSet<QUuid> &preSelectedTags)
{
    this->tags = tags;

    updateList(preSelectedTags);

    ui->listWidget->setFocus();
}


void ChooseTagsDialog::on_ChooseTagsDialog_rejected()
{
    on_cancelPushButton_clicked();
}


// Update the content of the listbox
void ChooseTagsDialog::updateList(const QSet<QUuid> &preSelectedTags)
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
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            tr("You must select at leat one tag."), {tr("OK")}, 0, 0);
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


void ChooseTagsDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("ChooseTagsDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
    });
}

