#ifndef PRESENTVALUECALCULATORDIALOG_H
#define PRESENTVALUECALCULATORDIALOG_H

#include <QDialog>
#include <QLocale>

namespace Ui {
class PresentValueCalculatorDialog;
}

class PresentValueCalculatorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PresentValueCalculatorDialog(QLocale locale, QWidget *parent = nullptr);
    ~PresentValueCalculatorDialog();

private slots:
    void on_closePushButton_clicked();
    void on_PresentValueCalculatorDialog_rejected();
    void on_pvToFvPushButton_clicked();
    void on_fvToPvPushButton_clicked();
    void on_annualDiscountRateDoubleSpinBox_valueChanged(double arg1);

private:
    Ui::PresentValueCalculatorDialog *ui;
    QLocale locale;

    // Methods
    double convertMonthlyRateStringToValue(const QString s);
    QString makeStringFromMonthlyRate(double value);
};

#endif // PRESENTVALUECALCULATORDIALOG_H
