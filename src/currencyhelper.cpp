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
        default: throw std::invalid_argument("noOfDigits is too big");
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
            QString s = QString("noOfDigits %1 is too big").arg(noOfDecimalDigits);
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


QString CurrencyHelper::formatAmount(double amount, CurrencyInfo cInfo, QLocale locale,
    bool addISOcode)
{
    if (cInfo.noOfDecimal > MAX_NO_OF_DECIMALS) {
        return "";
    }
    if (fabs(amount) > maxValueAllowedForAmountInDouble(cInfo.noOfDecimal)) {
        return "";
    }
    QString s = locale.toString(amount,'f',cInfo.noOfDecimal);
    if (addISOcode) {
        return QString("%1 %2").arg(s).arg(cInfo.isoCode);
    } else{
        return s;
    }
}


qint64 CurrencyHelper::add(qint64 a, qint64 b)
{
    // potential for saturation here...but 2 "max allowed" added will be below max(qint64)
    qint64 r = a + b;

    // Handle overflow
    if(r<0){
        if (r < -NATIVE_MAX_VALUE_ALLOWED){
            return -NATIVE_MAX_VALUE_ALLOWED ;
        } else {
            return r;
        }
    } else {
        if (r > NATIVE_MAX_VALUE_ALLOWED){
            return NATIVE_MAX_VALUE_ALLOWED ;
        } else {
            return r;
        }
    }
}


QMap<QString, QString> CurrencyHelper::getCountries(QLocale theLocale)
{
    if ( theLocale.language() == QLocale::Language::French ){
        return countries_fr;
    } else {
        return countries;
    }
}


QMap<QString, QString> CurrencyHelper::getCurrencies(QLocale::Language language)
{
    QMap<QString, QString> result;
    for (auto it = countries.begin(); it != countries.end(); ++it) {
        QString countryCode = it.key();
        // get info about associated currency for that country
        bool found;
        CurrencyInfo currInfo = getCurrencyInfoFromCountryCode(countryCode,
            language, found);
        if(!found){
            continue; // should not happen
        }
        // add to the map
        QString desc = QString("%1 (%2) - %3").arg(currInfo.isoCode).arg(currInfo.symbol)
            .arg(currInfo.name);
        if ( !(result.contains(currInfo.isoCode))){
            result.insert(currInfo.isoCode,desc);
        }
     }
    return result;
}


bool CurrencyHelper::countryExists(QString countryCode)
{
    return countries.contains(countryCode);
}


CurrencyInfo CurrencyHelper::getCurrencyInfoFromCountryCode(
    QString countryCode, QLocale::Language language, bool& found)
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
    currInfo.symbol = loc.currencySymbol(QLocale::CurrencySymbol); // in native language

    // Use English or French name based on the Locale
    if ( (language == QLocale::Language::French) && (currencyNamesFrench.contains(
        currInfo.isoCode)) ) {
        currInfo.name = currencyNamesFrench[currInfo.isoCode];
    } else if ( currencyNamesEnglish.contains(currInfo.isoCode) ) {
        // If not French, then it is English. isoCode must exist though.
        currInfo.name = currencyNamesEnglish[currInfo.isoCode];
    } else {
        // Fallback for unknown currencies (should be rare with comprehensive maps we have)
        currInfo.name = "Unknown";
    }

    // Special handling for symbols
    if (currInfo.symbol.isEmpty()) {
        currInfo.symbol = "Unknown";
    }

    if (currInfo.isoCode=="CVE"){
        currInfo.symbol="$"; // something is returned by Qt but it is not displayable (???)
    }
    if (currInfo.isoCode == "JPY") {
        currInfo.symbol = "¥"; // Use standard Yen symbol (U+00A5) instead of fullwidth (U+FFE5)
    }
    if (currInfo.isoCode == "CHF") {
        currInfo.symbol = "fr"; // choose the french symbol
    }

    currInfo.noOfDecimal = currencyDecimalDigits.value(currInfo.isoCode,2);
    return currInfo;
}


