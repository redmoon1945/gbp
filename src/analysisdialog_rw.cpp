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
#include <QChart>
#include <QPieSeries>
#include <QFileDialog>
#include <QMessageBox>
#include <QStringView>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include "gbpqmessage.h"


// --- For data attached to the legend list of Relative Weight ---
// Anonymous namespace means private to this .cpp file ---
namespace {
    struct LegendItemInfo {
        quint8 rank;        // position (1 = highest percentage)
        double amount;      // always positive
        double percentage;  // e.g. 4% => 4
        QUuid id;           // csd ID
    };
}
// so that it can be used as a QVariant, which is required by setData
Q_DECLARE_METATYPE(LegendItemInfo)


void AnalysisDialog::rwUpdateChart()
{
    if (!ready){
        return;
    }

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

    // Some Init and data def
    int noOfElements= ui->rwNoElementsSpinBox->value();
    QDate from = ui->globalFromDateEdit->date();
    QDate to   = ui->globalToDateEdit->date();
    QDate tomorrow = GbpController::getInstance().getTomorrow();
    RelWeightGrouping grouping = rwGetGroupingTypeSelected();

    // Clear Legends widget
    rwClearLegendWidgets();

    // *** step 1 : Build the relative weight bins for the pie chart by counting individual
    // contributions of each CSD if grouping=INCOMES or EXPENSES or each tag if grouping=TAGS.
    // Results : key = CSD or tag UUID, value = total amount contributed
    // in the period (never negative)
    QHash<QUuid,double> bins = {}; // hold the temp results, to be sorted later
    double grandTotal=0;
    QList<CombinedFeStreams::DailyInfo> listDi = chartRawData->getCombinedStreams();
    const qsizetype size = listDi.size();
    for (int var = 0; var < size; ++var) {

        // +++ Pre-processing +++
        CombinedFeStreams::DailyInfo di = listDi[var];
        if(di.used == false){
            continue;
        }
        QDate date = tomorrow.addDays(var);

        // +++ processing +++

        if( (date<from) || (date>to) ){
            continue;   // not in the interval
        }
        if( (grouping==RelWeightGrouping::RWGROUPING_INCOMES) ||
            (grouping==RelWeightGrouping::RWGROUPING_EXPENSES)){
            // set a generic pointer of the list of CSD (either incomes or expenses)
            QList<Fe> *prtCsdList = ((grouping==RelWeightGrouping::RWGROUPING_INCOMES)
                ?(&(di.incomesList)):(&(di.expensesList)));
            // Iterate through the list of CSD of that occurrence day
            for( int i=0; i<prtCsdList->count(); ++i ){
                Fe fed = prtCsdList->at(i);
                QSharedPointer<Csd> csdRef = fed.csdPtr.toStrongRef();
                if (csdRef.isNull()) {
                    continue; // should never happen
                }
                grandTotal = CurrencyHelper::add(grandTotal, fabs(fed.amount),
                    currInfo.noOfDecimal);
                double binValue = bins.value(csdRef->getId(),-1);
                if(binValue >= 0){// existing entry
                    binValue = CurrencyHelper::add(binValue, fabs(fed.amount),
                        currInfo.noOfDecimal);
                } else { // new entry
                    binValue = fabs(fed.amount);
                }
                bins.insert(csdRef->getId(), binValue); // replace current value or create new one
            }
        } else{
            // Illegal grouping : should never happen
            throw std::logic_error(QString("%1: Invalid grouping value").arg(Q_FUNC_INFO).toStdString());
        }
    }

    // *** step 2 : sort the bins by amount (biggest to smallest).
    QList<LegendItemInfo> tempList; // result of the sort
    foreach(QUuid id, bins.keys()){
        double d = bins.value(id);
        // rank will be set later after the sort
        LegendItemInfo p = {.rank=0, .amount = d, .percentage= (100*d/grandTotal), .id = id};
        tempList.append(p);
    }
    std::sort(tempList.begin(), tempList.end(), [](const LegendItemInfo& p1, const LegendItemInfo& p2) {
        return (p1.amount > p2.amount);
    });
    for(int counter=0; counter<tempList.size(); counter++){
        tempList[counter].rank = counter+1;
    }

    // *** step 3 : shrink to "n" elements + 1 "others" when required ***
    int noElements = ui->rwNoElementsSpinBox->value(); // 1 is minimum
    if( tempList.size() > noElements ){
        // rejected elements are regrouped in a new single element "others", with null QUuid
        int noRejected = tempList.size() - noElements;
        double cumulPercentageRejected = 0;
        double cumulAmountRejected = 0;
        for(int i=0; i<noRejected;i++){
            cumulPercentageRejected += tempList.at(noElements+i).percentage;
            cumulAmountRejected = CurrencyHelper::add(cumulAmountRejected,
                tempList.at(noElements+i).amount, currInfo.noOfDecimal);
        }
        tempList.remove(noElements,noRejected);
        QUuid nullId = QUuid::fromString(QStringView()); // will be null QUuid because not valid
        LegendItemInfo othersPair = {.amount = cumulAmountRejected,
            .percentage = cumulPercentageRejected, .id = nullId };
        tempList.append(othersPair);

        // re-sort new TempList because "others" may have been created ****
        std::sort(tempList.begin(), tempList.end(), [](const LegendItemInfo& p1,
            const LegendItemInfo& p2) {
            return (p1.amount > p2.amount);
        });
        for(int counter=0; counter<tempList.size(); counter++){
            tempList[counter].rank = counter+1;
        }
    }

    // *** step 4 : transform into pie data ***

    // recreate the pie
    chartRelativeWeight->removeAllSeries();
    seriesRelativeWeigth = new QPieSeries;

    // Adjust "margin so that we can see out labels even when big fonts. This is tricky...
    seriesRelativeWeigth->setPieSize(0.7); // smaller pie = more room for labels

    // Set rotation for Pie
    seriesRelativeWeigth->setPieStartAngle(ui->rwHorizontalSlider->value());
    seriesRelativeWeigth->setPieEndAngle(360+ui->rwHorizontalSlider->value());

    bool found;
    QList<QString> originalSliceNames;
    for(int i=0;i<tempList.size();i++){
        LegendItemInfo p = tempList.at(i);
        if (p.id.isNull()==true){
            // this is the "others" element
            seriesRelativeWeigth->append(tr("Others"), fabs(p.amount));
            originalSliceNames.append(tr("Others"));
        } else{
            // name is CSD name
            QString name;
            if ( (grouping==RelWeightGrouping::RWGROUPING_INCOMES) ||
                (grouping==RelWeightGrouping::RWGROUPING_EXPENSES) ) {
                QColor color;
                scenario->getCsdNameAndColorFromId(p.id, name, color,found); // always found
            } else{
                throw std::logic_error(QString("%1: Invalid grouping value")
                    .arg(Q_FUNC_INFO).toStdString()); // should never happen
            }
            originalSliceNames.append(name);
            seriesRelativeWeigth->append(name, p.amount);
        }

    }

    // *** step 5 : labels for slices ***
    seriesRelativeWeigth->setLabelsVisible(true);
    int slSize = seriesRelativeWeigth->slices().size();
    for (int i = 0; i < slSize; ++i) {
        QPieSlice *slice = seriesRelativeWeigth->slices().at(i);
        // change font
        QFont f = slice->labelFont();
        Util::changeFontSize(f, Util::FontResizeIntensity::AVERAGE,true,
            "AnalysisDialog::rwUpdateChart - labels for slice");
        slice->setLabelFont(f);
        if (ui->rwShowLabelsCheckBox->isChecked()==true) {
            slice->setLabelVisible(true);
        } else {
            slice->setLabelVisible(false);
        }
        // set label
        //slice->setLabel(QString("%1").arg(i+1));
        //slice->setLabelArmLengthFactor(0.5); // default is 15. Clipping problem (bug ?)
        // set color
        slice->setBrush(colorsRelativeWeigth[i]);
    }
    chartRelativeWeight->addSeries(seriesRelativeWeigth);

    // *** step 6 : Update legend (items the right listbox) ***
    ui->rwListWidget->clear();
    QFontMetrics fm(ui->rwListWidget->font());
    int sizeFontList = 0.9*fm.height();
    for (int i = 0; i < slSize; ++i) {
        QPieSlice *slice = seriesRelativeWeigth->slices().at(i);
        QListWidgetItem *item = new QListWidgetItem();

        //  add color marker
        QPixmap pix(sizeFontList, sizeFontList);
        pix.fill(slice->brush().color());
        item->setIcon(QIcon(pix));

        LegendItemInfo p = tempList.at(i);
        QString s = QString("%1").arg(originalSliceNames.at(i));
        item->setText(s);

        // We attach a LegendItemInfo varaible to the item so that we can used the data later
        item->setData(Qt::UserRole,QVariant::fromValue(p));

        ui->rwListWidget->addItem(item);
    }

}


