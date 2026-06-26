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

#ifndef ANALYSISDIALOG_H
#define ANALYSISDIALOG_H

#include <optional>
#include <QColor>
#include <QDialog>
#include <QChartView>
#include <QPieSeries>
#include <QPushButton>
#include <QTableWidget>
#include <QListWidget>
#include <QBarSeries>
#include <QBarCategoryAxis>
#include "combinedfestreams.h"
#include "qvalueaxis.h"
#include "choosetagsdialog.h"
#include "tags.h"
#include "choosetagsdialog.h"
#include "filtertags.h"

class CustomQChartView;
class QDateTimeAxis;
class QScatterSeries;

// Required metatype declaration to associate std::optional<double> (from Bin) to a QTableWidgetItem
//   Set : wi3->setData(Qt::UserRole,QVariant::fromValue(bin.incomeGrowthInPercentage));
//   Get : auto val = item->data(Qt::UserRole).value<std::optional<double>>();
Q_DECLARE_METATYPE(std::optional<double>)

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
    void on_rwNoElementsSpinBox_valueChanged(int arg1);
    void on_periodChartTypeIncomeRadioButton_clicked();
    void on_periodChartTypeExpensesRadioButton_clicked();
    void on_periodChartTypeIncomeAndExpensesRadioButton_clicked();
    void on_periodChartTypeDeltasRadioButton_clicked();
    void on_rwIncomesRadioButton_clicked();
    void on_rwExpensesRadioButton_clicked();
    void on_tagsSelectTagsPushButton_clicked();
    void on_tagsIncomesRadioButton_clicked();
    void on_tagsExpensesRadioButton_clicked();
    void on_rwClearSelectionPushButton_clicked();
    void on_globalExportCsvPushButton_clicked();
    void on_globalExportImagePushButton_clicked();
    void on_tabWidget_currentChanged(int index);
    void on_rwHorizontalSlider_valueChanged(int value);
    void on_rwShowLabelsCheckBox_checkStateChanged(const Qt::CheckState &arg1);
    void on_rwListWidget_itemSelectionChanged();
    void on_globalApplyDatesPushButton_clicked();
    void on_periodListMonthlyRadioButton_clicked();
    void on_periodListAnnualRadioButton_clicked();
    void on_periodChartMonthlyRadioButton_clicked();
    void on_periodChartAnnuallyRadioButton_clicked();
    void on_periodChartPrevPushButton_clicked();
    void on_periodChartNextPushButton_clicked();
    void on_periodChartBarCountSpinBox_valueChanged(int value);
    void on_periodHeatmapMonthlyRadioButton_clicked();
    void on_periodHeatmapYearlyRadioButton_clicked();
    void on_periodHeatmapExcludeCurrentCheckBox_checkStateChanged(Qt::CheckState state);
    void on_periodHeatmapExcludeLastCheckBox_checkStateChanged(Qt::CheckState state);
    void on_periodHeatmapIncomesRadioButton_clicked();
    void on_periodHeatmapExpensesRadioButton_clicked();
    void on_periodHeatmapDeltaRadioButton_clicked();
    void on_periodHeatmapFromColorPushButton_clicked();
    void on_periodHeatmapToColorPushButton_clicked();
    void on_periodHeatmapFromResetPushButton_clicked();
    void on_periodHeatmapToResetPushButton_clicked();
    void on_periodHeatmapCellSizeComboBox_currentIndexChanged(int index);
    void on_csdIncomesRadioButton_clicked();
    void on_csdExpensesRadioButton_clicked();
    void on_csdListWidget_itemSelectionChanged();
    void on_csdFitPushButton_clicked();
    void on_csdUnselectAllPushButton_clicked();
    void on_csdShowPointsCheckBox_stateChanged(int arg1);


protected:
    /**
     * @brief Re-runs DPI-aware column sizing once the dialog is placed on its target screen.
     * @details @c slotAnalysisPrepareContent() populates the Period List table and sizes its
     *          columns before the dialog is shown. On Windows with per-monitor DPI scaling the
     *          widget's font DPI is only finalised when it lands on its actual screen, so the
     *          pre-show measurements can be incorrect. This override calls
     *          @c UiUtil::resizeTableColumns() again with the now-correct metrics.
     *          The call is skipped when the table is empty (no scenario loaded yet).
     * @param event The show event forwarded to @c QDialog::showEvent().
     */
    void showEvent(QShowEvent* event) override;

