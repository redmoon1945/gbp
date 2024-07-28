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

#ifndef UTIL_H
#define UTIL_H

#include <QString>
#include <QLocale>
#include <QColor>
#include <QDate>
#include <QCoreApplication>
#include <sstream>
#include <iomanip>
#include <limits>


// All methods should be static, because there wont be any Util object created
class Util
{

    Q_DECLARE_TR_FUNCTIONS(Util)

public:

    Util();

    // generic structure to indicate succcess or error when doing anoperation.
    // Using this prevent us to return error using exception.
    struct OperationResult{
        bool success;
        QString errorStringUI;
        QString errorStringLog;
    };

    struct DateDifference {
        int years;
        int months;
        int days;

        bool operator==(const DateDifference &other) const
        {
            return    this->years  == other.years
                   && this->months == other.months
                   && this->days   == other.days;
        }

        bool operator!=(const DateDifference &other) const
        {
            return    this->years  != other.years
                   || this->months != other.months
                   || this->days   != other.days;
        }
    };

    // we have to redefine to prevent recursive include of util.h
    enum PeriodType {DAILY=0, WEEKLY=1,MONTHLY=2,END_OF_MONTHLY=3,YEARLY=4};

    // methods


    // static methods
    static void init();     // must be called as soon as possible when the application is started
    static QString getPeriodName(Util::PeriodType period, bool capitalizeFirstLetter, bool plural) ;
    static Util::DateDifference dateDifference(const QDate &from, const QDate &to);
    static QString elideText(QString str, int maxNoOfChar, bool elideRight);
    static qint64 quickPow10(uint n);
    static long double monthlyToAnnualGrowth(long double monthly);
    static long double annualToMonthlyGrowth(long double annual);
    static long double annualToDailyGrowth(long double annual);
    static bool areDoublesApproxEqual(double a, double b, double epsilon);
    static qint64 extractQint64FromDoubleWithNoFracPart(double amount, int &result)  ;
    static QString longDoubleToQString(long double value);
    static QList<double> doubleArrayToQlist(double* data, uint noElements);
    static QDateTime dateToDateTimeLocal(const QDate& date, const QTimeZone& tz);
    static long double presentValue(long double futureValue, double discountRate, int period);
    static long double presentValueConversionFactor(long double discountRate, int period);
    static bool isValidBoolString(const QString& input);
    static uint changeFontSize(bool aggressive, bool decreaseSize, uint originalSize);
    static QString getColorSmartName(QColor color, bool& found);
    static QString buildColorDisplayName(QColor color);
    static quint32 bitSet(quint32 number, quint32 n);
    static quint32 bitClear(quint32 number, quint32 n);
    static quint32 bitToggle(quint32 number, quint32 n);
    static quint32 bitCheck(quint32 number, quint32 n);
    static int noOfMonthDifference(QDate from , QDate to);

private:

    // for Period naming. To be modified by non-static methods
    static QString strDaily;
    static QString strWeekly;
    static QString strMonthly;
    static QString strEndOfMonthly;
    static QString strYearly;
    static QString strDailyPl;
    static QString strWeeklyPl;
    static QString strMonthlyPl;
    static QString strEndOfMonthlyPl;
    static QString strYearlyPl;
    static QString strDailyNc ;
    static QString strWeeklyNc ;
    static QString strMonthlyNc;
    static QString strEndOfMonthlyNc ;
    static QString strYearlyNc ;
    static QString strDailyPlNc ;
    static QString strWeeklyPlNc ;
    static QString strMonthlyPlNc ;
    static QString strEndOfMonthlyPlNc;
    static QString strYearlyPlNc ;

    // QT color names
    static QStringList qtColorNames;


private:



};

#endif // UTIL_H
