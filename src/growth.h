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

#ifndef Growth_H
#define Growth_H
#include <QJsonObject>
#include <QCoreApplication>
#include <QMap>
#include <QDate>
#include <QList>
#include "util.h"



/**
 * Specify a Growth Pattern, that is, how an inital amount changes over time. The pattern is built using one or more
 * "Growth value", expressed in annual percentage, ranging from -100% to 10000%.
 * Internally, the cummulative effect of successive growth values applied through time is captured in a
 * "cummulative growth factor" (CGF). At any time, the current value of the CGF can be multiplied by the initial amount
 * in order to get the "growth adjusted" value of the initial amount at that moment.
 *
 * Growth is always APPLIED on a MONTHLY basis (consequently, growth value cannot change within a single month).
 * However, for conveniance purpose, growth values are always SPECIFIED on an "annual base", which means that this
 * class internally convert theses values to monthly basis for calculation purpose.  Growth is always applied on
 * Day 1 of the month concerned.
 *
 * Growth values are internally always defined and persisted as decimal number (qint64), in order to keep an exact
 * representation of the growth value (which is not possible with a double). This means we also need to remember
 * the number of decimals used in the decimal values for conversion to double value. For example, if the qint64
 * holds 12345, with a no of decimals = 3, it means the value stores represents exactly 12.345 %. With no of decimals
 * = 5, the value represents 0.12345%
 *
 * There are 2 options available to define a growth pattern :
 * -> CONSTANT GROWTH :
 *        A constant annual growth value, that applies from -infinity to +infinity. For example, with a annual
 *        growth value of 5%, applied to an amount of 1000 starting to exist on 1 jan 2000, we have :
 *            1 jan 2000 : 1000 (growth does not apply on the first month of existence)
 *            1 feb 2000 : 1004.74 (1 month of monthly growth of 0.40741238%, CFG is now 1.0040741238)
 *            1 jan 2001 : 1050 (CGF is now = 1.05)
 *            1 jan 2002 : 1102.50 (CGF is now = 1.05 * 1.05 =  1.1025)
 * -> VARIABLE GROWTH :
 *        A set of (date, annual growth value) "pairs", each defining a new growth value to be applied
 *        from this date, until a new pair is defined or otherwise for ever.
 *        Before the first pair is defined, growth value is 0 (CGF is 1). Each time a new pair is defined,
 *        a new growth value becomes in force from that date.
 *        For example, lets say we have the following pairs defined :  (1 jan 2000, 10%) , (1 jan 2002, -10%).
 *        Here are the values of an amount of 1000 that starts existing on 1 jan 1998 and is submitted to this growth pattern:
 *            1 jan 1999 : 1000 (growth = 0%, CFG = 1)
 *            1 jan 2000 : 1100 (new growth value=10%, CGF is now = 1 + 10% = 1.1)
 *            1 jan 2001 : 1210 (growth value still at 10%, CFG is now = 1.1 + 10% = 1.21)
 *            1 jan 2002 : 1089 (new growth value=-10%, CFG is now = 1.21 - 10% = 1.089)
 *            1 jan 2003 : 980.10 (growth still at -10%, CFG is now = 1.089 - 10% = 0.9801)
 *        If the amount of 1000 starts existing on 1 jan 2003, then we have
 *            1 jan 2003 : 1000 (no growth applied on the first month of existence))
 *            1 jan 2004 : 900 (growth is -10% as lastly defined in 1 jan 2002, CFG is now at 1 - 10% =  0.9)
 *            1 jan 2005 : 810 (growth is -10% as lastly defined in 1 jan 2002, CFG is now at 0.9 - 10% = 0.81)
 */


class Growth
{
    Q_DECLARE_TR_FUNCTIONS(Growth)

public:

    enum Type {CONSTANT,VARIABLE, NONE};

    static uint NO_OF_DECIMALS;                       // To convert decimal to double. Should always be in the range [0..7]
    static double MAX_GROWTH_DOUBLE;                  // per year, in percentage
    static double MIN_GROWTH_DOUBLE;                  // per year, in percentage.
    static qint64 MAX_GROWTH_DECIMAL;
    static qint64 MIN_GROWTH_DECIMAL ;

