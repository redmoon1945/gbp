/*
 *  Copyright (C) 2024 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
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

#include "selectcountrydialog.h"
#include "ui_selectcountrydialog.h"
#include <QCoreApplication>

SelectCountryDialog::SelectCountryDialog(QLocale theLocale, QWidget *parent) :
    QDialog(parent),locale(theLocale),
    ui(new Ui::SelectCountryDialog)
{
    ui->setupUi(this);

    // fill combobox
    QList<QString> list = CurrencyHelper::getCountries(locale).values();
    list.sort();
    ui->countriesComboBox->addItems(list);
}


SelectCountryDialog::~SelectCountryDialog()
{
    delete ui;
}


void SelectCountryDialog::slotPrepareContent()
{
    // pre-select the country defined in the Locale
    selectCountryInComboBox("");
    updateCurrencyLabels();
}


void SelectCountryDialog::on_selectPushButton_clicked()
{
    // get selection
    QString countryCode = getCurrentlySelectedCountryCode();
    // get info about the associated currency
    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromCountryCode(locale, countryCode, found);
    if(!found){
        return; // should never happen
    }
    emit signalSelectCountryResult(countryCode, currInfo);
    hide();
    emit signalSelectCountryCompleted();    // follow our pattern, even if in this case, this is useless
}


void SelectCountryDialog::on_cancelPushButton_clicked()
{
    hide();
    emit signalSelectCountryCompleted();
}


void SelectCountryDialog::on_countriesComboBox_activated(int index)
{
    updateCurrencyLabels();
}


// if countryCode=="", then use the country associated to default Locale
void SelectCountryDialog::selectCountryInComboBox(QString countryCode)
{
    QString cCode;
    if (countryCode==""){
        // try to select the current Locale currency
        cCode = QLocale::territoryToCode(locale.territory());
    } else {
        cCode = countryCode;
    }
    // check that we have this country in our list
    QMap<QString, QString> countries = CurrencyHelper::getCountries(locale);
    if (!(countries.contains(cCode))){
        // set Canada as default
        cCode = "CA";
    }
    QString desc = countries.value(cCode);
    ui->countriesComboBox->setCurrentText(desc);
}


void SelectCountryDialog::updateCurrencyLabels()
{
    QString countryCode = getCurrentlySelectedCountryCode();
    // get info about the selected currency and format it in the label
    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromCountryCode(locale, countryCode, found);
    if(!found){
        return; // should never happen
    }
    ui->selectedCurrencyNameLabel->setText(currInfo.name);
    ui->selectedCurrencySymbolLabel->setText(currInfo.symbol);
    ui->selectedCurrencyIsoCodeLabel->setText(currInfo.isoCode);
}


QString SelectCountryDialog::getCurrentlySelectedCountryCode()
{
    QString comboText = ui->countriesComboBox->currentText();
    QMap<QString, QString> countries = CurrencyHelper::getCountries(locale);
    QString countryCode = countries.key(comboText);
    return countryCode;
}


// User has manually closed the dialog
void SelectCountryDialog::on_SelectCountryDialog_rejected()
{
    on_cancelPushButton_clicked();
}

