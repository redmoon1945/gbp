/*
 *  Copyright (C) 2024 Claude Dumas <claudedumas63@protonmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SELECTCOUNTRYDIALOG_H
#define SELECTCOUNTRYDIALOG_H

#include <QDialog>
#include "currencyhelper.h"

QT_BEGIN_NAMESPACE
namespace Ui {class SelectCountryDialog;}
QT_END_NAMESPACE


class SelectCountryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SelectCountryDialog(QLocale theLocale, QWidget *parent = nullptr);
    ~SelectCountryDialog();

public slots:
    // From client of SelectCountryDialog : prepare content before edition
    void slotPrepareContent();

signals:
    // For client of SelectCountryDialog : result of edition and edition completion notification
    void signalSelectCountryResult(QString countryCode, CurrencyInfo currInfo);
    void signalSelectCountryCompleted();

private slots:
    void on_selectPushButton_clicked();
    void on_cancelPushButton_clicked();
    void on_countriesComboBox_activated(int index);
    void on_SelectCountryDialog_rejected();

private:
    Ui::SelectCountryDialog *ui;
    QLocale locale;                 // this is the system locale

    void selectCountryInComboBox(QString countryCode);
    void updateCurrencyLabels();
    QString getCurrentlySelectedCountryCode();
};

#endif // SELECTCOUNTRYDIALOG_H
