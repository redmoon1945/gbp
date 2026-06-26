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

#ifndef CSDBREAKDOWNDIALOG_H
#define CSDBREAKDOWNDIALOG_H

#include <QDialog>
#include <qtablewidget.h>
#include "currencyhelper.h"
#include "festream.h"

namespace Ui {
class CsdBreakdownDialog;
}

class CsdBreakdownDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CsdBreakdownDialog(const QLocale &locale, QWidget *parent = nullptr);
    ~CsdBreakdownDialog();

public slots:

    /**
     * @brief To be called just before showing the Dialog. All the calculations to fill the tables
     * must be done in this methods.
     * @param newCurrInfo The currency info.
     * @param feStream The FeStream to breakdown.
     * @param maxDateScenario Max date above which no financial event can be generated.
     */
    void slotPrepareContent(const CurrencyInfo &newCurrInfo,
        QSharedPointer<const FeStream> feStream, QDate maxDateScenario);

signals:
    // For client of CsdBreakdownDialog : sending completion notification
    void signalCompleted();

private slots:
    void on_CsdBreakdownDialog_rejected();
    void on_closePushButton_clicked();
    void on_exportPushButton_clicked();
    void on_tabWidget_currentChanged(int index);

private:
    Ui::CsdBreakdownDialog *ui;

    enum class ReportType { MONTHLY, YEARLY };


    /**
     * @struct Item
     * @brief Represents a single row in the monthly or yearly breakdown table.
     */
    struct Item
    {
        QDate date;                             ///< Period start date (month or year).
        double amount;                          ///< Total amount for the period.
        std::optional<double> changeInPercent;  ///< Change vs. previous period in %; empty if none.

        Item();
    };


    QLocale locale;
    CurrencyInfo currInfo;

    // calculated monthly values (currency format). Index 0 is for month of Tomorrow.
    QList<Item> months;

    // calculated yearly values (currency format). Index 0 is for year of Tomorrow.
    QList<Item> years;

    /**
     * @brief Resizes the dialog width to fit the widest table once the dialog is shown.
     * @param event The show event forwarded to the base class.
     */
    void showEvent(QShowEvent* event) override;

    /**
     * @brief Export either the monthly or the yearly table data into a CSV file.
     * @param rType Type of data source.
     */
    void exportTextMonthlyYearlyReport(ReportType rType) ;
};

#endif // CSDBREAKDOWNDIALOG_H