void AnalysisDialog::rwClearLegendWidgets()
{
    ui->rwRankLabel->setText("");
    ui->rwAmountLabel->setText("");
    ui->rwPercentageLabel->setText("");
    ui->rwPercentageSignLabel->setVisible(false);
}


void AnalysisDialog::rwUpdateLegendWidgets(int rank, double amount, double percentage)
{
    if (rank==-1) {
        ui->rwRankLabel->setText("");
    } else {
        ui->rwRankLabel->setText(QString::number(rank));
    }

    QString amountString = CurrencyHelper::formatAmount(amount, currInfo, locale, false);
    ui->rwAmountLabel->setText(amountString);
    ui->rwPercentageLabel->setText(QString::number(percentage, 'f', 2));
    ui->rwPercentageSignLabel->setVisible(true);
}


AnalysisDialog::RelWeightGrouping AnalysisDialog::rwGetGroupingTypeSelected()
{
    if (ui->rwIncomesRadioButton->isChecked()==true) {
        return RelWeightGrouping::RWGROUPING_INCOMES;
    } else if (ui->rwExpensesRadioButton->isChecked()==true){
        return RelWeightGrouping::RWGROUPING_EXPENSES;
    } else {
        // should never happen
        throw std::logic_error(QString("%1: Analysis Dialog : no group selected")
            .arg(Q_FUNC_INFO).toStdString());
    }
}


