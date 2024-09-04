#ifndef VISUALIZEOCCURRENCESDIALOG_H
#define VISUALIZEOCCURRENCESDIALOG_H

#include "currencyhelper.h"
#include "festreamdef.h"
#include "growth.h"
#include <QDialog>

namespace Ui {
class VisualizeOccurrencesDialog;
}

class VisualizeOccurrencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VisualizeOccurrencesDialog(QLocale locale, QWidget *parent = nullptr);
    ~VisualizeOccurrencesDialog();

public slots:
    void slotPrepareContent(CurrencyInfo currInfo, Growth adjustedInflation, FeStreamDef* streamDef );

signals:
    // For client of VisualizeOccurrencesDialog : sending completion notification
    void signalCompleted();

private slots:
    void on_closePushButton_clicked();
    void on_VisualizeOccurrencesDialog_rejected();

private:
    Ui::VisualizeOccurrencesDialog *ui;

    // Variables
    QLocale locale;
    CurrencyInfo currInfo;

    // Methods
    void updateTextTab(Growth scenarioInflation, FeStreamDef* streamDef);
    void updateChartTab(Growth scenarioInflation, FeStreamDef* streamDef);
};

#endif // VISUALIZEOCCURRENCESDIALOG_H
