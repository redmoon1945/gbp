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

#include <QLocale>
#include "currencyhelper.h"
#include "util.h"
#include <limits>




CurrencyHelper::CurrencyHelper()
{
}


// no currency on Earth has more than 3 decimals
quint8 CurrencyHelper::maxValueAllowedForNoOfDecimalsForCurrency()
{
    return 3;
}


// A limit to the max value of an amount is required, to detect the potential overflowing the
// qint64 storage before it occurs and also to limit the values to the expected usage (good
// practice). We set arbitrarily the max limit of an amount to 1000 trillion - 1, assuming max
// number of decimals in a currency (3). This is way more than what is required in typical GBP
// usage, even with very depreciated currencies. Also, importantly, we want the max to be storable
// in a double, which is needed when comparing values. Double can store 15 digits in all cases
// (garanteed, it can be sometimes more).
//     largest value of qint64      =  9 223 372 036 854 775 807 , that is 19 digits
//     largest value of quint64     = 18 446 744 073 709 551 615 , that is 20 digits
//     established limit of amount  =    999 999 999 999 999 999 , that is 15 digits
qint64 CurrencyHelper::maxValueAllowedForAmount()
{
    return 999999999999999; //15 digits
}


// we expect noOfDigits to always be in the valid range, hence the error triggering an exception
// instead of setting a "result" parameter that would have to be checked at every call
double CurrencyHelper::maxValueAllowedForAmountInDouble(quint8 noOfDecimalDigits)
{
    if (noOfDecimalDigits > maxValueAllowedForNoOfDecimalsForCurrency()){
        throw std::invalid_argument("noOfDigits is too big");
    }
    qint64 max = maxValueAllowedForAmount();
    long double ld = static_cast<long double>(max)/Util::quickPow10(noOfDecimalDigits);
    double d = static_cast<double>(ld);
    return d;   // we know there wont be loss of precision
}


// Not localized. Minus sign not included
int CurrencyHelper::maxCharForMaxAmountInDouble(quint8 noOfDecimalDigits)
{
    double d = maxValueAllowedForAmountInDouble(noOfDecimalDigits);
    QString s = QString::number(d, 'f', noOfDecimalDigits);
    return s.length();
}


// No of guaranteed decimal digits precision :
// float : 6
// double : 15
// long double : 18
// qint64 : 18 (max is 9223372036854775808, 9999999999999999999 cannot be stored)
// https://www.exploringbinary.com/decimal-precision-of-binary-floating-point-numbers/
// So a qint64 can be stored completely in a long double and vice versa.
// But since we limit max value of an amount (qint64) to 15 digits), it will always fit in a double.
// If success, return result = 0;
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
    long double ld = static_cast<long double>(amount)/Util::quickPow10(noOfDecimal);
    return static_cast<double>(ld);// we know there wont be loss of precision
}


qint64 CurrencyHelper::amountDoubleToQint64(double amount, quint8 noOfDecimal, int &result)
{
    // check before using it
    if (noOfDecimal > maxValueAllowedForNoOfDecimalsForCurrency()){
        result = -2;
        return 0;
    }
    // check if amount is above the max allowed. If it pass, result will also pass
    if (fabs(amount)>maxValueAllowedForAmountInDouble(noOfDecimal)){
        result = -1;
        return 0;
    }
    long double ld = amount*Util::quickPow10(noOfDecimal);
    long double t = std::round(ld);

    if ( (t > std::numeric_limits<qint64>::max()) ||
        (t < std::numeric_limits<qint64>::min()) ){ // long double needed to compared to quint64
        // should never happen
        throw std::invalid_argument("result is too big");
    }
    result = 0;
    return static_cast<qint64>(t); // loss of precision possible here
}


// Format a raw amount (qint64) to currency string, taking into account the locale's decimal point and group separator
QString CurrencyHelper::quint64ToDoubleString(quint64 amount, CurrencyInfo cInfo, QLocale locale, bool addISOcode, int &result)
{
    double d = amountQint64ToDouble( amount, cInfo.noOfDecimal, result);
    if (result != 0){
        return "";
    }
    QString s = locale.toString(d,'f',cInfo.noOfDecimal);
    if (addISOcode) {
        return QString("%1 %2").arg(s).arg(cInfo.isoCode);
    } else{
        return s;
    }
}


