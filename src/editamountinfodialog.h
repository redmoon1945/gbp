#ifndef EDITAMOUNTINFODIALOG_H
#define EDITAMOUNTINFODIALOG_H

#include <QDialog>
#include <QDate>
#include "currencyhelper.h"

namespace Ui {
class EditAmountInfoDialog;
}

class EditAmountInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditAmountInfoDialog(QWidget *parent = nullptr);
    ~EditAmountInfoDialog();

public slots:
    // From client of EditAmountInfoDialog : Prepare Dialog before edition
    void slotPrepareContent(bool newEditMode, CurrencyInfo newCurrInfo,  QList<QDate> newExistingDates,QDate dateToEdit, qint64 amountToEdit);

signals:
    // For client of EditAmountInfoDialog : Send results of edition and notify of edition completion
    void signalEditElementResult(QDate date, qint64 amount);
    void signalEditElementCompleted();
private slots:

    void on_EditAmountInfoDialog_rejected();
    void on_closePushButton_clicked();
    void on_applyPushButton_clicked();

private:
    Ui::EditAmountInfoDialog *ui;

    bool editMode;      // edit an existing element (true) or add new element (false)
    QList<QDate> existingDates;
    CurrencyInfo currInfo;
};

#endif // EDITAMOUNTINFODIALOG_H
