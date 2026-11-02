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
#include "csvexporter.h"
#include "ui_analysisdialog.h"
#include "gbpcontroller.h"
#include "gbplogger.h"
#include "util.h"
#include "uiutil.h"
#include <QCursor>
#include <QFileDialog>
#include <QPieSeries>
#include <QPieSlice>
#include <QTimer>
#include <QToolTip>
#include <QMessageBox>
#include <QApplication>
#include <QPainter>
#include <algorithm>
#include <cmath>
#include "gbpqmessage.h"


namespace {
    // Width in pixels of the widest text of a column, in the table's font
    int maxColumnTextWidth(const QTableWidget* table, int col)
    {
        QFontMetrics fm(table->font());
        int width = 0;
        for (int r = 0; r < table->rowCount(); ++r) {
            const QTableWidgetItem* item = table->item(r, col);
            if (item != nullptr) {
                width = std::max(width, fm.horizontalAdvance(item->text()));
            }
        }
        return width;
    }
}


void AnalysisDialog::tagsUpdateNoTagsSelected()
{
    ui->tags_noTagsSelectedLabel->setText(
        tr("%1 selected").arg(locale.toString(selectedTags.size())));
}


void AnalysisDialog::tagsExportToCsvFile()
{
    // *** STEP 1 : Build the columns definitions ***
    QList<CsvColumnDescriptor> columns;
    columns.append({tr("Tag name"), CsvColumnType::string()});
    columns.append({tr("Total Amount"), CsvColumnType::currency(
        GbpController::getInstance().getExportTextNumberLocalized())});
    columns.append({tr("Weight (%)"), CsvColumnType::numberFull(
        GbpController::getInstance().getExportTextNumberLocalized())});
    columns.append({tr("Count"), CsvColumnType::integer()});


    // *** STEP 2 : Populate rows : no sorting by weight... ***
    QList<QList<QVariant>> theRows;
    for (auto it = tagsTableData.constBegin(); it != tagsTableData.constEnd(); ++it) {
        const QUuid& key = it.key();
        const TagContribution& data = it.value();
        theRows.append({data.name, data.amount, 100*data.weight, data.count});
    }

    // *** STEP 3 : Call exportToCsv() and handle the result ***
    CsvExportResult result = CsvExporter::exportToCsv("Tags Analysis",
        columns, theRows, locale, currInfo, '\t' );
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


void AnalysisDialog::tagsRedisplayTable()
{
    if (ready==false) {
        return;
    }

    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario.isNull()){
        return; // if no scenario loaded (should not happen)
    }

    // *** Get/validate To and From dates (shared global controls) ***
    QDate from     = ui->globalFromDateEdit->date();
    QDate to       = ui->globalToDateEdit->date();
    QDate tomorrow = GbpController::getInstance().getTomorrow();
    if (!from.isValid()) {
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            tr("\"From\" date is invalid"), {tr("OK")}, 0, 0);
        return;
    } else if (!to.isValid()) {
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            tr("\"To\" date is invalid"), {tr("OK")}, 0, 0);
        return;
    } else if (to < from) {
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            QString(tr("\"To\" date %1 cannot occur before \"From\" date %2"))
                .arg(to.toString(Qt::ISODate), from.toString(Qt::ISODate)), {tr("OK")}, 0, 0);
        return;
    } else if (from < tomorrow) {
        GbpQMessage::messageBoxQuestion(nullptr, GbpQMessage::Type::ERROR, tr("Error"),
            QString(tr("\"From\" date %1 cannot be smaller than \"tomorrow\" %2"))
                .arg(from.toString(Qt::ISODate), tomorrow.toString(Qt::ISODate)), {tr("OK")}, 0, 0);
        return;
    }

    // *** Rebuild tags data ***
    tagsTableData.clear();
    double totalWithoutTags;
    tagsRebuildData(from, to, tagsTableData, totalWithoutTags,
        tagsBreakdownData); // key is Tag ID

    // Block selection signals while refilling : the selection is restored explicitly below
    const QSignalBlocker blocker(ui->tagsTableWidget);

    // Disable sorting when filling the table
    ui->tagsTableWidget->setSortingEnabled(false);

    // *** Update the table with the content ***
    ui->tagsTableWidget->clearContents();
    ui->tagsTableWidget->setRowCount(tagsTableData.size());
    int row = 0;

    for (auto it = tagsTableData.constBegin(); it != tagsTableData.constEnd(); ++it) {
        const QUuid& key = it.key();
        const TagContribution& data = it.value();
        // Use key and value (read-only)
        QString s1 = data.name;
        QString s2 = CurrencyHelper::formatAmount(abs(data.amount),currInfo,locale,false);
        QString s3 = locale.toString(data.weight*100, 'f', 3);
        QString s4 = locale.toString(data.count);
        QTableWidgetItem* wi1 = new QTableWidgetItem(s1);
        wi1->setData(Qt::UserRole, key); // used to find the tag of the selected row
        QTableWidgetItem* wi2 = new NumericTableWidgetItem(s2);
        wi2->setData(Qt::UserRole, data.amount);
        QTableWidgetItem* wi3 = new NumericTableWidgetItem(s3);
        wi3->setData(Qt::UserRole, data.weight);
        QTableWidgetItem* wi4 = new NumericTableWidgetItem(s4);
        wi4->setData(Qt::UserRole, data.count);
        wi1->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        wi2->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        wi3->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        wi4->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tagsTableWidget->setItem(row,0,wi1);
        ui->tagsTableWidget->setItem(row,1,wi2);
        ui->tagsTableWidget->setItem(row,2,wi3);
        ui->tagsTableWidget->setItem(row,3,wi4);
        row++;
    }

    // update total amount and set the color
    ui->tags_totalAmountLabel->setText(CurrencyHelper::formatAmount(abs(totalWithoutTags),currInfo,
        locale,false));
    if (ui->tagsIncomesRadioButton->isChecked()==true) {
        ui->tags_totalAmountLabel->setStyleSheet(Util::getStyleSheetStringForColor(
            GbpController::getInstance().getIncomeColor()));
    } else {
        ui->tags_totalAmountLabel->setStyleSheet(Util::getStyleSheetStringForColor(
            GbpController::getInstance().getExpenseColor()));
    }

    // The bars of the Weight column end where the widest percentage text starts
    tagsWeightDelegate->setTextWidth(maxColumnTextWidth(ui->tagsTableWidget, 2));

    // re-enable sorting
    ui->tagsTableWidget->setSortingEnabled(true);

    // Fit each column to its actual content (tag name, formatted amount, weight, count are all
    // locale/content dependent), rather than a fixed width guess made once at startup.
    UiUtil::resizeTableColumns(ui->tagsTableWidget);

    // Restore the selected tag of the current mode if it is still displayed. Otherwise (e.g. this
    // mode is shown for the first time) select the first row.
    int rowToSelect = (ui->tagsTableWidget->rowCount() > 0) ? 0 : -1;
    for (int r = 0; r < ui->tagsTableWidget->rowCount(); ++r) {
        QTableWidgetItem* item = ui->tagsTableWidget->item(r, 0);
        if ( (item != nullptr)
            && (item->data(Qt::UserRole).toUuid() == tagsBreakdownTagId()) ) {
            rowToSelect = r;
            break;
        }
    }
    ui->tagsTableWidget->clearSelection();
    if (rowToSelect == -1) {
        tagsBreakdownTagId() = QUuid();
    } else {
        ui->tagsTableWidget->selectRow(rowToSelect);
        // The sort order may have moved the row out of view
        ui->tagsTableWidget->scrollToItem(ui->tagsTableWidget->item(rowToSelect, 0));
        tagsBreakdownTagId() = ui->tagsTableWidget->item(rowToSelect, 0)->data(Qt::UserRole)
            .toUuid();
    }
    tagsRedisplayBreakdown();
}


