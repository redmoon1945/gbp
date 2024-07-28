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

#include "util.h"
#include "float.h"



QString Util::strDaily = "";
QString Util::strWeekly = "";
QString Util::strMonthly = "";
QString Util::strEndOfMonthly = "";
QString Util::strYearly = "";
QString Util::strDailyPl = "";
QString Util::strWeeklyPl = "";
QString Util::strMonthlyPl = "";
QString Util::strEndOfMonthlyPl = "";
QString Util::strYearlyPl = "";
QString Util::strDailyNc = "";
QString Util::strWeeklyNc = "";
QString Util::strMonthlyNc = "";
QString Util::strEndOfMonthlyNc = "";
QString Util::strYearlyNc = "";
QString Util::strDailyPlNc = "";
QString Util::strWeeklyPlNc = "";
QString Util::strMonthlyPlNc = "";
QString Util::strEndOfMonthlyPlNc = "";
QString Util::strYearlyPlNc = "";

QStringList Util::qtColorNames = {};

Util::Util()
{

}


// must be called as soon as possible after the applicaiton starts
void Util::init()
{
    // init Period Names. We initialize static variables in a non-static context (cannot do otherwise because of translation)

    strDaily = tr("Day");
    strWeekly = tr("Week");
    strMonthly = tr("Month");
    strEndOfMonthly = tr("End-of-Month");
    strYearly = tr("Year");
    strDailyPl = tr("Days");
    strWeeklyPl = tr("Weeks");
    strMonthlyPl =tr("Months");
    strEndOfMonthlyPl = tr("Ends-of-Month");
    strYearlyPl =tr("Years");

    strDailyNc = tr("day");
    strWeeklyNc = tr("week");
    strMonthlyNc = tr("month");
    strEndOfMonthlyNc = tr("end-of-month");
    strYearlyNc = tr("year");
    strDailyPlNc = tr("days");
    strWeeklyPlNc = tr("weeks");
    strMonthlyPlNc = tr("months");
    strEndOfMonthlyPlNc = tr("ends-of-month");
    strYearlyPlNc = tr("years");

    // Get QT smart lolor names
    qtColorNames = QColor::colorNames();
}


QString Util::getPeriodName(Util::PeriodType period, bool capitalizeFirstLetter, bool plural)
{

    if ( !plural ){
        if (capitalizeFirstLetter){
            if (period==Util::DAILY) {
                return strDaily;
            } else if (period==Util::WEEKLY){
                return strWeekly;
            } else if (period==Util::MONTHLY){
                return strMonthly;
            } else if (period==Util::END_OF_MONTHLY){
                return strEndOfMonthly;
            } else {
                return strYearly;
            }
        } else {
            if (period==Util::DAILY) {
                return strDailyNc;
            } else if (period==Util::WEEKLY){
                return strWeeklyNc;
            } else if (period==Util::MONTHLY){
                return strMonthlyNc;
            } else if (period==Util::END_OF_MONTHLY){
                return strEndOfMonthlyNc;
            } else {
                return strYearlyNc;
            }
        }

    } else {
        if (capitalizeFirstLetter){
            if (period==DAILY) {
                return strDailyPl;
            } else if (period==WEEKLY){
                return strWeeklyPl;
            } else if (period==MONTHLY){
                return strMonthlyPl;
            } else if (period==END_OF_MONTHLY){
                return strEndOfMonthlyPl;
            } else {
                return strYearlyPl;
            }
        } else {
            if (period==DAILY) {
                return strDailyPlNc;
            } else if (period==WEEKLY){
                return strWeeklyPlNc;
            } else if (period==MONTHLY){
                return strMonthlyPlNc;
            } else if (period==END_OF_MONTHLY){
                return strEndOfMonthlyPlNc;
            } else {
                return strYearlyPlNc;
            }
        }
    }
}


QString Util::elideText(QString str, int maxNoOfChar, bool elideRight)
{
    static QString elideText = "...";
    if ( (str.length()==0) || (maxNoOfChar<elideText.length()) ){
        return "";
    }
    if (str.length()<=maxNoOfChar){
        return str;
    }
    if(elideRight){
        return str.left(maxNoOfChar-elideText.length())+elideText;
    } else{
        return elideText+str.right(maxNoOfChar-elideText.length());
    }
}


// n must be in [0..14]
qint64 Util::quickPow10(uint n)
{
    static qint64 pow10[15] = {
        1, 10, 100, 1000, 10000,
        100000, 1000000, 10000000, 100000000, 1000000000,
        10000000000,100000000000,1000000000000,10000000000000,100000000000000
    };
    if (n>14){
        throw std::out_of_range("must be within range [0..14]");
    }
    return pow10[n];
}

