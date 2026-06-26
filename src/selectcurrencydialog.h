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

#ifndef SELECTCURRENCYDIALOG_H
#define SELECTCURRENCYDIALOG_H

#include <QDialog>
#include "currencyhelper.h"

QT_BEGIN_NAMESPACE
namespace Ui {class SelectCurrencyDialog;}
QT_END_NAMESPACE


class SelectCurrencyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SelectCurrencyDialog(QLocale theLocale, QWidget *parent = nullptr);
    ~SelectCurrencyDialog();

public slots:
    // From client of SelectCurrencyDialog : prepare content before edition
    void slotPrepareContent();

signals:
    // For client of SelectCurrencyDialog : result of edition and edition completion notification
    void signalSelectCountryResult(CurrencyInfo currInfo);
    void signalSelectCountryCompleted();

private slots:
    void on_selectPushButton_clicked();
    void on_cancelPushButton_clicked();
    void on_currenciesComboBox_activated(int index);
    void on_SelectCurrencyDialog_rejected();

protected:
    void showEvent(QShowEvent* event) override;

private:
    Ui::SelectCurrencyDialog *ui;
    QLocale locale;                 // this is the system locale

    void selectCurrencyInComboBox(QString isoCode);
    void updateCurrencyLabels();
    QString getCurrentlySelectedIsoCode();
};

#endif // SELECTCURRENCYDIALOG_H