QUuid& AnalysisDialog::tagsBreakdownTagId()
{
    return (ui->tagsIncomesRadioButton->isChecked()) ?
        (tagsBreakdownTagIdIncome) : (tagsBreakdownTagIdExpense);
}


void AnalysisDialog::tagsRedisplayBreakdown()
{
    const QSignalBlocker blocker(ui->tagsBreakdownTableWidget);
    ui->tagsBreakdownTableWidget->setSortingEnabled(false);
    ui->tagsBreakdownTableWidget->clearContents();
    ui->tagsBreakdownTableWidget->setRowCount(0);

    // Nothing selected
    if ( tagsBreakdownTagId().isNull() || (tagsTableData.contains(tagsBreakdownTagId())==false) ) {
        ui->tagsBreakdownLabel->setText(tr("Select a tag above to see the Cash Stream "
            "Definitions contributing to it."));
        ui->tagsBreakdownTableWidget->setSortingEnabled(true);
        tagsRedisplayPie({});
        return;
    }

    const TagContribution& tagData = tagsTableData.value(tagsBreakdownTagId());
    const QHash<QUuid,CsdContribution> csdData = tagsBreakdownData.value(tagsBreakdownTagId());
    if (csdData.isEmpty()) {
        ui->tagsBreakdownLabel->setText(
            tr("No financial events for tag \"%1\" in the selected period.").arg(tagData.name));
        ui->tagsBreakdownTableWidget->setSortingEnabled(true);
        tagsRedisplayPie({});
        return;
    }
    // %2 is the number of CSDs contributing to the tag
    ui->tagsBreakdownLabel->setText(tr("Breakdown of tag \"%1\" (%2) :").arg(tagData.name,
        locale.toString(csdData.size())));

    // Order the CSDs from largest to smallest contribution, to assign the colors shared by the
    // table and the slices of the pie. If there are too many CSDs, the smallest ones are
    // grouped in a single gray "Others" slice.
    const int MAX_SLICES = 20;
    const QColor othersColor(150,150,150);
    QList<CsdContribution> sortedCsds = csdData.values();
    std::sort(sortedCsds.begin(), sortedCsds.end(),
        [](const CsdContribution& a, const CsdContribution& b) {
            return fabs(a.amount) > fabs(b.amount);
        });
    const int noOfColoredCsds = (sortedCsds.size() <= MAX_SLICES) ?
        (sortedCsds.size()) : (MAX_SLICES-1);
    QList<std::tuple<QString,double,QColor>> slices;
    double othersAmount = 0;

    const bool colorizeNames = GbpController::getInstance().getAllowDecorationColor();
    ui->tagsBreakdownTableWidget->setRowCount(sortedCsds.size());
    int row = 0;
    for (const CsdContribution& data : sortedCsds) {
        QColor sliceColor;
        if (row < noOfColoredCsds) {
            sliceColor = colorsRelativeWeigth[row % colorsRelativeWeigth.size()];
            slices.append({data.name, fabs(data.amount), sliceColor});
        } else {
            sliceColor = othersColor;
            othersAmount = CurrencyHelper::add(othersAmount, fabs(data.amount),
                currInfo.noOfDecimal);
        }
        // amounts of the tag and of the CSD have the same sign, so ratios are positive
        double pctOfTag = (tagData.amount==0) ? (0) : (data.amount/tagData.amount);
        QTableWidgetItem* wi1 = new QTableWidgetItem(data.name);
        QTableWidgetItem* wi2 = new NumericTableWidgetItem(
            CurrencyHelper::formatAmount(abs(data.amount),currInfo,locale,false));
        wi2->setData(Qt::UserRole, abs(data.amount));
        QTableWidgetItem* wi3 = new NumericTableWidgetItem(
            locale.toString(pctOfTag*100, 'f', 3));
        wi3->setData(Qt::UserRole, pctOfTag);
        QTableWidgetItem* wi4 = new NumericTableWidgetItem(locale.toString(data.count));
        wi4->setData(Qt::UserRole, data.count);
        if (colorizeNames && data.color.isValid()) {
            wi1->setForeground(QBrush(data.color));
        }
        wi1->setData(Qt::DecorationRole, sliceColor); // small square of the slice color
        wi1->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        wi2->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        wi3->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        wi4->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tagsBreakdownTableWidget->setItem(row,0,wi1);
        ui->tagsBreakdownTableWidget->setItem(row,1,wi2);
        ui->tagsBreakdownTableWidget->setItem(row,2,wi3);
        ui->tagsBreakdownTableWidget->setItem(row,3,wi4);
        row++;
    }

    if (othersAmount != 0) {
        slices.append({tr("Others"), othersAmount, othersColor});
    }

    // The bars of the "% of tag" column end where the widest percentage text starts
    tagsBreakdownPctDelegate->setTextWidth(maxColumnTextWidth(ui->tagsBreakdownTableWidget, 2));
    ui->tagsBreakdownTableWidget->setSortingEnabled(true);
    UiUtil::resizeTableColumns(ui->tagsBreakdownTableWidget);

    tagsRedisplayPie(slices);
}


