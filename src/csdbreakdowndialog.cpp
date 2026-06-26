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


#include "csdbreakdowndialog.h"
#include "csvexporter.h"
#include "gbplogger.h"
#include "ui_csdbreakdowndialog.h"
#include "gbpcontroller.h"
#include "gbpqmessage.h"
#include "uiutil.h"
#include <QApplication>
#include <QTimer>
#include <qfontdatabase.h>
#include "currencyhelper.h"
#include <QFileDialog>
#include <QMessageBox>
#include <algorithm>


CsdBreakdownDialog::CsdBreakdownDialog(const QLocale &locale, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CsdBreakdownDialog)
{
    ui->setupUi(this);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);

    this->locale = locale;

    QFont appFont = QApplication::font();

    // *** MONTHLY BREAKDOWN - TABLE CONTROLS ***

    // Explicitly set the app font: on Windows the platform theme overrides
    // QAbstractItemView with a smaller class-specific font (Segoe UI 9pt).
    ui->monthlyTableWidget->setFont(appFont);
    ui->monthlyTableWidget->setColumnCount(3);
    ui->monthlyTableWidget->setHorizontalHeaderLabels({tr("Month"),tr("Amount"), tr("Change (%)")});
    int maxNumCharsCurrency = 10;

    ui->monthlyTableWidget->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Interactive);

    // Set initial columns width

    QFontMetrics fmTable = QFontMetrics(appFont);
    int widthAmount = fmTable.horizontalAdvance(QString(9*1.4, '8'));
    ui->monthlyTableWidget->setColumnWidth(0, widthAmount); // month
    ui->monthlyTableWidget->setColumnWidth(1, widthAmount); // amount
    ui->monthlyTableWidget->setColumnWidth(2, widthAmount); // change

    ui->monthlyTableWidget->setSortingEnabled(false);
    ui->monthlyTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers); // no edition
    ui->monthlyTableWidget->horizontalHeader()->setFont(appFont);
    ui->monthlyTableWidget->horizontalHeader()->setMinimumHeight(fmTable.height() + 10);
    ui->monthlyTableWidget->verticalHeader()->hide();


    // *** YEARLY BREAKDOWN - TABLE CONTROLS ***

    // Same font override as monthlyTableWidget — see comment above.
    ui->yearlyTableWidget->setFont(appFont);
    ui->yearlyTableWidget->setColumnCount(3);
    ui->yearlyTableWidget->setHorizontalHeaderLabels({tr("Year"),tr("Amount"),tr("Change (%)")});

    ui->yearlyTableWidget->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Interactive);

    // Set initial columns width

    fmTable = QFontMetrics(appFont);

    widthAmount = fmTable.horizontalAdvance(QString(4*1.4, '8'));
    ui->yearlyTableWidget->setColumnWidth(0, widthAmount);  // year
    ui->yearlyTableWidget->setColumnWidth(1, widthAmount);  // amount
    ui->yearlyTableWidget->setColumnWidth(2, widthAmount);  // change

    ui->yearlyTableWidget->setSortingEnabled(false);
    ui->yearlyTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers); // no edition
    ui->yearlyTableWidget->horizontalHeader()->setFont(appFont);
    ui->yearlyTableWidget->horizontalHeader()->setMinimumHeight(fmTable.height() + 10);
    ui->yearlyTableWidget->verticalHeader()->hide();
}


CsdBreakdownDialog::~CsdBreakdownDialog()
{
    delete ui;
}


