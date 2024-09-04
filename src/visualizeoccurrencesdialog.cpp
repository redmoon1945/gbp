#include "visualizeoccurrencesdialog.h"
#include "ui_visualizeoccurrencesdialog.h"
#include "gbpcontroller.h"


VisualizeOccurrencesDialog::VisualizeOccurrencesDialog(QLocale locale, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::VisualizeOccurrencesDialog)
{
    ui->setupUi(this);
    this->locale = locale;
}


VisualizeOccurrencesDialog::~VisualizeOccurrencesDialog()
{
    delete ui;
}


// scenarioInflation is used only when streamDef is a Periodic stream def
void VisualizeOccurrencesDialog::slotPrepareContent(CurrencyInfo currInfo, Growth scenarioInflation, FeStreamDef *streamDef)
{
    this->currInfo = currInfo;
    updateTextTab(scenarioInflation, streamDef);
    updateChartTab(scenarioInflation, streamDef);
}


void VisualizeOccurrencesDialog::on_closePushButton_clicked()
{
    this->hide();
    ui->plainTextEdit->setPlainText(""); // dont hold the text, no use for that now
    emit signalCompleted();
}



void VisualizeOccurrencesDialog::on_VisualizeOccurrencesDialog_rejected()
{
    on_closePushButton_clicked();
}