void AnalysisDialog::tagsRedisplayPie(const QList<std::tuple<QString,double,QColor>>& slices)
{
    tagsPieChart->removeAllSeries();
    tagsPieHoveredText.clear();
    if (slices.isEmpty()) {
        return;
    }

    QPieSeries* series = new QPieSeries;
    series->setPieSize(0.85);
    double total = 0;
    for (const auto& [name, amount, color] : slices) {
        total += amount;
    }
    for (const auto& [name, amount, color] : slices) {
        QPieSlice* slice = series->append(name, amount);
        slice->setBrush(color);
        slice->setProperty("baseColor", color); // restored after the slice is hovered
        slice->setLabelVisible(false); // no caption : name, amount and % are in the tooltip
        slice->setLabel(QString("%1\n%2\n%3 %").arg(name,
            CurrencyHelper::formatAmount(amount, currInfo, locale, false),
            locale.toString((total==0) ? (0) : (100*amount/total), 'f', 1)));
    }
    // Tooltips are top-level windows, so they are never clipped by the chart. The text of the
    // hovered slice is remembered, so that eventFilter() can show it again when the chart view
    // gets its own tooltip event (otherwise, the view would hide it)
    // The hovered slice is highlighted by a lighter color rather than exploded : moving the
    // slice would make the mouse leave and enter it again, making the tooltip flicker
    connect(series, &QPieSeries::hovered, this, [this](QPieSlice* slice, bool state) {
        const QColor color = slice->property("baseColor").value<QColor>();
        slice->setBrush(state ? color.lighter(130) : color);
        if (state) {
            tagsPieHoveredText = slice->label();
            QToolTip::showText(QCursor::pos(), tagsPieHoveredText, tagsPieChartView->viewport());
        } else {
            tagsPieHoveredText.clear();
            // Hide only if no other slice was entered meanwhile (the "leave" signal of a slice
            // comes just before the "enter" signal of the next one)
            QTimer::singleShot(0, this, [this]() {
                if (tagsPieHoveredText.isEmpty()) {
                    QToolTip::hideText();
                }
            });
        }
    });
    tagsPieChart->addSeries(series); // take ownership
}


