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
#include "choosetagsdialog.h"
#include "tags.h"
#include "choosetagsdialog.h"
#include "filtertags.h"

namespace Ui {
class AnalysisDialog;
}

class AnalysisDialog : public QDialog
{
    Q_OBJECT

signals:
    // Choose tags : prepare before edition
    void signalChooseTagsPrepareContent(Tags tags, QSet<QUuid> preSelectedTags);


public slots:

    /**
     * @brief Prepare to show the Dialog : Reset and recalculate all the data and rebuild the
     * all the charts & tables before showing the Dialog.
     * @param chartRawData All the raw data already calculated.
     * @param availabletags All the tags defined in the current scenario.
     * @param currencyInfo Currency for the current scenario.
     * @param startingAmount Start amount currently defined in the Main Window for Cash Balance.
     */
    void slotAnalysisPrepareContent(QWeakPointer<CombinedFeStreams> chartRawDataRef,
        Tags availabletags, CurrencyInfo currencyInfo, double startingAmount);

    // Result and completion signal from Choose Tags
    void slotChooseTagsResult(QSet<QUuid> chosenTags);
    void slotChooseTagsCompleted(bool canceled);


public:
    explicit AnalysisDialog(QLocale theLocale, QWidget *parent = nullptr);
    ~AnalysisDialog();
    void themeChanged();


private slots:
    void on_closePushButton_clicked();
    void on_AnalysisDialog_rejected();
    void on_noElementsSpinBox_valueChanged(int arg1);
    void on_monthlyReportChartDurationSpinBox_valueChanged(int arg1);
    void on_monthlyReportChartRightToolButton_clicked();
    void on_monthlyReportChartLeftToolButton_clicked();
    void on_monthlyReportChartFromMonthComboBox_currentIndexChanged(int index);
    void on_monthlyReportChartFromYearSpinBox_valueChanged(int arg1);
    void on_monthlyReportChartIncomesRadioButton_clicked();
    void on_monthlyReportChartExpensesRadioButton_clicked();
    void on_monthlyReportChartIncomesExpensesRadioButton_clicked();
    void on_monthlyReportChartDeltasRadioButton_clicked();
    void on_yearlyReportChartFromYearSpinBox_valueChanged(int arg1);
    void on_yearlyReportChartDurationSpinBox_valueChanged(int arg1);
    void on_yearlyReportChartLeftToolButton_clicked();
    void on_yearlyReportChartRightToolButton_clicked();
    void on_yearlyReportChartIncomesRadioButton_clicked();
    void on_yearlyReportChartExpensesRadioButton_clicked();
    void on_yearlyReportChartIncomesExpensesRadioButton_clicked();
    void on_yearlyReportChartDeltasRadioButton_clicked();
    void on_incomesRelativeWeigthRadioButton_clicked();
    void on_expensesRelativeWeigthRadioButton_clicked();
    void on_tagsSelectTagsPushButton_clicked();
    void on_tagsIncomesRadioButton_clicked();
    void on_tagsExpensesRadioButton_clicked();
    void on_RW_ClearSelectionPushButton_clicked();
    void on_globalExportCsvPushButton_clicked();
    void on_globalExportImagePushButton_clicked();
    void on_tabWidget_currentChanged(int index);
    void on_RW_horizontalSlider_valueChanged(int value);
    void on_RW_ShowLabelsCheckBox_checkStateChanged(const Qt::CheckState &arg1);
    void on_RW_listWidget_itemSelectionChanged();
    void on_RW_ApplyDatesPushButton_clicked();
    void on_tags_ApplyDatesPushButton_clicked();

private:
    Ui::AnalysisDialog *ui;

    /**
     * @brief To enable QTableWidget sorting for column containing number.
     */
    class NumericTableWidgetItem : public QTableWidgetItem {
    public:
        NumericTableWidgetItem(const QString& text) : QTableWidgetItem(text) {}

        bool operator<(const QTableWidgetItem& other) const override;

    };

    // --- for Monthly/Yearly report ---

