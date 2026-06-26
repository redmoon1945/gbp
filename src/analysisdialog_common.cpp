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

#include "analysisdialog.h"
#include "customqchartview.h"
#include <QButtonGroup>
#include <QTimer>
#include "qgraphicslayout.h"
#include "ui_analysisdialog.h"
#include "gbpcontroller.h"
#include "gbplogger.h"
#include "util.h"
#include "uiutil.h"
#include <QChart>
#include <QPieSeries>
#include <QFileDialog>
#include <QMessageBox>
#include <QLegendMarker>
#include <QStringView>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include "gbpqmessage.h"


AnalysisDialog::AnalysisDialog(QLocale theLocale, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AnalysisDialog)
{
    ui->setupUi(this);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);

    locale = theLocale;

    QFont font ;
    QFont appFont = QApplication::font();


    // *** RELATIVE WEIGHT CONTROLS ***

    seriesRelativeWeigth = new QPieSeries;
    chartRelativeWeight = new QChart;
    chartRelativeWeight->addSeries(seriesRelativeWeigth); // take ownership
    chartRelativeWeight->setAnimationOptions(QChart::AllAnimations);
    chartRelativeWeight->legend()->hide();
    chartRelativeWeight->legend()->setAlignment(Qt::AlignRight);
    chartViewRelativeWeigth = new QChartView(chartRelativeWeight, ui->chartRelativeWeigthWidget);
    chartViewRelativeWeigth->setRenderHint(QPainter::Antialiasing);
    chartRelativeWeight->layout()->setContentsMargins(1, 1, 1, 1);
    chartRelativeWeight->setBackgroundRoundness(0);


    // Must have as many colors as max no of elements + 1., that is *** 26 ***
    // These colors are optimized.
    colorsRelativeWeigth = {
        QColor(228,26,28),    // red
        QColor(55,126,184),   // blue
        QColor(77,175,74),    // green
        QColor(152,78,163),   // purple
        QColor(255,127,0),    // orange
        QColor(255,255,51),   // yellow
        QColor(247,129,191),  // pink
        QColor(0,191,255),    // deep sky blue
        QColor(46,139,87),    // sea green
        QColor(255,69,0),     // red-orange
        QColor(138,43,226),   // blue violet
        QColor(60,179,113),   // medium sea green
        QColor(210,105,30),   // chocolate
        QColor(0,128,128),    // teal
        QColor(255,20,147),   // deep pink
        QColor(70,130,180),   // steel blue
        QColor(199,21,133),   // medium violet red
        QColor(218,165,32),   // goldenrod
        QColor(0,100,0),      // dark green
        QColor(123,104,238),  // medium slate blue
        QColor(139,69,19),    // saddle brown
        QColor(127,255,212),  // aquamarine
        QColor(244,164,96),   // sandy brown
        QColor(32,178,170),   // light sea green
        QColor(186,85,211),   // orchid
        QColor(0,206,209)     // dark turquoise
    };

    if(GbpController::getInstance().useDarkModeForChart()==true){
        chartRelativeWeight->setTheme(QChart::ChartThemeDark);
    } else {
        chartRelativeWeight->setTheme(QChart::ChartThemeLight);
    }

    // set current tab : make sure Relative Weight Pie chart is shown first
    ui->tabWidget->setCurrentIndex(0);
    ui->globalExportCsvPushButton->setVisible(true);
    ui->globalExportImagePushButton->setVisible(true);

    // Set global from/to dates (shared across all applicable tabs)
    QDate tomorrow = GbpController::getInstance().getTomorrow();
    ui->globalFromDateEdit->setDate(tomorrow);
    globalPreviousFromDate = tomorrow;
    ui->globalToDateEdit->setDate(tomorrow.addYears(5).addDays(-1));
    globalPreviousToDate = tomorrow.addYears(5).addDays(-1);

    // Widen global date widgets
    QFontMetrics fm = ui->globalFromDateEdit->fontMetrics();
    ui->globalFromDateEdit->setMinimumWidth(fm.averageCharWidth()*20);
    ui->globalToDateEdit->setMinimumWidth(fm.averageCharWidth()*20);

    // misc init
    ui->noElementsLabel->setText(tr("No of most significant items :"));
    ui->rwNoElementsSpinBox->setValue(10);
    ui->chartRelativeWeigthWidget->installEventFilter(this);

    // Set color of "incomes" and "expenses" radio buttons
    ui->rwIncomesRadioButton->setStyleSheet(Util::getStyleSheetStringForColor(
        GbpController::getInstance().getIncomeColor()));
    ui->rwExpensesRadioButton->setStyleSheet(Util::getStyleSheetStringForColor(
        GbpController::getInstance().getExpenseColor()));

    // Set smaller font for list box
    QFont rwListFont = appFont;
    Util::changeFontSize(rwListFont, Util::FontResizeIntensity::AVERAGE, true,
        "Analysis Relative weight - list box");
    ui->rwListWidget->setFont(rwListFont);

    // Set the amount string to be able to display max number without streching the Dialog
    // At this time, we dont have access to a currency, so use the max amount of decimal.
    int numChars = CurrencyHelper::maxCharForMaxAmountInDouble(
        CurrencyHelper::maxValueAllowedForNoOfDecimalsForCurrency());
    QFontMetrics fmAmount(appFont);
    int widthAmount = fm.horizontalAdvance(QString(numChars*1.5, '8'));
    ui->rwAmountLabel->setMinimumWidth(widthAmount);


    // *** PERIOD - CHART ***

    periodChartInitChart(PeriodType::MONTHLY);
    // make smaller stats font
    font = appFont;
    Util::changeFontSize(font, Util::FontResizeIntensity::AVERAGE, true,
        "Analysis Period chart - stats");
    ui->periodChartStatsLabel->setFont(font);

    periodChartInitChart(PeriodType::YEARLY);


    // *** PERIOD - LIST ***

    // Explicitly set the app font on the table widget. On Windows the platform theme
    // registers a class-specific font for QAbstractItemView (icon-title font, Segoe UI 9pt)
    // that overrides the application font for cell text rendering. The global override in
    // main.cpp covers most item views, but the explicit setFont here is the reliable
    // fallback for this specific widget.
    ui->detailsReportTableWidget->setFont(appFont);
    ui->detailsReportTableWidget->setColumnCount(8);
    ui->detailsReportTableWidget->setHorizontalHeaderLabels({tr("Month"),tr("Incomes"),
        tr("Δ Incomes (%)"), tr("Expenses"),tr("Δ Expenses (%)"),
        tr("Deltas"), tr("Δ Deltas (%)"), tr("Cash balance")});
    int maxNumCharsCurrency = 10;
    ui->detailsReportTableWidget->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Interactive);
    QFontMetrics fmTable = QFontMetrics(appFont);
    widthAmount = fmTable.horizontalAdvance(QString(9*1.4, '8'));
    ui->detailsReportTableWidget->setColumnWidth(0, widthAmount); // month/year
    widthAmount = fmTable.horizontalAdvance(QString(maxNumCharsCurrency*1.4, '8'));
    ui->detailsReportTableWidget->setColumnWidth(1, widthAmount); // income
    widthAmount = fmTable.horizontalAdvance(QString(7*1.4, '8'));
    ui->detailsReportTableWidget->setColumnWidth(2, widthAmount); // income growth
    widthAmount = fmTable.horizontalAdvance(QString(maxNumCharsCurrency*1.4, '8'));
    ui->detailsReportTableWidget->setColumnWidth(3, widthAmount); // expense
    widthAmount = fmTable.horizontalAdvance(QString(7*1.4, '8'));
    ui->detailsReportTableWidget->setColumnWidth(4, widthAmount); // expense growth
    widthAmount = fmTable.horizontalAdvance(QString(maxNumCharsCurrency*1.4, '8'));
    ui->detailsReportTableWidget->setColumnWidth(5, widthAmount); // delta
    widthAmount = fmTable.horizontalAdvance(QString(7*1.4, '8'));
    ui->detailsReportTableWidget->setColumnWidth(6, widthAmount); // delta growth
    widthAmount = fmTable.horizontalAdvance(QString(maxNumCharsCurrency*1.4, '8'));
    ui->detailsReportTableWidget->setColumnWidth(7, widthAmount); // cash balance
    ui->detailsReportTableWidget->setSortingEnabled(false);
    ui->detailsReportTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // Horizontal header: minimum height derived from font so text is never clipped when the
    // app font is larger than Qt's built-in default header height.
    ui->detailsReportTableWidget->horizontalHeader()->setFont(appFont);
    ui->detailsReportTableWidget->horizontalHeader()->setMinimumHeight(fmTable.height() + 10);
    ui->detailsReportTableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    ui->detailsReportTableWidget->verticalHeader()->hide();


    // *** TAGS ***

    // set some variables
    availableTags.clear();
    selectedTags.clear();
    ui->tagsTableWidget->setFont(appFont);
    ui->tagsTableWidget->setColumnCount(4); // tag name, amount, weight in percentage, count
    QStringList tagTableHeaders ={tr("Tag's name"), tr("Total amount"), tr("Weight (%1)").arg("%"),
        tr("Count")};
    QStringList tagTableHeadersTooltips = {"","",
        tr("<html>For this period of time, percentage of total amount "
           "for Csds associated with this tag, "
           "relative to the total amount for all Csds regardless of tags.</html>"),
        tr("<html>Number of individual financial events contributing to this tag.</html>")};
    for (int col = 0; col < 4; ++col) {
        QTableWidgetItem *headerItem = new QTableWidgetItem(tagTableHeaders[col]);
        if(col==2 || col==3){
            headerItem->setToolTip(tagTableHeadersTooltips[col]);
        }
        ui->tagsTableWidget->setHorizontalHeaderItem(col, headerItem);
    }
    ui->tagsTableWidget->setSortingEnabled(true);

    // Set color of "incomes" and "expenses" radio buttons
    ui->tagsIncomesRadioButton->setStyleSheet(Util::getStyleSheetStringForColor(
        GbpController::getInstance().getIncomeColor()));
    ui->tagsExpensesRadioButton->setStyleSheet(Util::getStyleSheetStringForColor(
        GbpController::getInstance().getExpenseColor()));

    // resize column of tag table
    //ui->tagsTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    QHeaderView *header = ui->tagsTableWidget->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch); // Column 0: stretch
    QFontMetrics fm2 = ui->tagsTableWidget->fontMetrics();
    header->setSectionResizeMode(1, QHeaderView::Fixed);
    ui->tagsTableWidget->setColumnWidth(1, fm2.averageCharWidth()*20);
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->tagsTableWidget->setColumnWidth(2, fm2.averageCharWidth()*20);
    header->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->tagsTableWidget->setColumnWidth(3, fm2.averageCharWidth()*12);
    // Horizontal header: minimum height derived from font so text is never clipped on Windows.
    header->setFont(appFont);
    header->setMinimumHeight(fm2.height() + 10);
    ui->tagsTableWidget->verticalHeader()->hide();

    // Force tag table to be initially sorted by weigth (descending)
    ui->tagsTableWidget->sortItems(2, Qt::DescendingOrder);

    // Set tags no of label to have smaller fonts and italic
    font = appFont;
    Util::changeFontSize(font, Util::FontResizeIntensity::WEAK, true,
        "Analysis Relative weight - No of tags");
    font.setItalic(true);
    ui->tags_noTagsSelectedLabel->setFont(font);

    // Init total amount to 0
    ui->tags_totalAmountLabel->setText("");

    // *** CSD COMPARISON ***

    ui->csdIncomesRadioButton->setStyleSheet(Util::getStyleSheetStringForColor(
        GbpController::getInstance().getIncomeColor()));
    ui->csdExpensesRadioButton->setStyleSheet(Util::getStyleSheetStringForColor(
        GbpController::getInstance().getExpenseColor()));

    ui->csdSplitter->setSizes(QList<int>({INT_MAX / 4, (INT_MAX / 4) * 3}));
    {
        ui->csdListWidget->setFont(appFont);
        int sz = ui->csdListWidget->fontMetrics().height();
        ui->csdListWidget->setIconSize(QSize(sz, sz));
    }
    ui->csdChartWidget->installEventFilter(this);
    csdInitChart();

    // *** HELP ***
    ui->helpTextEdit->setHtml(tr(
        "<h2>Analysis — Help</h2>"

        "<h3>Global date range</h3>"
        "<p>The <b>From</b> and <b>To</b> date controls at the top of the dialog define a "
        "shared analysis period. Press <b>Apply dates</b> to refresh all tabs that use "
        "this range: <i>Relative Weight</i>, <i>Period — Chart</i>, <i>Tags</i>, and "
        "<i>Compare CSD</i>. The <i>Period — List</i> and <i>Period — Heatmap</i> tabs always "
        "show the full scenario range and are unaffected.</p>"

        "<h3>Relative Weight</h3>"
        "<p>Shows how much each active Cash Stream Definition (CSD) contributes to your "
        "total income or expenses as a pie chart.</p>"
        "<ul>"
        "<li>Choose <b>Income</b> or <b>Expenses</b> mode with the radio buttons.</li>"
        "<li>Only financial events falling within the global <b>From / To</b> date range "
        "are counted. Press <b>Apply dates</b> at the top to refresh.</li>"
        "<li>Click a pie slice to select it: the rank, amount, and percentage of the "
        "corresponding CSD are highlighted in the legend below the chart.</li>"
        "<li>The <b>N elements</b> spinbox limits how many CSDs are shown; smaller CSDs "
        "are grouped into an &ldquo;Others&rdquo; slice.</li>"
        "<li>Use <b>Export legend…</b> to save the full ranked list (name, amount, "
        "percentage) as a CSV file.</li>"
        "</ul>"

        "<h3>Period — Chart</h3>"
        "<p>Bar chart that aggregates financial data by month or by year, letting you "
        "spot trends and seasonal patterns over time.</p>"
        "<ul>"
        "<li>Switch between <b>Monthly</b> and <b>Annual</b> granularity with the radio "
        "buttons at the top.</li>"
        "<li>Choose what to display: <b>Income</b>, <b>Expenses</b>, "
        "<b>Income + Expenses</b> (both bar sets side by side), or <b>Δ</b> "
        "(income minus expenses — positive bars are surplus, negative bars are deficit).</li>"
        "<li>The visible window is defined by the global <b>From / To</b> date range. "
        "The day of the month is ignored: the window snaps to the first day of the "
        "<i>From</i> month/year and the first day of the <i>To</i> month/year.</li>"
        "<li>Click a bar to see its exact value displayed below the chart.</li>"
        "<li>Use <b>Export image…</b> to save the chart as a PNG file.</li>"
        "</ul>"

        "<h3>Period — List</h3>"
        "<p>Tabular summary of income, expenses, Δ, and running cash balance for every "
        "month or year covered by the scenario. The full scenario range is always shown; "
        "the global date range has no effect on this tab.</p>"
        "<ul>"
        "<li>Switch between <b>Monthly</b> and <b>Annual</b> granularity.</li>"
        "<li>Columns: period, income, income change %, expenses, expense change %, "
        "Δ (income &minus; expenses), Δ change %, and end-of-period cash balance. "
        "Change percentages are relative to the previous period.</li>"
        "<li>The cash balance column uses the <b>starting amount</b> entered in the main "
        "window as its initial value.</li>"
        "<li>Click any column header to sort ascending or descending.</li>"
        "<li>Use <b>Export table…</b> to save the full table as a CSV file.</li>"
        "</ul>"

        "<h3>Period — Heatmap</h3>"
        "<p>Color-coded grid that encodes financial intensity, making it easy to identify "
        "the busiest or most profitable periods at a glance. The full scenario range is "
        "always shown; the global date range has no effect on this tab.</p>"
        "<ul>"
        "<li>Two display granularities: <b>Monthly</b> (years as rows, January–December "
        "as columns) and <b>Yearly</b> (years arranged left-to-right in a fixed "
        "10-column grid).</li>"
        "<li>Three display modes: <b>Income</b>, <b>Expenses</b>, and <b>Δ</b>.<br>"
        "Income/Expenses use a sequential palette: the lightest tint represents the "
        "smallest non-zero value, the fully saturated color represents the maximum.<br>"
        "Δ uses a diverging palette: a neutral center color for zero (breakeven), "
        "the <i>From</i> color for the most negative period, and the <i>To</i> color "
        "for the most positive period.</li>"
        "<li>Special cell states:<br>"
        "Periods <b>outside the scenario range</b> are shown with a <b>light gray</b> "
        "background and no text.<br>"
        "Periods <b>within the scenario but with no financial events</b> are shown with "
        "a <b>medium gray</b> background and a centered &bull;.<br>"
        "In Δ mode, a period where income exactly equals expenses is shown in the "
        "neutral center color (black by default).</li>"
        "<li>Use the color-picker buttons to customize the extreme colors. "
        "The <b>From</b> color (negative extreme, Δ mode only) and <b>To</b> color "
        "(maximum) can each be reset to their defaults.</li>"
        "<li>The <b>Cell size</b> combo adjusts the grid density for readability.</li>"
        "<li>Check <b>Exclude current month / year</b> (label adapts to the active "
        "granularity) to omit the partially-elapsed current period from the color "
        "scale calculation.</li>"
        "</ul>"

        "<h3>Tags</h3>"
        "<p>Breaks down total income or expenses by user-defined tags, showing how much "
        "each tag contributes to the overall total over a given period.</p>"
        "<ul>"
        "<li>Choose <b>Income</b> or <b>Expenses</b> mode.</li>"
        "<li>Only financial events falling within the global <b>From / To</b> date range "
        "are counted. Press <b>Apply dates</b> at the top to refresh.</li>"
        "<li>Click <b>Select tags…</b> to choose which tags to include in the analysis. "
        "The label shows how many tags are currently selected.</li>"
        "<li>The table lists each tag with its total <b>amount</b>, <b>weight</b> "
        "(percentage of the period total), and <b>event count</b> (number of individual "
        "financial events carrying that tag).</li>"
        "<li>A CSD can contribute to a tag even if the tag is not its primary tag, "
        "as long as the tag relationship is defined.</li>"
        "<li>Use <b>Export table…</b> to save the results as a CSV file.</li>"
        "</ul>"

        "<h3>Compare CSD</h3>"
        "<p>Plots the individual (non-cumulative) cashflow events of up to 10 Cash Stream "
        "Definitions side by side as step curves, making it easy to compare their timing "
        "and amounts.</p>"
        "<ul>"
        "<li>Choose <b>Income</b> or <b>Expenses</b> mode. Only active CSDs of the "
        "selected type appear in the list.</li>"
        "<li>Only financial events falling within the global <b>From / To</b> date range "
        "are shown. Press <b>Apply dates</b> at the top to refresh.</li>"
        "<li>Check up to <b>10 CSDs</b> from the list on the left. Each checked CSD is "
        "assigned a distinct color that it keeps for as long as it remains selected; "
        "a small colored square appears next to its name. "
        "Checking an eleventh CSD has no effect.</li>"
        "<li>Press <b>Unselect all</b> to clear all checkmarks and reset all color "
        "assignments at once.</li>"
        "<li>The chart shows each event as a horizontal step: the curve holds its value "
        "until the next event occurs, making gaps and clusters immediately visible.</li>"
        "<li>Whenever the selection or the Income/Expenses mode changes, the axes are "
        "automatically rescaled to fit the new data. Press <b>Fit</b> at any time to "
        "force the same reset manually.</li>"
        "<li>The chart supports <b>pan</b> (click and drag) and <b>zoom</b> "
        "(mouse wheel, Shift = horizontal only, Ctrl = vertical only).</li>"
        "<li>Click a data point to highlight it and display its date and value below "
        "the chart. Click the same point again to deselect it.</li>"
        "<li>Income/Expenses selection and individual checkmarks are preserved when you "
        "switch between the two modes; switching back to a mode restores its previous "
        "selection.</li>"
        "<li>Use <b>Export image…</b> to save the chart as a PNG file.</li>"
        "</ul>"
    ));

    // Set focus on "Close" button
    ui->closePushButton->setDefault(true);

    // *** PERIOD - HEATMAP defaults (set once; preserved across dialog re-opens) ***
    ui->periodHeatmapMonthlyRadioButton->setChecked(true);
    ui->periodHeatmapExcludeCurrentCheckBox->setText(tr("Exclude current month"));

    // *** Dialog children and connections ***

    // Create Dialog for selecting tags
    selectTagsDlg = new ChooseTagsDialog(this);
    selectTagsDlg->setModal(true);

    // connect emitters & receivers for Dialogs : choose Tags
    QObject::connect(this, &AnalysisDialog::signalChooseTagsPrepareContent, selectTagsDlg,
        &ChooseTagsDialog::slotPrepareContent);
    QObject::connect(selectTagsDlg, &ChooseTagsDialog::signalResult, this,
        &AnalysisDialog::slotChooseTagsResult);
    QObject::connect(selectTagsDlg, &ChooseTagsDialog::signalCompleted, this,
        &::AnalysisDialog::slotChooseTagsCompleted);

    ready = true;
}