void AnalysisDialog::tagsRebuildData( QDate from, QDate to, QHash<QUuid,TagContribution>& data,
    double& totalWithoutTags, QHash<QUuid,QHash<QUuid,CsdContribution>>& breakdown)
{
    // Reset output variables
    totalWithoutTags = 0;
    data.clear();
    breakdown.clear();

    // Init the structure for each selected tag
    bool found;
    QSet<QUuid> selectedTagsSet = selectedTags.getFilterTagIdSet();
    for (const QUuid& tagId : selectedTagsSet) {
        // Get tags's name
        Tag t = availableTags.getTag(tagId,found);
        if (found==false) {
            continue; // should never happen
        }
        // Add to "data"
        data.insert(tagId, {.name=t.getName(), .amount=0, .weight=0, .count=0});
    }


    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario.isNull()){
        return ; // if no scenario loaded (should not happen)
    }

    // Make sure the raw data is available (which should always be the case)
    QSharedPointer<CombinedFeStreams> chartRawData = chartRawDataRef.toStrongRef();
    if(chartRawData.isNull()){
        return ; // should never happen
    }

    // take a reference to all the relationships defined between Csds and Tags
    TagCsdRelationships r = scenario->getTagCsdRelationships() ;

    // are we processing income or expense related stuff ?
    bool incomeProcessing = (ui->tagsIncomesRadioButton->isChecked()? (true):(false));

    /*
     * The total and the count of financial events of each CSD in the period come from the
     * event data shared by the tabs (no new scan of the financial events if it is already
     * built for these dates). The total of each CSD is then added to each of its tags : this
     * gives the same result as adding each financial event to each tag of its CSD, but with
     * far fewer operations, since a CSD can have many tags.
     */
    eventDataEnsure(from, to);
    totalWithoutTags = (incomeProcessing==true) ? eventDaysTotalIncomes : eventDaysTotalExpenses;
    const QHash<QUuid, CsdEventTotal>& csdTotals =
        (incomeProcessing==true) ? eventCsdTotalsIncomes : eventCsdTotalsExpenses;
    for (auto it = csdTotals.constBegin(); it != csdTotals.constEnd(); ++it) {
        const QUuid& csdId = it.key();
        const CsdEventTotal& t = it.value();
        // Get all the tags associated to this CSD and for each add the contribution
        const QSet<QUuid> idSet = r.getRelationshipsForCsd(csdId);
        for (const QUuid& tagId : idSet) {
            // Add the contribution of this tag if it is in the list of selected tags
            if (selectedTags.containsTagId(tagId)==true) {
                TagContribution& v = data[tagId]; // already created above for selected tags
                v.amount = CurrencyHelper::add(v.amount, t.amount, currInfo.noOfDecimal);
                v.count += t.count;
                // Breakdown of this tag per CSD
                CsdContribution& c = breakdown[tagId][csdId];
                c.name = t.name;
                c.color = t.color;
                c.amount = t.amount;
                c.count = t.count;
            }
        }
    }

    // Calculate weight. totalWithoutTags an be 0 !
    for (auto it = data.begin(); it != data.end(); ++it) {
        const QUuid& key = it.key(); // Key is read-only
        TagContribution& value = it.value(); // Value is modifiable

        // If totalWithoutTags==0, we set weight to 0 since it is irrelevant
        if (totalWithoutTags==0) {
            value.weight = 0;
        } else{
            value.weight = value.amount/totalWithoutTags;
        }
    }


    return ;
}


