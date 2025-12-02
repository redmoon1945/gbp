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

// Must not include gbpcontroller.h, because it assumes a QApplication, which is not the case here.


#include "util.h"
#include "float.h"
#include "qregularexpression.h"
#include <QRandomGenerator64>
#include <iomanip>
#include <qfont.h>
#include <quuid.h>


QStringList Util::qtColorNames = {};




Util::Util()
{

}


// must be called as soon as possible after the application starts
void Util::init()
{
    // Get QT smart color names
    qtColorNames = QColor::colorNames();
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


bool Util::areDoublesApproxEqual(double a, double b, double epsilon=DBL_EPSILON)
{
    return fabs(a - b) <= epsilon;
    //return fabs(a - b) <= 16 * DBL_EPSILON * fmax(fabs(a), fabs(b));
}


qint64 Util::extractQint64FromDoubleWithNoFractionalPart(double amount, int &result)
{
    long double ld = amount; // must use a long double to compare to min/max of qint64
    if ( std::isnan(amount) ){
        result = -2;
        return 0;
    }
    if ( std::isinf(amount) ){
        result = -3;
        return 0;
    }
    if ( ld > std::numeric_limits<qint64>::max() ){
        result = -4;
        return 0;
    }
    if ( ld < std::numeric_limits<qint64>::min() ){
        result = -5;
        return 0;
    }

    // check if any fractional part
    double intPart;
    if (std::modf(amount, &intPart) != 0.0) {
        result = -1;
        return 0;
    }
    // return the result
    qint64 r = static_cast<qint64>(amount);
    result = 0;
    return r;
}


// Convert into a quint16 a double that contains no fractional part
// Result :
// 0 : success
// -1 : fail, double has a fractional part and so does not contain an int
// -2 : fail : double is bigger than maxValue
quint16 Util::extractQuint16FromDoubleWithNoFracPart(double amount, quint16 maxValue, int &result)
{
    if ( amount > maxValue ){
        result = -2;
        return 0;
    }
    quint16 r = static_cast<quint16>(amount);
    if ( amount != r ){
        result = -1;
        return 0;
    }
    result = 0;
    return r;
}


//  QString do not support long double for "number"
QString Util::longDoubleToQString(long double value)
{
    std::stringstream stream;
    stream << std::fixed << std::setprecision(
        std::numeric_limits<long double>::digits10 + 1) << value;
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


long double Util::presentValue(long double futureValue, double discountRate, int period)
{
    if (discountRate==0){
        return futureValue;
    }
    if (period==0) {
        return futureValue;
    }
    return futureValue / powl((1 + 0.01L*discountRate), period);
}


long double Util::futureValue(long double presentValue, double discountRate, int period)
{
    if (discountRate==0){
        return presentValue;
    }
    if (period==0) {
        return presentValue;
    }
    return presentValue * powl((1 + (0.01L*discountRate)), period);
}


long double Util::presentValueConversionFactor(long double discountRate, int period)
{
    if (discountRate<0){
        throw std::out_of_range("Discount rate must be >= 0");
    }
    if (discountRate==0){
        return 1;
    }
    if (period==0) {
        return 1;
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


uint Util::calculateFontResize(Util::FontResizeIntensity intensity, bool decreaseSize,
    uint originalSize)
{
    uint newSize;

    if (originalSize==0) {
        throw std::invalid_argument("Invalid original font size of 0");
    }

    switch (intensity) {
        case Util::FontResizeIntensity::WEAK:
            if (decreaseSize) {
                newSize = static_cast<uint>(originalSize * 0.90);
            } else {
                newSize = static_cast<uint>(originalSize * (1/0.9));
            }
        break;
        case Util::FontResizeIntensity::AVERAGE:
            if (decreaseSize) {
                newSize = static_cast<uint>(originalSize * 0.75);
            } else {
                newSize = static_cast<uint>(originalSize * (1/0.75));
            }
            break;
        case Util::FontResizeIntensity::AGGRESSIVE:
            if (decreaseSize) {
                newSize = static_cast<uint>(originalSize * 0.5);
            } else {
                newSize = static_cast<uint>(originalSize * 2);
            }
            break;
        case Util::FontResizeIntensity::EXTREME:
            if (decreaseSize) {
                newSize = static_cast<uint>(originalSize * 0.3333333);
            } else {
                newSize = static_cast<uint>(originalSize * 3);
            }
            break;
        default:
            throw std::invalid_argument("Unknown intensity factor"); // should never happen
            break;
        }

        // New size must not be 0
        if(newSize == 0){
            return 1;
        } else {
            return newSize;
        }
}



QString Util::getColorSmartName(QColor color, bool &found)
{
    found = false;
    QColor cmp;
    for(auto i : qtColorNames) {
        cmp.fromString(i);
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


QLocale Util::getLocale(QStringList arguments, bool& systemLocale){

    // Get the Default System Locale, which will be used in case of any error in the
    // "system locale override" argument passed or simply if it is not passed
    QLocale sysLocale = QLocale::system();
    systemLocale = true;

    for (int i = 0; i < arguments.size(); ++i) {
        QString arg = arguments.at(i);
        int index = arg.indexOf("-locale");
        if ( (0==index) && ((arg.length()==13)||(arg.length()==14)||(arg.length()==15)) ) {
            QString sub = arg.right(arg.length()-8);
            QStringList subList = sub.split('-');
            if (subList.length()!=2) {
                break;
            }
            QLocale::Language lang = QLocale::codeToLanguage(subList.at(0));
            if (lang==QLocale::AnyLanguage) {
                break; // unknown
            }
            QLocale::Territory territory = QLocale::codeToTerritory(subList.at(1));
            if (territory==QLocale::AnyTerritory) {
                break; // unknown
            }
            systemLocale = false;
            return QLocale(lang,territory); // See Qt doc for Qlocale for behavior
        }
    }

    return sysLocale;
}


// Calculate expansion of a Date interval. Only integer multiple of Day" is done.
// Input Parameters:
//   expansionFactor : Portion of the range. e.g. 0.1 will have the range grow by 10% total
//                     Valid values are : [0-0.1]
void Util::calculateZoomXaxis( QDateTime &min, QDateTime &max, double expansionFactor)
{
    // make sure min is not more recent than max. Should never happen.
    if (max < min) {
        throw std::invalid_argument("Max is smaller than Min");
    }
    if( (expansionFactor < 0) || (expansionFactor > 0.1) ){
        throw std::invalid_argument("expansionFactor is invalid");
    }

    if(min==max){
        min = min.addDays(-1);
        max = max.addDays(1);
    } else {
        uint dayDelta = min.daysTo(max); // at least = 1

        // at least 1 day from each side
        int noOfDaysToAdd = round((0.5*expansionFactor)*dayDelta);

        min = min.addDays(-noOfDaysToAdd);
        max = max.addDays(noOfDaysToAdd);
    }
}


// Calculate expansion of a Date interval.
// Input Parameters:
//   expansionFactor : Portion of the range. e.g. 0.1 will have the range grows
//                     by 10% total. Valid values are : [0-0.1]
void Util::calculateZoomYaxis(double &min, double &max, double expansionFactor)
{
    if ((max==0) && (min==0)) {
        min = -1;
        max = 1;
        return;
    }

    if (max < min) {
        throw std::invalid_argument("Max is smaller than Min");
    }
    if( (expansionFactor < 0) || (expansionFactor > 0.1) ){
        throw std::invalid_argument("expansionFactor is invalid");
    }

    double delta = max - min;
    double amountToAdd;
    if (delta==0) {
        // there is no delta, so expansionFactor has no meaning here.
        // We choose to have scale 0.1 total of the min
        amountToAdd = (0.05 * abs(min));
    } else {
        amountToAdd = ( (0.5*expansionFactor) * delta );
    }

    min = min - amountToAdd;
    max = max + amountToAdd;
}


// Find the min and max Y values in a list of QPointF. The X component of a QPointF is
// a QDateTime converted to Msec.
// Input Parameters:
//   ptList : the list of QPointF to search in.
//   from : do not considered QPointF for which X component is smaller than "from"
//   to : do not considered QPointF for which X component is greater than "to"
// Ouput Parameters:
//   min: the min found
//   max : the max found
// Return Value :
//   True if there is at least one point in this [from-to] range.
//   False otherwise, in which case value of min & max returned are invalid.
bool Util::findMinMaxInYvalues(const QList<QPointF> ptList, double from, double to, double &min,
    double &max)
{
    if(ptList.size()==0){
        return false;
    }
    min = DBL_MAX;
    max = -DBL_MAX;
    bool found = false;
    double dt;
    double val;
    foreach (QPointF pt, ptList) {
        dt = pt.x();
        if ( (dt>=from) && (dt<=to) ) {
            // at least 1 point is in the interval
            found = true;
            // we are inside the range, check for min/max
            double val = pt.y();
            if ( val < min ) {
                min  = val;
            }
            if (val > max) {
                max = val;
            }
        }
    }
    return found;
}


// Change the first character to upper or lower case, and make sure the other
// characters are all lower case
QString Util::wordCapitalize(bool upper, QString s)
{
    if(s.size()==0){
        return s;
    }

    QString res = s.toLower();
    if (upper==true) {
        if (s.size()==1) {
            return ( res.left(1).toUpper() );
        } else {
            return ( res.left(1).toUpper()+res.mid(1) );
        }
    } else {
        return res;
    }
}


void Util::changeFontSize(QFont &font, Util::FontResizeIntensity intensity, bool decreaseSize)
{
    int oldFontSize = font.pointSize();
    if (oldFontSize==0) {
        // this is illegal
        throw std::invalid_argument("Invalid font size of 0");
    }
    int newFontSize = Util::calculateFontResize(intensity, decreaseSize, oldFontSize);
    if (newFontSize==0) {
        // this is a problem in the calculation, should not happen. Font is not changed.
        return;
    }
    font.setPointSize(newFontSize);
}

QDate Util::isValidISO8601Date(const QString& dateStr, bool &valid)
{
    valid = false;

    // Check if the string matches the ISO 8601 format: YYYY-MM-DD
    QRegularExpression isoDateRegex("^\\d{4}-\\d{2}-\\d{2}$");
    if (!isoDateRegex.match(dateStr).hasMatch()) {
        return QDate();
    }

    // Parse the date to ensure it’s valid
    QDate date = QDate::fromString(dateStr, "yyyy-MM-dd");
    if (!date.isValid()) {
        return QDate();
    }
    valid = true;
    return date;
}


QColor Util::getOptimizedRed()
{

    static const QColor chatgptRed("#e5212a"); // Chat Gpt
    static const QColor grokRed("#e5212a");    // Grok

    return grokRed;
}


QColor Util::getOptimizedGreen()
{
    // Chatgpt
    static const QColor chatgptGreen("#007a1f"); // Chat Gpt
    static const QColor grokGreen("#008a00");    // Grok

    return grokGreen;
}



QColor Util::getOptimizedBlue()
{
    // Chatgpt
    static const QColor chatgptBlue("#004080"); // Chat Gpt
    static const QColor grokBlue("#0072ee");    // Grok

    return grokBlue;
}


QString Util::getStyleSheetStringForColor(QColor color)
{
    if (color.isValid()==false) {
        return "Invalid color";
    }
    QString s = QString("color: rgb(%1, %2, %3)").arg(color.red()).arg(color.green())
        .arg(color.blue()) ;
    return s;
}


QUuid Util::convertStringToQuuid(QString s, bool& success)
{
    success = false;

    QString newString = s.trimmed();

    // if String is bigger than 36, it cannot be a QUuid (no braces allowed)
    if(newString.length()>36){
        return QUuid(); // invalid QUuid
    }

    QUuid id = QUuid::fromString(newString);
    if (id.isNull()==false) {
        success = true;
    }

    return id;
}



QString Util::getFontResizeIntensityNames(Util::FontResizeIntensity intensity)
{
    switch (intensity) {
        case Util::FontResizeIntensity::WEAK:
            return tr("Weak");
            break;
        case Util::FontResizeIntensity::AVERAGE:
            return tr("Average");
            break;
        case Util::FontResizeIntensity::AGGRESSIVE:
            return tr("Aggressive");
            break;
        case Util::FontResizeIntensity::EXTREME:
            return tr("Extreme");
            break;
        default:
            return tr("Unknow");
            break;
    }
}


Util::ResultOfOperation::ResultOfOperation()
{
    init();
}


void Util::ResultOfOperation::init()
{
    status = ResultOfOperationStatus::ERROR;
    logErrorMessage = "";
    userErrorMessage = "";
}