// Format a double amount to currency string, taking into account the locale's decimal point and group separator
QString CurrencyHelper::formatAmount(double amount, CurrencyInfo cInfo, QLocale locale, bool addISOcode)
{
    QString s = locale.toString(amount,'f',cInfo.noOfDecimal);
    if (addISOcode) {
        return QString("%1 %2").arg(s).arg(cInfo.isoCode);
    } else{
        return s;
    }
}


qint64 CurrencyHelper::add(qint64 a, qint64 b)
{
    qint64 r = a + b;   // potential for saturation here...but 2 "max allowed" added will be below max(qint64)
    qint64 max = maxValueAllowedForAmount();
    if(r<0){
        if (r < -max){
            return -max ;
        } else {
            return r;
        }
    } else {
        if (r > max){
            return max ;
        } else {
            return r;
        }
    }
}


// Return a list of 2-letter country codes (key) and their names (value). Name are provided in current Locale's language if available,
// otherwise in English
QMap<QString, QString> CurrencyHelper::getCountries(QLocale theLocale)
{
    if ( theLocale.language() == QLocale::Language::French ){
        return countries_fr;
    } else {
        return countries;
    }
}


// build from list of countries. Key is currency code, value is Description
QMap<QString, QString> CurrencyHelper::getCurrencies(QLocale systemLocale)
{
    QMap<QString, QString> result;
    for (auto it = countries.begin(); it != countries.end(); ++it) {
        QString countryCode = it.key();
        // get info about associated currency for that country
        bool found;
        CurrencyInfo currInfo = getCurrencyInfoFromCountryCode(systemLocale, countryCode, found);
        if(!found){
            continue; // should not happen
        }
        // add to the map
        QString desc = QString("%1 (%2) - %3").arg(currInfo.isoCode).arg(currInfo.symbol).arg(currInfo.name);
        if ( !(result.contains(currInfo.isoCode))){
            result.insert(currInfo.isoCode,desc);
        }
     }
    return result;
}


bool CurrencyHelper::countryExists(QString countryCode)
{
    if(countries.contains(countryCode)){
        return true;
    }else{
        return false;
    }
}

// if not found, found is set to false, true otherwise
// Special patch for  USA' where curency name is fixed.
CurrencyInfo CurrencyHelper::getCurrencyInfoFromCountryCode(QLocale systemLocale, QString countryCode, bool& found)
{
    found = false;
    CurrencyInfo currInfo;
    QLocale::Territory country = QLocale::codeToTerritory(countryCode);
    if (country==QLocale::AnyTerritory){
        CurrencyInfo dummy = {.name="Unknown", .symbol="---", .isoCode="---", .noOfDecimal=2};
        return dummy;
    }
    found = true;

    QLocale loc(QLocale::Language::AnyLanguage,country);
    currInfo.isoCode = loc.currencySymbol(QLocale::CurrencyIsoCode);
    currInfo.symbol = loc.currencySymbol(QLocale::CurrencySymbol);
    currInfo.name = loc.currencySymbol(QLocale::CurrencyDisplayName);

    // patches for missing data from Qt
    if (currInfo.name=="") {
        if(currInfo.isoCode=="MVR"){                    // Maldives
            currInfo.name = "Maldivian Rufiyaa";
        } else if (currInfo.isoCode=="NGN") {           // Nigeria
            currInfo.name = "Nigerian Naira";
        } else if (currInfo.isoCode=="PGK") {           // Papua New Guinean
            currInfo.name = "Papua New Guinean Kina";
        } else if (currInfo.isoCode=="GBP") {           // UK
            currInfo.name = "British Pound";
        } else if (currInfo.isoCode=="RWF") {           // Rwanda
            currInfo.name = "Rwandan Franc";
        } else if (currInfo.isoCode=="ZMW") {           // Zambia
            currInfo.name = "Zambian Kwacha";
        } else if (currInfo.isoCode=="PAB") {           // Panama
            currInfo.name = "Balboa Panameño";
        } else if (currInfo.isoCode=="PYG") {           // Paraguay
            currInfo.name = "Paraguayan Guaraní";
        } else {
            currInfo.name = "Unknown";
        }
    }
    if (currInfo.symbol=="") {
        if (currInfo.isoCode=="MVR"){                   // Maldives
            currInfo.symbol="Rf";
        } else if (currInfo.isoCode=="PGK"){            // Papua New Guinean
            currInfo.symbol = "K";
        } else {
            currInfo.symbol= "Unknown";
        }
    }
    if (currInfo.isoCode=="CVE"){
        currInfo.symbol="$";                            // something is returned by Qt but it is not displayable (???)
    }
    if (currInfo.isoCode=="USD"){
        currInfo.name = "US Dollar";
    }

    currInfo.noOfDecimal = currencyDecimalDigits.value(currInfo.isoCode,2);
    return currInfo;
}


