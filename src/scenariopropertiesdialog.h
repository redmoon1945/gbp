#ifndef SCENARIOPROPERTIESDIALOG_H
#define SCENARIOPROPERTIESDIALOG_H

#include <QDialog>
#include <QLocale>

namespace Ui {
class ScenarioPropertiesDialog;
}

class ScenarioPropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ScenarioPropertiesDialog(QLocale theLocale, QWidget *parent = nullptr);
    ~ScenarioPropertiesDialog();

signals:
    // For client of ScenarioPropertiesDialog : send edition completion notification
    void signalScenarioPropertiesCompleted();

public slots:
    // From client of ScenarioPropertiesDialog : Prepare edition
    void slotPrepareContent();  // call this before show()

private slots:
    void on_closePushButton_clicked();
    void on_ScenarioPropertiesDialog_rejected();

private:
    Ui::ScenarioPropertiesDialog *ui;
    QLocale locale;
};

#endif // SCENARIOPROPERTIESDIALOG_H