// growth values are in percentage.
long double Util::monthlyToAnnualGrowth(long double monthly)
{
       return 100 * (pow(1+(monthly/100),12.0) - 1);
}


// growth values are in percentage.
long double Util::annualToMonthlyGrowth(long double annual)
{
    return 100 * (pow(1+(annual/100),1/12.0) - 1);
}


// Based on a 365-days year.
// Growth values are in percentage.
long double Util::annualToDailyGrowth(long double annual)
{
    return 100 * (pow(1+(annual/100),1/365.0) - 1);
}


// Comparing double is a very complex topic, but this implementation works for us.
// https://stackoverflow.com/questions/17333/how-do-you-compare-float-and-double-while-accounting-for-precision-loss
// https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
bool Util::areDoublesApproxEqual(double a, double b, double epsilon=DBL_EPSILON)
{
    return fabs(a - b) <= 16 * DBL_EPSILON * fmax(fabs(a), fabs(b));
}



// // No of guaranteed decimal digits precision :
// // float : 6
// // double : 15
// // long double : 18
// // qint64 : 18 (max is 9223372036854775808, 9999999999999999999 cannot be stored)
// // https://www.exploringbinary.com/decimal-precision-of-binary-floating-point-numbers/
// // So a qint64 can be stored completely in a long double and vice versa
// long double Util::qint64ToLongDouble(qint64 amount, quint8 noOfDecimal)
// {
//     long double d = static_cast<long double>(amount)/Util::quickPow10(noOfDecimal);
//     return d;
// }


// qint64 Util::longextractQint64FromDoubleWithNoFracPart(long double amount, quint8 noOfDecimal)
// {
//     long double ld = amount*Util::quickPow10(noOfDecimal);
//     long double t = std::round(ld);
//     if ( (t > std::numeric_limits<qint64>::max()) ||
//          (t < std::numeric_limits<qint64>::min()) ){ // long double needed to compared to quint64, potential of truncation for crazy number...
//         throw std::domain_error("amount is too big to fit in a quint64");
//     }
//     return static_cast<qint64>(t); // loss of precision possible here
// }


// Convert into a qint64 a double that contains no fractional part
// Result :
// 0 : success
// -1 : fail, double has a fractional part and so does not contain an int
// -2 : fail : double is out of range of qint64
qint64 Util::extractQint64FromDoubleWithNoFracPart(double d, int &result)
{
    long double ld = d; // must use a long double to compare to min/max of qint64
    if ( (ld > std::numeric_limits<qint64>::max()) ||
        (ld < std::numeric_limits<qint64>::min()) ){
        result = -2;
        return 0;
    }
//    if ( floor(d) != ceil(d) ){
    qint64 r = static_cast<qint64>(d);
    if ( d != r ){
        result = -1;
        return 0;
    }
    result = 0;
    return r;
}


// // Format a raw amount (qint64) to currency string, taking into account the locale's decimal point and group separator
// QString Util::quint64ToDoubleString(quint64 amount, quint8 noOfDecimal, QLocale locale)
// {
//     long double ld = Util::qint64ToLongDouble(amount,noOfDecimal);
//     double d = static_cast<double>(ld); // potential loss off precision,if amount has more than 15 significant digits
//     return locale.toString(d,'f',noOfDecimal);
// }


//  QString do not support long double for "number"
QString Util::longDoubleToQString(long double value)
{
    std::stringstream stream;
    stream << std::fixed << std::setprecision(std::numeric_limits<long double>::digits10 + 1) << value;
    return QString::fromStdString(stream.str());
}

// Useful for debugging onnly
QList<double> Util::doubleArrayToQlist(double *data, uint noElements)
{
    QList<double> list;
    list.reserve(noElements);
    for(int i=0; i<noElements;i++){
        list.append(data[i]);
    }
    return list;
}


QDateTime Util::dateToDateTimeLocal(const QDate& date, const QTimeZone& tz)
{
    return QDateTime(date, QTime(0,0,0),tz);
}


// Calculate present value of a future value using P = F / (1 + r)^n
// F = future value
// r = discount rate per period, in percentage
// n = number of period
long double Util::presentValue(long double futureValue, double discountRate, int period)
{
    if (discountRate==0){
        return futureValue;
    }
    return futureValue / powl((1 + 0.01L*discountRate), period);
}


// Calculate conversion factor to transform a future value F into a present value P,
// that is P = F * factor, where factor = 1 / ((1 + r)^n).
// Input parameters:
//   r = discount rate per period, in percentage. Must be >= 0
//   n = number of period AFTER present period. Can be neative.
long double Util::presentValueConversionFactor(long double discountRate, int period)
{
    if (discountRate<0){
        throw std::out_of_range("Discount rate must be >= 0");
    }
    return 1.0L/powl((1 + (0.01L*discountRate)), period);
}