AnalysisDialog::~AnalysisDialog()
{
    delete ui;
}


void AnalysisDialog::slotAnalysisPrepareContent( QWeakPointer<CombinedFeStreams> chartRawDataRef,
    Tags availabletags, CurrencyInfo currencyInfo, double startingAmount)
{
    this->chartRawDataRef = chartRawDataRef;
    this->currInfo = currencyInfo;
    this->startingAmount = startingAmount;

    // Enforce the "to" date upper bound: tomorrow + scenario duration - 1 day.
    // Done here (not in the constructor) because the scenario is guaranteed loaded at this point.
    {
        QSharedPointer<Scenario> scenario =
            GbpController::getInstance().getScenario().toStrongRef();
        if (!scenario.isNull()) {
            QDate tomorrow = GbpController::getInstance().getTomorrow();
            QDate maxToDate =
                tomorrow.addYears(scenario->getFeGenerationDuration()).addDays(-1);
            ui->globalToDateEdit->setMaximumDate(maxToDate);
            if (globalPreviousToDate > maxToDate) {
                globalPreviousToDate = maxToDate;
                ui->globalToDateEdit->setDate(maxToDate);
            }
        }
    }

    // *** RELATIVE WEIGTH ***
    rwUpdateChart();

    // *** PERIOD - LIST ***

    // calculate data : will be used by tables and charts.
    recalculate_BinsData(PeriodType::MONTHLY, ui->detailsReportTableWidget);
    recalculate_BinsData(PeriodType::YEARLY, ui->detailsReportTableWidget);
    // update report table for the currently selected granularity
    periodListRedisplayTableData(
        ui->periodListMonthlyRadioButton->isChecked()
            ? PeriodType::MONTHLY : PeriodType::YEARLY);
    // restore visibility based on current mode selection (preserves user's last choice)
    bool isMonthly = ui->periodChartMonthlyRadioButton->isChecked();
    if (chartViewMonthlyReport) chartViewMonthlyReport->setVisible(isMonthly);
    if (chartViewYearlyReport)  chartViewYearlyReport->setVisible(!isMonthly);
    periodChartOffset = 0;
    periodChartRedisplayChart(isMonthly ? PeriodType::MONTHLY : PeriodType::YEARLY);

    // *** PERIOD - HEATMAP ***
    {
        if (!periodHeatmapIncomeToColor.isValid())  periodHeatmapIncomeToColor  = Qt::green;
        if (!periodHeatmapExpenseToColor.isValid()) periodHeatmapExpenseToColor = Qt::red;
        if (!periodHeatmapDeltaFromColor.isValid()) periodHeatmapDeltaFromColor = Qt::red;
        if (!periodHeatmapDeltaToColor.isValid())   periodHeatmapDeltaToColor   = Qt::green;
    }
    periodHeatmapUpdateColorButtons();
    periodHeatmapRedisplayTable();

    // *** TAGS ***

    this->availableTags = availabletags;

    // Try to conserve the tags already selected from previous invocation of the dialog.
    // Scenario or tags set may have changed. Otherwise, we take all the tag available
    QSet<QUuid> formerSet = selectedTags.getFilterTagIdSet();
    availableTags.cleanIdList(formerSet);
    selectedTags.setFilterTagIdSet(formerSet);
    if (formerSet.size()==0) {
        // scenario may have changed or tags modified in the current scenario. In that case,
        // we select all the tags for convenience.
        selectedTags.setFilterTagIdSet(availableTags.getTagIdSetAsQset());
    }

    // Update no of tags selected
    tagsUpdateNoTagsSelected();

    // Rebuild data and update tag table
    tagsRedisplayTable();

    // *** CSD COMPARISON ***
    csdCheckedIncomeIds.clear();
    csdCheckedExpenseIds.clear();
    ui->csdListWidget->clear();
    csdRedisplayList();

    // Set focus on Close button
    ui->closePushButton->setFocus();

    LOG_DEBUG_INFO(QString("Analysis dialog invoked"));
}


