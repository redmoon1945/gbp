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
     * @brief scenarioInflation is used only when csd is a Periodic Csd.
     * @param currInfo
     * @param adjustedInflation
     * @param maxDateScenario
     * @param csd
     */
    void slotPrepareContent(CurrencyInfo currInfo, Growth adjustedInflation,
        QDate maxDateScenario, QWeakPointer<Csd> csd );

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

private:
    Ui::VisualizeOccurrencesDialog *ui;

    // *** Variables ***

    QLocale locale;
    CurrencyInfo currInfo;
    // last event limit date according to scenario (could not be in use). Limit set by Periodic
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
     * @brief Generate the financial events for that Csd (whether Periodic or Irregular). Return
     * the no of saturations that occurred.
     * @param scenarioInflation Inflation for the scenario (used only for Periodic).
     * @param weakCsdPtr A QWeakPointer<Csd> reference to the Csd.
     * @param saturationCount Saturation count that occurred during the generation.
     * @param minMax Min/Max of the value generated.
     * @return The generated Fe Stream.
     */
    FeStream generateFinancialEvents(Growth scenarioInflation, QWeakPointer<Csd> weakCsdPtr,
        uint& saturationCount, FeMinMaxInfo& minMax);

    /**
     * @brief Using the generated FeStream, display the results in the PlainText Widget, with a header
    // providing some useful information.
     * @param feStream The generated Fe Stream.
     * @param saturationCount Saturation count that occurred during the generation.
     * @param scenarioInflation Inflation for the scenario (used only for Periodic).
     * @param weakCsdPtr A QWeakPointer<Csd> reference to the Csd.
     */
    void updateTextTab(FeStream& feStream, uint saturationCount, Growth scenarioInflation,
        QWeakPointer<Csd> weakCsdPtr);

    /**
     * @brief Using the generated FeStream, update the chart data with representation in proper
     * currency.
     * @param feStream The generated Fe Stream.
     * @param saturationCount Saturation count that occurred during the generation.
     * @param scenarioInflation Inflation for the scenario (used only for Periodic).
     * @param minMax Min/Max of the value generated.
     */
    void updateChartTab(FeStream& feStream, uint saturationCount, Growth scenarioInflation,
        FeMinMaxInfo minMax);

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
};

#endif // VISUALIZEOCCURRENCESDIALOG_H
