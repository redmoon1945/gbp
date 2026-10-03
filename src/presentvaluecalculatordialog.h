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
    explicit PresentValueCalculatorDialog(QLocale locale);
    ~PresentValueCalculatorDialog();

public slots:
    // For client of PresentValueCalculatorDialog : prepare Dialog before edition
    void slotPrepareContent();

private slots:
    void on_closePushButton_clicked();
    void on_PresentValueCalculatorDialog_rejected();
    void on_pvToFvPushButton_clicked();
    void on_fvToPvPushButton_clicked();
    void on_annualDiscountRateDoubleSpinBox_valueChanged(double arg1);

protected:
    void showEvent(QShowEvent* event) override;

private:
    Ui::PresentValueCalculatorDialog *ui;
    QLocale locale;

    // Methods
    /**
     * @brief Gets the monthly discount rate to use for the PV <-> FV conversions.
     * @details Always computed from the annual discount rate spin box value. It must NOT be
     * read back from the monthly rate label : the label text is formatted with the user's
     * locale (e.g. "0,40741238 %" in French) and rounded to 8 decimals, so parsing it fails
     * for locales using a decimal comma or their own digits, which used to silently give a
     * rate of 0 (FV = PV).
     * @return The monthly discount rate, in percentage (e.g. 0.407412378 for 5% annually).
     */
    double getMonthlyDiscountRate() const;
    QString makeStringFromMonthlyRate(double value);
};

#endif // PRESENTVALUECALCULATORDIALOG_H
