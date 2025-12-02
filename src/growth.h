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

#ifndef Growth_H
#define Growth_H
#include <QJsonObject>
#include <QCoreApplication>
#include <QMap>
#include <QDate>
#include <QList>
#include "util.h"

/**
  * @class Growth
  * @brief The Growth class represents a financial growth pattern, either constant or variable,
  * applied on the 1st of every month, to an initial amount.
  * @details Growth class specifies a Growth Pattern, that is, how an inital amount changes over
  * time. The pattern is built using one or more "Growth value", expressed in annual percentage,
  * ranging from -100% to 10000%. Internally, the cumulative effect of successive growth values
  * applied through time is captured in a "cumulative growth factor" (CGF). At any time, the
  * current value of the CGF can be multiplied by the initial amount in order to get the
  * "growth adjusted" value of the initial amount at that moment.
  * Growth is always APPLIED on a MONTHLY basis (consequently, growth value cannot change within a
  * single month). However, for conveniance purpose, growth values are always SPECIFIED on an
  * "annual base", which means that this class internally convert theses values to monthly basis
  * for calculation purpose.  Growth is always applied on Day 1 of the month concerned.
  * Growth values are internally always defined and persisted as decimal number (qint64), in order
  * to keep an exact representation of the growth value (which is not possible with a double).
  * This means we also need to remember the number of decimals used in the decimal values for
  * conversion to double value. For example, if the qint64 holds 12345, with a no of decimals = 3,
  * it means the value stores represents exactly 12.345 %. With no of decimals = 5, the value
  * represents 0.12345%.
  * Swoth is never applied to the first financial event from Start Date.
  *
  * There are 2 options available to define a growth pattern :
  * -> CONSTANT GROWTH :
  *        A constant annual growth value, that applies from -infinity to +infinity. For example,
  *        with a annual growth value of 5%, applied to an amount of 1000 starting to exist on
  *        1 jan 2000, we have :
  *            1 jan 2000 : 1000 (growth does not apply on the first month of existence)
  *            1 feb 2000 : 1004.74 (1 month of monthly growth of 0.40741238%,
  *                         CFG is now 1.0040741238)
  *            1 jan 2001 : 1050 (CGF is now = 1.05)
  *            1 jan 2002 : 1102.50 (CGF is now = 1.05 * 1.05 =  1.1025)
  * -> VARIABLE GROWTH :
  *        A set of (date, annual growth value) "pairs", each defining a new growth value to be
  *        applied from this date, until a new pair is defined or otherwise for ever.
  *        Before the first pair is defined, growth value is 0 (CGF is 1). Each time a new pair
  *        is defined, a new growth value becomes in force from that date. For example, lets say we
  *        have the following pairs defined :  (1 jan 2000, 10%) , (1 jan 2002, -10%).
  *        Here are the values of an amount of 1000 that starts existing on 1 jan 1998 and is
  *        submitted to this growth pattern:
  *            1 jan 1999 : 1000 (growth = 0%, CFG = 1)
  *            1 jan 2000 : 1100 (new growth value=10%, CGF is now = 1 + 10% = 1.1)
  *            1 jan 2001 : 1210 (growth value still at 10%, CFG is now = 1.1 + 10% = 1.21)
  *            1 jan 2002 : 1089 (new growth value=-10%, CFG is now = 1.21 - 10% = 1.089)
  *            1 jan 2003 : 980.10 (growth still at -10%, CFG is now = 1.089 - 10% = 0.9801)
  *        If the amount of 1000 starts existing on 1 jan 2003, then we have
  *            1 jan 2003 : 1000 (no growth applied on the first month of existence))
  *            1 jan 2004 : 900 (growth is -10% as lastly defined in 1 jan 2002,
  *                         CFG is now at 1 - 10% =  0.9)
  *            1 jan 2005 : 810 (growth is -10% as lastly defined in 1 jan 2002, CFG is now
  *                         at 0.9 - 10% = 0.81)
  */
class Growth
{
    Q_DECLARE_TR_FUNCTIONS(Growth)

public:

    /// @brief Enum defining the type of growth pattern.
    enum class Type {CONSTANT,VARIABLE, NONE};