void AnalysisDialog::slotChooseTagsResult(QSet<QUuid> chosenTags)
{
    selectedTags.setFilterTagIdSet(chosenTags);

    // Update no of tags selected
    tagsUpdateNoTagsSelected();

    // Rebuild the data and update the tag chart
    tagsRedisplayTable();
}


void AnalysisDialog::slotChooseTagsCompleted(bool canceled)
{

}


void AnalysisDialog::themeChanged()
{
    if(GbpController::getInstance().useDarkModeForChart()==true){
        chartMonthlyReport->setTheme(QChart::ChartThemeDark);
        chartYearlyReport->setTheme(QChart::ChartThemeDark);
        chartRelativeWeight->setTheme(QChart::ChartThemeDark);
    } else {
        chartMonthlyReport->setTheme(QChart::ChartThemeLight);
        chartYearlyReport->setTheme(QChart::ChartThemeLight);
        chartRelativeWeight->setTheme(QChart::ChartThemeLight);
    }
    if (!binsMonthly.isEmpty()) {
        periodHeatmapUpdateColorButtons();
        periodHeatmapRedisplayTable();
    }
    csdThemeChanged();
}


bool AnalysisDialog::eventFilter(QObject *object, QEvent *event)
{
    if ( (event->type() == QEvent::Resize) && (object == ui->chartRelativeWeigthWidget) ){
        chartViewRelativeWeigth->resize(ui->chartRelativeWeigthWidget->size());
    }
    if ( (event->type() == QEvent::Resize) && (object == ui->periodChartWidget) ){
        chartViewMonthlyReport->resize(ui->periodChartWidget->size());
    }
    if ( (event->type() == QEvent::Resize) && (object == ui->periodChartWidget)){
        chartViewYearlyReport->resize(ui->periodChartWidget->size());
    }
    if ( (event->type() == QEvent::Resize) && (object == ui->csdChartWidget) ) {
        if (csdChartView) csdChartView->resize(ui->csdChartWidget->size());
    }

    return QObject::eventFilter(object, event);
}


