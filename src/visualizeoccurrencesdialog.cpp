/*
 *  Copyright (C) 2024-2026 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
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
#include <QTimer>
#include "csvexporter.h"
#include "customqchartview.h"
#include "ui_visualizeoccurrencesdialog.h"
#include "gbpcontroller.h"
#include "gbplogger.h"
#include "uiutil.h"
#include <qdatetimeaxis.h>
#include <qgraphicslayout.h>
#include <qvalueaxis.h>
#include <QMessageBox>
#include <QFileDialog>
#include "gbpqmessage.h"


VisualizeOccurrencesDialog::VisualizeOccurrencesDialog(QLocale locale, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::VisualizeOccurrencesDialog)
{
    ui->setupUi(this);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);

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


void VisualizeOccurrencesDialog::slotPrepareContent(CurrencyInfo currInfo, QSharedPointer<FeStream>
    feStream, uint noOfSaturations, Growth scenarioInflation, FeMinMaxInfo minMax,
    QDate maxDateScenario)
{
    this->currInfo = currInfo;
    maxDateScenarioFeGeneration = maxDateScenario;
    indexLastPointSelected = -1;

    // Make sure the FeStream is valid
    if (feStream.isNull()==true) {
        return; // should never happen
    }

    updateTextTab(feStream, noOfSaturations, scenarioInflation);
    updateChartTab(feStream, noOfSaturations, scenarioInflation, minMax);

    // Set focus on Close button
    ui->closePushButton->setFocus();
}


// Need some improvement to optimize speed
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
        arg(locale.toString(date, "yyyy-MMM-dd") ,
        locale.toString(pt.y(),'f',currInfo.noOfDecimal));
    ui->selectedPointLabel->setText(s);

}


void VisualizeOccurrencesDialog::on_closePushButton_clicked()
{
    // clean some objects we dont need (they will be regenerated before the form is re-shown)
    ui->plainTextEdit->clear(); // dont hold the text, no use for that now
    chart->removeAllSeries();

    this->hide();
    emit signalCompleted();
}


void VisualizeOccurrencesDialog::on_VisualizeOccurrencesDialog_rejected()
{
    on_closePushButton_clicked();
}


void VisualizeOccurrencesDialog::updateTextTab(QSharedPointer<FeStream> feStream,
    uint saturationCount, Growth scenarioInflation)
{

    QString amountString;
    QDate tomorrow = GbpController::getInstance().getTomorrow();
    QStringList resultStringList;
    int ok;

    // First, make a strong pointer to the underlying Csd
    QSharedPointer<Csd> strongCsdPtr = feStream->getCsdPtr().toStrongRef();
    if (strongCsdPtr.isNull()==true) {
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
                QString infString = QString("%1%").arg(Util::formatDouble(static_cast<double>(
                    Growth::fromDecimalToDouble(adjustedInflation.getAnnualConstantGrowth())),
                    locale, Util::DoubleFormatMode::Standard, {}));
                resultStringList.append( tr("Using constant adjusted annual inflation"
                    " of %1.").arg(colorizeStringWithHtml(infString,
                    Util::getOptimizedBlue()))) ;
                resultStringList.append( tr("Inflation can be applied from %1.").
                    arg( colorizeStringWithHtml(
                            locale.toString(nextEventDate, "yyyy-MMM-dd")
                            , Util::getOptimizedBlue())
                    ));
            } else if (Growth::Type::VARIABLE == scenarioInflation.getType()) {
                resultStringList.append(tr("Using variable inflation."));
            } else{
                // should not happen...do nothing
            }
        } else if (psd->getGrowthStrategy()==PeriodicCsd::GrowthStrategy::CUSTOM) {
            if (psd->getGrowth().getType()==Growth::Type::CONSTANT) {
                QString growthString = QString("%1%")
                    .arg(Util::formatDouble(static_cast<double>(Growth::fromDecimalToDouble(
                        psd->getGrowth().getAnnualConstantGrowth())),
                        locale, Util::DoubleFormatMode::Standard, {}));
                resultStringList.append( tr("Using custom constant annual growth of %1.")
                    .arg(colorizeStringWithHtml(growthString,Util::getOptimizedBlue())));
                resultStringList.append( tr("Growth can be applied from %1")
                        .arg(colorizeStringWithHtml(locale.toString(nextEventDate,
                        "yyyy-MMM-dd"),Util::getOptimizedBlue()))   );
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
            QString adrString = QString("%1%")
                .arg(Util::formatDouble(pvAnnualDiscountRate, locale,
                    Util::DoubleFormatMode::Standard, {}));
            QString s = tr("Converting Future Values to Present Values using an annual discount "
                " rate of %1.").arg(colorizeStringWithHtml(adrString, Util::getOptimizedBlue()));
            resultStringList.append(s);
        }
        if(saturationCount > 0){
            QString satCountrStr = Util::formatDouble(saturationCount, locale,
                Util::DoubleFormatMode::Standard, {.standard={.noOfDecimals=0}});
            QString maxSatStr = CurrencyHelper::formatAmount(
                (qint64)CurrencyHelper::maxValueAllowedForAmount(),
                currInfo, locale, false);
            resultStringList.append(tr("Amount was too big %1 times and have been capped to %2.")
                .arg( colorizeStringWithHtml(satCountrStr, Util::getOptimizedBlue()) )
                .arg( colorizeStringWithHtml(maxSatStr,Util::getOptimizedBlue()) )
                );
        }
        QString tomStr = locale.toString(tomorrow, "yyyy-MMM-dd");
        QString toStr = locale.toString(psd->getRealEndDate(maxDateScenarioFeGeneration),
            "yyyy-MMM-dd");
        resultStringList.append(tr("No financial event will be generated before "
            "tomorrow %1 and past %2.")
            .arg(colorizeStringWithHtml(tomStr, Util::getOptimizedBlue()))
            .arg(colorizeStringWithHtml(toStr, Util::getOptimizedBlue()))
            );
        QString noEventStr = Util::formatDouble(feStream->getNoOfElementsUsed(), locale,
            Util::DoubleFormatMode::Standard, {.standard={.noOfDecimals=0}});
        resultStringList.append(tr("%1 event(s) of type \"%2\" have been generated.")
            .arg(colorizeStringWithHtml(noEventStr, Util::getOptimizedBlue()))
            .arg((psd->getIsIncome())?(tr("income")):(tr("expense"))));

    } else {
        // Irregular

        // Standard Header
        bool usePvConversion = GbpController::getInstance().getUsePresentValue();
        double pvAnnualDiscountRate = GbpController::getInstance().getPvDiscountRate();
        if ((usePvConversion==true)&&(pvAnnualDiscountRate!=0)) {
            QString adrString = QString("%1%")
                .arg(Util::formatDouble(pvAnnualDiscountRate, locale,
                    Util::DoubleFormatMode::Standard, {}));
            QString s = tr("Converting Future Values to Present Values using an annual discount "
                " rate of %1.").arg(colorizeStringWithHtml(adrString, Util::getOptimizedBlue()));
            resultStringList.append(s);
        }
        if(saturationCount > 0){
            QString satCountrStr = Util::formatDouble(saturationCount, locale,
                Util::DoubleFormatMode::Standard, {.standard={.noOfDecimals=0}});
            QString maxSatStr = CurrencyHelper::formatAmount(
                (qint64)CurrencyHelper::maxValueAllowedForAmount(),
                currInfo, locale, false);
            resultStringList.append(tr("Amount was too big %1 times and have been capped to %2.")
                .arg( colorizeStringWithHtml(satCountrStr, Util::getOptimizedBlue()) )
                .arg( colorizeStringWithHtml(maxSatStr,Util::getOptimizedBlue()) )
            );
        }

        QString tomStr = locale.toString(tomorrow, "yyyy-MMM-dd");
        QString toStr = locale.toString(maxDateScenarioFeGeneration,
            "yyyy-MMM-dd");
        resultStringList.append(tr("No financial event will be generated before "
            "tomorrow %1 and past %2.")
            .arg(colorizeStringWithHtml(tomStr, Util::getOptimizedBlue()))
            .arg(colorizeStringWithHtml(toStr, Util::getOptimizedBlue()))
        );


        QString noEventStr = Util::formatDouble(feStream->getNoOfElementsUsed(), locale,
            Util::DoubleFormatMode::Standard, {.standard={.noOfDecimals=0}});
        resultStringList.append(tr("%1 %2 event(s) have been generated.")
            .arg(colorizeStringWithHtml(noEventStr, Util::getOptimizedBlue()))
            .arg((isd->getIsIncome())?(tr("income")):(tr("expense"))));
    }

    resultStringList.append("");

    // Print values into text widget
    double cummul = 0;
    uint size = feStream->size();
    uint eventIndex = 0;
    // The event index is an identifier, not a quantity - it must never get a thousands
    // separator (locale.toString(1234) would otherwise render e.g. "1,234", which would also
    // break the intended fixed 5-character zero-padded width below).
    QLocale indexLocale = locale;
    indexLocale.setNumberOptions(QLocale::OmitGroupSeparator);
    for (int var = 0; var < size; var++) {
        if (feStream->contains(var)==false) {
            continue;
        }

        // Build the index string
        eventIndex++;
        QString indexStr = colorizeStringWithHtml(
            QString("[%1]").arg( indexLocale.toString(eventIndex).rightJustified(5,
                indexLocale.zeroDigit().at(0)) ) ,
            Util::getOptimizedPurple());

        qint64 amount = feStream->get(var);

        QDate date = tomorrow.addDays(var);
        if ( amount > CurrencyHelper::maxValueAllowedForAmount() ){ // should not happen
            resultStringList.append(QString("%1 %2 : %3").arg(
                indexStr,
                locale.toString(date, locale.dateFormat(QLocale::ShortFormat)) ,
                tr("Amount is bigger than the maximum allowed")
            ));
        } else {
            double amountDouble = CurrencyHelper::amountQint64ToDouble(amount,
                currInfo.noOfDecimal, ok);
            if (ok != 0){
                resultStringList.append(QString("%1 %2 : %3")
                    .arg(indexStr)
                    .arg(date.toString(Qt::ISODate) , tr("Error during amount conversion")));
            } else {
                amountString = CurrencyHelper::formatAmount(amountDouble, currInfo, locale, false);
                cummul = CurrencyHelper::add(cummul, amountDouble, currInfo.noOfDecimal);
                QString cummulString = CurrencyHelper::formatAmount(cummul,
                    currInfo, locale, false);
                QString s;
                if (date<tomorrow){
                    // if event is in the past, mention it
                    s = s.append(tr("%1 *** PAST -> discarded ***").arg(indexStr));
                } else {
                    s = tr("%1 %2 : %3").arg(indexStr,
                        locale.toString(date,locale.dateFormat(
                            QLocale::ShortFormat)) , amountString);
                }
                resultStringList.append(s);
            }
        }
    }

    // Update the PlainText content
    QString r = resultStringList.join("<br>");
    ui->plainTextEdit->clear();
    ui->plainTextEdit->appendHtml(r);

    //  scroll to the beginning
    ui->plainTextEdit->moveCursor(QTextCursor::Start);
    ui->plainTextEdit->ensureCursorVisible();
}


void VisualizeOccurrencesDialog::updateChartTab(QSharedPointer<FeStream> feStream,
    uint saturationCount, Growth scenarioInflation, FeMinMaxInfo minMax)
{
    QDate tomorrow = GbpController::getInstance().getTomorrow();

    // regenerate Data
    QList<QPointF> timeData;
    // add new data
    int convResult;
    double amount;
    QDateTime momentInTime;
    const qsizetype size = feStream->size();
    for (int var = 0; var < size; ++var) {
        if (feStream->contains(var)==false) {
            continue;
        }
        qint64 amountInt = feStream->get(var);
        momentInTime.setDate(tomorrow.addDays(var));
        amount = CurrencyHelper::amountQint64ToDouble(amountInt, currInfo.noOfDecimal,
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
    Util::changeFontSize(xAxisFont, Util::FontResizeIntensity::AVERAGE, true,
        "VisualizeOccurrencesDialog - X Axis");
    setXaxisFontSize(xAxisFont.pointSize());

    //  Y axis
    QFont yAxisFont = axisY->labelsFont();
    Util::changeFontSize(yAxisFont, Util::FontResizeIntensity::AVERAGE, true,
        "VisualizeOccurrencesDialog - Y Axis");
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


QString VisualizeOccurrencesDialog::colorizeStringWithHtml(QString t, QColor color)
{
    if(color.isValid()==false){
        return "";
    }
    QString result = QString("<span style='color:%1'>%2</span>").arg(color.name()).arg(t);
    return result;
}


void VisualizeOccurrencesDialog::on_exportPushButton_clicked()
{

    // *** STEP 1 : Build the columns definitions ***
    QList<CsvColumnDescriptor> columns;
    columns.append({tr("Date"), CsvColumnType::date(CsvDateFormat::YearMonthDay,
        GbpController::getInstance().getExportTextDateLocalized())});
    columns.append({tr("Amount"), CsvColumnType::currency(
        GbpController::getInstance().getExportTextNumberLocalized())});
    columns.append({tr("Cumulative"), CsvColumnType::currency(
        GbpController::getInstance().getExportTextNumberLocalized())});

    // *** STEP 2 : Populate rows ***
    QList<QList<QVariant>> data;
    QList<QPointF> timeData = series->points();
    double cumul;
    foreach(QPointF pt, timeData){
        QDate date = (QDateTime::fromMSecsSinceEpoch(pt.x())).date();
        double value = pt.y();
        cumul = CurrencyHelper::add(cumul, value, currInfo.noOfDecimal);
        data.append({date, value, cumul});
    }

    // *** STEP 3 : Call exportToCsv() and handle the result ***
    CsvExportResult result = CsvExporter::exportToCsv(
        "Visualize occurence", columns, data, locale, currInfo, '\t' );
    switch (result.status) {
        case CsvExportResult::Status::Success:
            break;
        case CsvExportResult::Status::Canceled:
            return; // user closed the dialog — nothing to do
        case CsvExportResult::Status::FileOpenError:
            GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
                tr("Export process failed. Cannot open the "
                "file for saving"), {tr("OK")}, 0, 0);
            return;
        case CsvExportResult::Status::WriteError:
            GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
                tr("Export process failed. Write error."), {tr("OK")}, 0, 0);
            return;
        case CsvExportResult::Status::DataError:
            GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
                tr("Export process failed. Data error."), {tr("OK")}, 0, 0);
            return;
    }

}


void VisualizeOccurrencesDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("VisualizeOccurrencesDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
    });
}