    // Specify how growth is applied on the amount. 2 options :
    // A) For every occurrence of amount (noOfMonths = 1)
    //    Everytime the amount occurs for a given date, current cummulative growth for that date is applied
    //    using the current value of the "modification" factor.
    // B) For only one occurrence of amount every "noOfMonth" (noOfMonths > 1)
    //    The cummulative growth is modified every "noOfMonth" occurrences. For example, with monthly growth of 10%,
    //    intial amount of 100 and "noOfMonth" = 3, we would have the following monthly corrected amount series :
    //        100,100,100, 133.1,133.1,133.1, 177.1561,177.1561,177.1561, etc
    struct ApplicationStrategy{
        uint noOfMonths;
    };

    struct AdjustForGrowthResult {
        bool success;                   // saturation is NOT considered an error
        uint saturationCount;           // if != 0, some amount were bigger than allowed
        QString errorMessageUI;
        QString errorMessageLog;
    };

    // Constructors and destructor
    Growth() ;                                  // no growth
    Growth(const Growth& ag);                   // copy constuctor, copy-on-write
    virtual ~Growth();

    // Factories
    static Growth fromConstantAnnualPercentageDouble(double annualPercentage);
    static Growth fromConstantAnnualPercentageDecimal(qint64 annualPercentage);
    static Growth fromVariableDataAnnualBasisDecimal(QMap<QDate,qint64> newVariableGrowth);

    // operators
    Growth& operator=(const Growth &o);
    bool operator==(const Growth& o) const;
    bool operator!=(const Growth& o) const;

    // Methods
    QMap<QDate,qint64> adjustForGrowth(quint64 amount, QList<QDate> occurrenceDates, ApplicationStrategy appStrategy, double pvDiscountRate, QDate pvCalculationReferenceDate, AdjustForGrowthResult &ok) const;
    QJsonObject toJson() const;
    void changeByFactor(double factor, bool& capped);
    static Growth fromJson(const QJsonObject& jsonObject, Util::OperationResult &result);
    static qint64 fromDoubleToDecimal(long double d);
    static long double fromDecimalToDouble(qint64 i);


    // getters / setters (no setters)
    Type getType() const;
    qint64 getAnnualConstantGrowth() const;
    QMap<QDate, qint64> getAnnualVariableGrowth() const;
    long double getMonthlyConstantGrowth() const;
    QMap<QDate, long double> getMonthlyVariableGrowth() const;


private:

    // *** persistent data : ANNUAL GROWTH ***
    // *** this is what is stored in the JSON file ***
    // *** and known to the "outside world" ***
    Type type;
    // for constant type
    qint64 annualConstantGrowth ;               // percentage on ANNUAL basis, value expressed in decimal  (e.g. 8.123% = 8123, 10000% = 1000000)
    // for variable type
    QMap<QDate,qint64> annualVariableGrowth;    // Transitions list. Key is date with Day always set to 1, value is ANNUAL growth, expressed in decimal form (e.g. -8.123% = -8123 if no of decimal = 3)

    // *** Transient data for internal calculation : MONTHLY GROWTH ***
    // *** This is just an APPROXIMATION of persistent data ***
    // for constant type
    long double monthlyConstantGrowth ;                 // percentage on MONTHLY basis, expressed as double
    // for variable type
    QMap<QDate,long double> monthlyVariableGrowth;      // Like annualVariableGrowth, but value iss MONTHLY growth in percentage and in double


    void recalculateMonthlyData();
    long double calculateNewAmountConstantGrowth(QDate previousDate, QDate nextDate, long double previousAmount, long double previousMonthlyGrowth) const;
    QSharedPointer<long double> buildMonthlyMultiplierVector(uint noOfMonthSpan, QDate from) const;
    QSharedPointer<long double> buildPvMonthlyMultiplierVector(double annualDiscountrate, uint noOfMonthSpan, QDate firstOccurrence, QDate pvPresent) const;
    void isFactorsValid( QMap<QDate,qint64> factorsToBeChecked, Util::OperationResult &result );
};

#endif // Growth_H
