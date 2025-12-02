/*
 *  Copyright (C) 2024-2025 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
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

#ifndef CURRENCYHELPER_H
#define CURRENCYHELPER_H

#include <QtGlobal>
#include <QString>
#include <QLocale>
#include <QCoreApplication>
#include <QMap>


/**
 * @struct CurrencyInfo
 * @brief Information on a currency.
 */
struct CurrencyInfo{
    QString name;       // In English or French, depending on the requested language
    QString symbol;     // currency code, e.g. "$"
    QString isoCode;    // 3 char currency ISO code, e.g. "USD"
    int noOfDecimal;    // No of fractional decimal for that currency
};


/**
 * @class CurrencyHelper
 * @brief Helper methods provided to manipulate currency amounts, both in native format (qint64)
 * or natural format (double).
 * @details In Gbp, a currency amount is stored internally as a qint64. The decimal part is the
 * less significants digits. For example, 1245.67 dollars (it has 2 decimals) is represented as
 * the qint64 1234567. 12345 yen (it has 0 decimal) is represented as the qint64 12345.
 */
class CurrencyHelper
{
    Q_DECLARE_TR_FUNCTIONS(CurrencyHelper)

public:
    CurrencyHelper();

    /**
     * @brief Define the maximum no of decimal fractional units any currency can have.
     * @return The maximum no of decimal fractional units.
     */
    static quint8 maxValueAllowedForNoOfDecimalsForCurrency() ;

    /**
     * @brief Maximum value a positive amount can have in its "native" (stored) form of qint64, in
     * any currency, that is expressed in the fractional unit of the currency (e.g. in cents for
     * USD). For negative amount, it is the same limit but negative.
     * @details A limit to the max value of an amount is required, to detect the potential
     * overflowing of the quint64 storage before it occurs and also to limit the values to the
     * expected usage (good practice).
     * --- IMPORTANT --------------------------------------------------------------
     * We set arbitrarily the max value of an amount to be 14 significant digits.
     * ----------------------------------------------------------------------------
     * This is way more than what is required in typical GBP usages, even with comparatively
     * "depreciated" currencies. As an example, as of 2026,
     * -> USD (2 decimals), it corresponds to (1 trillion-0.01) dollars.
     * -> Yen (0 decimal), it corresponds to (100 trillion-1) yen.
     * -> CLF (4 decimals), it corresponds to roughly (10 billions -0.001) CLF
     * Also, importantly, we want the max to be storable in a double, which is often used to store
     * an amount in currency representation. Double can store 15 digits in all cases (garanteed,
     * it can be sometimes more).
     *      largest value of qint64             =  9 223 372 036 854 775 807 , that is 19 digits
     *      largest value of quint64            = 18 446 744 073 709 551 615 , that is 20 digits
     *      established limit of qint64 amount  =         99 999 999 999 999 , that is 14 digits
     * @see https://www.exploringbinary.com/decimal-precision-of-binary-floating-point-numbers/
     * @return The maximum value of any amount in quint64 format.
     */
    static quint64 maxValueAllowedForAmount();

    /**
     * @brief Max value for an amount expressed in natural currency unit.
     * @param noOfDecimalDigits No of fractional decimal units of the currency.
     * @throws std::invalid_argument if noOfDecimalDigits greater than the maximum allowed.
     * @return The max value.
     */
    static double maxValueAllowedForAmountInDouble(quint8 noOfDecimalDigits);

    /**
     * @brief Return the maximum no of characters for an unlocalized amount in natural format.
     * @details Natural format means 'f' format type, e.g. 123.45. Unlocalized means no group
     * separator.
     * @param noOfDecimalDigits No of fractional decimal units of the currency.
     * @return Return the maximum no of characters.
     */
    static uint maxCharForMaxAmountInDouble(quint8 noOfDecimalDigits);

    /**
     * @brief Convert an amount from native (qint64) representation to natural (double)
     * representation. Amount cannot be bigger than the max allowed. Conversion is exact (no digits
     * are lost).
     * @param amount The qint64 to be converted into double.
     * @param noOfDecimal No of decimal for the currency.
     * @param result 0 if success, -1 if absolute value of amount is bigger than the maximum
     * allowed, -2 if noOfDecimal is too big.
     * @return The double produced.
     */
    static double amountQint64ToDouble(qint64 amount, quint8 noOfDecimal, int &result)  ;

    /**
     * @brief Convert an amount (positive or negative) from natural (double) representation
     * to native (qint64) representation. Amount cannot be bigger than the max allowed.
     * Conversion is exact (no digits are lost) up to noOfDecimal.
     * @param amount The double to be converted to qint64.
     * @param noOfDecimal No of decimal for the currency.
     * @param result 0 if success, -1 if absolute value of amount is bigger than the maximum
     * allowed, -2 if noOfDecimal is too big.
     * @return The qint64 produced.
     */
    static qint64 amountDoubleToQint64(double amount, quint8 noOfDecimal, int &result) ;

    /**
     * @brief Convert an amount (positive or negative) from native (qint64) representation to a
     * String version of its natural (double) representation. Included are the decimal separator
     * and thousands separator, determined by the locale passed.
     * @details abs(Amount) cannot be bigger than the max allowed.
     * @param amount The qint64 amount to be converted into a String.
     * @param cInfo Info about the currency to use.
     * @param locale The QLocale to use to determine the decimal separator and thousands separator.
     * @param addISOcode If true, currency ISO code ill be added at the end of the String.
     * @param result 0 if success, -1 if absolute value of amount is bigger than the maximum
     * allowed, -2 if noOfDecimal is too big.
     * @return The String representation of amount.
     */
    static QString quint64ToDoubleString(qint64 amount, CurrencyInfo cInfo, QLocale locale,
        bool addISOcode, int &result)  ;

