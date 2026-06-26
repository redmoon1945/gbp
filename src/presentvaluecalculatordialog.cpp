#include "presentvaluecalculatordialog.h"
#include "gbplogger.h"
#include <QTimer>
#include "ui_presentvaluecalculatordialog.h"
#include "util.h"
#include "uiutil.h"

PresentValueCalculatorDialog::PresentValueCalculatorDialog(QLocale locale)
    : QDialog(NULL) // By passing NULL, we make this window independant, but MainWindow must close
                    // it before exiting
    , ui(new Ui::PresentValueCalculatorDialog)
{
    ui->setupUi(this);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);

    this->locale = locale;
    ui->pvDoubleSpinBox->setFocus();
    // set monthly discount rate
    double annualDiscoutRate = ui->annualDiscountRateDoubleSpinBox->value();
    double monthlyDiscountRate = (double)Util::annualToMonthlyGrowth(annualDiscoutRate);
    QString monthlyDiscountRateString = makeStringFromMonthlyRate(monthlyDiscountRate);
    ui->monthlyDiscountRateLabel->setText(monthlyDiscountRateString);

    // "pack" the dialog to fit the font. This is required when there is no "expanding" widgets
    this->adjustSize();

}


PresentValueCalculatorDialog::~PresentValueCalculatorDialog()
{
    delete ui;
}


void PresentValueCalculatorDialog::slotPrepareContent()
{
    ui->pvDoubleSpinBox->setFocus();
}


void PresentValueCalculatorDialog::on_closePushButton_clicked()
{
    hide();
}


void PresentValueCalculatorDialog::on_PresentValueCalculatorDialog_rejected()
{
    on_closePushButton_clicked();
}


void PresentValueCalculatorDialog::on_pvToFvPushButton_clicked()
{
    double pv = ui->pvDoubleSpinBox->value();
    QString monthlyDiscountRateString = ui->monthlyDiscountRateLabel->text();
    int noPeriod = ui->noOfMonthSpinBox->value();

    bool ok;
    double monthlyDiscountRate = convertMonthlyRateStringToValue(monthlyDiscountRateString);
    long double fv = Util::futureValue(pv, monthlyDiscountRate, noPeriod);
    ui->fvDoubleSpinBox->setValue(fv);
}


void PresentValueCalculatorDialog::on_fvToPvPushButton_clicked()
{
    double fv = ui->fvDoubleSpinBox->value();
    QString monthlyDiscountRateString = ui->monthlyDiscountRateLabel->text();
    int noPeriod = ui->noOfMonthSpinBox->value();

    bool ok;
    double monthlyDiscountRate = convertMonthlyRateStringToValue(monthlyDiscountRateString);
    long double pv = Util::presentValue(fv, monthlyDiscountRate, noPeriod);
    ui->pvDoubleSpinBox->setValue(pv);
}


void PresentValueCalculatorDialog::on_annualDiscountRateDoubleSpinBox_valueChanged(double arg1)
{
    double monthlyDiscountRate = (double)Util::annualToMonthlyGrowth(arg1);
    QString s = makeStringFromMonthlyRate(monthlyDiscountRate);
    ui->monthlyDiscountRateLabel->setText(s);
}


double PresentValueCalculatorDialog::convertMonthlyRateStringToValue(const QString s)
{
    QString s2 = s;
    if (s2.size()<3) {
        // should not happen
        return 0;
    }

    // remove the last 2 char (" %")
    s2.chop(2);

    // convert
    bool ok;
    double monthlyDiscountRate = s2.toDouble(&ok);
    if (ok==false) {
        return 0; // should never happen
    }

    return monthlyDiscountRate;
}


QString PresentValueCalculatorDialog::makeStringFromMonthlyRate(double value)
{
    QString s = QString("%1 %2")
        .arg(QString::number(value, 'f', 8))
        .arg("%");
    return s;
}


void PresentValueCalculatorDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("PresentValueCalculatorDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
    });
}