    /**
     * @brief Precision of stored value for growth percentage, expressed as no of decimals.
     * @details E.g., if NO_OF_DECIMALS =5, then 1% is stored as 100000 int value, which allows
     * a precision of 0.00001 for growth percentage.
     * WARNING : changing this value affect compatibility with other versions regarding file format.
     */
    static constexpr uint NO_OF_DECIMALS = 5;

    /// @brief Maximum annual growth percentage (10,000%).
    static constexpr double MAX_GROWTH_DOUBLE = 10000;

    /// @brief Minimum annual growth percentage (-100%).
    static constexpr double MIN_GROWTH_DOUBLE = -100;

    /// @brief Maximum annual growth in decimal form.
    static qint64 MAX_GROWTH_DECIMAL;

    /// @brief Minimum annual growth in decimal form. Negative number.
    static qint64 MIN_GROWTH_DECIMAL ;

    /// @brief For variable growth, this is the maximum no of values that can be defined.
    static constexpr double VARIABLE_MAX_NO_OF_ENTRIES = 10000;

    /**
     * @struct ApplicationStrategy
     * @brief Specify how growth is applied on the amount.
     * @details 2 options are available:
     *   A) For every occurrence of amount (noOfMonths = 1)
     *      Everytime the amount occurs for a given date, current cumulative growth for that
     *      date is applied using the current value of the "modification" factor.
     *   B) For only one occurrence of amount every "noOfMonth" (noOfMonths > 1).
     *      The cumulative growth is modified every "noOfMonth" occurrences. For example, with
     *      monthly growth of 10%, intial amount of 100 and "noOfMonth" = 3, we would have the
     *      following monthly corrected amount series :
     *        100,100,100, 133.1,133.1,133.1, 177.1561,177.1561,177.1561, etc
     */
    struct ApplicationStrategy{
        uint noOfMonths;
    };

    /**
     * @struct AdjustForGrowthResult
     * @brief Result structure for the adjustForGrowth method.
     */
    struct AdjustForGrowthResult {
        bool success;     ///< Indicates if the operation was successful. Saturation is NOT an error
        uint saturationCount;      ///< Number of times amounts exceeded allowed limits.
        QString errorMessageUI;    ///< User-facing error message.
        QString errorMessageLog;   ///< Log-friendly error message.
    };

    // Constructors and destructor
    Growth() ;                                  ///< no growth
    Growth(const Growth& ag);                   ///< copy constuctor, copy-on-write
    virtual ~Growth();

    // Factories

    /**
     * @brief Creates a Growth object with a constant annual growth rate from a double percentage.
     * @param annualPercentage Annual growth percentage (e.g., 5.0 means 5%).
     * @return Growth object with constant growth.
     * @throws std::domain_error If annualPercentage is out of bounds.
     */
    static Growth fromConstantAnnualPercentageDouble(double annualPercentage);
    /**
     * @brief Creates a Growth object with a constant annual growth rate from a decimal value.
     * @param annualPercentage Annual growth in decimal form. E.g. 500000 means 5% with 5 decimals.
     * @return Growth object with constant growth.
     * @throws std::domain_error If annualPercentage is out of bounds.
     */
    static Growth fromConstantAnnualPercentageDecimal(qint64 annualPercentage);
    /**
     * @brief Creates a Growth object with variable growth rates.
     * @param newVariableGrowth Map of dates to annual growth percentages in decimal form.
     * E.g., 500000 for 5% with 5 decimals. If empty, no growth is ever applied.
     * No more than VARIABLE_MAX_NO_OF_ENTRIES entries.
     * @return Growth object with variable growth.
     * @throws std::domain_error If growth values or dates are invalid.
     */
    static Growth fromVariableDataAnnualBasisDecimal(const QMap<QDate,qint64> newVariableGrowth);
    /**
     * @brief Creates a Growth object with variable growth rates.
     * @param newVariableGrowth Map of dates to annual growth percentages. No more than
     * No more than VARIABLE_MAX_NO_OF_ENTRIES entries.
     * @return Growth object with variable growth. If empty, no growth is ever applied.
     * @throws std::domain_error If growth values or dates are invalid.
     */
    static Growth fromVariableDataAnnualBasisDouble(const QMap<QDate,double> newVariableGrowth);