void AnalysisDialog::on_closePushButton_clicked()
{
    hide();
    // clear some now useless data, since they will be rebuild when the form is shown again.
    binsMonthly = {};
    binsYearly = {};
    availableTags.clear();
    tagsTableData.clear();
    chartRelativeWeight->removeAllSeries();
    seriesRelativeWeigth = nullptr; // rwUpdateChart() recreates it on next open
    chartMonthlyReport->removeAllSeries();
    chartYearlyReport->removeAllSeries();
    if (csdChart) csdChart->removeAllSeries();
    csdLastSelectedSeries = nullptr;
    csdLastSelectedIndex  = -1;
    ui->detailsReportTableWidget->setRowCount(0);
    ui->periodHeatmapTableWidget->clearContents();
    ui->periodHeatmapTableWidget->setRowCount(0);

    LOG_DEBUG_INFO(QString("Analysis dialog closed"));
}


void AnalysisDialog::on_AnalysisDialog_rejected()
{
    on_closePushButton_clicked();
}


void AnalysisDialog::exportChartAsImage(QWidget* chartWidget, QString desc)
{
    LOG_INFO("Initiating chart export in PNG format : "+desc);

    // get file name
    QString defaultExtensionUsed = "PNG files (*.png *.PNG)";
    QString filter = tr("PNG files (*.png *.PNG);;All files (*)");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Select an image file"),
        GbpController::getInstance().getLastDirExport(), filter, &defaultExtensionUsed);
    if (fileName != ""){
        // fix the filename to add the proper suffix
        QFileInfo fi(fileName);
        if(fi.suffix()==""){    // user has not specified an extension
            fileName.append(".png");
        }
        GbpController::getInstance().setLastDirExport(fi.absolutePath());

        bool successful;
        successful = chartWidget->grab().save(fileName,"PNG", 100) ;  // max quality
        if(successful == false){
            GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
                tr("Export failed. The creation of the image file did not succeed"), {tr("OK")}, 0, 0);
            GbpLogger::getInstance().logError(QString("Export failed. The creation of the image "
                "file %1 did not succeed").arg(REDACT(fileName)));
            return;
        }
        LOG_INFO(QString("Export to file %1 has succeeded")
            .arg(REDACT(fileName)));
    } else{
        LOG_INFO("Export canceled by user");
    }
}