// list of countries for which the currency has a no of decimal different from 2
QMap<QString,int> CurrencyHelper::currencyDecimalDigits = {
    {"BHD",3},{"BIF",0},{"CLF",4},{"CLP",0},{"DJF",0},{"GNF",0},{"IQD",3},{"ISK",0},{"JOD",3},{"JPY",0},
    {"KMF",0},{"KRW",0},{"KWD",3},{"LYD",3},{"OMR",3},{"PYG",0},{"RWF",0},{"TND",3},{"UGX",0},{"UYI",0},
    {"UYW",4},{"VND",0},{"VUV",0},{"XAF",0},{"XOF",0},{"XPF",0}
};


// DEFAULT (English)
// must be ISO 3166 alpha-2
// See https://github.com/umpirsky/country-list/tree/master/data
QMap<QString,QString> CurrencyHelper::countries = {
    {"AF","Afghanistan"},
    {"AX","Åland Islands"},
    {"AL","Albania"},
    {"DZ","Algeria"},
    {"AS","American Samoa"},
    {"AD","Andorra"},
    {"AO","Angola"},
    {"AI","Anguilla"},
    {"AQ","Antarctica"},
    {"AG","Antigua & Barbuda"},
    {"AR","Argentina"},
    {"AM","Armenia"},
    {"AW","Aruba"},
    {"AU","Australia"},
    {"AT","Austria"},
    {"AZ","Azerbaijan"},
    {"BS","Bahamas"},
    {"BH","Bahrain"},
    {"BD","Bangladesh"},
    {"BB","Barbados"},
    {"BY","Belarus"},
    {"BE","Belgium"},
    {"BZ","Belize"},
    {"BJ","Benin"},
    {"BM","Bermuda"},
    {"BT","Bhutan"},
    {"BO","Bolivia"},
    {"BA","Bosnia & Herzegovina"},
    {"BW","Botswana"},
    {"BV","Bouvet Island"},
    {"BR","Brazil"},
    {"IO","British Indian Ocean Territory"},
    {"VG","British Virgin Islands"},
    {"BN","Brunei"},
    {"BG","Bulgaria"},
    {"BF","Burkina Faso"},
    {"BI","Burundi"},
    {"KH","Cambodia"},
    {"CM","Cameroon"},
    {"CA","Canada"},
    {"CV","Cape Verde"},
    {"BQ","Caribbean Netherlands"},
    {"KY","Cayman Islands"},
    {"CF","Central African Republic"},
    {"TD","Chad"},
    {"CL","Chile"},
    {"CN","China"},
    {"CX","Christmas Island"},
    {"CC","Cocos (Keeling) Islands"},
    {"CO","Colombia"},
    {"KM","Comoros"},
    {"CG","Congo - Brazzaville"},
    {"CD","Congo - Kinshasa"},
    {"CK","Cook Islands"},
    {"CR","Costa Rica"},
    {"CI","Côte d’Ivoire"},
    {"HR","Croatia"},
    {"CU","Cuba"},
    {"CW","Curaçao"},
    {"CY","Cyprus"},
    {"CZ","Czechia"},
    {"DK","Denmark"},
    {"DJ","Djibouti"},
    {"DM","Dominica"},
    {"DO","Dominican Republic"},
    {"EC","Ecuador"},
    {"EG","Egypt"},
    {"SV","El Salvador"},
    {"GQ","Equatorial Guinea"},
    {"ER","Eritrea"},
    {"EE","Estonia"},
    {"SZ","Eswatini"},
    {"ET","Ethiopia"},
    {"FK","Falkland Islands"},
    {"FO","Faroe Islands"},
    {"FJ","Fiji"},
    {"FI","Finland"},
    {"FR","France"},
    {"GF","French Guiana"},
    {"PF","French Polynesia"},
    {"TF","French Southern Territories"},
    {"GA","Gabon"},
    {"GM","Gambia"},
    {"GE","Georgia"},
    {"DE","Germany"},
    {"GH","Ghana"},
    {"GI","Gibraltar"},
    {"GR","Greece"},
    {"GL","Greenland"},
    {"GD","Grenada"},
    {"GP","Guadeloupe"},
    {"GU","Guam"},
    {"GT","Guatemala"},
    {"GG","Guernsey"},
    {"GN","Guinea"},
    {"GW","Guinea-Bissau"},
    {"GY","Guyana"},
    {"HT","Haiti"},
    {"HM","Heard & McDonald Islands"},
    {"HN","Honduras"},
    {"HK","Hong Kong SAR China"},
    {"HU","Hungary"},
    {"IS","Iceland"},
    {"IN","India"},
    {"ID","Indonesia"},
    {"IR","Iran"},
    {"IQ","Iraq"},
    {"IE","Ireland"},
    {"IM","Isle of Man"},
    {"IL","Israel"},
    {"IT","Italy"},
    {"JM","Jamaica"},
    {"JP","Japan"},
    {"JE","Jersey"},
    {"JO","Jordan"},
    {"KZ","Kazakhstan"},
    {"KE","Kenya"},
    {"KI","Kiribati"},
    {"KW","Kuwait"},
    {"KG","Kyrgyzstan"},
    {"LA","Laos"},
    {"LV","Latvia"},
    {"LB","Lebanon"},
    {"LS","Lesotho"},
    {"LR","Liberia"},
    {"LY","Libya"},
    {"LI","Liechtenstein"},
    {"LT","Lithuania"},
    {"LU","Luxembourg"},
    {"MO","Macao SAR China"},
    {"MG","Madagascar"},
    {"MW","Malawi"},
    {"MY","Malaysia"},
    {"MV","Maldives"},
    {"ML","Mali"},
    {"MT","Malta"},
    {"MH","Marshall Islands"},
    {"MQ","Martinique"},
    {"MR","Mauritania"},
    {"MU","Mauritius"},
    {"YT","Mayotte"},
    {"MX","Mexico"},
    {"FM","Micronesia"},
    {"MD","Moldova"},
    {"MC","Monaco"},
    {"MN","Mongolia"},
    {"ME","Montenegro"},
    {"MS","Montserrat"},
    {"MA","Morocco"},
    {"MZ","Mozambique"},
    {"MM","Myanmar (Burma)"},
    {"NA","Namibia"},
    {"NR","Nauru"},
    {"NP","Nepal"},
    {"NL","Netherlands"},
    {"NC","New Caledonia"},
    {"NZ","New Zealand"},
    {"NI","Nicaragua"},
    {"NE","Niger"},
    {"NG","Nigeria"},
    {"NU","Niue"},
    {"NF","Norfolk Island"},
    {"KP","North Korea"},
    {"MK","North Macedonia"},
    {"MP","Northern Mariana Islands"},
    {"NO","Norway"},
    {"OM","Oman"},
    {"PK","Pakistan"},
    {"PW","Palau"},
    {"PS","Palestinian Territories"},
    {"PA","Panama"},
    {"PG","Papua New Guinea"},
    {"PY","Paraguay"},
    {"PE","Peru"},
    {"PH","Philippines"},
    {"PN","Pitcairn Islands"},
    {"PL","Poland"},
    {"PT","Portugal"},
    {"PR","Puerto Rico"},
    {"QA","Qatar"},
    {"RE","Réunion"},
    {"RO","Romania"},
    {"RU","Russia"},
    {"RW","Rwanda"},
    {"WS","Samoa"},
    {"SM","San Marino"},
    {"ST","São Tomé & Príncipe"},
    {"SA","Saudi Arabia"},
    {"SN","Senegal"},
    {"RS","Serbia"},
    {"SC","Seychelles"},
    {"SL","Sierra Leone"},
    {"SG","Singapore"},
    {"SX","Sint Maarten"},
    {"SK","Slovakia"},
    {"SI","Slovenia"},
    {"SB","Solomon Islands"},
    {"SO","Somalia"},
    {"ZA","South Africa"},
    {"GS","South Georgia & South Sandwich Islands"},
    {"KR","South Korea"},
    {"SS","South Sudan"},
    {"ES","Spain"},
    {"LK","Sri Lanka"},
    {"BL","St. Barthélemy"},
    {"SH","St. Helena"},
    {"KN","St. Kitts & Nevis"},
    {"LC","St. Lucia"},
    {"MF","St. Martin"},
    {"PM","St. Pierre & Miquelon"},
    {"VC","St. Vincent & Grenadines"},
    {"SD","Sudan"},
    {"SR","Suriname"},
    {"SJ","Svalbard & Jan Mayen"},
    {"SE","Sweden"},
    {"CH","Switzerland"},
    {"SY","Syria"},
    {"TW","Taiwan"},
    {"TJ","Tajikistan"},
    {"TZ","Tanzania"},
    {"TH","Thailand"},
    {"TL","Timor-Leste"},
    {"TG","Togo"},
    {"TK","Tokelau"},
    {"TO","Tonga"},
    {"TT","Trinidad & Tobago"},
    {"TN","Tunisia"},
    {"TR","Turkey"},
    {"TM","Turkmenistan"},
    {"TC","Turks & Caicos Islands"},
    {"TV","Tuvalu"},
    {"UM","U.S. Outlying Islands"},
    {"VI","U.S. Virgin Islands"},
    {"UG","Uganda"},
    {"UA","Ukraine"},
    {"AE","United Arab Emirates"},
    {"GB","United Kingdom"},
    {"US","United States"},
    {"UY","Uruguay"},
    {"UZ","Uzbekistan"},
    {"VU","Vanuatu"},
    {"VA","Vatican City"},
    {"VE","Venezuela"},
    {"VN","Vietnam"},
    {"WF","Wallis & Futuna"},
    {"EH","Western Sahara"},
    {"YE","Yemen"},
    {"ZM","Zambia"},
    {"ZW","Zimbabwe"},

};

