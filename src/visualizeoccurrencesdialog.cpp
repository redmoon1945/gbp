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
#include "customqchartview.h"
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
    this->locale = locale;
    initChart();
}


VisualizeOccurrencesDialog::~VisualizeOccurrencesDialog()
{
    delete ui;
}


// scenarioInflation is used only when streamDef is a Periodic stream def
void VisualizeOccurrencesDialog::slotPrepareContent(CurrencyInfo currInfo, Growth scenarioInflation,
    QDate maxDateScenario, FeStreamDef *streamDef)
{
    this->currInfo = currInfo;
    uint noOfSaturations;
    FeMinMaxInfo minMax;
    maxDateScenarioFeGeneration = maxDateScenario;
    indexLastPointSelected = -1;
    QList<Fe> feList = generateFinancialEvents(scenarioInflation, streamDef, noOfSaturations,
        minMax);

    // build the search vector to accelerate search when a point is clicked
    searchVector.resize(feList.count());
    QTime zero = QTime(0,0,0);
    for(int i=0;i<searchVector.size();i++){
        searchVector[i] = QDateTime(feList.at(i).occurrence,zero).toMSecsSinceEpoch();
    }

    // 50% of the space for each widget
    ui->splitter->setSizes(QList<int>({INT_MAX, INT_MAX}));

    updateTextTab(feList, noOfSaturations, scenarioInflation, streamDef);
    updateChartTab(feList, noOfSaturations, scenarioInflation, streamDef, minMax);

    feList.clear(); // get rid of it immediately
}


// Need some improvement toi optimize speed
void VisualizeOccurrencesDialog::mypoint_clicked(const QPointF pt)
{
    // find the index of the point in the series
    QList<QPointF> ptList = series->points();

    QDateTime dt = QDateTime::fromMSecsSinceEpoch(pt.x());
    if (dt.isValid()==false){
        return;
    }

    int index = ptList.indexOf(pt);
    if(index==-1){
        // should not happen
        return;
    }

    // // this does not appears to save time in a noticeable way
    // int index = binarySearch(searchVector,pt.x());
    // if(index==-1){
    //     qInfo()<<"Index not found for x="<<dt;
    //     return;
    // }


    // This section is quite slow for high number of points. Tried several things,
    // but nothing worked.

    // set selected points to normal color, then unselect
    // {
    // const QSignalBlocker blocker(series);
    if (indexLastPointSelected != -1){
        series->setPointSelected(indexLastPointSelected,false);
    }
    series->setPointSelected(index,true);
    indexLastPointSelected = index;
    // }

    // display
    QDate date = dt.date();
    QString s = tr("Selected Point :  Date=%1  Amount=%2").
        arg(locale.toString(date, locale.dateFormat(QLocale::ShortFormat))).
        arg(locale.toString(pt.y(),'f',currInfo.noOfDecimal));
    ui->selectedPointLabel->setText(s);

}