    /**
     * @struct Bin
     * @brief Info for a particular period (month or year).
     */
    struct Bin{
        /**
         * @brief Total income for this period (always >= 0).
         */
        double income;

        /**
         * @brief Total expense for this period (always >= 0).
         */
        double expense;

        /**
         * @brief Total delta (that is, income-expense) for this period. Can be negative.
         */
        double delta;

        /**
         * @brief Total cash balance at the end of this period. Can be negative.
         * Depends on startingAmount when computed.
         */
        double cashBalance;

        bool operator==(const Bin & o) const;

        bool operator!=(const Bin & o) const;
    };

    // which data source is selected (monthly and annualy report - chart)
    enum class GraphDataSourceType {DST_INCOME, DST_EXPENSE, DST_INCOME_EXPENSE, DST_DELTA};
    // For Relative Weight chart
    enum class ReportType { MONTHLY, YEARLY };

    // --- For Relative Weight ---

    /**
     * @brief Enum to define the type of grouping in Relative Weight tab.
     */
    enum class RelWeightGrouping { RWGROUPING_INCOMES, RWGROUPING_EXPENSES };

    // --- for Tags table ---

    /**
     * @struct TagContribution
     * @brief Hold info about the contribution of a tag in all incomes or expenses financial
     * events.
     */
    struct TagContribution{
        QString name;     /// tag name
        double amount;    /// total amount for that tag, for either incomes or expenses
        double weight;    /// Percentage of the total amount (either for incomes or expenses)

        bool operator==(const TagContribution& o) const;
        bool operator!=(const TagContribution& o) const;
    };

    // *** variables ***

    // +++ Children dialogs +++
    ChooseTagsDialog* selectTagsDlg;

    bool ready=false; // true if init is completed
    CurrencyInfo currInfo;
    QLocale locale;
    double startingAmount;

    // +++ Relative Weight Pie Chart +++
    QChartView *chartViewRelativeWeigth = nullptr;
    QChart* chartRelativeWeight;
    QPieSeries* seriesRelativeWeigth;
    QList<QColor> colorsRelativeWeigth;

    // +++ Monthly Report BarChart (stay the same for the whole life of the app) +++
    QChart* chartMonthlyReport;
    QChartView *chartViewMonthlyReport = nullptr;
    QBarCategoryAxis* chartMonthlyReportAxisX;
    QValueAxis* chartMonthlyReportAxisY;

    // +++ Yearly Report BarChart (stay the same for the whole life of the app) +++
    QChart* chartYearlyReport;
    QChartView *chartViewYearlyReport = nullptr;
    QBarCategoryAxis* chartYearlyReportAxisX;
    QValueAxis* chartYearlyReportAxisY;

    // +++ Tags bar BarChart +++
    QChart* chartTags;
    QChartView *chartViewTags = nullptr;
    QBarCategoryAxis* chartTagsAxisX;
    QValueAxis* chartTagsAxisY;

    // +++ Data +++

    QDate rwPreviousFromDate;
    QDate rwPreviousToDate;

    QDate tagsPreviousFromDate;
    QDate tagsPreviousToDate;

    /**
     * @brief Reference to the external Raw data (FE suite)).
     */
    QWeakPointer<CombinedFeStreams> chartRawDataRef;

    /**
     * @brief All the Monthly bins from tomorrow till end of the scenario. Key Day=1.
     */
    QMap<QDate,Bin> binsMonthly;

    /**
     * @brief All the Yearly bins from tomorrow till end of the scenario. Key Day=1, Key month=1.
     */
    QMap<QDate,Bin> binsYearly;

    // All the available tags for the current scenario (regarless selection).
    Tags availableTags;

    /**
     * @brief Tags that have been explicitly selected by the user for the contribution analysis.
     * Only tags in this list will be shown in the table.
     */
    FilterTags selectedTags;

    /**
     * @brief Table used to hold the Tags analysis data displayed in the table and also used
     * for exporting to CSV file.
     */
    QHash<QUuid,TagContribution> tagsTableData;

    // +++ methods +++

