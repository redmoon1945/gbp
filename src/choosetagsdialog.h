#ifndef CHOOSETAGSDIALOG_H
#define CHOOSETAGSDIALOG_H

#include <QDialog>
#include <qlistwidget.h>
#include "tags.h"

namespace Ui {
class ChooseTagsDialog;
}


/**
 * @brief Dialog used to choose tags in a list of available tags. The Ids of the
 * selected tags are returned in a Qset
 */
class ChooseTagsDialog : public QDialog
{
    Q_OBJECT

public:

    explicit ChooseTagsDialog(QWidget *parent = nullptr);
    ~ChooseTagsDialog();

signals:
    // For client of ChooseTagsDialog : send result and edition completion notification
    void signalResult(QSet<QUuid> selectedTagsIds);
    void signalCompleted(bool canceled);

public slots:
    // From client of ChooseTagsDialog: Prepare edition. Call this before show()
    void slotPrepareContent(Tags tags, QSet<QUuid> preSelectedTags);

private slots:
    void on_ChooseTagsDialog_rejected();
    void on_cancelPushButton_clicked();
    void on_applyPushButton_clicked();
    void on_selectAllPushButton_clicked();
    void on_unselectAllPushButton_clicked();

private:
    Ui::ChooseTagsDialog *ui;

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

#endif // CHOOSETAGSDIALOG_H