void AnalysisDialog::recalculate_BinsData(PeriodType rTypr,
    QTableWidget* tableWidget)
{
    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario.isNull()){
        return; // if no scenario loaded (should not happen)
    }

    // Make sure the raw data is available (which should always be the case)
    QSharedPointer<CombinedFeStreams> chartRawData = chartRawDataRef.toStrongRef();
    if(chartRawData.isNull()){
        return; // should never happen
    }

    // Build a generic ptr to bin set according to rTypr (less code to duplicate)
    QMap<QDate,Bin>* binsPtr;
    if (rTypr==PeriodType::MONTHLY) {
        binsPtr = &binsMonthly;
    } else {
        binsPtr = &binsYearly;
    }

    // Reset bin container set to empty
    (*binsPtr).clear();

    // Init and resize bins data for maximum range , that is from "tomorrow" to
    // "max scenario limit", irrespective of the span of rawdata we have. Each period init has
    // an entry. The "key" dates corresponds to the beginning of a bin and act as an identifier
    // for this bin.
    QDate tomorrow = GbpController::getInstance().getTomorrow();
    qint64 tomorrowJulianDay = tomorrow.toJulianDay();
    QDate to = tomorrow.addYears(scenario->getFeGenerationDuration()).addDays(-1);
    QDate toMonthYear = QDate(to.year(), (rTypr==PeriodType::MONTHLY)?(to.month()):(1), 1);
    QDate date = QDate(tomorrow.year(), (rTypr==PeriodType::MONTHLY)?(tomorrow.month()):(1), 1);
    while( date <= toMonthYear ){
        (*binsPtr).insert(date, Bin());
        if(rTypr==PeriodType::MONTHLY){
            date = date.addMonths(1);
        } else {
            date = date.addYears(1);
        }
    }

    double eopCashBalance = startingAmount; // cumulative cash balance at the end of a period

    // For each and every bin, add the contribution of all raw data elements fitting this bin.
    // Order of iteration must be asc. date, to correctly calculate cumulative cash balance
    // The growth element will be calculated in a second pass.
    QList<QDate> binsKeys = (*binsPtr).keys(); // keys are placed in ascending order
    QList<CombinedFeStreams::DailyInfo> listDi = chartRawData->getCombinedStreams();
    foreach(QDate binDate, binsKeys){
        Bin binData = Bin();
        binData.cashBalance = eopCashBalance;
        // Get the list of all dates included in this bin
        QList<QDate> dateList = periodListGetListOfDatesCoveredbyBin(rTypr, binDate);

        // Check if raw data exist for each date. Order of iteration must be asc. date,
        // to correctly calculate cumulative cash balance If yes, add to contribution
        // and update eopCashBalance
        foreach(QDate d, dateList){
            if (d<tomorrow) {
                continue; // we dont have data before tomorrow
            }
            // Convert date to index in CombinedFeStreams
            int index = d.toJulianDay() - tomorrowJulianDay;
            // Get the DailyInfo concerned
            if (index < listDi.size()) {
                const CombinedFeStreams::DailyInfo di = listDi[index];
                // Proceeed with calculation
                binData.income = CurrencyHelper::add(binData.income, di.totalIncomes,
                    currInfo.noOfDecimal);
                binData.expense = CurrencyHelper::add(binData.expense, fabs(di.totalExpenses),
                    currInfo.noOfDecimal);
                double dailyDelta = CurrencyHelper::add(di.totalIncomes, di.totalExpenses,
                    currInfo.noOfDecimal);
                binData.delta = CurrencyHelper::add(binData.delta, dailyDelta,
                    currInfo.noOfDecimal);
                eopCashBalance = CurrencyHelper::add(eopCashBalance, dailyDelta,
                    currInfo.noOfDecimal);
                binData.cashBalance = eopCashBalance;
                // write back the final value of the bin data
                (*binsPtr).insert(binDate, binData);
            }

        }
    }

    // second pass for growth percentage calculation
    QMap<QDate, Bin>::iterator prev = binsPtr->begin();
    if (prev != binsPtr->end()) {
        QMap<QDate, Bin>::iterator it = std::next(prev);
        for (; it != binsPtr->end(); prev = it, ++it) {
            const Bin &prevBin = prev.value();
            Bin &curBin = it.value();
            bool undefined;

            double v = Util::percentageChange(prevBin.income, curBin.income, undefined);
            curBin.incomeGrowthInPercentage = undefined ? std::nullopt : std::optional<double>(v);

            v = Util::percentageChange(prevBin.expense, curBin.expense, undefined);
            curBin.expenseGrowthInPercentage = undefined ? std::nullopt : std::optional<double>(v);

            v = Util::percentageChange(prevBin.delta, curBin.delta, undefined);
            curBin.deltaGrowthInPercentage = undefined ? std::nullopt : std::optional<double>(v);
        }
    }
}


