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

#ifndef VISUALIZEOCCURRENCESDIALOG_H
#define VISUALIZEOCCURRENCESDIALOG_H

#include "currencyhelper.h"
#include "customqchartview.h"
#include "festream.h"
#include "growth.h"
#include "fe.h"
#include <QDialog>
#include <QChartView>
#include <qdatetimeaxis.h>
#include <qlineseries.h>
#include <QScatterSeries>
#include <qvalueaxis.h>


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

    /**
     * @brief To be called just before showing the Dialog.
     * @param currInfo Currency info.
     * @param feStreamGrowth The FeStream to visualize.
     */

    /**
     * @brief To be called just before showing the Dialog. All the required calculations are
     * performed in this method. The Csd refered to by the FeStream must exist.
     * @param currInfo Currency info.
     * @param feStream The FeStream to visualize.
     * @param saturationCount No of saturationthat occurred when the FeStream was calculated.
     * @param scenarioInflation Scenario's inflation. Used only for Period Csd.
     * @param minMax Min/max value for the FeStream
     * @param maxDateScenario Maximum date above which the scenario prevents any financial event
     * to be generated.
     */
    void slotPrepareContent(CurrencyInfo currInfo, QSharedPointer<FeStream> feStream,
        uint saturationCount, Growth scenarioInflation, FeMinMaxInfo minMax, QDate maxDateScenario);

    /**
     * @brief To catch point selection signal in chart.
     * @param pt
     */
    void mypoint_clicked(const QPointF pt);

signals:
    // For client of VisualizeOccurrencesDialog : sending completion notification
    void signalCompleted();

private slots:
    void on_closePushButton_clicked();
    void on_VisualizeOccurrencesDialog_rejected();
    void on_fitPushButton_clicked();
    void on_exportPushButton_clicked();

protected:
    void showEvent(QShowEvent* event) override;

private:
    Ui::VisualizeOccurrencesDialog *ui;

    // *** Variables ***

    QLocale locale;
    CurrencyInfo currInfo;

    // Last event limit date according to scenario (could not be in use). Limit set by Periodic
    // Cds could override that value (if it is smaller)
    QDate maxDateScenarioFeGeneration;

    // Curve
    CustomQChartView *chartView;
    QChart* chart;
    QDateTimeAxis *axisX ;
    QValueAxis *axisY;
    QScatterSeries *series;
    QDateTime xMin, xMax;
    uint xAxisFontSize;
    uint yAxisFontSize;
    std::vector<double> searchVector;
    int indexLastPointSelected = -1;


    // *** Methods ***

    bool eventFilter(QObject *object, QEvent *event) override;

    /**
     * @brief Using the generated FeStream, display the results in the PlainText Widget, with a header
    // providing some useful information.
     * @param feStream The generated Fe Stream.
     * @param saturationCount Saturation count that occurred during the generation.
     * @param scenarioInflation Inflation for the scenario (used only for Periodic).
     */
    void updateTextTab(QSharedPointer<FeStream> feStream, uint saturationCount,
        Growth scenarioInflation);

    /**
     * @brief Using the generated FeStream, update the chart data with representation in proper
     * currency.
     * @param feStream The generated Fe Stream.
     * @param saturationCount Saturation count that occurred during the generation.
     * @param scenarioInflation Inflation for the scenario (used only for Periodic).
     * @param minMax Min/Max of the value generated.
     */
    void updateChartTab(QSharedPointer<FeStream> feStream, uint saturationCount,
        Growth scenarioInflation,  FeMinMaxInfo minMax);

    void initChart();
    void reduceAxisFontSize();
    void setXaxisFontSize(uint fontSize);
    void setYaxisFontSize(uint fontSize);
    int binarySearch(const std::vector<double>& vec, double target);
    void themeChanged();
    void setSeriesCharacteristics();
    void replaceChartSeries(QList<QPointF> data);
    void rescaleChart();
    void changeYaxisLabelFormat();

    /**
     * @brief Add HTML tags to make a string colored in HTML.
     * @param t The string to color.
     * @return The colored string.
     */
    QString colorizeStringWithHtml(QString t, QColor color);
};

#endif // VISUALIZEOCCURRENCESDIALOG_H
