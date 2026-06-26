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
#include "util.h"
#include "uiutil.h"
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include "gbpqmessage.h"







void AnalysisDialog::periodListRedisplayTableData(PeriodType rTypr)
{
    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario.isNull()){
        return; // if no scenario loaded (should not happen)
    }
    // choose the right bin map; both modes share the single table widget
    const QMap<QDate,Bin>* binsPtr;
    if (rTypr==PeriodType::MONTHLY) {
        binsPtr = &binsMonthly;
    } else {
        binsPtr = &binsYearly;
    }
    QTableWidget* tableWidget = ui->detailsReportTableWidget;
    // Update the first column header to reflect the active granularity
    tableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem(
        rTypr == PeriodType::MONTHLY ? tr("Month") : tr("Year")));

    // determine how many rows in the table. We display all month/years between
    // tomorrow and max date as established by the scenario
    QDate tomorrow = GbpController::getInstance().getTomorrow();
    QDate maxDate = tomorrow.addYears(scenario->getFeGenerationDuration()).addDays(-1);
    int noRows;
    int noOfMonths = 1 + (12*maxDate.year()+maxDate.month()) -
        (12*tomorrow.year()+tomorrow.month());
    int noOfYears = 1 + (maxDate.year()) - (tomorrow.year());
    if (rTypr==PeriodType::MONTHLY) {
        noRows = noOfMonths;
    } else{
        noRows = noOfYears;
    }

    // Generate the list of dates
    QList<QDate> dateList;
    QDate date = tomorrow;
    if (rTypr==PeriodType::MONTHLY) {
        for (int var = 0; var < noOfMonths; ++var) {
            dateList.append(QDate(date.year(),date.month(),1));
            date = date.addMonths(1);
        }
    } else{
        for (int var = 0; var < noOfMonths; ++var) {
            dateList.append(QDate(date.year(),1,1));
            date = date.addYears(1);
        }
    }

    // fill the table
    tableWidget->clearContents();
    tableWidget->setRowCount(noRows); // must be done BEFORE inserting item...
    int row = 0;
    QString s1,s2,s3,s4,s5,s6,s7,s8; // one for each column

    const QFont appFont = QApplication::font();
    QBrush negAmountColor = QBrush(GbpController::getInstance().getExpenseColor());
    QBrush posAmountColor = QBrush(GbpController::getInstance().getIncomeColor());
    foreach(QDate date, dateList){
        Bin bin;
        if ( true == binsPtr->contains(date)){
            bin = binsPtr->value(date);
        } else {
            bin = Bin(); // should never happen
        }

        // get growth values and build column items
        QStringList sl;
        binGrowthValuesToStrings(bin, sl, locale);
        s3 = sl[0]; // income growth
        s5 = sl[1]; // expense growth
        s7 = sl[2]; // delta growth
        if (rTypr==PeriodType::MONTHLY) {
            s1 = locale.toString(date,"yyyy MMMM");
        } else {
            s1 = locale.toString(date,"yyyy");
        }
        s2 = CurrencyHelper::formatAmount(bin.income,currInfo,locale,false);
        s4 = CurrencyHelper::formatAmount(bin.expense,currInfo,locale,false);
        s6 = CurrencyHelper::formatAmount(bin.delta,currInfo,locale,false);
        s8 = CurrencyHelper::formatAmount(bin.cashBalance,currInfo,locale,false);
        QTableWidgetItem* wi1 = new QTableWidgetItem(s1);
        QTableWidgetItem* wi2 = new QTableWidgetItem(s2);
        QTableWidgetItem* wi3 = new QTableWidgetItem(s3);
        QTableWidgetItem* wi4 = new QTableWidgetItem(s4);
        QTableWidgetItem* wi5 = new QTableWidgetItem(s5);
        QTableWidgetItem* wi6 = new QTableWidgetItem(s6);
        QTableWidgetItem* wi7 = new QTableWidgetItem(s7);
        QTableWidgetItem* wi8 = new QTableWidgetItem(s8);

        // format items
        wi1->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); // date
        wi2->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        wi3->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);  // income growth
        wi4->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        wi5->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);  // expense growth
        wi6->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        wi7->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);  // delta growth
        wi8->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        // set items' colors
        if(bin.incomeGrowthInPercentage.has_value()){
            if(bin.incomeGrowthInPercentage<0){
                wi3->setForeground(negAmountColor);
            } else if (bin.incomeGrowthInPercentage>0) {
                wi3->setForeground(posAmountColor);
            }
        }
        if(bin.expenseGrowthInPercentage.has_value()){
            if(bin.expenseGrowthInPercentage<0){
                wi5->setForeground(negAmountColor);
            } else if (bin.expenseGrowthInPercentage>0) {
                wi5->setForeground(posAmountColor);
            }
        }
        if(bin.deltaGrowthInPercentage.has_value()){
            if(bin.deltaGrowthInPercentage<0){
                wi7->setForeground(negAmountColor);
            } else if (bin.deltaGrowthInPercentage>0) {
                wi7->setForeground(posAmountColor);
            }
        }
        if(bin.delta<0){
            wi6->setForeground(negAmountColor);
        } else if (bin.delta>0) {
            wi6->setForeground(posAmountColor);
        }
        if(bin.cashBalance<0){
            wi8->setForeground(negAmountColor);
        } else if (bin.cashBalance>0) {
            wi8->setForeground(posAmountColor);
        }

        // associate data to items
        wi1->setData(Qt::UserRole,date);
        wi2->setData(Qt::UserRole,bin.income);
        wi3->setData(Qt::UserRole,QVariant::fromValue(bin.incomeGrowthInPercentage));
        wi4->setData(Qt::UserRole,bin.expense);
        wi5->setData(Qt::UserRole,QVariant::fromValue(bin.expenseGrowthInPercentage));
        wi6->setData(Qt::UserRole,bin.delta);
        wi7->setData(Qt::UserRole,QVariant::fromValue(bin.deltaGrowthInPercentage));
        wi8->setData(Qt::UserRole,bin.cashBalance);

        // add to intrinsic table model
        tableWidget->setItem(row,0,wi1);
        tableWidget->setItem(row,1,wi2);
        tableWidget->setItem(row,2,wi3);
        tableWidget->setItem(row,3,wi4);
        tableWidget->setItem(row,4,wi5);
        tableWidget->setItem(row,5,wi6);
        tableWidget->setItem(row,6,wi7);
        tableWidget->setItem(row,7,wi8);
        row++;
    }

    UiUtil::resizeTableColumns(ui->detailsReportTableWidget);

    tableWidget->scrollToTop();
}