AnalysisDialog::Bin::Bin()
    : income(0.0),
      incomeGrowthInPercentage(std::nullopt),
      expense(0.0),
      expenseGrowthInPercentage(std::nullopt),
      delta(0.0),
      deltaGrowthInPercentage(std::nullopt),
      cashBalance(0.0)
{
}


bool AnalysisDialog::Bin::operator==(const Bin &o) const
{
    return (this->income == o.income)
        && (this->incomeGrowthInPercentage  == o.incomeGrowthInPercentage)
        && (this->expense == o.expense)
        && (this->expenseGrowthInPercentage == o.expenseGrowthInPercentage)
        && (this->delta == o.delta)
        && (this->deltaGrowthInPercentage   == o.deltaGrowthInPercentage)
        && (this->cashBalance == o.cashBalance);
}


bool AnalysisDialog::Bin::operator!=(const Bin &o) const
{
    return !(*this==o);
}


void AnalysisDialog::on_globalExportCsvPushButton_clicked()
{
    int index = ui->tabWidget->currentIndex();

    switch (index) {
        case 0:
            // Legend of Relative Weight
            rwExportLegendAsCsvFile();
            break;
        case 2:
            // Period - list (monthly or annual depending on selection)
            if (ui->periodListMonthlyRadioButton->isChecked()) {
                periodListExport(PeriodType::MONTHLY);
            } else {
                periodListExport(PeriodType::YEARLY);
            }
            break;
        case 4:
            // Tags
            tagsExportToCsvFile();
            break;
        default:
            return;
            break;
    }
}


