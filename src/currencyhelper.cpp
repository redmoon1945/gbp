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

#include <QLocale>
#include <QFontMetrics>
#include <QSet>
#include <cmath>
#include <limits>
#include "currencyhelper.h"
#include "util.h"


const quint64 CurrencyHelper::NATIVE_MAX_VALUE_ALLOWED =    99999999999999; //14 digits
const double CurrencyHelper::MAX_VALUE_ALLOWED_0_DECIMAL = 99999999999999;
const double CurrencyHelper::MAX_VALUE_ALLOWED_1_DECIMAL =  9999999999999.9;
const double CurrencyHelper::MAX_VALUE_ALLOWED_2_DECIMAL =   999999999999.99;
const double CurrencyHelper::MAX_VALUE_ALLOWED_3_DECIMAL =    99999999999.999;
const double CurrencyHelper::MAX_VALUE_ALLOWED_4_DECIMAL =     9999999999.9999;
const qint8 CurrencyHelper::MAX_NO_OF_DECIMALS = 4; // CLF and UYW have 4 fractional decimal units

CurrencyHelper::CurrencyHelper()
{
}


CurrencyInfo::CurrencyInfo()
    : name("US Dollar"), symbol("$"), isoCode("USD"), noOfDecimal(2)
{
}

CurrencyInfo::CurrencyInfo(QString aName, QString aSymbol, QString aIsoCode, int aNoOfDecimal)
    : name(aName), symbol(aSymbol), isoCode(aIsoCode), noOfDecimal(aNoOfDecimal)
{
}


quint8 CurrencyHelper::maxValueAllowedForNoOfDecimalsForCurrency()
{
    return MAX_NO_OF_DECIMALS;
}


quint64 CurrencyHelper::maxValueAllowedForAmount()
{
    return NATIVE_MAX_VALUE_ALLOWED;
}


double CurrencyHelper::maxValueAllowedForAmountInDouble(quint8 noOfDecimalDigits)
{
    switch (noOfDecimalDigits) {
        case 0: return MAX_VALUE_ALLOWED_0_DECIMAL;
        case 1: return MAX_VALUE_ALLOWED_1_DECIMAL;
        case 2: return MAX_VALUE_ALLOWED_2_DECIMAL;
        case 3: return MAX_VALUE_ALLOWED_3_DECIMAL;
        case 4: return MAX_VALUE_ALLOWED_4_DECIMAL;
        default:
            throw std::invalid_argument(QString("%1: noOfDigits is too big")
                .arg(Q_FUNC_INFO).toStdString());
    }
}


uint CurrencyHelper::maxCharForMaxAmountInDouble(quint8 noOfDecimalDigits)
{
    switch (noOfDecimalDigits) {
        case 0: return 14;
        case 1: return 15;
        case 2: return 15;
        case 3: return 15;
        case 4: return 15;
        default:
            QString fString = QString("%1").arg(Q_FUNC_INFO);
            QString s = QString("%1: noOfDigits %2 is too big")
                .arg(fString).arg(noOfDecimalDigits);
            throw std::invalid_argument(s.toStdString());
    }
}


double CurrencyHelper::amountQint64ToDouble(qint64 amount, quint8 noOfDecimal, int &result)
{
    // check if amount is above the max allowed
    if (abs(amount)>maxValueAllowedForAmount()){
        result = -1;
        return 0;
    }
    if (noOfDecimal > maxValueAllowedForNoOfDecimalsForCurrency()){
        result = -2;
        return 0;
    }
    result = 0;
    double d = static_cast<double>(amount)/Util::quickPow10(noOfDecimal);
    return d;
}


qint64 CurrencyHelper::amountDoubleToQint64(double amount, quint8 noOfDecimal, int &result)
{
    // check before using it
    if (noOfDecimal > maxValueAllowedForNoOfDecimalsForCurrency()){
        result = -2;
        return 0;
    }

    // NaN and infinite values cannot be converted (comparisons with NaN are always false, so
    // NaN would pass the max check below)
    if (!std::isfinite(amount)){
        result = -3;
        return 0;
    }

    // check if amount is above the max allowed.
    if (fabs(amount)>maxValueAllowedForAmountInDouble(noOfDecimal)){
        result = -1;
        return 0;
    }

    double d = amount*Util::quickPow10(noOfDecimal);
    double t = std::round(d);
    result = 0;
    return static_cast<qint64>(t);
}


