#ifndef SETFILTERTAGSDIALOG_H
#define SETFILTERTAGSDIALOG_H

#include <QDialog>
#include <qlistwidget.h>
#include "tags.h"
#include "filtertags.h"

namespace Ui {
class SetFilterTagsDialog;
}

class SetFilterTagsDialog : public QDialog
{
    Q_OBJECT

public:

    explicit SetFilterTagsDialog(QWidget *parent = nullptr);
    ~SetFilterTagsDialog();

signals:
    // For client of ManageTagsChooseTagsDialog : send result and edition completion notification
    void signalResult(FilterTags::Mode mode, QSet<QUuid> filterTagIdSet);
    void signalCompleted(bool canceled);

public slots:
    // From client of SetFilterTagsDialog: Prepare edition. Call this before show()
    void slotPrepareContent(Tags tags, QSet<QUuid> preSelectedTags, FilterTags::Mode mode);

private slots:
    void on_SetFilterTagsDialog_rejected();
    void on_cancelPushButton_clicked();
    void on_applyPushButton_clicked();
    void on_selectAllPushButton_clicked();
    void on_unselectAllPushButton_clicked();

private:
    Ui::SetFilterTagsDialog *ui;

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
    Tags tags;  // copy of all tags for the scenario

    // Methods
    void updateList(QSet<QUuid> preSelectedTags);


};

#endif // SETFILTERTAGSDIALOG_H