// FRENCH
// must be ISO 3166 alpha-2
// See https://github.com/umpirsky/country-list/tree/master/data
QMap<QString,QString> CurrencyHelper::countries_fr = {
    {"AF","Afghanistan"},
    {"ZA","Afrique du Sud"},
    {"AL","Albanie"},
    {"DZ","Algérie"},
    {"DE","Allemagne"},
    {"AD","Andorre"},
    {"AO","Angola"},
    {"AI","Anguilla"},
    {"AQ","Antarctique"},
    {"AG","Antigua-et-Barbuda"},
    {"SA","Arabie saoudite"},
    {"AR","Argentine"},
    {"AM","Arménie"},
    {"AW","Aruba"},
    {"AU","Australie"},
    {"AT","Autriche"},
    {"AZ","Azerbaïdjan"},
    {"BS","Bahamas"},
    {"BH","Bahreïn"},
    {"BD","Bangladesh"},
    {"BB","Barbade"},
    {"BE","Belgique"},
    {"BZ","Belize"},
    {"BJ","Bénin"},
    {"BM","Bermudes"},
    {"BT","Bhoutan"},
    {"BY","Biélorussie"},
    {"BO","Bolivie"},
    {"BA","Bosnie-Herzégovine"},
    {"BW","Botswana"},
    {"BR","Brésil"},
    {"BN","Brunéi Darussalam"},
    {"BG","Bulgarie"},
    {"BF","Burkina Faso"},
    {"BI","Burundi"},
    {"KH","Cambodge"},
    {"CM","Cameroun"},
    {"CA","Canada"},
    {"CV","Cap-Vert"},
    {"CL","Chili"},
    {"CN","Chine"},
    {"CY","Chypre"},
    {"CO","Colombie"},
    {"KM","Comores"},
    {"CG","Congo-Brazzaville"},
    {"CD","Congo-Kinshasa"},
    {"KP","Corée du Nord"},
    {"KR","Corée du Sud"},
    {"CR","Costa Rica"},
    {"CI","Côte d’Ivoire"},
    {"HR","Croatie"},
    {"CU","Cuba"},
    {"CW","Curaçao"},
    {"DK","Danemark"},
    {"DJ","Djibouti"},
    {"DM","Dominique"},
    {"EG","Égypte"},
    {"AE","Émirats arabes unis"},
    {"EC","Équateur"},
    {"ER","Érythrée"},
    {"ES","Espagne"},
    {"EE","Estonie"},
    {"SZ","Eswatini"},
    {"VA","État de la Cité du Vatican"},
    {"FM","États fédérés de Micronésie"},
    {"US","États-Unis"},
    {"ET","Éthiopie"},
    {"FJ","Fidji"},
    {"FI","Finlande"},
    {"FR","France"},
    {"GA","Gabon"},
    {"GM","Gambie"},
    {"GE","Géorgie"},
    {"GS","Géorgie du Sud et îles Sandwich du Sud"},
    {"GH","Ghana"},
    {"GI","Gibraltar"},
    {"GR","Grèce"},
    {"GD","Grenade"},
    {"GL","Groenland"},
    {"GP","Guadeloupe"},
    {"GU","Guam"},
    {"GT","Guatemala"},
    {"GG","Guernesey"},
    {"GN","Guinée"},
    {"GQ","Guinée équatoriale"},
    {"GW","Guinée-Bissau"},
    {"GY","Guyana"},
    {"GF","Guyane française"},
    {"HT","Haïti"},
    {"HN","Honduras"},
    {"HU","Hongrie"},
    {"BV","Île Bouvet"},
    {"CX","Île Christmas"},
    {"IM","Île de Man"},
    {"NF","Île Norfolk"},
    {"AX","Îles Åland"},
    {"KY","Îles Caïmans"},
    {"CC","Îles Cocos"},
    {"CK","Îles Cook"},
    {"FO","Îles Féroé"},
    {"HM","Îles Heard et McDonald"},
    {"FK","Îles Malouines"},
    {"MP","Îles Mariannes du Nord"},
    {"MH","Îles Marshall"},
    {"UM","Îles mineures éloignées des États-Unis"},
    {"PN","Îles Pitcairn"},
    {"SB","Îles Salomon"},
    {"TC","Îles Turques-et-Caïques"},
    {"VG","Îles Vierges britanniques"},
    {"VI","Îles Vierges des États-Unis"},
    {"IN","Inde"},
    {"ID","Indonésie"},
    {"IQ","Irak"},
    {"IR","Iran"},
    {"IE","Irlande"},
    {"IS","Islande"},
    {"IL","Israël"},
    {"IT","Italie"},
    {"JM","Jamaïque"},
    {"JP","Japon"},
    {"JE","Jersey"},
    {"JO","Jordanie"},
    {"KZ","Kazakhstan"},
    {"KE","Kenya"},
    {"KG","Kirghizistan"},
    {"KI","Kiribati"},
    {"KW","Koweït"},
    {"RE","La Réunion"},
    {"LA","Laos"},
    {"LS","Lesotho"},
    {"LV","Lettonie"},
    {"LB","Liban"},
    {"LR","Libéria"},
    {"LY","Libye"},
    {"LI","Liechtenstein"},
    {"LT","Lituanie"},
    {"LU","Luxembourg"},
    {"MK","Macédoine du Nord"},
    {"MG","Madagascar"},
    {"MY","Malaisie"},
    {"MW","Malawi"},
    {"MV","Maldives"},
    {"ML","Mali"},
    {"MT","Malte"},
    {"MA","Maroc"},
    {"MQ","Martinique"},
    {"MU","Maurice"},
    {"MR","Mauritanie"},
    {"YT","Mayotte"},
    {"MX","Mexique"},
    {"MD","Moldavie"},
    {"MC","Monaco"},
    {"MN","Mongolie"},
    {"ME","Monténégro"},
    {"MS","Montserrat"},
    {"MZ","Mozambique"},
    {"MM","Myanmar (Birmanie)"},
    {"NA","Namibie"},
    {"NR","Nauru"},
    {"NP","Népal"},
    {"NI","Nicaragua"},
    {"NE","Niger"},
    {"NG","Nigéria"},
    {"NU","Niue"},
    {"NO","Norvège"},
    {"NC","Nouvelle-Calédonie"},
    {"NZ","Nouvelle-Zélande"},
    {"OM","Oman"},
    {"UG","Ouganda"},
    {"UZ","Ouzbékistan"},
    {"PK","Pakistan"},
    {"PW","Palaos"},
    {"PA","Panama"},
    {"PG","Papouasie-Nouvelle-Guinée"},
    {"PY","Paraguay"},
    {"NL","Pays-Bas"},
    {"BQ","Pays-Bas caribéens"},
    {"PE","Pérou"},
    {"PH","Philippines"},
    {"PL","Pologne"},
    {"PF","Polynésie française"},
    {"PR","Porto Rico"},
    {"PT","Portugal"},
    {"QA","Qatar"},
    {"HK","R.A.S. chinoise de Hong Kong"},
    {"MO","R.A.S. chinoise de Macao"},
    {"CF","République centrafricaine"},
    {"DO","République dominicaine"},
    {"RO","Roumanie"},
    {"GB","Royaume-Uni"},
    {"RU","Russie"},
    {"RW","Rwanda"},
    {"EH","Sahara occidental"},
    {"BL","Saint-Barthélemy"},
    {"KN","Saint-Christophe-et-Niévès"},
    {"SM","Saint-Marin"},
    {"MF","Saint-Martin"},
    {"SX","Saint-Martin (partie néerlandaise)"},
    {"PM","Saint-Pierre-et-Miquelon"},
    {"VC","Saint-Vincent-et-les-Grenadines"},
    {"SH","Sainte-Hélène"},
    {"LC","Sainte-Lucie"},
    {"SV","Salvador"},
    {"WS","Samoa"},
    {"AS","Samoa américaines"},
    {"ST","Sao Tomé-et-Principe"},
    {"SN","Sénégal"},
    {"RS","Serbie"},
    {"SC","Seychelles"},
    {"SL","Sierra Leone"},
    {"SG","Singapour"},
    {"SK","Slovaquie"},
    {"SI","Slovénie"},
    {"SO","Somalie"},
    {"SD","Soudan"},
    {"SS","Soudan du Sud"},
    {"LK","Sri Lanka"},
    {"SE","Suède"},
    {"CH","Suisse"},
    {"SR","Suriname"},
    {"SJ","Svalbard et Jan Mayen"},
    {"SY","Syrie"},
    {"TJ","Tadjikistan"},
    {"TW","Taïwan"},
    {"TZ","Tanzanie"},
    {"TD","Tchad"},
    {"CZ","Tchéquie"},
    {"TF","Terres australes françaises"},
    {"IO","Territoire britannique de l’océan Indien"},
    {"PS","Territoires palestiniens"},
    {"TH","Thaïlande"},
    {"TL","Timor oriental"},
    {"TG","Togo"},
    {"TK","Tokelau"},
    {"TO","Tonga"},
    {"TT","Trinité-et-Tobago"},
    {"TN","Tunisie"},
    {"TM","Turkménistan"},
    {"TR","Turquie"},
    {"TV","Tuvalu"},
    {"UA","Ukraine"},
    {"UY","Uruguay"},
    {"VU","Vanuatu"},
    {"VE","Venezuela"},
    {"VN","Vietnam"},
    {"WF","Wallis-et-Futuna"},
    {"YE","Yémen"},
    {"ZM","Zambie"},
    {"ZW","Zimbabwe"}
};