private:
    Ui::AnalysisDialog *ui;

    // ---------------------------------------------------------------------------------------------
    // ---------------------------- Data Structures and classes ------------------------------------
    // ---------------------------------------------------------------------------------------------

    /**
     * @brief To enable QTableWidget sorting for column containing number.
     */
    class NumericTableWidgetItem : public QTableWidgetItem {
    public:
        NumericTableWidgetItem(const QString& text) : QTableWidgetItem(text) {}

        bool operator<(const QTableWidgetItem& other) const override;

    };

    // For Period - *
    enum class PeriodType { MONTHLY, YEARLY };


    /**
     * @struct Bin
     * @brief Info for a particular period (month or year). Contains derived data
     * like incomeGrowth,expenseGrowth, deltaGrowth (for conveniance)
     */
    struct Bin{
        /**
         * @brief Total income for this period (always >= 0).
         */
        double income;

        /**
         * @brief Income growth from previous period to this one, in percentage.
         * std::nullopt if undefined.
         * @see Util::percentageChange()
         */
        std::optional<double> incomeGrowthInPercentage;

        /**
         *
         * @brief Total expense for this period (always >= 0).
         */
        double expense;

        /**
         * @brief Expense growth from previous period to this one, in percentage.
         * std::nullopt if undefined.
         * @see Util::percentageChange()
         */
        std::optional<double> expenseGrowthInPercentage;

        /**
         * @brief Total delta (that is, income-expense) for this period. Can be negative.
         * @note Directly derived from income and expense.
         */
        double delta;

        /**
         * @brief Delta growth from previous period to this one, in percentage.
         * std::nullopt if undefined.
         * @see Util::percentageChange()
         */
        std::optional<double> deltaGrowthInPercentage;


        /**
         * @brief Total cash balance at the end of this period. Can be negative.
         * Depends on startingAmount when computed.
         */
        double cashBalance;

        Bin();

        bool operator==(const Bin & o) const;

        bool operator!=(const Bin & o) const;
    };



    // --- Period - Chart ---

    // which data source is selected (monthly and annualy report - chart)
    enum class PeriodChartSourceType {DST_INCOME, DST_EXPENSE, DST_INCOME_EXPENSE, DST_DELTA};


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
        int count = 0;    /// number of individual financial events contributing to this tag

        bool operator==(const TagContribution& o) const;
        bool operator!=(const TagContribution& o) const;
    };


    // ---------------------------------------------------------------------------------------------
    // -------------------------------------- Variables --------------------------------------------
    // ---------------------------------------------------------------------------------------------

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

    // +++ Period Chart pan state +++
    int periodChartOffset{0};

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

    QDate globalPreviousFromDate;
    QDate globalPreviousToDate;

    /**
     * @brief Reference to the external Raw data (FE suite)).
     */
    QWeakPointer<CombinedFeStreams> chartRawDataRef;

    // +++ PeriodHeatmap colors +++
    /**
     * @brief Maximum (fully saturated) color for income mode. Cells blend from a theme-derived
     * near-background tint toward this color as income increases.
     * Lazy-initialized to Qt::green; not persisted to config.
     */
    QColor periodHeatmapIncomeToColor;

    /**
     * @brief Maximum (fully saturated) color for expense mode. Cells blend from a theme-derived
     * near-background tint toward this color as expense increases.
     * Lazy-initialized to Qt::red; not persisted to config.
     */
    QColor periodHeatmapExpenseToColor;

    /**
     * @brief Color representing the negative extreme in delta mode. Cells with the largest
     * negative delta blend toward this color. Lazy-initialized to Qt::red; not persisted to config.
     */
    QColor periodHeatmapDeltaFromColor;

    /**
     * @brief Color representing the positive extreme in delta mode. Cells with the largest
     * positive delta blend toward this color. Lazy-initialized to Qt::green; not persisted to config.
     */
    QColor periodHeatmapDeltaToColor;

    // 0 = Normal (1×), 1 = Big (1.5×), 2 = Bigger (2×)
    int periodHeatmapCellSizeIndex = 1;

    /**
     * @brief All the Monthly bins from tomorrow till end of the scenario. One entry per month.
     * @note Key Day=1.
     */
    QMap<QDate,Bin> binsMonthly;

    /**
     * @brief All the Yearly bins from tomorrow till end of the scenario. One entry per year.
     * @note Key Day=1, Key month=1.
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

    // +++ CSD Comparison tab +++

    QSet<QUuid> csdCheckedIncomeIds;
    QSet<QUuid> csdCheckedExpenseIds;

    QChart*            csdChart     = nullptr;
    CustomQChartView*  csdChartView = nullptr;
    QDateTimeAxis*     csdAxisX     = nullptr;
    QValueAxis*        csdAxisY     = nullptr;
    uint csdXAxisFontSize = 8;
    uint csdYAxisFontSize = 8;
    QScatterSeries*    csdLastSelectedSeries = nullptr;
    int                csdLastSelectedIndex  = -1;
    QMap<QUuid, int>   csdColorAssign[2];    // persistent color slot per UUID, [0]=income [1]=expense



    // ---------------------------------------------------------------------------------------------
    // -------------------------------------- Methods-- --------------------------------------------
    // ---------------------------------------------------------------------------------------------

    /**
     * @brief Takes care of rezising a Chart Widget.
     */
    bool eventFilter(QObject *object, QEvent *event) override;

    /**
     * @brief Completely recalculate and redisplay the Relative Weight Pie chart, the legend
     * and the associated widgets. From and To dates must be valid
     */
    void rwUpdateChart();

    /**
     * @brief Clear the widgets associated to the legend, that is the rank, amount and percentage
     * values.
     */
    void rwClearLegendWidgets();

    /**
     * @brief Update the content of the widgets associated to the legend, that is the rank,
     * amount and percentage
     * values.
     * @param rank The new rank value (0 not allowed). If -1, then set label empty.
     * @param amount The new amount.
     * @param percentage The new percentage.
     */
    void rwUpdateLegendWidgets(int rank, double amount, double percentage);

    /**
     * @brief Generate data for all the possible bins (annual and monthly reports, both for
     * Chart and Table format), from tomorrow till end of the scenario.
     * annualDiscountRate is in percentage.
     * @param rTypr Type of report (monthly or annual).
     * @param tableWidget. The histogram widget involved.
     */
    void recalculate_BinsData(PeriodType rTypr, QTableWidget* tableWidget);

    /**
     * @brief Completely redisplay the content of the Monthly or Yearly Report Table,
     * based on the current content of the calculated bins.
     * @details Both must have exactly the same number of columns.
     * @param rTypr Type of table.
     */
    void periodListRedisplayTableData(PeriodType rTypr);

    /**
     * @brief Completely rebuild and redisplay the Period Chart bar chart for the given
     * granularity, using the current bins data and the currently selected data source
     * (income, expenses, income+expenses, or deltas).
     * @details Selects the appropriate QChart and QChartView (monthly or yearly), clears all
     * existing series, rebuilds bar sets, recalculates stats (mean, std deviation, median,
     * sum), and updates the chart axis ranges. If no data is available, the chart is cleared.
     * @param type Granularity of the chart to redisplay (monthly or yearly).
     */
    void periodChartRedisplayChart(PeriodType type);

    /**
     * @brief Convert to QString each growth value of a bin. Returned in the QStringList.
     * Index 0 is incomeGrowth, 1 is expenseGrowth, 2 is deltaGrowth.
     * @details The final string is guaranteed to be 9 char or less. Max value displayed in
     * standard format is abs(]100000).   E.g.
     * -1.234e7
     * -99,999.9
     * @param bin The Bin
     * @param strings the set of results.
     */
    void binGrowthValuesToStrings(Bin bin, QStringList& strings, QLocale locale);

    /**
     * @brief Export the Period List table data (monthly or yearly) to a CSV file.
     * @details Builds column definitions for period date, income, income growth, expenses,
     * expense growth, delta, delta growth, and cash balance; populates rows from the
     * corresponding bins; then calls CsvExporter::exportToCsv() and handles the result.
     * @param rType Granularity of the data to export (monthly or yearly).
     */
    void periodListExport(PeriodType rType);

    /**
     * @brief Export a Chart in a PNG file.
     * @param chartWidget The Chart.
     * @param desc Short name of the chart for logging purpose.
     */
    void exportChartAsImage(QWidget* chartWidget, QString desc);

    /**
     * @brief Export the content of the legend in Relative Weight tab to a Csv file.
     */
    void rwExportLegendAsCsvFile();


    /**
     * @brief Fully rebuild and repaint the periodHeatmap table from the current bins and color settings.
     *
     * @details
     * The table has one row per year and 12 columns (Jan–Dec), spanning the full range of
     * @c binsMonthly. Three display modes are supported, selected via the radio buttons:
     *
     * **Income / Expense mode** (sequential color scheme):
     * - The minimum color is derived dynamically from the max color's hue, with near-background
     *   brightness (dark theme: value≈45, light theme: value≈220) and fixed saturation (230).
     *   This makes the smallest non-zero cell look like a faint tint rather than a dramatic color.
     * - Each cell is mapped linearly: t = value / maxValue, blending from the derived min color
     *   to the user-configurable max color (@c periodHeatmapIncomeToColor / @c periodHeatmapExpenseToColor).
     * - A cell whose relevant value is zero (no income, or no expense this month) is shown as
     *   "no events" even if the other type has a value.
     *
     * **Delta mode** (diverging color scheme):
     * - The neutral center (delta == 0) is a theme-aware color: pure black in dark mode,
     *   mid-gray (128,128,128) in light mode.
     * - Positive and negative sides are normalized independently: each side's largest absolute
     *   value maps to t=1 (full saturation of its color), so both extremes always reach their
     *   respective configured colors (@c periodHeatmapDeltaToColor for positive,
     *   @c periodHeatmapDeltaFromColor for negative).
     *
     * **Cell special states:**
     * - Outside the scenario period (key absent from @c binsMonthly): backward-diagonal hatch
     *   (\\), subtle contrast, theme-aware gray tones.
     * - Month present in bins but with zero income AND zero expense: forward-diagonal hatch (/),
     *   stronger contrast, distinct from the out-of-period pattern.
     *
     * **Legend:** the bottom bar shows colored swatches for the actual min and max values in the
     * current mode, plus hatched swatches for "No events" and "Outside period". All legend
     * widgets are hidden when @c binsMonthly is empty.
     *
     * @note Must be called after @c binsMonthly is populated and colors are initialized.
     * Typically triggered by mode changes, color picker changes, and theme changes.
     */
    void periodHeatmapRedisplayTable();

    /**
     * @brief Apply a solid background color to a color-picker push button.
     * @param btn The button to style.
     * @param color The color to display.
     */
    void periodHeatmapSetColorButtonStyle(QPushButton* btn, QColor color);

    /**
     * @brief Refresh the visibility and style of the min/max color controls in the toolbar.
     * @details The min-color label, picker and reset button are only shown in delta mode.
     * Also sizes the reset buttons to a font-metric-derived square and updates button colors
     * to reflect the current per-mode color settings.
     */
    void periodHeatmapUpdateColorButtons();

    /**
     * @brief Build the X-axis category label of a bar chart for a given period and date.
     * @details For monthly granularity, returns a compact @c "M-YY" string (e.g. @c "3-25"
     * for March 2025). For yearly granularity, returns the full four-digit year string.
     * @param type Granularity of the chart (monthly or yearly).
     * @param date The date representing the start of the bin period.
     * @return The formatted category label string.
     */
    QString periodChartBuildCategoryName(PeriodType type, QDate date) const;

    /**
     * @brief Create and initialize the QChart and QChartView for the given granularity.
     * @details Allocates the QChart, QChartView, QBarCategoryAxis, and QValueAxis for the
     * monthly or yearly chart; sets up series animations, legend, axis fonts, tick counts,
     * and the initial theme. The chart view is embedded in periodChartWidget, and only the
     * monthly view is made visible initially. Must be called once at startup for each
     * granularity before any data is displayed.
     * @param type Granularity of the chart to initialize (monthly or yearly).
     */
    void periodChartInitChart(PeriodType type);

    /**
     * @brief Compute the start and end dates of the visible range for the Period Chart.
     * @details Derives both dates from the current UI controls: the from-year spin box and,
     * for monthly granularity, the from-month combo box define the start date; the duration
     * spin box defines how many months or years to advance to reach the end date.
     * @param type Granularity of the chart (monthly or yearly).
     * @param startDate Output: first date of the visible range.
     * @param endDate Output: last date of the visible range.
     */
    void periodChartGetStartEndDates(PeriodType type, QDate& startDate, QDate& endDate) const;
    void periodChartUpdateNavButtons(PeriodType type);

    /**
     * @brief Compute the Y-axis min and max values across the bar sets relevant to the
     * currently selected data source.
     * @details Inspects only the bar sets that correspond to the active PeriodChartSourceType:
     * income and/or expense sets for their maximum, and the positive/negative delta sets for
     * both maximum and minimum. Sets not involved in the current view are ignored. Both
     * outputs are initialised to 0/1 before scanning so the axis always has a valid range
     * even when all values are zero.
     * @param type Granularity of the chart (monthly or yearly), used to resolve the active
     * data source via periodChartFindWhichSetIsToBeUsed().
     * @param incomes Bar set of income values.
     * @param expenses Bar set of expense values.
     * @param deltasPositive Bar set of positive delta (surplus) values.
     * @param deltasNegative Bar set of negative delta (deficit) values.
     * @param minY Output: minimum Y value to use for the axis range.
     * @param maxY Output: maximum Y value to use for the axis range.
     */
    void periodChartGetMinMax(PeriodType type, QBarSet* incomes, QBarSet* expenses,
        QBarSet* deltasPositive, QBarSet* deltasNegative, double& minY, double& maxY);

    /**
     * @brief Determine which data source is currently selected in the Period Chart UI.
     * @details Reads the state of the four chart-type radio buttons and returns the
     * corresponding PeriodChartSourceType value. Throws std::logic_error if none of
     * the radio buttons is checked, which should never happen in normal operation.
     * @param type Unused; reserved for future per-granularity source selection.
     * @return The active PeriodChartSourceType (DST_INCOME, DST_EXPENSE,
     * DST_INCOME_EXPENSE, or DST_DELTA).
     */
    AnalysisDialog::PeriodChartSourceType periodChartFindWhichSetIsToBeUsed(PeriodType type);

    /**
     * @brief Return every calendar day covered by the Period List bin that contains the given date.
     * @details For monthly granularity, enumerates all days from the first to the last day
     * of the month of @p binDate. For yearly granularity, enumerates all days from
     * January 1st to December 31st of the year of @p binDate.
     * @param type Granularity of the bin (monthly or yearly).
     * @param binDate Any date within the target bin; only its month and/or year are used.
     * @return Ordered list of all QDate values within the bin period.
     */
    QList<QDate> periodListGetListOfDatesCoveredbyBin(PeriodType type, QDate binDate);

    /**
     * @brief Return the grouping type currently selected in the Relative Weight tab.
     * @details Reads the state of the incomes/expenses radio buttons and returns the
     * corresponding RelWeightGrouping value. Throws std::logic_error if neither button
     * is checked, which should never happen in normal operation.
     * @return RWGROUPING_INCOMES or RWGROUPING_EXPENSES.
     */
    RelWeightGrouping rwGetGroupingTypeSelected();


    // Tags

    /**
     * @brief Completely rebuild the Tags data and redisplay the Tags table.
     * @details Validates the from/to date range from the UI, then calls tags_rebuildData()
     * to recompute per-tag contributions via tagsRebuildData(), and repopulates tagsTableWidget with the
     * resulting name, amount, weight, and count for each tag.
     */
    void tagsRedisplayTable();

    /**
     * @brief Recompute per-tag contribution data from raw streams for the given date range.
     * @details Initialises an entry in @p data for each currently selected tag, then walks
     * the CombinedFeStreams daily records within [@p from, @p to], resolving CSD-to-tag
     * relationships and accumulating amount, weight, and count per tag. Income or expense
     * streams are processed depending on the active radio button. The Tags table widget is
     * NOT updated by this method; @p data is cleared before being populated.
     * @param from Start of the date range (inclusive).
     * @param to End of the date range (inclusive).
     * @param data Output hash keyed by tag UUID; cleared and repopulated by this call.
     * @param totalWithoutTags Output: total income (>0) or expense (<0) for the period,
     * computed independently of tag filtering.
     */
    void tagsRebuildData(QDate from, QDate to, QHash<QUuid,TagContribution>& data,
        double &totalWithoutTags);

    /**
     * @brief Update the "N selected" label in the Tags tab to reflect the current
     * number of selected tags.
     */
    void tagsUpdateNoTagsSelected();

    /**
     * @brief Export the Tags analysis data to a CSV file.
     * @details Builds column definitions for tag name, total amount, weight (%), and count;
     * populates rows from the current tagsTableData; then calls CsvExporter::exportToCsv()
     * and handles the result (success, cancellation, or error).
     */
    void tagsExportToCsvFile();

    // CSD Comparison tab

    void csdInitChart();
    void csdThemeChanged();
    void csdRedisplayList();
    void csdRedisplayChart();
    void csdRescaleChart();
    void csdPointClicked(QScatterSeries* series, const QPointF& pt);
    void csdClearSelectedPoint();

};

#endif // ANALYSISDIALOG_H