QString CurrencyHelper::quint64ToDoubleString(qint64 amount, CurrencyInfo cInfo, QLocale locale,
    bool addISOcode, int &result)
{
    double d = amountQint64ToDouble( amount, cInfo.noOfDecimal, result);
    if (result != 0){
        return "";
    }
    return formatAmount(d, cInfo, locale, addISOcode);
}


QString CurrencyHelper::formatAmount(double amount, const CurrencyInfo &cInfo,
    const QLocale &locale, bool addISOcode)
{
    if (cInfo.noOfDecimal > MAX_NO_OF_DECIMALS) {
        return "";
    }
    if (fabs(amount) > maxValueAllowedForAmountInDouble(cInfo.noOfDecimal)) {
        return "";
    }
    QString s = Util::formatDouble(amount, locale, Util::DoubleFormatMode::Standard,
        {.standard={.noOfDecimals=cInfo.noOfDecimal}});
    if (addISOcode) {
        return s + ' ' + cInfo.isoCode;
    }
    return s;
}


QString CurrencyHelper::formatAmount(qint64 amount, const CurrencyInfo &cInfo,
    const QLocale &locale, bool addISOcode)
{
    int result;
    double d = amountQint64ToDouble(amount, cInfo.noOfDecimal, result);
    if (result != 0) {
        return "";
    }
    return formatAmount(d, cInfo, locale, addISOcode);
}


qint64 CurrencyHelper::add(qint64 a, qint64 b)
{
    constexpr qint64 MAX = NATIVE_MAX_VALUE_ALLOWED;
    constexpr qint64 QMIN = std::numeric_limits<qint64>::min();

    // Both non-negative
    if (a >= 0 && b >= 0) {
        if (a > MAX - b) {
            return MAX;
        }
        return a + b;
    }

    // Both negative
    if (a < 0 && b < 0) {
        if (a == QMIN || b == QMIN) {
            return -MAX;
        }
        if (a < -MAX - b) {
            return -MAX;
        }
        return a + b;
    }

    // Mixed signs: no overflow possible
    qint64 r = a + b;
    if (r > MAX) {
        return MAX;
    }
    if (r < -MAX) {
        return -MAX;
    }
    return r;

    // OLD VERSION
    // // potential for saturation here...but 2 "max allowed" added will be below max(qint64)
    // qint64 r = a + b;
    // // Handle overflow
    // if(r<0){
    //     if (r < -NATIVE_MAX_VALUE_ALLOWED){
    //         return -NATIVE_MAX_VALUE_ALLOWED ;
    //     } else {
    //         return r;
    //     }
    // } else {
    //     if (r > NATIVE_MAX_VALUE_ALLOWED){
    //         return NATIVE_MAX_VALUE_ALLOWED ;
    //     } else {
    //         return r;
    //     }
    // }
}


double CurrencyHelper::add(double a, double b, uint noOfDecimalDigits)
{
    double theMax = maxValueAllowedForAmountInDouble(noOfDecimalDigits);

    // Handle NaN inputs explicitly
    if (std::isnan(a) || std::isnan(b)) {
        return 0.0;
    }

    double result = a + b;

    // Handle NaN result (e.g., +Inf + -Inf)
    if (std::isnan(result)) {
        return 0.0;
    }

    // Clamp to allowed range (also handles ±Inf)
    if (result > theMax){
        return theMax;
    }
    if (result < -theMax) {
        return -theMax;
    }

    return result;
}


QMap<QString, QString> CurrencyHelper::getCurrencies(QLocale::Language language)
{
    QMap<QString, QString> result;
    for (auto it = currencies.constBegin(); it != currencies.constEnd(); ++it) {
        if (!it.value().selectable) {
            continue;
        }
        CurrencyInfo currInfo = makeCurrencyInfo(it.key(), it.value(), language);
        QString desc = QString("%1 (%2) - %3").arg(currInfo.isoCode).arg(currInfo.symbol)
            .arg(currInfo.name);
        result.insert(currInfo.isoCode, desc);
    }
    return result;
}


