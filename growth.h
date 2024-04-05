/*
 *  Copyright (C) 2024 Claude Dumas <claudedumas63@protonmail.com>
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

#ifndef Growth_H
#define Growth_H
#include <QJsonObject>
#include <QCoreApplication>
#include <QMap>
#include <QDate>
#include <QList>
#include "util.h"



/**
 * Specify Growth, that is, how an amount of money changes over time, in percentage. Growth is always applied on a MONTHLY basis (growth cannot change within a single month).
 * However, for conveniance purpose, growth values are always specified on an "annual base", which means that this class internally convert theses values to monthly
 * basis for calculation purpose.
 * Growth values are internally always defined and persisted as decimal number (qint64), in order to keep an exact representation of the growth value (which is not possible with a double).
 * This means we also need to remember the number of decimals used in the decimal values to convert to nornmal double one.
 * For example, if the qint64 holds 12345, with a no of decimals = 3, it means the value stores represents exactly 12.345 %.
 * Growth is always applied on Day 1 of the month concerned.
 * There are 2 options available to define a growth pattern :
 * -> CONSTANT GROWTH : A constant annual growth value, that applies to past, present and future.
 * -> VARIABLE GROWTH : A set of (date, annual growth change value) that defines CHANGES in annual growth rate
 *    compared to before. Initial growth change value before the oldest dates defined is 0 (meaning no change).
 *    Each time a new (date, growth change value) is defined, it changes the current "growth" factor by the
 *    given percentage and become the new monthly growth value, to be applied every month. For example, if one has 10%
 *    monthly growth (so a multiplier of 1.1) and then a new ( a date, -10%) is defined (0.9 multiplier), then, from now on,
 *    the new monthy growth will be -0.01% (multiplier = .99)
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
    // A) For every occurence of amount (noOfMonths = 1)
    //    Everytime the amount occurs for a given date, current cummulative growth for that date is applied
    //    using the current value of the "modification" factor.
    // B) For only one occurence of amount every "noOfMonth" (noOfMonths > 1)
    //    The cummulative growth is applied every "noOfMonth" occurences. For example, with monthly growth of 10%,
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

    // Methods
    QMap<QDate,qint64> adjustForGrowth(qint64 amount, QList<QDate> occurenceDates, ApplicationStrategy appStrategy, AdjustForGrowthResult &ok) const;
    QJsonObject toJson() const;
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
    QMap<QDate,qint64> annualVariableGrowth;    // Key is date with Day always set to 1, value is ANNUAL growth CHANGE, expressed in decimal form (e.g. -8.123% = -8123 if no of decimal = 3)

    // *** Transient data for internal calculation : MONTHLY GROWTH ***
    // *** This is just an APPROXIMATION of persistent data ***
    // for constant type
    long double monthlyConstantGrowth ;                 // percentage on MONTHLY basis
    // for variable type
    QMap<QDate,long double> monthlyVariableGrowth;      // Key is date with Day always set to 1, value is MONTHLY growth CHANGE


    void recalculateMonthlyData();
    long double calculateNewAmountConstantGrowth(QDate previousDate, QDate nextDate, long double previousAmount, long double previousMonthlyGrowth) const;
    uint noOfMonthSpanned(QDate from , QDate to) const;
    QSharedPointer<long double> buildMonthlyMultiplierVector(uint noOfMonthSpan, QDate from) const;
    void isFactorsValid( QMap<QDate,qint64> factorsToBeChecked, Util::OperationResult &result );
};

#endif // Growth_H