void CsdBreakdownDialog::slotPrepareContent(const CurrencyInfo &newCurrInfo,
    QSharedPointer<const FeStream> feStream, QDate maxDateScenario)
{
    // Remember currency info
    this->currInfo = newCurrInfo;

    // Calculate some information required
    QDate firstDate = GbpController::getInstance().getTomorrow();
    QDate lastDate = feStream->getLastDate();
    quint32 noOfDays = feStream->size();
    uint noOfMonthsCovered = 0;
    uint noOfYearsCovered = 0;
    try {
        noOfMonthsCovered = 1 + Util::noOfMonthsDifference(firstDate, lastDate);
    } catch (std::invalid_argument ex) {
        // should never happen
        LOG_ERROR(QString("noOfMonthsDifference unexpectedly throws a std::invalid_argument "
            "exception %1").arg(ex.what()));
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            tr("An unexpected error occured : %1. See log.").arg(ex.what()), {tr("OK")}, 0, 0);
        return;
    }
    try {
        noOfYearsCovered = 1 + Util::noOfYearsDifference(firstDate, lastDate);
    } catch (std::invalid_argument ex) {
        // should never happen
        LOG_ERROR(QString("noOfYearsDifference unexpectedly throws a std::invalid_argument "
            "exception %1").arg(ex.what()));
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            tr("An unexpected error occured : %1. See log.").arg(ex.what()), {tr("OK")}, 0, 0);
        return;
    }

    // Is it for an income or an expense
    QWeakPointer<Csd> wpCsd = feStream->getCsdPtr();
    QSharedPointer<Csd> spCsd = wpCsd.toStrongRef();
    if (spCsd.isNull()==true) {
        // should never happen
        LOG_ERROR(QString("FeStream weak pointer cannot be converted to shared pointer"));
        return;
    }
    bool isIncome = spCsd->getIsIncome();

    // re-allocate value vectors and init to 0.
    months.resize(noOfMonthsCovered);
    years.resize(noOfYearsCovered);

    // Fill vectors by iterating the FeStream. Calculation is done in double (currency unit).
    // Saturation is taken care of.
    QDate currentDate = firstDate;
    uint size = feStream->size();
    for (int i=0; i<size; i++) {
        qint64 amountInt = feStream->get(i);
        if ( (amountInt != 0) && (amountInt != -1) ) {
            // convert to double
            int result;
            double amountDouble = CurrencyHelper::amountQint64ToDouble(amountInt,
                currInfo.noOfDecimal, result);
            // for months
            uint monthsPassed = Util::noOfMonthsDifference(firstDate, currentDate);
            months[monthsPassed].amount = CurrencyHelper::add(
                months[monthsPassed].amount, amountDouble, currInfo.noOfDecimal);
            // for years
            uint yearsPassed = Util::noOfYearsDifference(firstDate, currentDate);
            years[yearsPassed].amount = CurrencyHelper::add(
                years[yearsPassed].amount, amountDouble, currInfo.noOfDecimal);
        }
        currentDate = currentDate.addDays(1);
    }

    // Then calculate the changes
    for(int i=0;i<noOfMonthsCovered;i++){
        std::optional<double> v; // initially holds no value (std::nullopt)
        months[i].changeInPercent = std::nullopt;
        if ( i > 0) {
            bool undefined = true;
            double tempV = Util::percentageChange(months[i-1].amount, months[i].amount, undefined);
            if( undefined == false ){
                months[i].changeInPercent = tempV;
            }
        }
    }
    for(int i=0;i<noOfYearsCovered;i++){
        std::optional<double> v; // initially holds no value (std::nullopt)
        years[i].changeInPercent = std::nullopt;
        if ( i > 0) {
            bool undefined = true;
            double tempV = Util::percentageChange(years[i-1].amount, years[i].amount, undefined);
            if( undefined == false ){
                years[i].changeInPercent = tempV;
            }
        }
    }

    // prepare some variables we need
    QBrush negAmountColor = QBrush(GbpController::getInstance().getExpenseColor());
    QBrush posAmountColor = QBrush(GbpController::getInstance().getIncomeColor());
    QString s1; // for col 1 (first one, date)
    QString s2; // for col 2 (amount)
    QString s3; // for column 3 (change)
    // fill monthly table. For each cell, add the currency value as "user data"
    ui->monthlyTableWidget->clearContents();
    ui->monthlyTableWidget->setRowCount(noOfMonthsCovered);
    currentDate = firstDate;
    for(int row=0; row<noOfMonthsCovered; row++){
        s1 = locale.toString(currentDate,"yyyy MMMM");
        s2 = CurrencyHelper::formatAmount(months[row].amount,currInfo,locale,false);
        if (months[row].changeInPercent.has_value()==true) {
            s3 = Util::formatDouble(months[row].changeInPercent.value(), locale,
                Util::DoubleFormatMode::Mixed,
                {.standard={.noOfDecimals=1}, .exponential={.significantDigits=3},
                 .mixed={.maxThreshold=100000, .minThreshold=0.1}});
        } else {
            s3 = "";
        }

        QTableWidgetItem* wi1 = new QTableWidgetItem(s1);
        QTableWidgetItem* wi2 = new QTableWidgetItem(s2);
        QTableWidgetItem* wi3 = new QTableWidgetItem(s3);

        wi1->setData(Qt::UserRole,currentDate);
        wi1->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

        wi2->setData(Qt::UserRole,months[row].amount);
        wi2->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        wi3->setData(Qt::UserRole,
            months[row].changeInPercent.has_value() ?
                QVariant(months[row].changeInPercent.value()) : QVariant());
        wi3->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

        // set color for amount
        if(months[row].amount != 0){
            if(isIncome==false){
                wi2->setForeground(negAmountColor);
            } else {
                wi2->setForeground(posAmountColor);
            }
        }

        // set color for change
        if( months[row].changeInPercent.has_value()==true ){
            if(months[row].changeInPercent.value() < 0){
                wi3->setForeground(negAmountColor);
            } else {
                wi3->setForeground(posAmountColor);
            }
        }

        ui->monthlyTableWidget->setItem(row,0,wi1);
        ui->monthlyTableWidget->setItem(row,1,wi2);
        ui->monthlyTableWidget->setItem(row,2,wi3);

        currentDate = currentDate.addMonths(1);
    }

    // fill yearly table. For each cell, add the currency value as "user data"
    ui->yearlyTableWidget->clearContents();
    ui->yearlyTableWidget->setRowCount(noOfYearsCovered);
    currentDate = firstDate;
    for(int row=0; row<noOfYearsCovered; row++){
        s1 = locale.toString(currentDate,"yyyy");
        s2 = CurrencyHelper::formatAmount(years[row].amount,currInfo,locale,false);
        if (years[row].changeInPercent.has_value()==true) {
            s3 = Util::formatDouble(years[row].changeInPercent.value(), locale,
                Util::DoubleFormatMode::Mixed,
                {.standard={.noOfDecimals=1}, .exponential={.significantDigits=3},
                 .mixed={.maxThreshold=100000, .minThreshold=0.1}});
        } else {
            s3 = "";
        }

        QTableWidgetItem* wi1 = new QTableWidgetItem(s1);
        QTableWidgetItem* wi2 = new QTableWidgetItem(s2);
        QTableWidgetItem* wi3 = new QTableWidgetItem(s3);

        wi1->setData(Qt::UserRole,currentDate);
        wi1->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

        wi2->setData(Qt::UserRole,years[row].amount);
        wi2->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        wi3->setData(Qt::UserRole, years[row].changeInPercent.has_value() ?
            QVariant(years[row].changeInPercent.value()) : QVariant());
        wi3->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

        // set color for amount
        if(years[row].amount != 0){
            if(isIncome==false){
                wi2->setForeground(negAmountColor);
            } else {
                wi2->setForeground(posAmountColor);
            }
        }

        // set color for change
        if( years[row].changeInPercent.has_value()==true ){
            if(years[row].changeInPercent.value() < 0){
                wi3->setForeground(negAmountColor);
            } else {
                wi3->setForeground(posAmountColor);
            }
        }

        ui->yearlyTableWidget->setItem(row,0,wi1);
        ui->yearlyTableWidget->setItem(row,1,wi2);
        ui->yearlyTableWidget->setItem(row,2,wi3);

        currentDate = currentDate.addYears(1);
    }

    // resize columns according to actual content
    UiUtil::resizeTableColumns(ui->monthlyTableWidget);
    UiUtil::resizeTableColumns(ui->yearlyTableWidget);

    ui->monthlyTableWidget->scrollToTop();
    ui->yearlyTableWidget->scrollToTop();


    ui->closePushButton->setFocus();

}


void CsdBreakdownDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);

    int scrollBarW = qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);

    auto tablePixelWidth = [&](QTableWidget* tw) -> int {
        int w = tw->frameWidth() * 2 + scrollBarW;
        for (int i = 0; i < tw->columnCount(); ++i)
            w += tw->columnWidth(i);
        if (tw->showGrid())
            w += tw->columnCount() - 1;
        return w;
    };

    int chromeMonthly = ui->monthlyTableWidget->mapTo(this, QPoint(0, 0)).x() * 2;
    int chromeYearly  = ui->yearlyTableWidget->mapTo(this, QPoint(0, 0)).x() * 2;
    int biggestTableWidth = std::max(
        tablePixelWidth(ui->monthlyTableWidget) + chromeMonthly,
        tablePixelWidth(ui->yearlyTableWidget)  + chromeYearly);
    int newDialogWidth = std::min(biggestTableWidth, 1300);
    resize(newDialogWidth, height());

    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("CsdBreakdownDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
    });
}


void CsdBreakdownDialog::on_CsdBreakdownDialog_rejected()
{
    on_closePushButton_clicked();
}


void CsdBreakdownDialog::on_closePushButton_clicked()
{
    this->hide();

    // cleanup value vectors
    months.clear();
    years.clear();

    // Signal the end
    emit signalCompleted();
}


