/*
 *  Copyright (C) 2024 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
 *  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
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

#ifndef CURRENCYHELPER_H
#define CURRENCYHELPER_H

#include <QtGlobal>
#include <QString>
#include <QLocale>
#include <QCoreApplication>
#include <QMap>



struct CurrencyInfo{
    QString name;
    QString symbol;     // e.g. "$"
    QString isoCode;    // e..g. "USD"
    int noOfDecimal;
};


// in Gbp, currency is stored as a qint64. The decimal part are the less significants digits. For example,
// * 1245.67 dollars (it has 2 decimals) is represented as the qint64 1234567
// * 12345 yen (it has 0 decimal) is represented as the qint64 12345
class CurrencyHelper
{
    // this is to be able to use the "tr" translation function
    Q_DECLARE_TR_FUNCTIONS(CurrencyHelper)

private:
    static QMap<QString,int> currencyDecimalDigits ;
    static QMap<QString,QString> countries ;
    static QMap<QString,QString> countries_fr ;

public:
    CurrencyHelper();

    static quint8 maxValueAllowedForNoOfDecimalsForCurrency();

    // Max value for the abs of an amount expressed in qint64.
    static qint64 maxValueAllowedForAmount();

    // Max value for an amount expressed in a currency unit
    static double maxValueAllowedForAmountInDouble(quint8 noOfDecimalDigits);

    // Max value for an amount expressed in a currency unit
    static int maxCharForMaxAmountInDouble(quint8 noOfDecimalDigits);


    // Convert an amount from qint64 to double representation. Amount cannot be bigger than the max allowed.
    // Conversion is exact (no digits are lost)
    // Result :
    //  0 : success
    // -1 : failed => Absolute value of Amount is bigger than the maximum allowed
    // -2 : failed => noOfDecimal is too big
    static double amountQint64ToDouble(qint64 amount, quint8 noOfDecimal, int &result)  ;

    // Convert an amount from double to qint64 representation. Amount cannot be bigger than the max allowed.
    // Conversion is exact (no digits are lost) up to noOfDecimal
    // Result :
    //  0 : success
    // -1 : failed => Absolute value of Amount is bigger than the maximum allowed
    // -2 : failed => noOfDecimal is too big
    static qint64 amountDoubleToQint64(double amount, quint8 noOfDecimal, int &result) ;

    // Convert an amount from qint64 representation to real currency unit string with decimal and thousands separator,
    // taking account the Locale passed. Currency ISO code is optional. Amount cannot be bigger than the max allowed.
    // Conversion is exact (no digits are lost)
    // Result :
    //  0 : success
    // -1 : failed => Absolute value of Amount is bigger than the maximum allowed
    // -2 : failed => noOfDecimal is too big
    static QString quint64ToDoubleString(quint64 amount, CurrencyInfo cInfo, QLocale locale, bool addISOcode, int &result)  ;

    // Convert an amount from double representation to real currency unit string with decimal and thousands separator,
    // taking account the Locale passed. Currency ISO code is optional. Amount cannot be bigger than the max allowed.
    // Conversion is exact (no digits are lost)
    static QString formatAmount(double amount, CurrencyInfo cInfo, QLocale locale, bool addISOcode)  ;

    // add two amounts and saturate if it goes above the max allowed
    static qint64 add(qint64 a, qint64 b );

    static QMap<QString, QString> getCountries(QLocale theLocale) ;
    static QMap<QString,QString> getCurrencies(QLocale systemLocale);
    static bool countryExists(QString countryCode);

    // Get Info related to a currency tied to a given country, identified by 3-characters ISO 3166
    static CurrencyInfo getCurrencyInfoFromCountryCode(QLocale systemLocale, QString countryCode, bool& found);

};

#endif // CURRENCYHELPER_H