void VisualizeOccurrencesDialog::on_closePushButton_clicked()
{
    // clean some objects we dont need
    ui->plainTextEdit->clear(); // dont hold the text, no use for that now
    chart->removeAllSeries();

    this->hide();
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
    // build for the maximum range set by scenario, but if the Periodic Stream Def set its own
    // limit date arealier, the latter will take precedence
    DateRange fromto = DateRange(GbpController::getInstance().getTomorrow(),
        maxDateScenarioFeGeneration);
    if(streamDef->getStreamType()==FeStreamDef::PERIODIC){
        PeriodicFeStreamDef* psd = (PeriodicFeStreamDef *)streamDef;
        result = psd->generateEventStream(fromto, maxDateScenarioFeGeneration, scenarioInflation,
            (usePvConversion)?(pvAnnualDiscountRate):(0),
            GbpController::getInstance().getTomorrow(), saturationCount, minMax);

    } else {
        IrregularFeStreamDef* isd = (IrregularFeStreamDef *)streamDef;
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
        resultStringList.append(tr("No event will be generated before tomorrow %1 and past %2.")
            .arg(locale.toString(tomorrow, locale.dateFormat(QLocale::ShortFormat)))
            .arg(locale.toString(psd->getRealEndDate(maxDateScenarioFeGeneration),
            locale.dateFormat(QLocale::ShortFormat))));
        resultStringList.append(tr("%1 %2 event(s) have been generated.\n").arg(feList.count()).arg(
            (psd->getIsIncome())?(tr("income")):(tr("expense"))));

        // transform events values into text log
        long double cummul = 0;
        foreach(Fe fe, feList){
            if (abs(fe.amount) > CurrencyHelper::maxValueAllowedForAmount() ){ // should not happen
                resultStringList.append(QString("%1 : %2").
                    arg(locale.toString(fe.occurrence, locale.dateFormat(QLocale::ShortFormat))).
                    arg(tr("Amount is bigger than the maximum allowed")));
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
                    QString s = tr("%1 : %2 (cummul=%3)").
                        arg(locale.toString(fe.occurrence,
                            locale.dateFormat(QLocale::ShortFormat))).
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
        resultStringList.append(tr("No event will be generated before tomorrow %1 and past %2")
            .arg(locale.toString(tomorrow, locale.dateFormat(QLocale::ShortFormat)))
            .arg(locale.toString(maxDateScenarioFeGeneration, locale.dateFormat(
            QLocale::ShortFormat))));
        resultStringList.append(tr("%1 %2 event(s) have been generated.\n").arg(feList.count()).
            arg((isd->getIsIncome())?(tr("income")):(tr("expense"))));

        // transform into text log
        long double cummul = 0;
        QString amountString,cummulString,s;
        foreach(Fe fe, feList){
            if (abs(fe.amount) > CurrencyHelper::maxValueAllowedForAmount() ){ // should not happen
                resultStringList.append(QString("%1 : %2").
                    arg(locale.toString(fe.occurrence, locale.dateFormat(QLocale::ShortFormat))).
                    arg(tr("Amount is bigger than the maximum allowed")));
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
                    s = tr("%1 : %2 (cummul=%3)").
                        arg(locale.toString(fe.occurrence, locale.dateFormat(QLocale::ShortFormat))).
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

// Using the generated FeList, update the chart data with representation in proper currency
void VisualizeOccurrencesDialog::updateChartTab(QList<Fe> feList, uint saturationCount,
    Growth scenarioInflation,  FeStreamDef *streamDef, FeMinMaxInfo minMax)
{
    // regenerate Data
    QList<QPointF> timeData;
    // add new data
    int convResult;
    double amount;
    QDateTime momentInTime;
    foreach(Fe fe, feList){
        momentInTime.setDate(fe.occurrence);
        amount = CurrencyHelper::amountQint64ToDouble(abs(fe.amount), currInfo.noOfDecimal,
            convResult);
        if (convResult==0) {
            timeData.append({static_cast<qreal>(momentInTime.toMSecsSinceEpoch()), amount});
        }
    }

    replaceChartSeries(timeData);
    rescaleChart();
    changeYaxisLabelFormat();

    ui->selectedPointLabel->setText(tr("Selected Point :"));
}

void VisualizeOccurrencesDialog::initChart()
{
    // Step 1 : Create the chart
    chart = new QChart();
    chart->legend()->hide();
    chart->setTitle(tr("Financial Events"));
    //chart->layout()->setContentsMargins(1, 1, 1, 1);
    //chart->setBackgroundRoundness(0);
    chart->setLocale(locale);
    chart->setLocalizeNumbers(true);    series = new QScatterSeries();

    // Step 2 : create X axis
    axisX = new QDateTimeAxis;
    axisX->setTickCount(6);
    axisX->setFormat(locale.dateFormat(QLocale::ShortFormat));
    axisX->setRange(QDateTime(QDate(2000,1,1),QTime(0,0,0)),
        QDateTime(QDate(2001,1,1),QTime(0,0,0)));
    chart->addAxis(axisX, Qt::AlignBottom);

    // Step 3 : create Y axis
    axisY = new QValueAxis;
    axisY->setTickCount(6);
    axisY->setRange(0,1);
    chart->addAxis(axisY, Qt::AlignLeft);

    // Step 4 : create empty series and set characteristics
    QList<QPointF> timeData = {};
    replaceChartSeries(timeData);

    // reduce font size for axis
    reduceAxisFontSize();

    // Step 9
    chartView = new CustomQChartView(chart,
        GbpController::getInstance().getWheelRotatedAwayZoomIn(), ui->widget);
    chartView->setRenderHint(QPainter::Antialiasing, true);

    //axisX->setRange(first,second);
    // Weird...looks like we have to "reserve" space for the very first invocation of this dialog.
    // There is something I dont understand here
    //axisY->setRange(0, CurrencyHelper::maxValueAllowedForAmountInDouble(3));

    // remove focus on Fit button
    ui->closePushButton->setFocus();

    // configure dark or light mode for chart. reduceAxisFontSize() must have been called once
    // before
    themeChanged();
}


// for chart resizing
bool VisualizeOccurrencesDialog::eventFilter(QObject *object, QEvent *event)
{
    if ( (event->type() == QEvent::Resize) && (object == ui->widget) ){
        chartView->resize(ui->widget->size());
    }
    return QObject::eventFilter(object, event);
}


// Set X axis range according to max DATA range
void VisualizeOccurrencesDialog::on_fitPushButton_clicked()
{
    rescaleChart();
}


void VisualizeOccurrencesDialog::reduceAxisFontSize()
{
    // X axis
    QFont xAxisFont = axisX->labelsFont();
    xAxisFontSize = Util::changeFontSize(2, true, xAxisFont.pointSize()); // set for ever
    setXaxisFontSize(xAxisFontSize);

    //  Y axis
    QFont yAxisFont = axisY->labelsFont();
    yAxisFontSize = Util::changeFontSize(2, true, yAxisFont.pointSize()); // set for ever
    setYaxisFontSize(yAxisFontSize);
}


void VisualizeOccurrencesDialog::setXaxisFontSize(uint fontSize){
    QFont xAxisFont = axisX->labelsFont();
    xAxisFont.setPointSize(fontSize);
    axisX->setLabelsFont(xAxisFont);
}


void VisualizeOccurrencesDialog::setYaxisFontSize(uint fontSize){
    QFont yAxisFont = axisY->labelsFont();
    yAxisFont.setPointSize(fontSize);
    axisY->setLabelsFont(yAxisFont);
}


int VisualizeOccurrencesDialog::binarySearch(const std::vector<double>& vec, double target) {
    int low = 0;
    int high = vec.size() - 1;

    while (low <= high) {
        int mid = (low + high) / 2;
        if (vec[mid] == target) {
            return mid; // found the target
        } else if (vec[mid] < target) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return -1; // not found
}


void VisualizeOccurrencesDialog::themeChanged()
{
    if(GbpController::getInstance().getIsDarkModeSet()==true){
        chart->setTheme(QChart::ChartThemeDark);
        chart->setBackgroundBrush(QBrush(QColor("black")));
    } else {
        chart->setTheme(QChart::ChartThemeLight);
        chart->setBackgroundBrush(QBrush(QColor("white")));
    }
    setSeriesCharacteristics();
    // Changing theme "sometimes" change font size (???). Set them again to be sure
    // it stays constant
    setXaxisFontSize(xAxisFontSize);
    setYaxisFontSize(yAxisFontSize);
}

void VisualizeOccurrencesDialog::setSeriesCharacteristics(){
    if(GbpController::getInstance().getIsDarkModeSet()==true){
        // point color
        series->setBrush(GbpController::getInstance().getDarkModePointColor());
        // selected point color
        series->setSelectedColor(GbpController::getInstance().
                                        getDarkModeSelectedPointColor());
    } else {
        // point color
        series->setBrush(GbpController::getInstance().getLightModePointColor());
        // selected point color
        series->setSelectedColor(GbpController::getInstance().
                                        getLightModeSelectedPointColor());
    }
    series->setBorderColor(Qt::transparent);    // no border on points
    series->setMarkerSize(GbpController::getInstance().getChartPointSize());
}


void VisualizeOccurrencesDialog::replaceChartSeries(QList<QPointF> data)
{
    // first destroy the current series and all the data they have
    chart->removeAllSeries();

    // rebuild
    series = new QScatterSeries(); // only true data, for markers only, superimposed

    // set colors and characteristics for the series
    setSeriesCharacteristics();

    // intercept point selection
    connect(series, SIGNAL(clicked(QPointF)), this, SLOT(mypoint_clicked(QPointF)));

    // fill series with data
    series->append(data); // take ownership

    // attach to chart
    chart->addSeries(series);  // chart takes ownership

    // re-attach axis
    series->attachAxis(axisX);
    series->attachAxis(axisY);

    // no point have been selected yet
    indexLastPointSelected = -1;
}


// Always rescaled to "fit"
void VisualizeOccurrencesDialog::rescaleChart()
{
    // Get chart raw data
    QList<QPointF> timeData = series->points();

    // Get current currency
    bool found;
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario();
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromCountryCode(locale,
        scenario->getCountryCode(), found);
    if(!found){
        return; // should never happen
    }

    // *** Rescale X axis. Data can be empty. ***
    QDateTime xFrom;
    QDateTime xTo;    // required also for Y axis re-scaling
    if (timeData.size()==0) {
        // no data : have a X axis scale of 1 year (arbitrary)
        xFrom = QDateTime(GbpController::getInstance().getTomorrow(),QTime(0,0,0));
        xTo = xFrom.addYears(1).addDays(-1);
    } else if (timeData.size()==1){
        // Surround the unique point by a =/- 1 day, so it is centered
        xFrom = QDateTime::fromMSecsSinceEpoch(timeData.first().x()).addDays(-1);
        xTo = QDateTime::fromMSecsSinceEpoch(timeData.last().x()).addDays(1);
    } else {
        xFrom = QDateTime::fromMSecsSinceEpoch(timeData.first().x());
        xTo = QDateTime::fromMSecsSinceEpoch(timeData.last().x());
    }
    // Add margin around xMin/xMax and set range
    QDateTime displayXfrom = xFrom;
    QDateTime displayXto = xTo;
    Util::calculateZoomXaxis(displayXfrom, displayXto, GbpController::getInstance().getPercentageMainChartScaling()/100.0);
    axisX->setRange(displayXfrom, displayXto);

    // *** Rescale Y axis ***
    double yFrom ;
    double yTo ;
    if (timeData.size()==0){
        // no data
        yFrom = 0;
        yTo = 1;
    } else {
        bool result = Util::findMinMaxInYvalues(timeData, xFrom.toMSecsSinceEpoch(), xTo.toMSecsSinceEpoch(),
            yFrom, yTo);
        // if no data is in the interval [xFrom-xTo], set arbitrary limits
        if(result==false){
            yFrom = 0;
            yTo = 1;
        }
    }
    // Add margin around yMin/yMax
    double displayYfrom = yFrom;
    double displayYto = yTo;
    Util::calculateZoomYaxis(displayYfrom, displayYto, GbpController::getInstance().getPercentageMainChartScaling()/100.0);
    axisY->setRange(displayYfrom, displayYto);
}




void VisualizeOccurrencesDialog::changeYaxisLabelFormat()
{
    QString countryCode = GbpController::getInstance().getScenario()->getCountryCode();
    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromCountryCode(locale, countryCode,
        found);
    if(!found){
        return;// should never happen
    }
    QString yValFormat = QString("\%.%1f").arg(currInfo.noOfDecimal);
    axisY->setLabelFormat(yValFormat);
}