// Code from https://nasauber.de/blog/2019/calculating-the-difference-between-two-qdates/
// Author : Tobias Leupold <tl@stonemx.de>
Util::DateDifference Util::dateDifference(const QDate &from, const QDate &to)
{
    if (from > to) {
        return dateDifference(to, from);
    }

    int dateDay = from.day();
    if (from.month() == 2 && dateDay == 29
        && ! QDate::isLeapYear(to.year())) {
        // If we calculate the timespan to a February 29 for a non-leap year, we use February 28
        // instead (the last day in February). This will also make birthdays for people born on
        // February 29 being calculated correctly (February 28, the last day in February, for
        // non-leap years)
        dateDay = 28;
    }

    int years = to.year() - from.year();
    int months = to.month() - from.month();
    if (to.month() < from.month()
        || ((to.month() == from.month()) && (to.day() < dateDay))) {
        years--;
        months += 12;
    }
    if (to.day() < dateDay) {
        months--;
    }

    int remainderMonth = to.month() - (to.day() < dateDay);
    int remainderYear = to.year();
    if (remainderMonth == 0) {
        remainderMonth = 12;
        remainderYear--;
    }

    const auto daysOfRemainderMonth = QDate(remainderYear, remainderMonth, 1).daysInMonth();
    const auto remainderDay = dateDay > daysOfRemainderMonth ? daysOfRemainderMonth : dateDay;

    int days = QDate(remainderYear, remainderMonth, remainderDay).daysTo(to);

    return { years, months, days };
}


bool Util::isValidBoolString(const QString& input) {
    QString lowerCaseInput = input.toLower(); // Convert the input string to lowercase for case-insensitive comparison
    return lowerCaseInput == "true" || lowerCaseInput == "false";
}


uint Util::changeFontSize(bool aggressive, bool decreaseSize, uint originalSize)
{
    if (aggressive) {
        if (decreaseSize) {
            return( static_cast<uint>(originalSize * 0.75));
        } else {
            return( static_cast<uint>(originalSize * 1.25));
        }
    } else {
        if (decreaseSize) {
            return( static_cast<uint>(originalSize * 0.90));
        } else {
            return( static_cast<uint>(originalSize * 1.1));
        }
    }
}


QString Util::getColorSmartName(QColor color, bool &found)
{
    found = false;
    QColor cmp;
    for(auto i : qtColorNames) {
        cmp.setNamedColor(i);
        if(cmp == color){
            found = true;
            return i;
        }
    }
    return ""; // not found
}


QString Util::buildColorDisplayName(QColor color)
{
    QString s = QString(tr("Red:%1  Green:%2  Blue:%3")).arg(color.red()).arg(color.green()).arg(color.blue());
    bool found;

    // we disable smart color names, because Qt does not offer a localized version
    // QString smartNames = Util::getColorSmartName(color,found);
    // if(found){
    //     s = s.append(QString("  (%1)").arg(smartNames));
    // }
    return s;
}

// Set nth bit of number to 1. First bit (less significant) is n=1
quint32 Util::bitSet(quint32 number, quint32 n)
{
    return number | ( (quint32)1 << (n-1) );
}


// Set the nth bit of number to 0. First bit (less significant) is n=1
quint32 Util::bitClear(quint32 number, quint32 n)
{
    return number & ~((quint32)1 << (n-1));
}


// toggle the nth bit of number. First bit (less significant) is n=1
quint32 Util::bitToggle(quint32 number, quint32 n)
{
    return number ^ ((quint32)1 << (n-1));
}

// Check if nth bit of number is set or not. First bit (less significant) is n=1
quint32 Util::bitCheck(quint32 number, quint32 n)
{
    return (number >> (n-1)) & (quint32)1;
}


// Calculate the difference in months between 2 dates. Dates's year must be > 0.
// Result is negative if "to" occurs before "from".
// 2 dates inside the same month produce a result of 0.
int Util::noOfMonthDifference(QDate from, QDate to)
{
    if (from.isValid()==false){
        throw std::invalid_argument("from is an invalid date");
    }
    if (from.year()<0){
        throw std::invalid_argument("from year must not be < 0");
    }
    if (to.isValid()==false){
        throw std::invalid_argument("to is an invalid date");
    }
    if (to.year()<0){
        throw std::invalid_argument("to year must not be < 0");
    }
    return ( (12*to.year())+to.month()) - ( (12*from.year())+from.month()) ;
}