void AnalysisDialog::binGrowthValuesToStrings(Bin bin, QStringList& strings, QLocale locale)
{
    strings.clear();

    // Income growth
    if (!bin.incomeGrowthInPercentage.has_value()) {
        strings.append(""); // no value defined
    } else {
        QString s = Util::formatDouble(bin.incomeGrowthInPercentage.value(), locale,
            Util::DoubleFormatMode::Mixed,
            {.standard={.noOfDecimals=1}, .exponential={.significantDigits=3},
            .mixed={.maxThreshold=100000, .minThreshold=0.1}});
        strings.append(s);
    }

    // Expense growth
    if (!bin.expenseGrowthInPercentage.has_value()) {
        strings.append(""); // no value defined
    } else {
        QString s = Util::formatDouble(bin.expenseGrowthInPercentage.value(), locale,
            Util::DoubleFormatMode::Mixed,
            {.standard={.noOfDecimals=1}, .exponential={.significantDigits=3},
            .mixed={.maxThreshold=100000, .minThreshold=0.1}});
        strings.append(s);
    }

    // Delta growth
    if (!bin.deltaGrowthInPercentage.has_value()) {
        strings.append(""); // no value defined
    } else {
        QString s = Util::formatDouble(bin.deltaGrowthInPercentage.value(), locale,
            Util::DoubleFormatMode::Mixed,
            {.standard={.noOfDecimals=1}, .exponential={.significantDigits=3},
            .mixed={.maxThreshold=100000, .minThreshold=0.1}});
        strings.append(s);
    }
}




