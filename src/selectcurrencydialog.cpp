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

#include "selectcurrencydialog.h"
#include "gbplogger.h"
#include <QTimer>
#include "ui_selectcurrencydialog.h"
#include "util.h"
#include "uiutil.h"
#include <QCoreApplication>

SelectCurrencyDialog::SelectCurrencyDialog(QLocale theLocale, QWidget *parent) :
    QDialog(parent),locale(theLocale),
    ui(new Ui::SelectCurrencyDialog)
{
    ui->setupUi(this);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);

    // fill combobox with currencies sorted by ISO code (QMap is already sorted by key)
    QMap<QString, QString> currencies = CurrencyHelper::getCurrencies(locale.language());
    for (auto it = currencies.constBegin(); it != currencies.constEnd(); ++it) {
        ui->currenciesComboBox->addItem(it.value(), it.key()); // userData = ISO code
    }

    QFont appFont = QApplication::font();

    // reduce size of instructions
    QFont font = appFont;
    font.setItalic(true);
    Util::changeFontSize(font, Util::FontResizeIntensity::WEAK, true,
        "SelectCurrencyDialog - instructions");
    ui->instructions->setFont(font);

    // "pack" the dialog to fit the font. This is required when there is no "expanding" widgets
    this->adjustSize();

}


SelectCurrencyDialog::~SelectCurrencyDialog()
{
    delete ui;
}


void SelectCurrencyDialog::slotPrepareContent()
{
    // pre-select the currency of the system locale
    selectCurrencyInComboBox(locale.currencySymbol(QLocale::CurrencyIsoCode));
    updateCurrencyLabels();

    ui->currenciesComboBox->setFocus();
}


void SelectCurrencyDialog::on_selectPushButton_clicked()
{
    QString isoCode = getCurrentlySelectedIsoCode();
    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromIsoCode(isoCode,
        locale.language(), found);
    if(!found){
        return; // should never happen
    }
    emit signalSelectCountryResult(currInfo);
    hide();
    emit signalSelectCountryCompleted();    // follow our pattern, even if in this case, this is useless
}


void SelectCurrencyDialog::on_cancelPushButton_clicked()
{
    hide();
    emit signalSelectCountryCompleted();
}


void SelectCurrencyDialog::on_currenciesComboBox_activated(int index)
{
    updateCurrencyLabels();
}


// if isoCode=="", pre-select based on the system locale; fall back to CAD
void SelectCurrencyDialog::selectCurrencyInComboBox(QString isoCode)
{
    QString code = isoCode.isEmpty() ? locale.currencySymbol(QLocale::CurrencyIsoCode) : isoCode;
    int idx = ui->currenciesComboBox->findData(code);
    if (idx == -1) {
        idx = ui->currenciesComboBox->findData("CAD");
    }
    if (idx != -1) {
        ui->currenciesComboBox->setCurrentIndex(idx);
    }
}


void SelectCurrencyDialog::updateCurrencyLabels()
{
    QString isoCode = getCurrentlySelectedIsoCode();
    bool found;
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromIsoCode(isoCode,
        locale.language(), found);
    if(!found){
        return; // should never happen
    }
    ui->selectedCurrencyNameLabel->setText(currInfo.name);
    ui->selectedCurrencySymbolLabel->setText(currInfo.symbol);
    // Arabic currency symbols (e.g. AED "د.إ") are RTL text. Qt auto-detects the text direction
    // from content and re-applies RTL layout on every setText() call, which right-justifies the
    // label even when AlignLeft is set in the UI. Resetting layoutDirection and using
    // AlignAbsolute after setText() forces physical left alignment unconditionally.
    ui->selectedCurrencySymbolLabel->setLayoutDirection(Qt::LeftToRight);
    ui->selectedCurrencySymbolLabel->setAlignment(
        Qt::AlignAbsolute | Qt::AlignLeft | Qt::AlignVCenter);
    ui->selectedCurrencyIsoCodeLabel->setText(currInfo.isoCode);
}


QString SelectCurrencyDialog::getCurrentlySelectedIsoCode()
{
    return ui->currenciesComboBox->currentData().toString();
}


// User has manually closed the dialog
void SelectCurrencyDialog::on_SelectCurrencyDialog_rejected()
{
    on_cancelPushButton_clicked();
}


void SelectCurrencyDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("SelectCurrencyDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
    });
}