void CsdBreakdownDialog::exportTextMonthlyYearlyReport(ReportType rType)
{
    // Get current scenario
    QSharedPointer<Scenario> scenario = GbpController::getInstance().getScenario().toStrongRef();
    if(scenario.isNull()){
        return; // if no scenario loaded (should not happen)
    }

    // *** STEP 1 : Build the columns definitions
    QList<CsvColumnDescriptor> columns;
    if (rType == ReportType::MONTHLY) {
        columns.append({tr("Month"), CsvColumnType::date(CsvDateFormat::YearMonth,
            GbpController::getInstance().getExportTextDateLocalized())});
    } else {
        columns.append({tr("Year"), CsvColumnType::date(CsvDateFormat::Year,
            GbpController::getInstance().getExportTextDateLocalized())});
    }
    columns.append({tr("Amount"), CsvColumnType::currency(
        GbpController::getInstance().getExportTextNumberLocalized())});
    columns.append({tr("Change (%)"), CsvColumnType::currency(
        GbpController::getInstance().getExportTextNumberLocalized())});

    // *** STEP 2 : Populate rows ***
    QList<QList<QVariant>> data;
    QDate tomorrow = GbpController::getInstance().getTomorrow();
    if (rType == ReportType::MONTHLY) {
        qsizetype size = months.size();
        QDate date = QDate(tomorrow.year(), tomorrow.month(),1);
        for (qsizetype i = 0; i < size; ++i) {
            std::optional<double> v = months.at(i).changeInPercent;
            if (v.has_value()==true) {
                data.append({date, months.at(i).amount,months.at(i).changeInPercent.value()});
            } else {
                data.append({date, months.at(i).amount,QVariant()});
            }
            date = date.addMonths(1);
        }
    } else {
        qsizetype size = years.size();
        QDate date = QDate(tomorrow.year(), 1, 1);
        for (qsizetype i = 0; i < size; ++i) {
            std::optional<double> v = years.at(i).changeInPercent;
            if (v.has_value()==true) {
                data.append({date, years.at(i).amount,years.at(i).changeInPercent.value()});
            } else {
                data.append({date, years.at(i).amount,QVariant()});
            }
            date = date.addYears(1);
        }
    }

    // *** STEP 3 : Call exportToCsv() and handle the result ***
    QString opName = QString("Csd Breakdown %1")
        .arg((rType == ReportType::MONTHLY)?("Monthly"):("Yearly"));
    CsvExportResult result = CsvExporter::exportToCsv(opName, columns, data, locale,
        currInfo, '\t');
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


void CsdBreakdownDialog::on_exportPushButton_clicked()
{
    int index = ui->tabWidget->currentIndex();

    switch (index) {
        case 0:
            // Monthly table report
            exportTextMonthlyYearlyReport(ReportType::MONTHLY);
            break;
        case 1:
            // Annual table report
            exportTextMonthlyYearlyReport(ReportType::YEARLY);
            break;
        default:
            return;
            break;
    }

}


void CsdBreakdownDialog::on_tabWidget_currentChanged(int index)
{
}




CsdBreakdownDialog::Item::Item() {
    date = QDate();
    amount = 0;
    changeInPercent = std::nullopt;
}