void AnalysisDialog::periodListExport(PeriodType rType) {

    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario.isNull()){
        return; // if no scenario loaded (should not happen)
    }

    // *** STEP 1 : Build the columns definitions ***
    QList<CsvColumnDescriptor> columns;
    if (rType == PeriodType::MONTHLY) {
        columns.append({tr("Month"), CsvColumnType::date(CsvDateFormat::YearMonth,
            GbpController::getInstance().getExportTextDateLocalized())});
    } else {
        columns.append({tr("Year"), CsvColumnType::date(CsvDateFormat::Year,
            GbpController::getInstance().getExportTextDateLocalized())});
    }
    columns.append({tr("Income"), CsvColumnType::currency(
        GbpController::getInstance().getExportTextNumberLocalized())});
    columns.append({tr("Income growth"), CsvColumnType::numberFull(
        GbpController::getInstance().getExportTextNumberLocalized())});
    columns.append({tr("Expenses"), CsvColumnType::currency(
        GbpController::getInstance().getExportTextNumberLocalized())});
    columns.append({tr("Expense growth"), CsvColumnType::numberFull(
        GbpController::getInstance().getExportTextNumberLocalized())});
    columns.append({tr("Delta"), CsvColumnType::currency(
        GbpController::getInstance().getExportTextNumberLocalized())});
    columns.append({tr("Delta growth"), CsvColumnType::numberFull(
        GbpController::getInstance().getExportTextNumberLocalized())});
    columns.append({tr("Cash balance"), CsvColumnType::currency(
        GbpController::getInstance().getExportTextNumberLocalized())});


    // *** STEP 2 : Populate rows ***

    QList<QList<QVariant>> data;
    QMap<QDate,Bin>* binsPtr;
    if (rType == PeriodType::MONTHLY) {
        binsPtr = &binsMonthly;
    } else {
        binsPtr = &binsYearly;
    }

    for (QMap<QDate, Bin>::const_iterator it = binsPtr->constBegin(); it != binsPtr->constEnd(); ++it) {
        const QDate& date = it.key();
        const Bin&   bin  = it.value();
        data.append({date, bin.income,
            (bin.incomeGrowthInPercentage.has_value()?bin.incomeGrowthInPercentage.value():
                QVariant{}),
            bin.expense,
            (bin.expenseGrowthInPercentage.has_value()?bin.expenseGrowthInPercentage.value():
                QVariant{}),
            bin.delta,
            (bin.deltaGrowthInPercentage.has_value()?bin.deltaGrowthInPercentage.value():
                QVariant{}),
            bin.cashBalance}
        );
    }

    // *** STEP 3 : Call exportToCsv() and handle the result ***
    CsvExportResult result = CsvExporter::exportToCsv(
        (rType==PeriodType::MONTHLY)?("Monthly report"):("Yearly report"),
        columns, data, locale, currInfo, '\t' );
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





void AnalysisDialog::on_periodListMonthlyRadioButton_clicked()
{
    periodListRedisplayTableData(PeriodType::MONTHLY);
}



void AnalysisDialog::on_periodListAnnualRadioButton_clicked()
{
    periodListRedisplayTableData(PeriodType::YEARLY);
}


QList<QDate> AnalysisDialog::periodListGetListOfDatesCoveredbyBin(PeriodType type, QDate binDate)
{
    QList<QDate> answer;
    if (type == PeriodType::MONTHLY) {
        // Month
        QDate fDate = QDate(binDate.year(), binDate.month(), 1);
        QDate tDate = QDate(binDate.year(), binDate.month(), binDate.daysInMonth());
        QDate date = fDate;
        while(date <= tDate){
            // add the element to the list
            answer.append(date);
            // increase date
            date = date.addDays(1);
        }
    } else {
        // Year
        QDate fDate = QDate(binDate.year(),1,1);
        QDate tDate = QDate(binDate.year(),12,31);
        QDate date = fDate;
        while(date <= tDate){
            // add the element to the list
            answer.append(date);
            // increase date
            date = date.addDays(1);
        }
    }
    return answer;
}