    // operators
    Growth& operator=(const Growth &o);
    bool operator==(const Growth& o) const;
    bool operator!=(const Growth& o) const;

    // Methods

    /**
     * @brief Adjusts an amount for growth over a series of dates. Amounts are >= 0.
     * @details Given a series of occurrences in time for a given initially fixed amount, adjust
     * each amount for each occurrence date to take into account the growth pattern defined by
     * this object. Also convert the amounts to present value if requested.
     * If a calculated amount goes over the max allowed for any amount in CurrencyHelper, it is set
     * to this max value and the no of times it occurred is returned as a warning in the "saturation"
     * field of AdjustForGrowthResult returned. Growth is 0 before occurrenceDates.first.
     * Growth is never applied for the first date in occurrenceDates, which is the reference date
     * for that amount (first occurrence). It is also not applied whole the whole month
     * corresponding to that date. There is NO LIMITATION on the timespan of the occurrence dates
     * list, because growth need to be calculated from the first occurrence, event if it is before
     * "today".
     * @param amount Initial amount to adjust (must be >= 0).
     * @param occurrenceDates Sorted list of dates for amount occurrences. Max no of entries if
     * NOT limited.
     * @param appStrategy Strategy defining how growth is applied.
     * @param pvDiscountRate Annual discount rate for present value calculation (percentage, >= 0).
     * @param pvCalculationReferenceDate Present value reference date.
     * @param ok Result of the operation, including success and error details.
     * @return List of adjusted amounts (>=0), with matching indexes with occurrenceDates.
     */
    QList<quint64> adjustForGrowth(quint64 amount, QList<QDate> occurrenceDates,
        ApplicationStrategy appStrategy, double pvDiscountRate, QDate pvCalculationReferenceDate,
        AdjustForGrowthResult &ok) const;

    /**
     * @brief Serializes the Growth object to JSON.
     * @return QJsonObject containing persistent data.
     */
    QJsonObject toJson() const;

    /**
     * @brief Deserializes a Growth object from JSON.
     * @param jsonObject JSON object containing growth data.
     * @param result Operation result with success and error details.
     * @return Growth object created from JSON.
     */
    static Growth fromJson(const QJsonObject& jsonObject, Util::ResultOfOperation &result);

        /**
     * @brief Multiplies all growth values by a factor.
     * @details Resulting growth value(s) are capped to the max allowed (MAX_GROWTH_DECIMAL) and
     * if this happens at least once, capped is set to true.
     * @param factor Multiplier for growth values (must be >= 0).
     * @param capped Set to true if any growth value was capped to MAX_GROWTH_DECIMAL.
     * @throws std::domain_error If factor is negative or type is unknown.
     */
    void changeByFactor(double factor, bool& capped);

    /**
     * @brief Converts a double percentage to decimal form.
     * @param d Annual growth percentage (e.g., 5.0 for 5%).
     * @return Decimal representation (qint64).
     */
    static qint64 fromDoubleToDecimal(long double d);

    /**
     * @brief Converts a decimal value to double percentage.
     * @param i Decimal representation of growth.
     * @return Annual growth percentage as double.
     */
    static long double fromDecimalToDouble(qint64 i);


    // getters
    Type getType() const;
    qint64 getAnnualConstantGrowth() const;
    QMap<QDate, qint64> getAnnualVariableGrowth() const;
    long double getMonthlyConstantGrowth() const;
    QMap<QDate, long double> getMonthlyVariableGrowth() const;


private:

    // --- persistent data : ANNUAL GROWTH ---
    // --- this is what is stored in the JSON file ---
    // --- and known to the "outside world" ---

    /// @brief Growth type (CONSTANT, VARIABLE, or NONE).
    Type type;

    /**
     * @brief For constant type : annual constant growth percentage in decimal form value expressed
     * in decimal (e.g. 8.123% = 8123, 10000% = 1000000).
     */
    qint64 annualConstantGrowth ;

    /**
     * @brief For variable type : map of dates to annual growth percentages in decimal form.
     * @details Transitions list. Key is date with Day always set to 1, value is ANNUAL growth,
     * expressed in decimal form (e.g. -8.123% = -8123 if no of decimal = 3)
     */
    QMap<QDate,qint64> annualVariableGrowth;