bool AnalysisDialog::TagContribution::operator==(const TagContribution &o) const
{
    return ( (this->amount==o.amount) && (this->name==o.name) && (this->weight==o.weight)
        && (this->count==o.count) );
}

bool AnalysisDialog::TagContribution::operator!=(const TagContribution &o) const
{
    return !(*this==o);
}


bool AnalysisDialog::NumericTableWidgetItem::operator<(const QTableWidgetItem &other) const
{
    bool ok1, ok2;
    double v1 = data(Qt::UserRole).toDouble(&ok1);
    double v2 = other.data(Qt::UserRole).toDouble(&ok2);

    if (ok1 && ok2)
        return v1 < v2;
    // fallback to text compare
    return text() < other.text();
}


void AnalysisDialog::WeightBarDelegate::paint(QPainter* painter,
    const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);
    const QString text = opt.text;

    // Native background only (selection color, focus) : the bar and the text are drawn below
    opt.text.clear();
    const QWidget* widget = opt.widget;
    QStyle* style = (widget != nullptr) ? (widget->style()) : (QApplication::style());
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

    const bool selected = opt.state.testFlag(QStyle::State_Selected);
    const QColor textColor = opt.palette.color((selected) ? (QPalette::HighlightedText) :
        (QPalette::Text));
    const QFontMetrics& fm = opt.fontMetrics;
    const int margin = fm.horizontalAdvance(QLatin1Char(' '));
    const int textW = std::max(textWidth, fm.horizontalAdvance(text));
    const QRect r = opt.rect.adjusted(margin, 0, -margin, 0);

    painter->save();

    // Percentage, right aligned in its own area
    painter->setPen(textColor);
    painter->drawText(QRect(r.left() + r.width() - textW, r.top(), textW, r.height()),
        Qt::AlignRight | Qt::AlignVCenter, text);

    // Bar at the left of the text : a faint track for 100 %, filled up to the weight. Gray (the
    // text color, half transparent), or the text color of the selection on the selected row
    const int barHeight = std::max(4, fm.height() / 2);
    const QRect track(r.left(), r.top() + (r.height() - barHeight) / 2,
        r.width() - textW - margin, barHeight);
    if (track.width() > 0) {
        double weight = std::clamp(std::fabs(index.data(Qt::UserRole).toDouble()), 0.0, 1.0);
        QColor trackColor = textColor;
        trackColor.setAlpha(35);
        QColor barColor = textColor;
        if (!selected) {
            barColor.setAlpha(140);
        }
        painter->fillRect(track, trackColor);
        painter->fillRect(QRectF(track.left(), track.top(), track.width() * weight,
            track.height()), barColor);
    }

    painter->restore();
}