// Compute the financial events for this FeStreamDef, which can be  either periodic or irregular ,
// and display the results in the PlainText Widget
void VisualizeOccurrencesDialog::updateTextTab(Growth scenarioInflation, FeStreamDef *streamDef)
{

    QString amountString;

    if(streamDef->getStreamType()==FeStreamDef::PERIODIC){

        // *** PERIODIC ***

        PeriodicFeStreamDef* psd = (PeriodicFeStreamDef *)streamDef; // cast to what is is in reality

        // Build headers
        QStringList resultStringList;
        int ok;
        resultStringList.append(tr("Dates are in ISO 8601 format (YYYY-MM-DD)."));
        if (psd->getGrowthStrategy()==PeriodicFeStreamDef::GrowthStrategy::INFLATION){
            // adjust inflation if required
            Growth adjustedInflation = scenarioInflation;
            bool capped;
            adjustedInflation.changeByFactor(psd->getInflationAdjustmentFactor(),capped);

            if (Growth::CONSTANT == scenarioInflation.getType()) {
                QString infString = QString("%1%").arg(static_cast<double>(Growth::fromDecimalToDouble(adjustedInflation.getAnnualConstantGrowth())));
                resultStringList.append(QString(tr("Using constant adjusted annual inflation of %1.")).arg(infString));
            } else if (Growth::VARIABLE == scenarioInflation.getType()) {
                resultStringList.append(tr("Using variable inflation."));
            } else{
                // should not happen...do nothing
            }
        } else if (psd->getGrowthStrategy()==PeriodicFeStreamDef::GrowthStrategy::CUSTOM) {
            if (psd->getGrowth().getType()==Growth::CONSTANT) {
                resultStringList.append(QString(tr("Using custom constant growth of %1 percent.")).arg(
                    static_cast<double>(Growth::fromDecimalToDouble(psd->getGrowth().getAnnualConstantGrowth()))));
            } else if (psd->getGrowth().getType()==Growth::VARIABLE){
                resultStringList.append(QString(tr("Using custom variable growth.")));
            } else {
                // should not happen...do nothing
            }
        } else {
            resultStringList.append(QString(tr("No growth of any kind is applied.")));
        }
        bool usePvConversion = GbpController::getInstance().getUsePresentValue();
        double pvAnnualDiscountRate = GbpController::getInstance().getPvDiscountRate();
        if ((usePvConversion==true)&&(pvAnnualDiscountRate!=0)) {
            QString s = QString(tr("Converting Future Values to Present Values using an annual discount rate of %1 percent.")).arg(pvAnnualDiscountRate);
            resultStringList.append(s);
        }

        // Generate financial events
        uint saturationCount;
        QList<Fe> feList = psd->generateEventStream(psd->getValidityRange(), scenarioInflation, (usePvConversion)?(pvAnnualDiscountRate):(0),
                                                    GbpController::getInstance().getTomorrow(), saturationCount);
        if(saturationCount > 0){
            resultStringList.append(QString(tr("Amount was too big %1 times and have been capped to %2.")).arg(saturationCount).arg(CurrencyHelper::maxValueAllowedForAmountInDouble(currInfo.noOfDecimal)));
        }
        resultStringList.append(QString(tr("%1 %2 event(s) have been generated for the whole Validity Range.\n")).arg(feList.count()).arg(
            (psd->getIsIncome())?(tr("income")):(tr("expense"))));

        // transform events values into text log
        long double cummul = 0;
        QDate tomorrow = GbpController::getInstance().getTomorrow();
        foreach(Fe fe, feList){
            if (abs(fe.amount) > CurrencyHelper::maxValueAllowedForAmount() ){ // should never happen because of saturation
                resultStringList.append(QString("%1 : %2").arg(fe.occurrence.toString(Qt::ISODate)).arg(tr("Amount is bigger than the maximum allowed")));
            } else {
                double amountDouble = CurrencyHelper::amountQint64ToDouble(abs(fe.amount), currInfo.noOfDecimal, ok);
                if (ok != 0){
                    resultStringList.append(QString("%1 : %2").arg(fe.occurrence.toString(Qt::ISODate)).arg(tr("Error during amount conversion")));
                } else {
                    amountString = CurrencyHelper::formatAmount(amountDouble, currInfo, locale, true);
                    cummul += amountDouble;
                    QString cummulString = CurrencyHelper::formatAmount(static_cast<double>(cummul), currInfo, locale, true);
                    QString s = QString(tr("%1 : %2 (cummul=%3)")).arg(fe.occurrence.toString(Qt::ISODate)).arg(amountString).arg(cummulString);
                    if (fe.occurrence<tomorrow){
                        // if event is in the past, mention it
                        s = s.append(tr("  *** PAST -> discarded ***"));
                    }
                    resultStringList.append(s);
                }
            }
        }

        // update Plain Text
        ui->plainTextEdit->setPlainText(resultStringList.join("\n"));

    } else {

        // *** IRREGULAR ***

        IrregularFeStreamDef* isd = (IrregularFeStreamDef *)streamDef; // cast to what is is in reality

        // Build headers
        QStringList resultStringList;
        int ok;
        resultStringList.append(tr("Dates are in ISO 8601 format (YYYY-MM-DD)."));
        bool usePvConversion = GbpController::getInstance().getUsePresentValue();
        double pvAnnualDiscountRate = GbpController::getInstance().getPvDiscountRate();
        if ((usePvConversion==true)&&(pvAnnualDiscountRate!=0)) {
            QString s = QString(tr("Converting Future Values to Present Values using an annual discount rate of %1 percent.")).arg(pvAnnualDiscountRate);
            resultStringList.append(s);
        }

        // Generate the financial events
        uint saturationCount;
        QList<Fe> feList = isd->generateEventStream(DateRange::INFINITE, (usePvConversion)?(pvAnnualDiscountRate):(0), GbpController::getInstance().getTomorrow(), saturationCount);
        if(saturationCount > 0){
            resultStringList.append(QString(tr("Amount was too big %1 times and have been capped to %2.")).arg(saturationCount).arg(CurrencyHelper::maxValueAllowedForAmountInDouble(currInfo.noOfDecimal)));
        }
        resultStringList.append(QString(tr("%1 %2 event(s) have been generated.\n")).arg(feList.count()).arg((isd->getIsIncome())?(tr("income")):(tr("expense"))));

        // transform into text log
        long double cummul = 0;
        QDate tomorrow = GbpController::getInstance().getTomorrow();
        QString amountString,cummulString,s;
        foreach(Fe fe, feList){
            if (abs(fe.amount) > CurrencyHelper::maxValueAllowedForAmount() ){ // should never happen because of saturation
                resultStringList.append(QString("%1 : %2").arg(fe.occurrence.toString(Qt::ISODate)).arg(tr("Amount is bigger than the maximum allowed")));
            } else {
                double amountDouble = CurrencyHelper::amountQint64ToDouble(abs(fe.amount), currInfo.noOfDecimal, ok);
                if (ok != 0){
                    // should not happen
                    resultStringList.append(QString("%1 : %2").arg(fe.occurrence.toString(Qt::ISODate)).arg(tr("Error during amount conversion")));
                } else {
                    amountString = CurrencyHelper::formatAmount(amountDouble, currInfo, locale, true);
                    cummul += amountDouble;
                    cummulString = CurrencyHelper::formatAmount(static_cast<double>(cummul), currInfo, locale, true);
                    s = QString(tr("%1 : %2 (cummul=%3)")).arg(fe.occurrence.toString(Qt::ISODate)).arg(amountString).arg(cummulString);
                    if (fe.occurrence<tomorrow){
                        // if event is in the past, mention it
                        s = s.append(tr("  *** PAST -> discarded ***"));
                    }
                    resultStringList.append(s);
                }
            }
        }

        // Update the PlainText content
        QString r = resultStringList.join("\n");
        ui->plainTextEdit->setPlainText(r);
    }



}


void VisualizeOccurrencesDialog::updateChartTab(Growth scenarioInflation,  FeStreamDef *streamDef)
{

}