    /**
     * @brief Takes care of rezising a Chart Widget.
     */
    bool eventFilter(QObject *object, QEvent *event) override;

    /**
     * @brief Completely recalculate and redisplay the Relative Weight Pie chart, the legend
     * and the associated widgets. From and To dates must be valid
     */
    void updateRelativeWeightChart();

    /**
     * @brief Clear the widgets associated to the legend, that is the rank, amount and percentage
     * values.
     */
    void clearLegendWidgets();

    /**
     * @brief Update the content of the widgets associated to the legend, that is the rank,
     * amount and percentage
     * values.
     * @param rank The new rank value (0 not allowed). If -1, then set label empty.
     * @param amount The new amount.
     * @param percentage The new percentage.
     */
    void updateLegendWidgets(int rank, double amount, double percentage);


    /**
     * @brief Generate data for all the possible bins (annual and monthly reports, both for
     * Chart and Table format), from tomorrow till end of the scenario.
     * annualDiscountRate is in percentage.
     * @param rTypr Type of report (monthly or annual).
     * @param tableWidget. The histogram widget involved.
     */
    void recalculate_BinsData(ReportType rTypr, QTableWidget* tableWidget);

    void redisplay_MonthlyYearlyReportTableData(ReportType rTypr, QTableWidget* tableWidget);
    void redisplay_ReportChart(ReportType type);
    uint noOfMonthDifference(const QDate& from, const QDate& to) const ;
    uint noOfYearDifference(const QDate& from, const QDate& to) const ;

    /**
     * @brief Export either the monthly or the yearly table data into a CSV file.
     * @param rType Type of data source.
     */
    void exportTextMonthlyYearlyReport(ReportType rType) ;

    /**
     * @brief Export a Chart in a PNG file.
     * @param chartWidget The Chart.
     * @param desc Short name of the chart for logging purpose.
     */
    void exportChartAsImage(QWidget* chartWidget, QString desc);

    /**
     * @brief Export the content of the legend in Relative Weight tab to a Csv file.
     */
    void exportRelativeWeightLegendAsCsvFile();

    void fillMonthlyReportComboBoxWithMonthNames() const;
    QString buildBarChartCategoryName(ReportType type, QDate date) const;
    void initReportChart(ReportType type);
    void getStartEndDateReportChart(ReportType type, QDate& startDate, QDate& endDate ) const;
    void getMinMaxReportChart(ReportType type, QBarSet* incomes, QBarSet* expenses,
        QBarSet* deltasPositive, QBarSet* deltasNegative, double& minY, double& maxY) ;
    AnalysisDialog::GraphDataSourceType findWhichSetsIsToBeUsedReportChart(ReportType type);
    void setMonthlyYearlyChartTitle(QChart* chartPtr, GraphDataSourceType dsType);
    void calculateStatsForGraph(const QList<double> data, double& mean, double& stdDeviation,
        double& sum);
    QList<QDate> getListOfDatesCoveredbyBin(ReportType type, QDate binDate);
    RelWeightGrouping getGroupingTypeSelected();


    // Tags

    /**
     * @brief Completely rebuild the data and then update the Tags table.
     */
    void redisplay_TagTable();

    /**
     * @brief From raw data, fully rebuild the data required to update the content
     * of the tags table. Tags table is NOT updated. Data is put in "data" (cleared first).
     * @details Key of the HashMap is Tag Id, value is info needed for the display.
     * @param from Do not collect data before this date.
     * @param to Do not collect data beyond this date.
     * @param data Resulting data.
     * @param totalWithoutTags Resulting total in this period, without considering any tag.
     * It is either an income (>0) or expense (<0).
     */
    void tags_rebuildData(QDate from, QDate to, QHash<QUuid,TagContribution>& data,
        double &totalWithoutTags);

    /**
     * @brief Update the no of tags selected Label.
     */
    void tags_UpdateNoTagsSelected();

    /**
     * @brief Export in a CSV file the tags analysis.
     */
    void exportTagsAnalysisAsCsv();

};

#endif // ANALYSISDIALOG_H