    /**
     * @brief Convert an amount from natural (double) representation to a String version. Included
     * are the decimal separator and thousands separator, determined by the locale passed.
     * @details Amount cannot be bigger than the max allowed.
     * @param amount The double amount to be converted into a String.
     * @param cInfo Info about the currency to use.
     * @param locale The QLocale to use to determine the decimal separator and thousands separator.
     * @param addISOcode If true, currency ISO code ill be added at the end of the String.
     * @return The String representation of amount.
     */
    static QString formatAmount(double amount, CurrencyInfo cInfo, QLocale locale, bool addISOcode)  ;

    /**
     * @brief Add 2 amounts (positive or negative) expressed in their native (qint64) format.
     * The result is capped to the maximum allowed for amount (negative or positive depending
     * on the result).
     * @param a First amount to add.
     * @param b Second amount to add.
     * @return Resulting amount, potentially capped to max value allowed (negative or positive
     * depending on the result).
     */
    static qint64 add(qint64 a, qint64 b );

    /**
     * @brief Return a list of 2-letter ISO 3166 alpha-2 country codes (key) and their
     * names (value). Names are provided in current Locale's language if available, otherwise
     * in English
     * @param theLocale The locale.
     * @return The list.
     */
    static QMap<QString, QString> getCountries(QLocale theLocale) ;

    /**
     * @brief Get a list associating 2-letter ISO 3166 alpha-2 country codes (key) with their
     * currency (value) in the form of a String with format "%1 (%2) - %3")
     * where %1=currency ISO code, %2=currency symbol, %3=currency name in the specified language.
     * @param language The language for the currency name (English or French).
     * @return The list.
     */
    static QMap<QString,QString> getCurrencies(QLocale::Language language);

    /**
     * @brief Indicates if a country exists in the GBP list of countries.
     * @param countryCode The 2-letter ISO 3166 alpha-2 country code.
     * @return True if it exists, false otherwise.
     */
    static bool countryExists(QString countryCode);

    /**
     * @brief Get CurrencyInfo for the currency tied to a specific country.
     * @details The info returned are:
     * - currency 3-letter ISO 3166 currency code.
     * - currency symbol.
     * - currency name in the specified language (English or French).
     * - No of decimal for that currency.
     * @param countryCode The 2-letter ISO 3166 alpha-2 country code.
     * @param language The language for the currency name (English or French).
     * @param found True if the country has been found, false otherwise.
     * @return The currency info.
     */
    static CurrencyInfo getCurrencyInfoFromCountryCode(QString countryCode,
        QLocale::Language language, bool& found );


private:

    /**
     * @brief Max value allowed for a positive amount, in native format (qint64).
     * Negative amount has the same limit but negative.
     * @see maxValueAllowedForAmount()
     */
    static const quint64 NATIVE_MAX_VALUE_ALLOWED;

    /**
     * @brief Max positive amount value in natural (double) format when currency
     * has 0 fractional decimal units. Negative amount has the same limit but negative.
     */
    static const double MAX_VALUE_ALLOWED_0_DECIMAL;

    /**
     * @brief Max positive amount value in natural (double) format when currency
     * has 1 fractional decimal units. Negative amount has the same limit but negative.
     */
    static const double MAX_VALUE_ALLOWED_1_DECIMAL;

    /**
     * @brief Max positive amount value in natural (double) format when currency
     * has 2 fractional decimal units. Negative amount has the same limit but negative.
     */
    static const double MAX_VALUE_ALLOWED_2_DECIMAL;

    /**
     * @brief Max positive amount value in natural (double) format when currency
     * has 3 fractional decimal units. Negative amount has the same limit but negative.
     */
    static const double MAX_VALUE_ALLOWED_3_DECIMAL;

    /**
     * @brief Max positive amount value in natural (double) format when currency
     * has 4 fractional decimal units. Negative amount has the same limit but negative.
     */
    static const double MAX_VALUE_ALLOWED_4_DECIMAL;

    /**
     * @brief Maximum no of decimal fractional unit a currency can have.
     * @see maxValueAllowedForNoOfDecimalsForCurrency()
     */
    static const qint8 MAX_NO_OF_DECIMALS;

    /**
     * @brief List of countries for which the currency has a no of decimal different from 2.
     * Key = 3-letter ISO 3166 currency code.
     * Value = No of decimals.
     */
    static QMap<QString,int> currencyDecimalDigits ;

    /**
     * @brief List of countries. Key is 2-letter ISO 3166 alpha-2 country codes, value
     * is country name in English
     * @details We maintain our own list, because Qt has bugs.
     * @see See https://github.com/umpirsky/country-list/tree/master/data
     */
    static QMap<QString,QString> countries ;

    /**
     * @brief List of countries in french. Key is 2-letter ISO 3166 alpha-2 country codes, value
     * is country name in French.
     * @details We maintain our own list, because Qt has bugs.
     */
    static QMap<QString,QString> countries_fr ;

    static QMap<QString, QString> currencyNamesEnglish;

    static QMap<QString, QString> currencyNamesFrench;
};

#endif // CURRENCYHELPER_H
