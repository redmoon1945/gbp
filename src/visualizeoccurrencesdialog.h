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

#ifndef VISUALIZEOCCURRENCESDIALOG_H
#define VISUALIZEOCCURRENCESDIALOG_H

#include "currencyhelper.h"
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
    void slotPrepareContent(CurrencyInfo currInfo, Growth adjustedInflation, QDate maxDateScenarioFeGeneration, FeStreamDef* streamDef );

signals:
    // For client of VisualizeOccurrencesDialog : sending completion notification
    void signalCompleted();

private slots:
    void on_closePushButton_clicked();
    void on_VisualizeOccurrencesDialog_rejected();

    void on_showPointsCheckBox_stateChanged(int arg1);

private:
    Ui::VisualizeOccurrencesDialog *ui;

    // *** Variables ***
    QLocale locale;
    CurrencyInfo currInfo;
    QDate maxDateScenarioFeGeneration;
    // Curve
    QChartView *chartView = nullptr;
    QChart* chart;
    QDateTimeAxis *axisX ;
    QValueAxis *axisY;
    QLineSeries *series;

    // Methods
    bool eventFilter(QObject *object, QEvent *event) override;
    QList<Fe> generateFinancialEvents(Growth scenarioInflation, FeStreamDef *streamDef, uint& saturationCount, FeMinMaxInfo& minMax);
    void updateTextTab(QList<Fe> feList, uint saturationCount, Growth scenarioInflation,  FeStreamDef *streamDef);
    void updateChartTab(QList<Fe> feList, uint saturationCount, Growth scenarioInflation,  FeStreamDef *streamDef,FeMinMaxInfo minMax);
    void initChart();
};

#endif // VISUALIZEOCCURRENCESDIALOG_H
