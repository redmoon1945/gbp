/*
 *  Copyright (C) 2024-2025 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
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
#include "gbplogger.h"
#include <qdatetimeaxis.h>
#include <qgraphicslayout.h>
#include <qvalueaxis.h>
#include <QMessageBox>
#include <QFileDialog>


VisualizeOccurrencesDialog::VisualizeOccurrencesDialog(QLocale locale, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::VisualizeOccurrencesDialog)
{
    ui->setupUi(this);
    ui->widget->installEventFilter(this);   // pass resize event received by "widget" to the Chart widget
    this->locale = locale;
    initChart();
    // 50% of the space for each widget
    ui->splitter->setSizes(QList<int>({INT_MAX, INT_MAX}));
}


VisualizeOccurrencesDialog::~VisualizeOccurrencesDialog()
{
    delete ui;
}


void VisualizeOccurrencesDialog::slotPrepareContent(CurrencyInfo currInfo, Growth scenarioInflation,
    QDate maxDateScenario, QWeakPointer<Csd> csd)
{
    this->currInfo = currInfo;
    uint noOfSaturations;
    FeMinMaxInfo minMax;
    maxDateScenarioFeGeneration = maxDateScenario;
    indexLastPointSelected = -1;

    // Generate the flow of Fe.
    FeStream feStream = generateFinancialEvents(scenarioInflation, csd, noOfSaturations, minMax);

    updateTextTab(feStream, noOfSaturations, scenarioInflation, csd);
    updateChartTab(feStream, noOfSaturations, scenarioInflation,minMax);

    // Set focus on Close button
    ui->closePushButton->setFocus();
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
    QString s = tr("Selected point :  Date=%1  Amount=%2").
        arg(locale.toString(date, locale.dateFormat(QLocale::ShortFormat)) ,
        locale.toString(pt.y(),'f',currInfo.noOfDecimal));
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


FeStream VisualizeOccurrencesDialog::generateFinancialEvents(Growth scenarioInflation,
    QWeakPointer<Csd> weakCsdPtr, uint& saturationCount, FeMinMaxInfo& minMax){

    QDate tomorrow = GbpController::getInstance().getTomorrow();
    DateRange fromto = DateRange(tomorrow, maxDateScenarioFeGeneration);
    int maxNoOfdays = fromto.getNoOfDays();
    FeStream result(maxNoOfdays, weakCsdPtr);

    // info on PV conversion
    bool usePvConversion = GbpController::getInstance().getUsePresentValue();
    double pvAnnualDiscountRate = GbpController::getInstance().getPvDiscountRate();

    // Generate financial events
    // build for the maximum range set by scenario, but if the Periodic Csd set its own
    // limit date arealier, the latter will take precedence
    if (QSharedPointer<PeriodicCsd> pPtr =
        qSharedPointerDynamicCast<PeriodicCsd>(weakCsdPtr)) {
        // This is Periodic Csd
        pPtr->generateEventStream(result, tomorrow, fromto, maxDateScenarioFeGeneration,
            scenarioInflation,(usePvConversion)?(pvAnnualDiscountRate):(0), tomorrow,
            saturationCount, minMax);
    } else if (QSharedPointer<IrregularCsd> iPtr =
        qSharedPointerDynamicCast<IrregularCsd>(weakCsdPtr) ){
        // This is Irregular Csd
        iPtr->generateEventStream(result, tomorrow, fromto, maxDateScenarioFeGeneration,
            (usePvConversion)?(pvAnnualDiscountRate):(0), tomorrow, saturationCount, minMax);
    } else {
        return result; // should never happen
    }

    return result;
}



void VisualizeOccurrencesDialog::updateTextTab(FeStream& feStream, uint saturationCount, Growth
    scenarioInflation, QWeakPointer<Csd> weakCsdPtr)
{

    QString amountString;
    QDate tomorrow = GbpController::getInstance().getTomorrow();
    QStringList resultStringList;
    int ok;

    // First, make a strong pointer
    QSharedPointer<Csd> strongCsdPtr = weakCsdPtr.toStrongRef();
    if (strongCsdPtr.isNull()) {
        return; // should not happen
    }

    // Determine if csd is a Periodic or an Irregular Csd. One of this pointer will be null.
    QSharedPointer<PeriodicCsd> psd = qSharedPointerDynamicCast<PeriodicCsd>(strongCsdPtr);
    QSharedPointer<IrregularCsd> isd = qSharedPointerDynamicCast<IrregularCsd>(strongCsdPtr);
    if ( (psd.isNull()) && (isd.isNull()) ) {
        return; // should not happen
    }

    if (psd.isNull() == false) {

        // Periodic

        // Get next event data from start
        QDate nextEventDate = psd->getNextEventDate(psd->getStartDate());

        // Special additionnal header for Periodic
        if (psd->getGrowthStrategy()==PeriodicCsd::GrowthStrategy::INFLATION){
            // adjust inflation if required
            Growth adjustedInflation = scenarioInflation;
            bool capped;
            adjustedInflation.changeByFactor(psd->getInflationAdjustmentFactor(),capped);


            if (Growth::Type::CONSTANT == scenarioInflation.getType()) {
                QString infString = QString("%1%").arg(static_cast<double>(
                    Growth::fromDecimalToDouble(adjustedInflation.getAnnualConstantGrowth())));
                resultStringList.append( tr("Using constant adjusted annual inflation"
                    " of %1 percent.").arg(infString)) ;
                resultStringList.append( tr("Inflation can be applied from %1.").
                    arg(locale.toString(nextEventDate, locale.dateFormat(QLocale::ShortFormat))) );
            } else if (Growth::Type::VARIABLE == scenarioInflation.getType()) {
                resultStringList.append(tr("Using variable inflation."));
            } else{
                // should not happen...do nothing
            }
        } else if (psd->getGrowthStrategy()==PeriodicCsd::GrowthStrategy::CUSTOM) {
            if (psd->getGrowth().getType()==Growth::Type::CONSTANT) {
                resultStringList.append( tr("Using custom constant growth of %1 percent.")
                    .arg( static_cast<double>(Growth::fromDecimalToDouble(
                        psd->getGrowth().getAnnualConstantGrowth()))));
                resultStringList.append( tr("Growth can be applied from %1")
                    .arg(locale.toString(nextEventDate, locale.dateFormat(QLocale::ShortFormat))));
            } else if (psd->getGrowth().getType()==Growth::Type::VARIABLE){
                resultStringList.append(tr("Using custom variable growth."));
            } else {
                // should not happen...do nothing
            }
        } else {
            resultStringList.append(tr("No growth of any kind is applied."));
        }

        // Standard Header
        bool usePvConversion = GbpController::getInstance().getUsePresentValue();
        double pvAnnualDiscountRate = GbpController::getInstance().getPvDiscountRate();
        if ((usePvConversion==true)&&(pvAnnualDiscountRate!=0)) {
            QString s = tr("Converting Future Values to Present Values using an annual discount "
                " rate of %1 percent.").arg(pvAnnualDiscountRate);
            resultStringList.append(s);
        }
        if(saturationCount > 0){
            resultStringList.append(tr("Amount was too big %1 times and have been capped to %2.")
                .arg(saturationCount)
                .arg(CurrencyHelper::formatAmount(CurrencyHelper::maxValueAllowedForAmountInDouble(
                    currInfo.noOfDecimal), currInfo, locale, true)));
        }
        resultStringList.append(tr("No financial event will be generated before tomorrow %1 and past %2.")
            .arg(locale.toString(tomorrow, locale.dateFormat(QLocale::ShortFormat)) ,
            locale.toString(psd->getRealEndDate(maxDateScenarioFeGeneration),
                locale.dateFormat(QLocale::ShortFormat))));
        resultStringList.append(tr("%1 %2 event(s) have been generated.\n")
            .arg(feStream.getNoOfElementsUsed())
            .arg((psd->getIsIncome())?(tr("income")):(tr("expense"))));

    } else {
        // Irregular

        // Standard Header
        bool usePvConversion = GbpController::getInstance().getUsePresentValue();
        double pvAnnualDiscountRate = GbpController::getInstance().getPvDiscountRate();
        if ((usePvConversion==true)&&(pvAnnualDiscountRate!=0)) {
            QString s = tr("Converting Future Values to Present Values using an annual discount "
                " rate of %1 percent.").arg(pvAnnualDiscountRate);
            resultStringList.append(s);
        }
        if(saturationCount > 0){
            resultStringList.append(tr("Amount was too big %1 times and have been capped to %2.")
                .arg(saturationCount)
                .arg(CurrencyHelper::maxValueAllowedForAmountInDouble(currInfo.noOfDecimal)));
        }
        resultStringList.append(
            tr("No financial event will be generated before tomorrow %1 and past %2.").arg(
                locale.toString(tomorrow, locale.dateFormat(QLocale::ShortFormat)) ,
                locale.toString(maxDateScenarioFeGeneration,locale.dateFormat(QLocale::ShortFormat))
            )
        );
        resultStringList.append(
            tr("%1 %2 event(s) have been generated.\n")
                .arg(feStream.getNoOfElementsUsed())
                .arg((isd->getIsIncome())?(tr("income")):(tr("expense")))
            );
    }


    // Print values into text widget
    long double cummul = 0;
    const QList<qint64> list = feStream.getAmountSet();
    const qsizetype size = feStream.getNoOfDays();
    for (int var = 0; var < size; ++var) {
        if (list[var] == -1) {
            continue; // unused
        }
        QDate date = tomorrow.addDays(var);
        if (abs(list[var]) > CurrencyHelper::maxValueAllowedForAmount() ){ // should not happen
            resultStringList.append(QString("%1 : %2").
                arg(locale.toString(date, locale.dateFormat(QLocale::ShortFormat)) ,
                tr("Amount is bigger than the maximum allowed")));
        } else {
            double amountDouble = CurrencyHelper::amountQint64ToDouble(list[var],
                currInfo.noOfDecimal, ok);
            if (ok != 0){
                resultStringList.append(QString("%1 : %2")
                    .arg(date.toString(Qt::ISODate) , tr("Error during amount conversion")));
            } else {
                amountString = CurrencyHelper::formatAmount(amountDouble, currInfo, locale, true);
                cummul += amountDouble;
                QString cummulString;
                if (cummul > CurrencyHelper::maxValueAllowedForAmountInDouble(currInfo.noOfDecimal)) {
                    cummulString = CurrencyHelper::formatAmount(
                        CurrencyHelper::maxValueAllowedForAmountInDouble(currInfo.noOfDecimal),
                        currInfo, locale, true);
                } else {
                    cummulString = CurrencyHelper::formatAmount(static_cast<double>(cummul),
                        currInfo, locale, true);
                }
                QString s;
                if (date<tomorrow){
                    // if event is in the past, mention it
                    s = s.append(tr("  *** PAST -> discarded ***"));
                } else {
                    s = tr("%1 : %2 (cummul=%3)").arg(locale.toString(date,locale.dateFormat(
                        QLocale::ShortFormat)) , amountString , cummulString);
                }
                resultStringList.append(s);
            }
        }
    }

    // Update the PlainText content
    QString r = resultStringList.join("\n");
    ui->plainTextEdit->setPlainText(r);
}


void VisualizeOccurrencesDialog::updateChartTab(FeStream& feStream, uint saturationCount,
    Growth scenarioInflation, FeMinMaxInfo minMax)
{
    QDate tomorrow = GbpController::getInstance().getTomorrow();

    // regenerate Data
    QList<QPointF> timeData;
    // add new data
    int convResult;
    double amount;
    QDateTime momentInTime;
    QList<qint64> feList = feStream.getAmountSet();
    const qsizetype size = feList.count();
    for (int var = 0; var < size; ++var) {
        if (feList[var]==-1) {
            continue;
        }
        momentInTime.setDate(tomorrow.addDays(var));
        amount = CurrencyHelper::amountQint64ToDouble(feList[var], currInfo.noOfDecimal,
            convResult);
        if (convResult==0) {
            timeData.append({static_cast<qreal>(momentInTime.toMSecsSinceEpoch()), amount});
        }
    }

    replaceChartSeries(timeData);
    rescaleChart();
    changeYaxisLabelFormat();

    ui->selectedPointLabel->setText(tr("Selected point :"));
}

void VisualizeOccurrencesDialog::initChart()
{
    // Step 1 : Create the chart
    chart = new QChart();
    chart->legend()->hide();
    chart->setTitle(tr("Financial events"));
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
    Util::changeFontSize(xAxisFont, Util::FontResizeIntensity::AVERAGE, true);
    setXaxisFontSize(xAxisFont.pointSize());

    //  Y axis
    QFont yAxisFont = axisY->labelsFont();
    Util::changeFontSize(yAxisFont, Util::FontResizeIntensity::AVERAGE, true);
    setYaxisFontSize(yAxisFont.pointSize());
}


void VisualizeOccurrencesDialog::setXaxisFontSize(uint fontSize){
    if (fontSize==0) {
        std::invalid_argument("Invalid font size of 0");
    } else if (fontSize>1000){
        std::invalid_argument("Invalid font size greater than 1000");
    }
    xAxisFontSize = fontSize;
    QFont xAxisFont = axisX->labelsFont();
    xAxisFont.setPointSize(xAxisFontSize);
    axisX->setLabelsFont(xAxisFont);
}


void VisualizeOccurrencesDialog::setYaxisFontSize(uint fontSize){
    if (fontSize==0) {
        std::invalid_argument("Invalid font size of 0");
    } else if (fontSize>1000){
        std::invalid_argument("Invalid font size greater than 1000");
    }
    yAxisFontSize = fontSize;
    QFont yAxisFont = axisY->labelsFont();
    yAxisFont.setPointSize(yAxisFontSize);
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
    if(GbpController::getInstance().useDarkModeForChart()==true){
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
    if(GbpController::getInstance().useDarkModeForChart()==true){
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
    QString yValFormat = QString("\%.%1f").arg(currInfo.noOfDecimal);
    axisY->setLabelFormat(yValFormat);
}


void VisualizeOccurrencesDialog::on_exportPushButton_clicked()
{
    // *** get a file name ***
    QString defaultExtensionUsed = "CSV files (*.csv *.CSV)";
    QString filter = tr("CSV files (*.csv *.CSV);;Text files (*.txt *.TXT);;All files (*)");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Select a file"),
        GbpController::getInstance().getLastDirExport(), filter, &defaultExtensionUsed);
    if (fileName == ""){
        return;
    }
    // *** fix the filename to add the proper suffix ***
    QFileInfo fi(fileName);
    if(fi.suffix()==""){    // user has not specified an extension
        fileName.append(".csv");
    }
    GbpController::getInstance().setLastDirExport(fi.absolutePath());
    LOG_INFO(QString("Attempting to export occurences to text file \"%1\" ...")
        .arg(REDACT(fileName)));

    QFile file(fileName);
    if (false == file.open(QFile::WriteOnly | QFile::Truncate)){
        QMessageBox::critical(nullptr,tr("Error"),tr("Cannot open the file for writing"));
        LOG_ERROR("Export failed : Cannot open the file for saving");
        return;
    }

    // *** export to the file ***

    // write header
    QString s = QString("%1\t%2\n").arg(tr("Date"),tr("Amount"));
    file.write(s.toUtf8());

    // set date format
    QString dateFormat = "yyyy-MM-dd"; // ISO
    if (GbpController::getInstance().getExportTextDateLocalized()==true) {
        dateFormat = locale.dateFormat(QLocale::ShortFormat);
    }

    // write data
    QString valueString;
    QString dateString;
    QDate date;
    QList<QPointF> timeData = series->points();
    foreach(QPointF pt, timeData){
        date = (QDateTime::fromMSecsSinceEpoch(pt.x())).date();
        double value = pt.y();
        dateString = locale.toString(date, dateFormat);
        if (GbpController::getInstance().getExportTextAmountLocalized()) {
            // Localized
            valueString = CurrencyHelper::formatAmount(value, currInfo, locale, false);
        } else {
            // not localized
            valueString = QString::number(value,'f', currInfo.noOfDecimal);
        }

        s = QString("%1\t%2\n").arg(dateString,valueString);
        file.write(s.toUtf8());
    }
    file.close();
    LOG_INFO("Exporting occurences succeeded");

}