void AnalysisDialog::rwExportLegendAsCsvFile()
{

    // *** STEP 1 : Build the columns definitions ***
    QList<CsvColumnDescriptor> columns;
    columns.append({tr("Rank"), CsvColumnType::integer(
        GbpController::getInstance().getExportTextNumberLocalized())});
    columns.append({tr("Percentage"), CsvColumnType::numberFull(
        GbpController::getInstance().getExportTextNumberLocalized())});
    columns.append({tr("Csd name"), CsvColumnType::string()});
    columns.append({tr("Amount"), CsvColumnType::currency(
        GbpController::getInstance().getExportTextNumberLocalized())});


    // *** STEP 2 : Populate rows ***
    QList<QList<QVariant>> data;
    // determine how many elements there are. We display all month/years between
    // tomorrow and max date as established by the scenario
    int noRows = ui->rwListWidget->count();
    for (int var = 0; var < noRows; ++var) {
        // Get line
        QListWidgetItem* item = ui->rwListWidget->item(var);
        QString name = item->text();
        LegendItemInfo info = item->data(Qt::UserRole).value<LegendItemInfo>();
        data.append({info.rank, info.percentage, item->text(), info.amount});
    }

    // *** STEP 3 : Call exportToCsv() and handle the result ***
    CsvExportResult result = CsvExporter::exportToCsv(
        tr("Relative Weight Legend"), columns, data, locale, currInfo, '\t' );
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


void AnalysisDialog::on_rwNoElementsSpinBox_valueChanged(int arg1)
{
    rwUpdateChart();
}


void AnalysisDialog::on_rwIncomesRadioButton_clicked()
{
    rwUpdateChart();
    LOG_DEBUG_INFO("Analysis Dialog : Income radiobutton clicked");
}


void AnalysisDialog::on_rwExpensesRadioButton_clicked()
{
    rwUpdateChart();
    LOG_DEBUG_INFO("Analysis Dialog : Expense radiobutton clicked");
}


void AnalysisDialog::on_rwClearSelectionPushButton_clicked()
{
    ui->rwListWidget->clearSelection();
    ui->rwListWidget->setCurrentItem(nullptr);
    rwClearLegendWidgets();
}


void AnalysisDialog::on_rwHorizontalSlider_valueChanged(int value)
{
    int step = 30;
    int snapped = ((value + step/2) / step) * step;
    if (snapped != value){
        ui->rwHorizontalSlider->setValue(snapped);
    }
    // redraw the pie
    seriesRelativeWeigth->setPieStartAngle(snapped);
    seriesRelativeWeigth->setPieEndAngle(360+snapped);
}


void AnalysisDialog::on_rwShowLabelsCheckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    for (QPieSlice *s : seriesRelativeWeigth->slices()) {
        s->setLabelVisible(ui->rwShowLabelsCheckBox->isChecked());
    }
}


void AnalysisDialog::on_rwListWidget_itemSelectionChanged()
{
    // remove stand out for all
    QList<QPieSlice *> list = seriesRelativeWeigth->slices();
    for (auto slice : seriesRelativeWeigth->slices()) {
        slice->setExploded(false);
    }

    // Clear Legend widgets
    rwClearLegendWidgets();

    // Get selection : row index is equivalent to rank
    QItemSelectionModel* selectionModel = ui->rwListWidget->selectionModel();
    QModelIndexList selectedRows = selectionModel->selectedRows();

    // If there is element selected, just return
    if (selectedRows.size()==0) {
        return;
    }

    // make the slice stand out
    double cummulPercentage=0;
    double cummulAmount=0;
    uint lastRank=-1;
    foreach (const QModelIndex &index, selectedRows) {
        // set slice appearence
        QPieSlice *slice = seriesRelativeWeigth->slices().at(index.row());
        slice->setExploded(true);
        slice->setExplodeDistanceFactor(0.10); // 10% of radius
        // calculate cumulative amount and percentage
        LegendItemInfo info = index.data(Qt::UserRole).value<LegendItemInfo>();
        cummulAmount = CurrencyHelper::add(cummulAmount, info.amount, currInfo.noOfDecimal);
        cummulPercentage += info.percentage;
        lastRank = info.rank;
    }

    // fill the Legend widgets
    if (selectedRows.size()==1) {
        rwUpdateLegendWidgets(lastRank, cummulAmount, cummulPercentage);
    } else {
        // rank is irrelevant if more than 1 row selected
        rwUpdateLegendWidgets(-1, cummulAmount, cummulPercentage);
    }

}