int CurrencyHelper::currencyAmountMaxPixelWidth(const CurrencyInfo &cInfo,
    const QLocale &locale, const QFontMetrics &fm)
{
    qint64 maxAmount = static_cast<qint64>(NATIVE_MAX_VALUE_ALLOWED);
    QString s = formatAmount(maxAmount, cInfo, locale, false);
    for (QChar &c : s) {
        if (c.isDigit()) c = '8';
    }
    return fm.horizontalAdvance(s);
}


CurrencyInfo CurrencyHelper::getCurrencyInfoFromIsoCode(
    const QString &isoCode, QLocale::Language language, bool& found)
{
    auto it = currencies.constFind(isoCode);
    if (it == currencies.constEnd()) {
        found = false;
        CurrencyInfo dummy("Unknown", "---", isoCode, 2);
        return dummy;
    }
    found = true;
    return makeCurrencyInfo(it.key(), it.value(), language);
}


bool CurrencyHelper::currencyIsoCodeExists(QString isoCode)
{
    return currencies.contains(isoCode);
}


CurrencyInfo CurrencyHelper::getCurrencyInfoFromCountryCode(
    const QString &countryCode, QLocale::Language language, bool& found)
{
    found = false;
    CurrencyInfo dummy("Unknown", "---", "---", 2);

    // 2 uppercase letters only (QLocale also accepts other forms, like UN M.49 region codes)
    if (countryCode.length() != 2) {
        return dummy;
    }
    for (QChar c : countryCode) {
        if (c < 'A' || c > 'Z') {
            return dummy;
        }
    }
    QLocale::Territory country = QLocale::codeToTerritory(countryCode);
    if (country == QLocale::AnyTerritory) {
        return dummy;
    }

    // All the Qt locales of that country must agree on one currency. Do not use
    // QLocale(AnyLanguage, country) : when Qt cannot find a locale for it (e.g. Vanuatu), it
    // silently returns the default locale, so the currency would depend on the user's system.
    QSet<QString> isoCodes;
    const QList<QLocale> locales = QLocale::matchingLocales(QLocale::AnyLanguage,
        QLocale::AnyScript, country);
    for (const QLocale &loc : locales) {
        isoCodes.insert(loc.currencySymbol(QLocale::CurrencyIsoCode));
    }
    if (isoCodes.size() != 1) {
        return dummy;
    }

    // The currency must be in our own list
    return getCurrencyInfoFromIsoCode(*isoCodes.constBegin(), language, found);
}


CurrencyInfo CurrencyHelper::makeCurrencyInfo(const QString &isoCode, const CurrencyDef &def,
    QLocale::Language language)
{
    QString name = (language == QLocale::Language::French) ? def.nameFr : def.nameEn;
    return CurrencyInfo(name, def.symbol, isoCode, def.noOfDecimal);
}