void AnalysisDialog::on_globalExportImagePushButton_clicked()
{
    int index = ui->tabWidget->currentIndex();

    switch (index) {
        case 0:
            // Relative Weight
            exportChartAsImage(ui->chartRelativeWeigthWidget, tr("Relative weight"));
            break;
        case 1:
            // Details chart — export whichever mode is currently active
            if (ui->periodChartMonthlyRadioButton->isChecked()) {
                exportChartAsImage(ui->periodChartWidget, tr("Monthly report chart"));
            } else {
                exportChartAsImage(ui->periodChartWidget, tr("Annual report chart"));
            }
            break;
        case 5:
            // CSD Comparison
            exportChartAsImage(ui->csdChartWidget, tr("CSD comparison"));
            break;
        default:
            return;
            break;
    }
}


void AnalysisDialog::on_tabWidget_currentChanged(int index)
{
    switch (index) {
        case 0:
            // Relative Weight
            ui->globalExportCsvPushButton->setText(tr("Export legend..."));
            ui->globalExportCsvPushButton->setVisible(true);
            ui->globalExportImagePushButton->setVisible(true);
            break;
        case 1:
            // Details chart (monthly or annual)
            ui->globalExportCsvPushButton->setVisible(false);
            ui->globalExportImagePushButton->setVisible(true);
            break;
        case 2:
            // Period - list
            ui->globalExportCsvPushButton->setText(tr("Export table..."));
            ui->globalExportCsvPushButton->setVisible(true);
            ui->globalExportImagePushButton->setVisible(false);
            break;
        case 3:
            // Period - periodHeatmap
            ui->globalExportCsvPushButton->setVisible(false);
            ui->globalExportImagePushButton->setVisible(false);
            break;
        case 4:
            // Tags
            ui->globalExportCsvPushButton->setText(tr("Export table..."));
            ui->globalExportCsvPushButton->setVisible(true);
            ui->globalExportImagePushButton->setVisible(false);
            break;
        case 5:
            // CSD Comparison
            ui->globalExportCsvPushButton->setVisible(false);
            ui->globalExportImagePushButton->setVisible(true);
            break;
        case 6:
            // Help
            ui->globalExportCsvPushButton->setVisible(false);
            ui->globalExportImagePushButton->setVisible(false);
            break;
        default:
            return;
            break;
    }
}


void AnalysisDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);

    // Re-run column sizing now that the widget is on its target screen.
    // slotAnalysisPrepareContent() populates the table and sizes columns before show(),
    // so those measurements use the pre-show font DPI. On Windows with per-monitor DPI
    // scaling the widget's font DPI is only finalised once it lands on its actual screen.
    // UiUtil::resizeTableColumns() also sets setMinimumSectionSize() from the application
    // font, so no separate header-height pass is needed here.
    if (ui->detailsReportTableWidget->rowCount() > 0)
        UiUtil::resizeTableColumns(ui->detailsReportTableWidget);

    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("AnalysisDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
    });
}


void AnalysisDialog::on_globalApplyDatesPushButton_clicked()
{
    QDate from     = ui->globalFromDateEdit->date();
    QDate to       = ui->globalToDateEdit->date();
    QDate tomorrow = GbpController::getInstance().getTomorrow();

    auto revert = [this]() {
        ui->globalFromDateEdit->setDate(globalPreviousFromDate);
        ui->globalToDateEdit->setDate(globalPreviousToDate);
    };

    if (!from.isValid()) {
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            QString(tr("\"From\" date %1 is invalid")).arg(from.toString(Qt::ISODate)), {tr("OK")}, 0, 0);
        revert();
        return;
    }
    if (!to.isValid()) {
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            QString(tr("\"To\" date %1 is invalid")).arg(to.toString(Qt::ISODate)), {tr("OK")}, 0, 0);
        revert();
        return;
    }
    if (to < from) {
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            QString(tr("\"To\" date %1 cannot occur before \"From\" date %2"))
                .arg(to.toString(Qt::ISODate), from.toString(Qt::ISODate)), {tr("OK")}, 0, 0);
        revert();
        return;
    }
    if (from < tomorrow) {
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            QString(tr("\"From\" date %1 cannot occur before \"tomorrow\" %2"))
                .arg(from.toString(Qt::ISODate), tomorrow.toString(Qt::ISODate)), {tr("OK")}, 0, 0);
        revert();
        return;
    }
    if (to < tomorrow) {
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            QString(tr("\"To\" date %1 cannot occur before \"tomorrow\" %2"))
                .arg(to.toString(Qt::ISODate), tomorrow.toString(Qt::ISODate)), {tr("OK")}, 0, 0);
        revert();
        return;
    }
    {
        QSharedPointer<Scenario> scenario =
            GbpController::getInstance().getScenario().toStrongRef();
        if (!scenario.isNull()) {
            QDate maxToDate =
                tomorrow.addYears(scenario->getFeGenerationDuration()).addDays(-1);
            if (to > maxToDate) {
                GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
                    QString(tr("\"To\" date %1 cannot exceed the scenario end date %2"))
                        .arg(to.toString(Qt::ISODate), maxToDate.toString(Qt::ISODate)), {tr("OK")}, 0, 0);
                revert();
                return;
            }
        }
    }

    globalPreviousFromDate = from;
    globalPreviousToDate   = to;

    rwUpdateChart();
    bool isMonthly = ui->periodChartMonthlyRadioButton->isChecked();
    periodChartOffset = 0;
    periodChartRedisplayChart(isMonthly ? PeriodType::MONTHLY : PeriodType::YEARLY);
    tagsRedisplayTable();
    csdRedisplayChart();

    LOG_DEBUG_INFO(QString("Global dates applied: %1 to %2")
        .arg(from.toString(), to.toString()));
}