    // --- Transient data for internal calculation : MONTHLY GROWTH ---
    // --- This is just an APPROXIMATION of persistent data ---

    /**
     * @brief For constant type : monthly constant growth percentage as double.
     */
    long double monthlyConstantGrowth ;

    /**
     * @brief For variable type : map of dates to monthly growth percentages as double.
     * @details Like annualVariableGrowth, but value is MONTHLY growth in percentage and in double.
     */
    QMap<QDate,long double> monthlyVariableGrowth;


    // --- Methods ---

    /**
     * @brief Recalculates monthly growth data from annual growth data.
     */
    void recalculateMonthlyData();

    /**
     * @brief Calculates the new amount after applying constant growth over a date range.
     * @details First month has no growth applied (reference value).
     * @param previousDate Start date of the period.
     * @param nextDate End date of the period.
     * @param previousAmount Initial amount.
     * @param previousMonthlyGrowth Monthly growth percentage.
     * @return New amount after growth.
     * @throws std::invalid_argument If dates are invalid or nextDate is before previousDate.
     */
    long double calculateNewAmountConstantGrowth(QDate previousDate, QDate nextDate,
        long double previousAmount, long double previousMonthlyGrowth) const;

    /**
     * @brief Builds a vector of cumulative growth factors for a date range.
     * @details Build a QT-wrapped long double vector containing, for each month in a given date
     * interval, a "multiplier" (the CGF or Cumulative Growth Factor) to be applied against an
     * initial and constant amount in order to give the "cumulative growth adjusted" value for
     * this amount.
     * @param noOfMonthSpan Number of months to cover. Must be > 0.
     * @param from The date of the first occurrence of the amount and is always assigned CGF of "1"
     * (no growth, as it is the reference value).
     * @return An array of cumulative growth factors.
     * @throws std::domain_error If noOfMonthSpan is 0 or from is invalid.
     */
    QList<long double> buildMonthlyMultiplierVector(uint noOfMonthSpan, QDate from) const;

    /**
     * @brief Builds a vector of present value multipliers for a date range.
     * @details Warning : first occurrence date will probably be different from PV present date
     * (before or after). It means we have to find the first value of the PV factor to be assigned
     * to the first value of the vector.
     * @param annualDiscountrate Annual discount rate (percentage).
     * @param noOfMonthSpan Number of months to cover from (and including) firstOccurrence.
     * Must be > 0.
     * @param firstOccurrence Start date of occurrences.
     * @param pvPresent Present value reference date.
     * @return An array of present value multipliers.
     * @throws std::domain_error If inputs are invalid.
     */
    QList<long double> buildPvMonthlyMultiplierVector(double annualDiscountrate,
        uint noOfMonthSpan, QDate firstOccurrence, QDate pvPresent) const;

    /**
     * @brief Validates variable growth factors, which are in decimal form.
     * @param factorsToBeChecked Map of dates to growth values to validate.
     * @param result Operation result with validation details.
     */
    void areFactorsValid( QMap<QDate,qint64> factorsToBeChecked, Util::ResultOfOperation &result );

    /**
     * @brief Validates variable growth factors, which are in double form
     * (direct annual percentage).
     * @param factorsToBeChecked Map of dates to growth values to validate.
     * @param result Operation result with validation details.
     */
    void areFactorsValid( QMap<QDate,double> factorsToBeChecked, Util::ResultOfOperation &result );

    /**
     * @brief Convert a Growth Type enum value into an int, for JSon streaming purpose.
     * @details For compatibility reason with old versions of GBP, we must always
     * have for the int representation : CONSTANT=0  VARIABLE=1  NONE=2.     * @param type The StreamType to convert into an int.
     * @return The conversion result.
     */
    static int convertTypeFromEnumToInt(Type theType);

    /**
     * @brief Convert an int to a Growth Type value, for JSon streaming purpose.
     * @details For compatibility reason with old versions of GBP, we must always
     * have for the int representation : CONSTANT=0  VARIABLE=1  NONE=2.
     * @param value The int to convert.
     * @param result Growth Type resulting from the conversion.
     * @return True if the conversion was successful, false otherwise.
     */
    static bool convertTypeFromIntToEnum(int value, Growth::Type& result);
};

#endif // Growth_H
