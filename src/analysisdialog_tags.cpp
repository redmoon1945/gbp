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
#include <QFileDialog>
#include <QMessageBox>
#include "gbpqmessage.h"


void AnalysisDialog::tagsUpdateNoTagsSelected()
{
    ui->tags_noTagsSelectedLabel->setText(tr("%1 selected").arg(selectedTags.size()));
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
    tagsRebuildData(from, to, tagsTableData, totalWithoutTags); // key is Tag ID

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
        QString s3 = QString::number(data.weight*100, 'f', 3);
        QString s4 = QString::number(data.count);
        QTableWidgetItem* wi1 = new QTableWidgetItem(s1);
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
        locale,true));
    if (ui->tagsIncomesRadioButton->isChecked()==true) {
        ui->tags_totalAmountLabel->setStyleSheet(Util::getStyleSheetStringForColor(
            GbpController::getInstance().getIncomeColor()));
    } else {
        ui->tags_totalAmountLabel->setStyleSheet(Util::getStyleSheetStringForColor(
            GbpController::getInstance().getExpenseColor()));
    }

    // re-enable sorting
    ui->tagsTableWidget->setSortingEnabled(true);
}


void AnalysisDialog::tagsRebuildData( QDate from, QDate to, QHash<QUuid,TagContribution>& data,
    double& totalWithoutTags)
{
    // Reset output variables
    totalWithoutTags = 0;
    data.clear();

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

    QList<CombinedFeStreams::DailyInfo> listDi = chartRawData->getCombinedStreams();
    const qsizetype diSize = listDi.size();
    QDate tomorrow = GbpController::getInstance().getTomorrow();

    /*
     * Strategy : for each existing DailyInfo, look in the incomeList/expenseList and for each
     * Csd found, get the list of all linked tags. For each tag found,
     * add the contribution to "data".
     */

    for (int var = 0; var < diSize; ++var) {

        // +++ Pre-processing +++

        CombinedFeStreams::DailyInfo di = listDi[var];
        if(di.used == false){
            continue;
        }
        QDate diDate = tomorrow.addDays(var);


        // +++ Processing +++

        // Make sure this date is inside the user-selected period
        if ( (diDate<from) || (diDate>to) ) {
            continue;
        }

        // Add total income and expense
        if (incomeProcessing==true) {
            totalWithoutTags = CurrencyHelper::add(totalWithoutTags, di.totalIncomes,
                currInfo.noOfDecimal);
        } else {
            totalWithoutTags = CurrencyHelper::add(totalWithoutTags, di.totalExpenses,
                currInfo.noOfDecimal);
        }

        // Proceed by iterating through all the elements for that day and distribute the
        // contribution of each tag
        QList<Fe>* feListPtr = (incomeProcessing==true)?(&(di.incomesList)):(&(di.expensesList));
        foreach (const Fe fe, *feListPtr) {
            // Get a pointer to the CSD
            QSharedPointer<Csd> csdPtr = fe.csdPtr.toStrongRef();
            if(csdPtr.isNull()){
                continue;   // should never happen
            }
            // Get all the tags associated to this CSD and for each add the contribution
            const QSet<QUuid> idSet = r.getRelationshipsForCsd(csdPtr->getId());
            for (const QUuid& tagId : idSet) {
                // Add the contribution of this tag if it is in the list of selected tags
                if (selectedTags.containsTagId(tagId)==true) {
                    TagContribution v = data.value(tagId, {}); // may already exist or not
                    v.amount = CurrencyHelper::add(v.amount, fe.amount, currInfo.noOfDecimal);
                    v.count++;
                    data.insert(tagId,v);
                }
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