QMap<QString,int> CurrencyHelper::currencyDecimalDigits = {
    {"BHD",3},{"BIF",0},{"CLF",4},{"CLP",0},{"DJF",0},{"GNF",0},{"IQD",3},{"ISK",0},{"JOD",3},
    {"JPY",0},{"KMF",0},{"KRW",0},{"KWD",3},{"LYD",3},{"OMR",3},{"PYG",0},{"RWF",0},{"TND",3},
    {"UGX",0},{"UYI",0},{"UYW",4},{"VND",0},{"VUV",0},{"XAF",0},{"XOF",0},{"XPF",0}
};


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

// Comprehensive list of currency names in English (ISO 4217)
QMap<QString, QString> CurrencyHelper::currencyNamesEnglish = {
    {"AED", "United Arab Emirates Dirham"},
    {"AFN", "Afghan Afghani"},
    {"ALL", "Albanian Lek"},
    {"AMD", "Armenian Dram"},
    {"ANG", "Netherlands Antillean Guilder"},
    {"AOA", "Angolan Kwanza"},
    {"ARS", "Argentine Peso"},
    {"AUD", "Australian Dollar"},
    {"AWG", "Aruban Florin"},
    {"AZN", "Azerbaijani Manat"},
    {"BAM", "Bosnia-Herzegovina Convertible Mark"},
    {"BBD", "Barbadian Dollar"},
    {"BDT", "Bangladeshi Taka"},
    {"BGN", "Bulgarian Lev"},
    {"BHD", "Bahraini Dinar"},
    {"BIF", "Burundian Franc"},
    {"BMD", "Bermudian Dollar"},
    {"BND", "Brunei Dollar"},
    {"BOB", "Bolivian Boliviano"},
    {"BRL", "Brazilian Real"},
    {"BSD", "Bahamian Dollar"},
    {"BTC", "Bitcoin"},
    {"BTN", "Bhutanese Ngultrum"},
    {"BWP", "Botswanan Pula"},
    {"BYN", "Belarusian Ruble"},
    {"BZD", "Belize Dollar"},
    {"CAD", "Canadian Dollar"},
    {"CDF", "Congolese Franc"},
    {"CHF", "Swiss Franc"},
    {"CLF", "Chilean Unit of Account"},
    {"CLP", "Chilean Peso"},
    {"CNY", "Chinese Yuan"},
    {"COP", "Colombian Peso"},
    {"CRC", "Costa Rican Colón"},
    {"CUC", "Cuban Convertible Peso"},
    {"CUP", "Cuban Peso"},
    {"CVE", "Cape Verdean Escudo"},
    {"CZK", "Czech Koruna"},
    {"DJF", "Djiboutian Franc"},
    {"DKK", "Danish Krone"},
    {"DOP", "Dominican Peso"},
    {"DZD", "Algerian Dinar"},
    {"EGP", "Egyptian Pound"},
    {"ERN", "Eritrean Nakfa"},
    {"ETB", "Ethiopian Birr"},
    {"EUR", "Euro"},
    {"FJD", "Fijian Dollar"},
    {"FKP", "Falkland Islands Pound"},
    {"GBP", "British Pound"},
    {"GEL", "Georgian Lari"},
    {"GGP", "Guernsey Pound"},
    {"GHS", "Ghanaian Cedi"},
    {"GIP", "Gibraltar Pound"},
    {"GMD", "Gambian Dalasi"},
    {"GNF", "Guinean Franc"},
    {"GTQ", "Guatemalan Quetzal"},
    {"GYD", "Guyanaese Dollar"},
    {"HKD", "Hong Kong Dollar"},
    {"HNL", "Honduran Lempira"},
    {"HRK", "Croatian Kuna"},
    {"HTG", "Haitian Gourde"},
    {"HUF", "Hungarian Forint"},
    {"IDR", "Indonesian Rupiah"},
    {"ILS", "Israeli New Shekel"},
    {"IMP", "Manx Pound"},
    {"INR", "Indian Rupee"},
    {"IQD", "Iraqi Dinar"},
    {"IRR", "Iranian Rial"},
    {"ISK", "Icelandic Króna"},
    {"JEP", "Jersey Pound"},
    {"JMD", "Jamaican Dollar"},
    {"JOD", "Jordanian Dinar"},
    {"JPY", "Yen"},
    {"KES", "Kenyan Shilling"},
    {"KGS", "Kyrgyzstani Som"},
    {"KHR", "Cambodian Riel"},
    {"KMF", "Comorian Franc"},
    {"KPW", "North Korean Won"},
    {"KRW", "South Korean Won"},
    {"KWD", "Kuwaiti Dinar"},
    {"KYD", "Cayman Islands Dollar"},
    {"KZT", "Kazakhstani Tenge"},
    {"LAK", "Lao Kip"},
    {"LBP", "Lebanese Pound"},
    {"LKR", "Sri Lankan Rupee"},
    {"LRD", "Liberian Dollar"},
    {"LSL", "Lesotho Loti"},
    {"LYD", "Libyan Dinar"},
    {"MAD", "Moroccan Dirham"},
    {"MDL", "Moldovan Leu"},
    {"MGA", "Malagasy Ariary"},
    {"MKD", "Macedonian Denar"},
    {"MMK", "Myanmar Kyat"},
    {"MNT", "Mongolian Tugrik"},
    {"MOP", "Macanese Pataca"},
    {"MRU", "Mauritanian Ouguiya"},
    {"MUR", "Mauritian Rupee"},
    {"MVR", "Maldivian Rufiyaa"},
    {"MWK", "Malawian Kwacha"},
    {"MXN", "Mexican Peso"},
    {"MYR", "Malaysian Ringgit"},
    {"MZN", "Mozambican Metical"},
    {"NAD", "Namibian Dollar"},
    {"NGN", "Nigerian Naira"},
    {"NIO", "Nicaraguan Córdoba"},
    {"NOK", "Norwegian Krone"},
    {"NPR", "Nepalese Rupee"},
    {"NZD", "New Zealand Dollar"},
    {"OMR", "Omani Rial"},
    {"PAB", "Panamanian Balboa"},
    {"PEN", "Peruvian Sol"},
    {"PGK", "Papua New Guinean Kina"},
    {"PHP", "Philippine Peso"},
    {"PKR", "Pakistani Rupee"},
    {"PLN", "Polish Zloty"},
    {"PYG", "Paraguayan Guaraní"},
    {"QAR", "Qatari Riyal"},
    {"RON", "Romanian Leu"},
    {"RSD", "Serbian Dinar"},
    {"RUB", "Russian Rubles"},
    {"RWF", "Rwandan Franc"},
    {"SAR", "Saudi Riyal"},
    {"SBD", "Solomon Islands Dollar"},
    {"SCR", "Seychellois Rupee"},
    {"SDG", "Sudanese Pound"},
    {"SEK", "Swedish Krona"},
    {"SGD", "Singapore Dollar"},
    {"SHP", "Saint Helena Pound"},
    {"SLL", "Sierra Leonean Leone"},
    {"SOS", "Somali Shilling"},
    {"SRD", "Surinamese Dollar"},
    {"SSP", "South Sudanese Pound"},
    {"STN", "São Tomé and Príncipe Dobra"},
    {"SVC", "Salvadoran Colón"},
    {"SYP", "Syrian Pound"},
    {"SZL", "Swazi Lilangeni"},
    {"THB", "Thai Baht"},
    {"TJS", "Tajikistani Somoni"},
    {"TMT", "Turkmenistani Manat"},
    {"TND", "Tunisian Dinar"},
    {"TOP", "Tongan Paʻanga"},
    {"TRY", "Turkish Lira"},
    {"TTD", "Trinidad and Tobago Dollar"},
    {"TWD", "New Taiwan Dollar"},
    {"TZS", "Tanzanian Shilling"},
    {"UAH", "Ukrainian Hryvnia"},
    {"UGX", "Ugandan Shilling"},
    {"USD", "US Dollar"},
    {"UYU", "Uruguayan Peso"},
    {"UYW", "Uruguayan Nominal Wage Index Unit"},
    {"UZS", "Uzbekistani Som"},
    {"VES", "Venezuelan Bolívar"},
    {"VND", "Vietnamese Dong"},
    {"VUV", "Vanuatu Vatu"},
    {"WST", "Samoan Tala"},
    {"XAF", "Central African CFA Franc"},
    {"XCD", "East Caribbean Dollar"},
    {"XOF", "West African CFA Franc"},
    {"XPF", "CFP Franc"},
    {"YER", "Yemeni Rial"},
    {"ZAR", "South African Rand"},
    {"ZMW", "Zambian Kwacha"},
    {"ZWL", "Zimbabwean Dollar"}
};

