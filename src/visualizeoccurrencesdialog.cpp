/*
 *  Copyright (C) 2024 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
 *  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/#AGPL/>.
 */

#include "visualizeoccurrencesdialog.h"
#include "ui_visualizeoccurrencesdialog.h"
#include "gbpcontroller.h"
#include <qdatetimeaxis.h>
#include <qgraphicslayout.h>
#include <qvalueaxis.h>


VisualizeOccurrencesDialog::VisualizeOccurrencesDialog(QLocale locale, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::VisualizeOccurrencesDialog)
{
    ui->setupUi(this);
    ui->widget->installEventFilter(this);   // pass resize event received by "widget" to the Chart widget
    initChart();
    this->locale = locale;
}


VisualizeOccurrencesDialog::~VisualizeOccurrencesDialog()
{
    delete ui;
}


// scenarioInflation is used only when streamDef is a Periodic stream def
void VisualizeOccurrencesDialog::slotPrepareContent(CurrencyInfo currInfo, Growth scenarioInflation,
    QDate maxDateScenarioFeGeneration, FeStreamDef *streamDef)
{
    this->currInfo = currInfo;
    uint noOfSaturations;
    FeMinMaxInfo minMax;
    this->maxDateScenarioFeGeneration = maxDateScenarioFeGeneration;
    QList<Fe> feList = generateFinancialEvents(scenarioInflation, streamDef, noOfSaturations,
        minMax);

    // 50% of the space for each widget
    ui->splitter->setSizes(QList<int>({INT_MAX, INT_MAX}));

    updateTextTab(feList, noOfSaturations, scenarioInflation, streamDef);
    updateChartTab(feList, noOfSaturations, scenarioInflation, streamDef, minMax);
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


// Generate the financial events for that Stream Definition. Return the no of saturations
// that occured.
QList<Fe> VisualizeOccurrencesDialog::generateFinancialEvents(Growth scenarioInflation,
    FeStreamDef *streamDef, uint& saturationCount, FeMinMaxInfo& minMax){

    QList<Fe> result;

    // info on PV conversion
    bool usePvConversion = GbpController::getInstance().getUsePresentValue();
    double pvAnnualDiscountRate = GbpController::getInstance().getPvDiscountRate();


    // Generate financial events
    if(streamDef->getStreamType()==FeStreamDef::PERIODIC){
        PeriodicFeStreamDef* psd = (PeriodicFeStreamDef *)streamDef;

        // build fromto
        DateRange fromto = DateRange(GbpController::getInstance().getTomorrow(),
            (psd->getUseScenarioForEndDate()==true)?(maxDateScenarioFeGeneration):
            (psd->getEndDate()));

        result = psd->generateEventStream(fromto,
            maxDateScenarioFeGeneration, scenarioInflation, (usePvConversion)?
            (pvAnnualDiscountRate):(0), GbpController::getInstance().getTomorrow(), saturationCount,
            minMax);

    } else {
        IrregularFeStreamDef* isd = (IrregularFeStreamDef *)streamDef;

        // build fromto
        DateRange fromto = DateRange(GbpController::getInstance().getTomorrow(),
            maxDateScenarioFeGeneration);

        result = isd->generateEventStream(fromto, maxDateScenarioFeGeneration,
            (usePvConversion)?(pvAnnualDiscountRate):(0),
            GbpController::getInstance().getTomorrow(), saturationCount, minMax);
    }

    return result;
}


// Using the generated FeList, display the results in the PlainText Widget, with a header
// providing some useful information
void VisualizeOccurrencesDialog::updateTextTab(QList<Fe> feList, uint saturationCount, Growth
    scenarioInflation,  FeStreamDef *streamDef)
{

    QString amountString;

    if(streamDef->getStreamType()==FeStreamDef::PERIODIC){

        // *** PERIODIC ***

        PeriodicFeStreamDef* psd = (PeriodicFeStreamDef *)streamDef; // cast to what it is

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
                QString infString = QString("%1%").arg(static_cast<double>(
                    Growth::fromDecimalToDouble(adjustedInflation.getAnnualConstantGrowth())));
                resultStringList.append(
                    tr("Using constant adjusted annual inflation of %1.").arg(infString) );
            } else if (Growth::VARIABLE == scenarioInflation.getType()) {
                resultStringList.append(tr("Using variable inflation."));
            } else{
                // should not happen...do nothing
            }
        } else if (psd->getGrowthStrategy()==PeriodicFeStreamDef::GrowthStrategy::CUSTOM) {
            if (psd->getGrowth().getType()==Growth::CONSTANT) {
                resultStringList.append(
                    tr("Using custom constant growth of %1 percent.").arg(
                    static_cast<double>(Growth::fromDecimalToDouble(
                        psd->getGrowth().getAnnualConstantGrowth()))));
            } else if (psd->getGrowth().getType()==Growth::VARIABLE){
                resultStringList.append(tr("Using custom variable growth."));
            } else {
                // should not happen...do nothing
            }
        } else {
            resultStringList.append(tr("No growth of any kind is applied."));
        }
        bool usePvConversion = GbpController::getInstance().getUsePresentValue();
        double pvAnnualDiscountRate = GbpController::getInstance().getPvDiscountRate();
        if ((usePvConversion==true)&&(pvAnnualDiscountRate!=0)) {
            QString s = tr("Converting Future Values to Present Values using an annual discount "
                " rate of %1 percent.").arg(pvAnnualDiscountRate);
            resultStringList.append(s);
        }
        if(saturationCount > 0){
            resultStringList.append(tr("Amount was too big %1 times and have been capped to %2.")
                .arg(saturationCount).arg(CurrencyHelper::maxValueAllowedForAmountInDouble(
                currInfo.noOfDecimal)));
        }
        QDate tomorrow = GbpController::getInstance().getTomorrow();
        resultStringList.append(tr("No event generated before tomorrow %1.").arg(
            tomorrow.toString(Qt::ISODate)));
        resultStringList.append(tr("Scenario does not allow events past %1.").arg(
            maxDateScenarioFeGeneration.toString(Qt::ISODate)));
        resultStringList.append(tr("%1 %2 event(s) have been generated.\n").arg(feList.count()).arg(
            (psd->getIsIncome())?(tr("income")):(tr("expense"))));

        // transform events values into text log
        long double cummul = 0;
        foreach(Fe fe, feList){
            if (abs(fe.amount) > CurrencyHelper::maxValueAllowedForAmount() ){ // should not happen
                resultStringList.append(QString("%1 : %2").arg(fe.occurrence.toString(
                    Qt::ISODate)).arg(tr("Amount is bigger than the maximum allowed")));
            } else {
                double amountDouble = CurrencyHelper::amountQint64ToDouble(abs(fe.amount),
                    currInfo.noOfDecimal, ok);
                if (ok != 0){
                    resultStringList.append(QString("%1 : %2").arg(fe.occurrence.toString(
                        Qt::ISODate)).arg(tr("Error during amount conversion")));
                } else {
                    amountString = CurrencyHelper::formatAmount(amountDouble, currInfo, locale,
                        true);
                    cummul += amountDouble;
                    QString cummulString = CurrencyHelper::formatAmount(static_cast<double>(cummul),
                        currInfo, locale, true);
                    QString s = tr("%1 : %2 (cummul=%3)").arg(fe.occurrence.toString(Qt::ISODate)).
                        arg(amountString).arg(cummulString);
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

        IrregularFeStreamDef* isd = (IrregularFeStreamDef *)streamDef; // cast to what is is

        // Build headers
        QStringList resultStringList;
        int ok;
        resultStringList.append(tr("Dates are in ISO 8601 format (YYYY-MM-DD)."));
        bool usePvConversion = GbpController::getInstance().getUsePresentValue();
        double pvAnnualDiscountRate = GbpController::getInstance().getPvDiscountRate();
        if ((usePvConversion==true)&&(pvAnnualDiscountRate!=0)) {
            QString s = QString(tr("Converting Future Values to Present Values using an annual "
                "discount rate of %1 percent.")).arg(pvAnnualDiscountRate);
            resultStringList.append(s);
        }
        if(saturationCount > 0){
            resultStringList.append(tr("Amount was too big %1 times and have been capped to %2.")
                .arg(saturationCount).arg(CurrencyHelper::maxValueAllowedForAmountInDouble(
                currInfo.noOfDecimal)));
        }
        QDate tomorrow = GbpController::getInstance().getTomorrow();
        resultStringList.append(tr("No event generated before tomorrow %1.").arg(
            tomorrow.toString(Qt::ISODate)));
        resultStringList.append(tr("Scenario does not allow events past %1.").arg(
            maxDateScenarioFeGeneration.toString(Qt::ISODate)));
        resultStringList.append(tr("%1 %2 event(s) have been generated.\n").arg(feList.count()).
            arg((isd->getIsIncome())?(tr("income")):(tr("expense"))));

        // transform into text log
        long double cummul = 0;
        QString amountString,cummulString,s;
        foreach(Fe fe, feList){
            if (abs(fe.amount) > CurrencyHelper::maxValueAllowedForAmount() ){ // should not happen
                resultStringList.append(QString("%1 : %2").arg(fe.occurrence.toString(Qt::ISODate))
                    .arg(tr("Amount is bigger than the maximum allowed")));
            } else {
                double amountDouble = CurrencyHelper::amountQint64ToDouble(abs(fe.amount),
                    currInfo.noOfDecimal, ok);
                if (ok != 0){
                    // should not happen
                    resultStringList.append(QString("%1 : %2").arg(fe.occurrence.toString(
                        Qt::ISODate)).arg(tr("Error during amount conversion")));
                } else {
                    amountString = CurrencyHelper::formatAmount(amountDouble, currInfo, locale,
                        true);
                    cummul += amountDouble;
                    cummulString = CurrencyHelper::formatAmount(static_cast<double>(cummul),
                        currInfo, locale, true);
                    s = tr("%1 : %2 (cummul=%3)").arg(fe.occurrence.toString(Qt::ISODate)).
                        arg(amountString).arg(cummulString);
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

// Using the generated FeList, update the chart data with double representation in proper currency
void VisualizeOccurrencesDialog::updateChartTab(QList<Fe> feList, uint saturationCount,
    Growth scenarioInflation,  FeStreamDef *streamDef, FeMinMaxInfo minMax)
{
    // remove existing data
    chart->removeAllSeries();

    // add new data
    int convResult;
    double amount;
    series = new QLineSeries();
    QDateTime momentInTime;
    foreach(Fe fe, feList){
        momentInTime.setDate(fe.occurrence);
        amount = CurrencyHelper::amountQint64ToDouble(abs(fe.amount), currInfo.noOfDecimal,
            convResult);
        if (convResult==0) {
            series->append(momentInTime.toMSecsSinceEpoch(), amount);
        }
    }

    // show symbols
    series->setPointsVisible(ui->showPointsCheckBox->isChecked());
    series->setMarkerSize(5);

    // find min max for X axis (date)
    QDateTime xMin, xMax;
    if (feList.size()==0) {
        xMin = QDateTime(QDate::currentDate(),QTime(0,0));
        xMax = xMin.addDays(1);
    } else if (feList.size()==1){
        xMin = QDateTime(feList[0].occurrence.addDays(-1),QTime(0,0));
        xMax = QDateTime(feList[0].occurrence.addDays(1),QTime(0,0));
    } else {
        xMin = QDateTime(feList[0].occurrence,QTime(0,0));
        xMax = QDateTime(feList[feList.size()-1].occurrence,QTime(0,0));
    }

    // min/max for Y axis, in currency format
    double doubleYmin;
    double doubleYmax;
    if (feList.size()==0) {
        doubleYmin = 0;
        doubleYmax = 1;
    } else if (feList.size()==1){
        // 10% above and below the value
        if (feList[0].amount==0) {
            doubleYmin = 0;
            doubleYmax = 1;
        } else {
            double val = CurrencyHelper::amountQint64ToDouble(abs(feList[0].amount), currInfo.noOfDecimal, convResult);;
            doubleYmin = val*0.75;
            doubleYmax = val*1.25;
        }
    } else {
        // 2 or more
        double tempMin = CurrencyHelper::amountQint64ToDouble(abs(minMax.yMin), currInfo.noOfDecimal, convResult);
        double tempMax = CurrencyHelper::amountQint64ToDouble(abs(minMax.yMax), currInfo.noOfDecimal, convResult);
        double delta = tempMax-tempMin;
        if (delta==0) {
            if (tempMin==0) {
                doubleYmin = 0;
                doubleYmax = 1;
            } else {
                doubleYmin = fmax(0,0.95*tempMin);
                doubleYmax = fmax(0,1.05*tempMax);
            }
        } else {
            doubleYmin = fmax(0,tempMin - 0.05*delta);
            doubleYmax = fmax(0,tempMax + 0.05*delta);
        }
    }

    // currency may be different
    QString yAxisFormat = QString("\%#.%1f").arg(currInfo.noOfDecimal);
    axisY->setLabelFormat(yAxisFormat);

    chart->addSeries(series);

    series->attachAxis(axisX);
    series->attachAxis(axisY);

    // set ranges for axes
    axisX->setRange(xMin,xMax);
    axisY->setRange(doubleYmin, doubleYmax);

    // set background color
    if (GbpController::getInstance().getChartDarkMode()==true) {
        // dark mode enabled
        chart->setTheme(QChart::ChartThemeDark);
        series->setColor(GbpController::getInstance().getCurveDarkModeColor());
    } else {
        // light mode enabled
        chart->setTheme(QChart::ChartThemeLight);
        series->setColor(GbpController::getInstance().getCurveLightModeColor());
    }

}

void VisualizeOccurrencesDialog::initChart()
{
    series = new QLineSeries();

    QDateTime momentInTime;

    // dummy data for debugging
    momentInTime.setDate(QDate(2001,1,1));
    series->append(momentInTime.toMSecsSinceEpoch(), 35);
    momentInTime.setDate(QDate(2001,1,2));
    series->append(momentInTime.toMSecsSinceEpoch(), 2);

    chart = new QChart();
    chart->addSeries(series);
    chart->legend()->hide();
    chart->layout()->setContentsMargins(1, 1, 1, 1);
    chart->setBackgroundRoundness(0);

    axisX = new QDateTimeAxis;
    axisX->setTickCount(6);
    axisX->setFormat("dd MMM yy");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    axisY = new QValueAxis;
    axisY->setLabelFormat("%f");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chartView = new QChartView(chart, ui->widget);  // this is where we tie the chart to the parent widget
    chartView->setRenderHint(QPainter::Antialiasing);
}



// for chart resizing
bool VisualizeOccurrencesDialog::eventFilter(QObject *object, QEvent *event)
{
    if ( (event->type() == QEvent::Resize) && (object == ui->widget) ){
        chartView->resize(ui->widget->size());
    }
    return QObject::eventFilter(object, event);
}

void VisualizeOccurrencesDialog::on_showPointsCheckBox_stateChanged(int arg1)
{
    series->setPointsVisible(ui->showPointsCheckBox->isChecked());
}