// Never remove an entry and never change its no of decimals : see the Doxygen of "currencies".
// To retire a currency, set "selectable" to false.
const QMap<QString, CurrencyHelper::CurrencyDef> CurrencyHelper::currencies = {
    {"AED", {"United Arab Emirates Dirham",
        "Dirham des Émirats arabes unis",
        "د.إ.\u200F", 2, true}},
    {"AFN", {"Afghan Afghani", "Afghani afghan", "؋", 2, true}},
    {"ALL", {"Albanian Lek", "Lek albanais", "Lekë", 2, true}},
    {"AMD", {"Armenian Dram", "Dram arménien", "֏", 2, true}},
    {"ANG", {"Netherlands Antillean Guilder",
        "Florin des Antilles néerlandaises",
        "ANG", 2, false}},
    {"AOA", {"Angolan Kwanza", "Kwanza angolais", "Kz", 2, true}},
    {"ARS", {"Argentine Peso", "Peso argentin", "$", 2, true}},
    {"AUD", {"Australian Dollar", "Dollar australien", "$", 2, true}},
    {"AWG", {"Aruban Florin", "Florin arubais", "Afl.", 2, true}},
    {"AZN", {"Azerbaijani Manat", "Manat azerbaïdjanais", "₼", 2, true}},
    {"BAM", {"Bosnia-Herzegovina Convertible Mark",
        "Mark convertible de Bosnie-Herzégovine",
        "KM", 2, true}},
    {"BBD", {"Barbadian Dollar", "Dollar barbadien", "$", 2, true}},
    {"BDT", {"Bangladeshi Taka", "Taka bangladais", "৳", 2, true}},
    {"BGN", {"Bulgarian Lev", "Lev bulgare", "BGN", 2, false}},
    {"BHD", {"Bahraini Dinar", "Dinar bahreïni", "د.ب.\u200F", 3, true}},
    {"BIF", {"Burundian Franc", "Franc burundais", "FBu", 0, true}},
    {"BMD", {"Bermudian Dollar", "Dollar bermudien", "$", 2, true}},
    {"BND", {"Brunei Dollar", "Dollar brunéien", "$", 2, true}},
    {"BOB", {"Bolivian Boliviano", "Boliviano bolivien", "Bs", 2, true}},
    {"BRL", {"Brazilian Real", "Réal brésilien", "R$", 2, true}},
    {"BSD", {"Bahamian Dollar", "Dollar bahamien", "$", 2, true}},
    {"BTC", {"Bitcoin", "Bitcoin", "BTC", 2, false}},
    {"BTN", {"Bhutanese Ngultrum", "Ngultrum bhoutanais", "Nu.", 2, true}},
    {"BWP", {"Botswanan Pula", "Pula botswanais", "P", 2, true}},
    {"BYN", {"Belarusian Ruble", "Rouble biélorusse", "Br", 2, true}},
    {"BZD", {"Belize Dollar", "Dollar bélizien", "$", 2, true}},
    {"CAD", {"Canadian Dollar", "Dollar canadien", "$", 2, true}},
    {"CDF", {"Congolese Franc", "Franc congolais", "FC", 2, true}},
    {"CHF", {"Swiss Franc", "Franc suisse", "fr", 2, true}},
    {"CLF", {"Chilean Unit of Account", "Unité de compte chilienne", "CLF", 4, false}},
    {"CLP", {"Chilean Peso", "Peso chilien", "$", 0, true}},
    {"CNY", {"Chinese Yuan", "Yuan chinois", "¥", 2, true}},
    {"COP", {"Colombian Peso", "Peso colombien", "$", 2, true}},
    {"CRC", {"Costa Rican Colón", "Colón costaricien", "₡", 2, true}},
    {"CUC", {"Cuban Convertible Peso", "Peso cubain convertible", "CUC", 2, false}},
    {"CUP", {"Cuban Peso", "Peso cubain", "$", 2, true}},
    {"CVE", {"Cape Verdean Escudo", "Escudo cap-verdien", "$", 2, true}},
    {"CZK", {"Czech Koruna", "Couronne tchèque", "Kč", 2, true}},
    {"DJF", {"Djiboutian Franc", "Franc djiboutien", "Fdj", 0, true}},
    {"DKK", {"Danish Krone", "Couronne danoise", "kr.", 2, true}},
    {"DOP", {"Dominican Peso", "Peso dominicain", "RD$", 2, true}},
    {"DZD", {"Algerian Dinar", "Dinar algérien", "د.ج.\u200F", 2, true}},
    {"EGP", {"Egyptian Pound", "Livre égyptienne", "ج.م.\u200F", 2, true}},
    {"ERN", {"Eritrean Nakfa", "Nakfa érythréen", "Nfk", 2, true}},
    {"ETB", {"Ethiopian Birr", "Birr éthiopien", "ብር", 2, true}},
    {"EUR", {"Euro", "Euro", "€", 2, true}},
    {"FJD", {"Fijian Dollar", "Dollar fidjien", "$", 2, true}},
    {"FKP", {"Falkland Islands Pound", "Livre des îles Malouines", "£", 2, true}},
    {"GBP", {"British Pound", "Livre sterling", "£", 2, true}},
    {"GEL", {"Georgian Lari", "Lari géorgien", "₾", 2, true}},
    {"GGP", {"Guernsey Pound", "Livre de Guernesey", "GGP", 2, false}},
    {"GHS", {"Ghanaian Cedi", "Cedi ghanéen", "GH₵", 2, true}},
    {"GIP", {"Gibraltar Pound", "Livre de Gibraltar", "£", 2, true}},
    {"GMD", {"Gambian Dalasi", "Dalasi gambien", "D", 2, true}},
    {"GNF", {"Guinean Franc", "Franc guinéen", "FG", 0, true}},
    {"GTQ", {"Guatemalan Quetzal", "Quetzal guatémaltèque", "Q", 2, true}},
    {"GYD", {"Guyanaese Dollar", "Dollar guyanien", "$", 2, true}},
    {"HKD", {"Hong Kong Dollar", "Dollar de Hong Kong", "HK$", 2, true}},
    {"HNL", {"Honduran Lempira", "Lempira hondurien", "L", 2, true}},
    {"HRK", {"Croatian Kuna", "Kuna croate", "HRK", 2, false}},
    {"HTG", {"Haitian Gourde", "Gourde haïtienne", "G", 2, true}},
    {"HUF", {"Hungarian Forint", "Forint hongrois", "Ft", 2, true}},
    {"IDR", {"Indonesian Rupiah", "Roupie indonésienne", "Rp", 2, true}},
    {"ILS", {"Israeli New Shekel", "Nouveau shekel israélien", "₪", 2, true}},
    {"IMP", {"Manx Pound", "Livre mannoise", "IMP", 2, false}},
    {"INR", {"Indian Rupee", "Roupie indienne", "₹", 2, true}},
    {"IQD", {"Iraqi Dinar", "Dinar irakien", "د.ع.\u200F", 3, true}},
    {"IRR", {"Iranian Rial", "Rial iranien", "ریال", 2, true}},
    {"ISK", {"Icelandic Króna", "Couronne islandaise", "kr.", 0, true}},
    {"JEP", {"Jersey Pound", "Livre de Jersey", "JEP", 2, false}},
    {"JMD", {"Jamaican Dollar", "Dollar jamaïcain", "$", 2, true}},
    {"JOD", {"Jordanian Dinar", "Dinar jordanien", "د.أ.\u200F", 3, true}},
    {"JPY", {"Yen", "Yen japonais", "¥", 0, true}},
    {"KES", {"Kenyan Shilling", "Shilling kényan", "Ksh", 2, true}},
    {"KGS", {"Kyrgyzstani Som", "Som kirghize", "сом", 2, true}},
    {"KHR", {"Cambodian Riel", "Riel cambodgien", "៛", 2, true}},
    {"KMF", {"Comorian Franc", "Franc comorien", "CF", 0, true}},
    {"KPW", {"North Korean Won", "Won nord-coréen", "₩", 2, true}},
    {"KRW", {"South Korean Won", "Won sud-coréen", "₩", 0, true}},
    {"KWD", {"Kuwaiti Dinar", "Dinar koweïtien", "د.ك.\u200F", 3, true}},
    {"KYD", {"Cayman Islands Dollar", "Dollar des îles Caïmans", "$", 2, true}},
    {"KZT", {"Kazakhstani Tenge", "Tenge kazakh", "₸", 2, true}},
    {"LAK", {"Lao Kip", "Kip laotien", "₭", 2, true}},
    {"LBP", {"Lebanese Pound", "Livre libanaise", "ل.ل.\u200F", 2, true}},
    {"LKR", {"Sri Lankan Rupee", "Roupie srilankaise", "රු.", 2, true}},
    {"LRD", {"Liberian Dollar", "Dollar libérien", "$", 2, true}},
    {"LSL", {"Lesotho Loti", "Loti lesothan", "LSL", 2, false}},
    {"LYD", {"Libyan Dinar", "Dinar libyen", "د.ل.\u200F", 3, true}},
    {"MAD", {"Moroccan Dirham", "Dirham marocain", "د.م.\u200F", 2, true}},
    {"MDL", {"Moldovan Leu", "Leu moldave", "L", 2, true}},
    {"MGA", {"Malagasy Ariary", "Ariary malgache", "Ar", 2, true}},
    {"MKD", {"Macedonian Denar", "Denar macédonien", "ден.", 2, true}},
    {"MMK", {"Myanmar Kyat", "Kyat birman", "K", 2, true}},
    {"MNT", {"Mongolian Tugrik", "Tugrik mongol", "₮", 2, true}},
    {"MOP", {"Macanese Pataca", "Pataca macanaise", "MOP$", 2, true}},
    {"MRU", {"Mauritanian Ouguiya", "Ouguiya mauritanienne", "أ.م.", 2, true}},
    {"MUR", {"Mauritian Rupee", "Roupie mauricienne", "Rs", 2, true}},
    {"MVR", {"Maldivian Rufiyaa", "Rufiyaa maldivienne", "ރ.", 2, true}},
    {"MWK", {"Malawian Kwacha", "Kwacha malawite", "MK", 2, true}},
    {"MXN", {"Mexican Peso", "Peso mexicain", "$", 2, true}},
    {"MYR", {"Malaysian Ringgit", "Ringgit malaisien", "RM", 2, true}},
    {"MZN", {"Mozambican Metical", "Metical mozambicain", "MTn", 2, true}},
    {"NAD", {"Namibian Dollar", "Dollar namibien", "$", 2, true}},
    {"NGN", {"Nigerian Naira", "Naira nigérian", "₦", 2, true}},
    {"NIO", {"Nicaraguan Córdoba", "Córdoba nicaraguayen", "C$", 2, true}},
    {"NOK", {"Norwegian Krone", "Couronne norvégienne", "kr", 2, true}},
    {"NPR", {"Nepalese Rupee", "Roupie népalaise", "नेरू", 2, true}},
    {"NZD", {"New Zealand Dollar", "Dollar néo-zélandais", "$", 2, true}},
    {"OMR", {"Omani Rial", "Rial omanais", "ر.ع.\u200F", 3, true}},
    {"PAB", {"Panamanian Balboa", "Balboa panaméen", "B/.", 2, true}},
    {"PEN", {"Peruvian Sol", "Sol péruvien", "S/", 2, true}},
    {"PGK", {"Papua New Guinean Kina", "Kina papouan-néo-guinéen", "K", 2, true}},
    {"PHP", {"Philippine Peso", "Peso philippin", "₱", 2, true}},
    {"PKR", {"Pakistani Rupee", "Roupie pakistanaise", "Rs", 2, true}},
    {"PLN", {"Polish Zloty", "Zloty polonais", "zł", 2, true}},
    {"PYG", {"Paraguayan Guaraní", "Guaraní paraguayen", "₲", 0, true}},
    {"QAR", {"Qatari Riyal", "Riyal qatari", "ر.ق.\u200F", 2, true}},
    {"RON", {"Romanian Leu", "Leu roumain", "lei", 2, true}},
    {"RSD", {"Serbian Dinar", "Dinar serbe", "RSD", 2, true}},
    {"RUB", {"Russian Rubles", "Rouble russe", "₽", 2, true}},
    {"RWF", {"Rwandan Franc", "Franc rwandais", "RF", 0, true}},
    {"SAR", {"Saudi Riyal", "Riyal saoudien", "ر.س.\u200F", 2, true}},
    {"SBD", {"Solomon Islands Dollar", "Dollar des îles Salomon", "$", 2, true}},
    {"SCR", {"Seychellois Rupee", "Roupie seychelloise", "SR", 2, true}},
    {"SDG", {"Sudanese Pound", "Livre soudanaise", "ج.س.", 2, true}},
    {"SEK", {"Swedish Krona", "Couronne suédoise", "kr", 2, true}},
    {"SGD", {"Singapore Dollar", "Dollar de Singapour", "$", 2, true}},
    {"SHP", {"Saint Helena Pound", "Livre de Sainte-Hélène", "£", 2, true}},
    {"SLE", {"Sierra Leonean Leone", "Leone sierra-léonais", "Le", 2, true}},
    {"SLL", {"Sierra Leonean Leone", "Leone sierra-léonais", "SLL", 2, false}},
    {"SOS", {"Somali Shilling", "Shilling somalien", "S", 2, true}},
    {"SRD", {"Surinamese Dollar", "Dollar surinamais", "$", 2, true}},
    {"SSP", {"South Sudanese Pound", "Livre sud-soudanaise", "£", 2, true}},
    {"STN", {"São Tomé and Príncipe Dobra", "Dobra de São Tomé et Príncipe", "Db", 2, true}},
    {"SVC", {"Salvadoran Colón", "Colón salvadorien", "SVC", 2, false}},
    {"SYP", {"Syrian Pound", "Livre syrienne", "ل.س.\u200F", 2, true}},
    {"SZL", {"Swazi Lilangeni", "Lilangeni swazi", "E", 2, true}},
    {"THB", {"Thai Baht", "Baht thaïlandais", "฿", 2, true}},
    {"TJS", {"Tajikistani Somoni", "Somoni tadjik", "сом.", 2, true}},
    {"TMT", {"Turkmenistani Manat", "Manat turkmène", "TMT", 2, true}},
    {"TND", {"Tunisian Dinar", "Dinar tunisien", "د.ت.\u200F", 3, true}},
    {"TOP", {"Tongan Paʻanga", "Paʻanga tongien", "T$", 2, true}},
    {"TRY", {"Turkish Lira", "Lire turque", "₺", 2, true}},
    {"TTD", {"Trinidad and Tobago Dollar", "Dollar de Trinité-et-Tobago", "$", 2, true}},
    {"TWD", {"New Taiwan Dollar", "Nouveau dollar taïwanais", "$", 2, true}},
    {"TZS", {"Tanzanian Shilling", "Shilling tanzanien", "TSh", 2, true}},
    {"UAH", {"Ukrainian Hryvnia", "Hryvnia ukrainienne", "₴", 2, true}},
    {"UGX", {"Ugandan Shilling", "Shilling ougandais", "USh", 0, true}},
    {"USD", {"US Dollar", "Dollar américain", "$", 2, true}},
    {"UYU", {"Uruguayan Peso", "Peso uruguayen", "$", 2, true}},
    {"UYW", {"Uruguayan Nominal Wage Index Unit",
        "Unité indexée sur les salaires nominaux uruguayens",
        "UYW", 4, false}},
    {"UZS", {"Uzbekistani Som", "Som ouzbek", "soʻm", 2, true}},
    {"VES", {"Venezuelan Bolívar", "Bolívar vénézuélien", "Bs.S", 2, true}},
    {"VND", {"Vietnamese Dong", "Dong vietnamien", "₫", 0, true}},
    {"VUV", {"Vanuatu Vatu", "Vatu vanuatuan", "VT", 0, true}},
    {"WST", {"Samoan Tala", "Tala samoan", "WS$", 2, true}},
    {"XAF", {"Central African CFA Franc", "Franc CFA d’Afrique centrale", "FCFA", 0, true}},
    {"XCD", {"East Caribbean Dollar", "Dollar des Caraïbes orientales", "$", 2, true}},
    {"XCG", {"Caribbean Guilder", "Florin caribéen", "Cg.", 2, true}},
    {"XOF", {"West African CFA Franc", "Franc CFA d’Afrique de l’Ouest", "F\u202FCFA", 0, true}},
    {"XPF", {"CFP Franc", "Franc CFP", "FCFP", 0, true}},
    {"YER", {"Yemeni Rial", "Rial yéménite", "ر.ي.\u200F", 2, true}},
    {"ZAR", {"South African Rand", "Rand sud-africain", "R", 2, true}},
    {"ZMW", {"Zambian Kwacha", "Kwacha zambien", "K", 2, true}},
    {"ZWG", {"Zimbabwean Gold", "Or zimbabwéen", "ZWG", 2, true}},
    {"ZWL", {"Zimbabwean Dollar", "Dollar zimbabwéen", "ZWL", 2, false}}
};