// Comprehensive list of currency names in French (ISO 4217)
QMap<QString, QString> CurrencyHelper::currencyNamesFrench = {
    {"AED", "Dirham des Émirats arabes unis"},
    {"AFN", "Afghani afghan"},
    {"ALL", "Lek albanais"},
    {"AMD", "Dram arménien"},
    {"ANG", "Florin des Antilles néerlandaises"},
    {"AOA", "Kwanza angolais"},
    {"ARS", "Peso argentin"},
    {"AUD", "Dollar australien"},
    {"AWG", "Florin arubais"},
    {"AZN", "Manat azerbaïdjanais"},
    {"BAM", "Mark convertible de Bosnie-Herzégovine"},
    {"BBD", "Dollar barbadien"},
    {"BDT", "Taka bangladais"},
    {"BGN", "Lev bulgare"},
    {"BHD", "Dinar bahreïni"},
    {"BIF", "Franc burundais"},
    {"BMD", "Dollar bermudien"},
    {"BND", "Dollar brunéien"},
    {"BOB", "Boliviano bolivien"},
    {"BRL", "Réal brésilien"},
    {"BSD", "Dollar bahamien"},
    {"BTC", "Bitcoin"},
    {"BTN", "Ngultrum bhoutanais"},
    {"BWP", "Pula botswanais"},
    {"BYN", "Rouble biélorusse"},
    {"BZD", "Dollar bélizien"},
    {"CAD", "Dollar canadien"},
    {"CDF", "Franc congolais"},
    {"CHF", "Franc suisse"},
    {"CLF", "Unité de compte chilienne"},
    {"CLP", "Peso chilien"},
    {"CNY", "Yuan chinois"},
    {"COP", "Peso colombien"},
    {"CRC", "Colón costaricien"},
    {"CUC", "Peso cubain convertible"},
    {"CUP", "Peso cubain"},
    {"CVE", "Escudo cap-verdien"},
    {"CZK", "Couronne tchèque"},
    {"DJF", "Franc djiboutien"},
    {"DKK", "Couronne danoise"},
    {"DOP", "Peso dominicain"},
    {"DZD", "Dinar algérien"},
    {"EGP", "Livre égyptienne"},
    {"ERN", "Nakfa érythréen"},
    {"ETB", "Birr éthiopien"},
    {"EUR", "Euro"},
    {"FJD", "Dollar fidjien"},
    {"FKP", "Livre des îles Malouines"},
    {"GBP", "Livre sterling"},
    {"GEL", "Lari géorgien"},
    {"GGP", "Livre de Guernesey"},
    {"GHS", "Cedi ghanéen"},
    {"GIP", "Livre de Gibraltar"},
    {"GMD", "Dalasi gambien"},
    {"GNF", "Franc guinéen"},
    {"GTQ", "Quetzal guatémaltèque"},
    {"GYD", "Dollar guyanien"},
    {"HKD", "Dollar de Hong Kong"},
    {"HNL", "Lempira hondurien"},
    {"HRK", "Kuna croate"},
    {"HTG", "Gourde haïtienne"},
    {"HUF", "Forint hongrois"},
    {"IDR", "Roupie indonésienne"},
    {"ILS", "Nouveau shekel israélien"},
    {"IMP", "Livre mannoise"},
    {"INR", "Roupie indienne"},
    {"IQD", "Dinar irakien"},
    {"IRR", "Rial iranien"},
    {"ISK", "Couronne islandaise"},
    {"JEP", "Livre de Jersey"},
    {"JMD", "Dollar jamaïcain"},
    {"JOD", "Dinar jordanien"},
    {"JPY", "Yen japonais"},
    {"KES", "Shilling kényan"},
    {"KGS", "Som kirghize"},
    {"KHR", "Riel cambodgien"},
    {"KMF", "Franc comorien"},
    {"KPW", "Won nord-coréen"},
    {"KRW", "Won sud-coréen"},
    {"KWD", "Dinar koweïtien"},
    {"KYD", "Dollar des îles Caïmans"},
    {"KZT", "Tenge kazakh"},
    {"LAK", "Kip laotien"},
    {"LBP", "Livre libanaise"},
    {"LKR", "Roupie srilankaise"},
    {"LRD", "Dollar libérien"},
    {"LSL", "Loti lesothan"},
    {"LYD", "Dinar libyen"},
    {"MAD", "Dirham marocain"},
    {"MDL", "Leu moldave"},
    {"MGA", "Ariary malgache"},
    {"MKD", "Denar macédonien"},
    {"MMK", "Kyat birman"},
    {"MNT", "Tugrik mongol"},
    {"MOP", "Pataca macanaise"},
    {"MRU", "Ouguiya mauritanienne"},
    {"MUR", "Roupie mauricienne"},
    {"MVR", "Rufiyaa maldivienne"},
    {"MWK", "Kwacha malawite"},
    {"MXN", "Peso mexicain"},
    {"MYR", "Ringgit malaisien"},
    {"MZN", "Metical mozambicain"},
    {"NAD", "Dollar namibien"},
    {"NGN", "Naira nigérian"},
    {"NIO", "Córdoba nicaraguayen"},
    {"NOK", "Couronne norvégienne"},
    {"NPR", "Roupie népalaise"},
    {"NZD", "Dollar néo-zélandais"},
    {"OMR", "Rial omanais"},
    {"PAB", "Balboa panaméen"},
    {"PEN", "Sol péruvien"},
    {"PGK", "Kina papouan-néo-guinéen"},
    {"PHP", "Peso philippin"},
    {"PKR", "Roupie pakistanaise"},
    {"PLN", "Zloty polonais"},
    {"PYG", "Guaraní paraguayen"},
    {"QAR", "Riyal qatari"},
    {"RON", "Leu roumain"},
    {"RSD", "Dinar serbe"},
    {"RUB", "Rouble russe"},
    {"RWF", "Franc rwandais"},
    {"SAR", "Riyal saoudien"},
    {"SBD", "Dollar des îles Salomon"},
    {"SCR", "Roupie seychelloise"},
    {"SDG", "Livre soudanaise"},
    {"SEK", "Couronne suédoise"},
    {"SGD", "Dollar de Singapour"},
    {"SHP", "Livre de Sainte-Hélène"},
    {"SLL", "Leone sierra-léonais"},
    {"SOS", "Shilling somalien"},
    {"SRD", "Dollar surinamais"},
    {"SSP", "Livre sud-soudanaise"},
    {"STN", "Dobra de São Tomé et Príncipe"},
    {"SVC", "Colón salvadorien"},
    {"SYP", "Livre syrienne"},
    {"SZL", "Lilangeni swazi"},
    {"THB", "Baht thaïlandais"},
    {"TJS", "Somoni tadjik"},
    {"TMT", "Manat turkmène"},
    {"TND", "Dinar tunisien"},
    {"TOP", "Paʻanga tongien"},
    {"TRY", "Lire turque"},
    {"TTD", "Dollar de Trinité-et-Tobago"},
    {"TWD", "Nouveau dollar taïwanais"},
    {"TZS", "Shilling tanzanien"},
    {"UAH", "Hryvnia ukrainienne"},
    {"UGX", "Shilling ougandais"},
    {"USD", "Dollar américain"},
    {"UYU", "Peso uruguayen"},
    {"UYW", "Unité indexée sur les salaires nominaux uruguayens"},
    {"UZS", "Som ouzbek"},
    {"VES", "Bolívar vénézuélien"},
    {"VND", "Dong vietnamien"},
    {"VUV", "Vatu vanuatuan"},
    {"WST", "Tala samoan"},
    {"XAF", "Franc CFA d’Afrique centrale"},
    {"XCD", "Dollar des Caraïbes orientales"},
    {"XOF", "Franc CFA d’Afrique de l’Ouest"},
    {"XPF", "Franc CFP"},
    {"YER", "Rial yéménite"},
    {"ZAR", "Rand sud-africain"},
    {"ZMW", "Kwacha zambien"},
    {"ZWL", "Dollar zimbabwéen"}
};
