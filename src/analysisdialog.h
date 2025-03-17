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

#ifndef ANALYSISDIALOG_H
#define ANALYSISDIALOG_H

#include <QDialog>
#include <QChartView>
#include <QPieSeries>
#include <QTableWidget>
#include <QBarSeries>
#include <QBarCategoryAxis>
#include "combinedfestreams.h"
#include "qvalueaxis.h"

namespace Ui {
class AnalysisDialog;
}

class AnalysisDialog : public QDialog
{
    Q_OBJECT

public slots:
     // prepare Dialog before showing
    void slotAnalysisPrepareContent(QMap<QDate,CombinedFeStreams::DailyInfo> chartRawData, CurrencyInfo currencyInfo);

public:
    explicit AnalysisDialog(QLocale theLocale, QWidget *parent = nullptr);
    ~AnalysisDialog();
    void themeChanged();

private slots:
    void on_closePushButton_clicked();
    void on_AnalysisDialog_rejected();
    void on_noElementsSpinBox_valueChanged(int arg1);
    void on_fromDateEdit_userDateChanged(const QDate &date);
    void on_toDateEdit_userDateChanged(const QDate &date);
    void on_exportImageRelativeWeigthPushButton_clicked();
    void on_exportMonthlyReportToTextPushButton_clicked();
    void on_exportYearlyReportToTextPushButton_clicked();
    void on_incomesRelativeWeigthRadioButton_toggled(bool checked);
    void on_monthlyReportChartDurationSpinBox_valueChanged(int arg1);
    void on_monthlyReportChartRightToolButton_clicked();
    void on_monthlyReportChartLeftToolButton_clicked();
    void on_monthlyReportChartFromMonthComboBox_currentIndexChanged(int index);
    void on_monthlyReportChartFromYearSpinBox_valueChanged(int arg1);
    void on_monthlyReportChartIncomesRadioButton_clicked();
    void on_monthlyReportChartExpensesRadioButton_clicked();
    void on_monthlyReportChartIncomesExpensesRadioButton_clicked();
    void on_monthlyReportChartDeltasRadioButton_clicked();
    void on_exportImageMonthlyReportChartPushButton_clicked();
    void on_yearlyReportChartFromYearSpinBox_valueChanged(int arg1);
    void on_yearlyReportChartDurationSpinBox_valueChanged(int arg1);
    void on_yearlyReportChartLeftToolButton_clicked();
    void on_yearlyReportChartRightToolButton_clicked();
    void on_yearlyReportChartIncomesRadioButton_clicked();
    void on_yearlyReportChartExpensesRadioButton_clicked();
    void on_yearlyReportChartIncomesExpensesRadioButton_clicked();
    void on_yearlyReportChartDeltasRadioButton_clicked();
    void on_exportImageYearlyReportChartPushButton_clicked();

private:
    Ui::AnalysisDialog *ui;

    // temp structure to sort things
    struct Pair{
        double amount;  // always positive
        double percentage; // e.g. 4% => 4, always positive
        QUuid id;
    };
    // for Monthly report
    struct MonthlyYearlyReport{
        double income;
        double expense;
        double delta;
    };
    enum ReportType { MONTHLY, YEARLY };

    // *** variables ***
    bool ready=false; // true if init is completed
    CurrencyInfo currInfo;
    QLocale locale;
    // Relative Weight Pie Chart
    QChartView *chartViewRelativeWeigth = nullptr;
    QChart* chartRelativeWeigth;
    QPieSeries* seriesRelativeWeigth;
    QList<QColor> colorsRelativeWeigth; // for Pie Chart
    // Monthly Report BarChart (stay the same for the whole life of the app)
    QChart* chartMonthlyReport;
    QChartView *chartViewMonthlyReport = nullptr;
    QBarCategoryAxis* chartMonthlyReportAxisX;
    QValueAxis* chartMonthlyReportAxisY;
    // Yearly Report BarChart (stay the same for the whole life of the app)
    QChart* chartYearlyReport;
    QChartView *chartViewYearlyReport = nullptr;
    QBarCategoryAxis* chartYearlyReportAxisX;
    QValueAxis* chartYearlyReportAxisY;
    // Common data
    QMap<QDate,CombinedFeStreams::DailyInfo> chartRawData;// Raw data
    QMap<QDate,MonthlyYearlyReport> binsMonthly;          // for Monthly report data. Day=1
    QMap<QDate,MonthlyYearlyReport> binsYearly;           // for Yearly report data. Day=1, month=1

    // methods
    bool eventFilter(QObject *object, QEvent *event) override;
    void updateRelativeWeightChart();
    void recalculate_MonthlyYearlyReportData(ReportType rTypr, QTableWidget* tableWidget);
    void redisplay_MonthlyYearlyReportTableData(ReportType rTypr, QTableWidget* tableWidget);
    void redisplay_ReportChart(ReportType type, bool usePresentValues);
    uint noOfMonthDifference(const QDate& from, const QDate& to) const ;
    uint noOfYearDifference(const QDate& from, const QDate& to) const ;
    void exportTextMonthlyYearlyReport(ReportType rType) ;
    void exportChartAsImage(QWidget* chartWidget);
    void fillMonthlyReportComboBoxWithMonthNames() const;
    QString buildBarChartCategoryName(ReportType type, QDate date) const;
    void initReportChart(ReportType type);
    void getStartEndDateReportChart(ReportType type, QDate& startDate, QDate& endDate ) const;
    void getMinMaxReportChart(ReportType type, QBarSet* incomes, QBarSet* expenses, QBarSet* deltasPositive, QBarSet* deltasNegative, double& minY, double& maxY) ;
    void findWhichSetsIsToBeUsedReportChart(ReportType type, bool& incomesSet, bool& expensesSet, bool& deltasSet);
    void setMonthlyYearlyChartTitle(QChart* chartPtr, bool useIncomes, bool useExpenses, bool useDeltas);
};

#endif // ANALYSISDIALOG_H