QSize AnalysisDialog::WeightBarDelegate::sizeHint(const QStyleOptionViewItem& option,
    const QModelIndex& index) const
{
    QSize s = QStyledItemDelegate::sizeHint(option, index);
    const QFontMetrics& fm = option.fontMetrics;
    // Room for the margins, a bar at least 8 "M" wide and the widest percentage text
    int textW = std::max(textWidth, fm.horizontalAdvance(index.data(Qt::DisplayRole).toString()));
    int w = 3 * fm.horizontalAdvance(QLatin1Char(' ')) + 8 * fm.horizontalAdvance(QLatin1Char('M'))
        + textW;
    return QSize(std::max(s.width(), w), s.height());
}


void AnalysisDialog::on_tagsSelectTagsPushButton_clicked()
{
    // send for display
    emit signalChooseTagsPrepareContent(availableTags, selectedTags.getFilterTagIdSet());
    selectTagsDlg->show();
}


void AnalysisDialog::on_tagsIncomesRadioButton_clicked()
{
    tagsRedisplayTable();
    LOG_DEBUG_INFO(QString("Analysis Dialog - Tags : Income radiobutton clicked"));
}


void AnalysisDialog::on_tagsExpensesRadioButton_clicked()
{
    tagsRedisplayTable();
    LOG_DEBUG_INFO(QString("Analysis Dialog - Tags : Expense radiobutton clicked"));
}


void AnalysisDialog::on_tagsTableWidget_itemSelectionChanged()
{
    QList<QTableWidgetItem*> selected = ui->tagsTableWidget->selectedItems();
    tagsBreakdownTagId() = QUuid();
    for (QTableWidgetItem* item : selected) {
        if (item->column() == 0) {
            tagsBreakdownTagId() = item->data(Qt::UserRole).toUuid();
            break;
        }
    }
    tagsRedisplayBreakdown();
}


