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
#include "festreamdef.h"
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
    void slotPrepareContent(CurrencyInfo currInfo, Growth adjustedInflation,
        QDate maxDateScenario, FeStreamDef* streamDef );
    // to catch point selection signal in chart
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
    // Stream def could override that value (if it is smaller)
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

    // Methods
    bool eventFilter(QObject *object, QEvent *event) override;
    QList<Fe> generateFinancialEvents(Growth scenarioInflation, FeStreamDef *streamDef, uint& saturationCount, FeMinMaxInfo& minMax);
    void updateTextTab(QList<Fe> feList, uint saturationCount, Growth scenarioInflation,  FeStreamDef *streamDef);
    void updateChartTab(QList<Fe> feList, uint saturationCount, Growth scenarioInflation,  FeStreamDef *streamDef,FeMinMaxInfo minMax);
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
