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

#ifndef UTIL_H
#define UTIL_H

#include "currencyhelper.h"
#include <optional>
#include <QString>
#include <QLocale>
#include <QColor>
#include <QDate>
#include <QCoreApplication>
#include <QPointF>


// Must not include gbpcontroller.h, because it assumes a QApplication, which is not the case here.
// All methods should be static, because there wont be any Util object created

class Util
{

    Q_DECLARE_TR_FUNCTIONS(Util)

public:

    Util();

    // *** Enum, Structures ***

    /**
     * @brief Formatting mode for formatDouble().
     */
    enum class DoubleFormatMode {
        Standard,     ///< Always use fixed/shortest format; thresholds ignored.
        Exponential,  ///< Always use exponential format; thresholds ignored.
        Mixed         ///< Use maxThreshold / minThreshold to switch to exponential.
    };

    /**
     * @brief Parameters for formatDouble().
     */
    struct DoubleFormatParams {
        struct StandardParams {
            std::optional<quint8> noOfDecimals; ///< nullopt: shortest, 0: integer, >0: fixed.
        };
        struct ExponentialParams {
            quint8 significantDigits = 3;  ///< Significant digits in exponential format (>= 1).
            bool includeExpSign = false;   ///< Keep '+' in exponent (e.g. "1.2e+4" vs "1.2e4").
        };
        struct MixedParams {
            double maxThreshold = 0.0;  ///< |amount| >= maxThreshold → exponential.
            double minThreshold = 0.0;  ///< Non-zero |amount| < minThreshold → exponential.
        };

        bool includePlusSign = false;  ///< Prepend '+' to positive values (all modes).
        StandardParams standard;
        ExponentialParams exponential;
        MixedParams mixed;
    };

    /**
     * @brief Intensity of a font resizing operation.
     */
    enum class FontResizeIntensity { WEAK, AVERAGE, AGGRESSIVE, EXTREME };

    /**
     * @brief Indicate how an operation has completed its task.
     * @details Used only in ResultOfOperation structure.
     * -> SUCCCESS : no associated message
     * -> ERROR : 1 associated error message to the user
     */
    enum class ResultOfOperationStatus {SUCCESS, ERROR};

    /**
     * @struct ResultOfOperation
     * @brief Generic structure to indicate success or error
     * for an operation that has completed its task.
     * @details Log message is in english only and is intended only or logging.
     * User message is multi-lingual.
     */
    struct ResultOfOperation{
        ResultOfOperationStatus status;
        QString userErrorMessage;
        QString logErrorMessage;

        /**
         * @brief Init the structure automatically with a status of ERROR.
         */
        ResultOfOperation();

        /**
         * @brief Init the structure with a status of ERROR.
         */
        void init();
    };

    /**
     * @struct DateDifference
     * @brief Used when calculating the difference between 2 dates.
     */
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


    // *** Methods ***


    static void init();     // must be called as soon as possible when the application is started

    static Util::DateDifference dateDifference(const QDate &from, const QDate &to);

    /**
     * @brief Truncate a string to a maximum number of characters, adding ellipsis.
     * @param str The string to elide.
     * @param maxNoOfChar Maximum number of characters allowed (including ellipsis).
     * @param elideRight If true, elide from the right (e.g., "Long text..."), if false, elide
     * from the left (e.g., "...text").
     * @return The elided string.
     */
    static QString elideText(const QString &str, int maxNoOfChar, bool elideRight);

    static qint64 quickPow10(uint n);

    /**
     * @brief Convert monthly growth rate to equivalent annual growth rate.
     * @param monthly Monthly growth rate as a decimal (e.g., 0.01 for 1% monthly growth).
     * @return Equivalent annual growth rate as a decimal.
     */
    static long double monthlyToAnnualGrowth(long double monthly);

    /**
     * @brief Convert annual growth rate to equivalent monthly growth rate.
     * @param annual Annual growth rate as a decimal (e.g., 0.12 for 12% annual growth).
     * @return Equivalent monthly growth rate as a decimal.
     */
    static long double annualToMonthlyGrowth(long double annual);

    /**
     * @brief Convert annual growth rate to equivalent daily growth rate.
     * @param annual Annual growth rate as a decimal (e.g., 0.12 for 12% annual growth).
     * @return Equivalent daily growth rate as a decimal.
     */
    static long double annualToDailyGrowth(long double annual);

    /**
     * @brief Compare two double values within a specified range (epsilon).
     * @details The 2 values must be in the same approx range.
     * @param a The first double to compare.
     * @param b The second double to compare.
     * @param epsilon The value range.
     * @return True if the values are within epsilon of each other, false otherwise.
     */
    static bool areDoublesApproxEqual(double a, double b, double epsilon);

    /**
     * @brief Convert into a qint64 a double that contains no fractional part.
     * @param amount The double value to convert. Can be negative.
     * @param result Indicate if the operation is a success :
     *    0 : success
     *   -1 : fail, amount has a fractional part and so does not contain an int.
     *   -2 : fail : amount is not a number (NAN).
     *   -3 : fail : amount is either +infinite or -infinite
     *   -4 : fail : amount is too large positively to fit in a qint64
     *   -5 : fail : amount is too large negatively to fit in a qint64
     * @return The converted value, if the operation was a success. If not, 0 is returned.
     */
    static qint64 extractQint64FromDoubleWithNoFractionalPart(double amount, int &result)  ;

    static quint16 extractQuint16FromDoubleWithNoFracPart(double amount, quint16 maxValue,
        int &result)  ;

    static QString longDoubleToQString(long double value);

    /**
     * @brief Formats a double value as a localized string.
     * @details Converts the given amount to a string using the specified locale and mode.
     * Handles locales with non-ASCII digits (e.g., Arabic numerals).
     *
     * @param amount The double value to format.
     * @param locale The QLocale to use for formatting (decimal separator, digit characters).
     * @param mode Formatting mode: Standard, Exponential, or Mixed (see DoubleFormatMode).
     * @param params Formatting parameters (see DoubleFormatParams).
     * @return The formatted string representation of the amount.
     * @throws std::invalid_argument if params.maxThreshold or params.minThreshold is negative,
     *         or params.expSignificantDigits == 0.
     *
     * @par Example
     * - formatDouble(123.45, locale, DoubleFormatMode::Standard, {}) returns "123.45"
     * - formatDouble(123.45, locale, DoubleFormatMode::Standard, {.noOfDecimals=4})
     *       returns "123.4500"
     * - formatDouble(1234567.0, locale, DoubleFormatMode::Exponential, {}) returns "1.23e6"
     * - formatDouble(1234567.0, locale, DoubleFormatMode::Mixed,
     *       {.noOfDecimals=2, .maxThreshold=1e6, .expSignificantDigits=4}) returns "1.235e6"
     * - formatDouble(0.00042, locale, DoubleFormatMode::Mixed,
     *       {.noOfDecimals=2, .minThreshold=0.001}) returns "4.2e-4"
     */
    static QString formatDouble(double amount, const QLocale &locale, DoubleFormatMode mode,
        DoubleFormatParams params);

    static QList<double> doubleArrayToQlist(double* data, uint noElements);

    static QDateTime dateToDateTimeLocal(const QDate& date, const QTimeZone& tz);

    /**
     * @brief Calculate the present value P of a future value F,
     * that is P = F/factor, where factor = (1 + r)^n.
     * @param discountRate Discount rate per period ("r"), in percentage. Must be >= 0.
     * @param period Number of period ("n") AFTER present period. Can be negative.
     * @return The present value.
     * @throws std::invalid_argument if discountRate<0.
     */
    static long double presentValue(long double futureValue, double discountRate, int period);


    /**
     * @brief Calculate the future value F of a present value P,
     * that is P*factor = F, where factor = (1 + r)^n.
     * @param discountRate Discount rate per period ("r"), in percentage. Must be >= 0.
     * @param period Number of period ("n") AFTER present period. Can be negative.
     * @return The future value.
     * @throws std::invalid_argument if discountRate<0.
     */
    static long double futureValue(long double presentValue, double discountRate, int period);

    /**
     * @brief Calculate conversion factor to transform a past or future value into a present value.
     * @details Computes the discount factor using the formula: 1 / ((1 + r)^n)
     * where r = discountRate/100 and n = period.
     * For future values (period > 0): Discounts a future value V to present value P, so that
     * P = F * factor , with factor = 1 / ((1 + r)^n), which reduces the value.
     * Example: $100 in 2 years at 5% becomes $100 / (1.05)^2 = $90.70 today.
     * For past values (period < 0): Compounds a past value A to present value P, so that
     * P = F * factor , where factor = (1 + r)^|n| (equivalent to 1 / ((1 + r)^n) when n < 0),
     * which increases the value.
     * Example: $100 from 2 years ago at 5% becomes $100 * (1.05)^2 = $110.25 today.
     * @param discountRate Discount rate per period, in percentage (e.g., 5.0 for 5%). Must be >= 0.
     * @param period Number of periods from the present: positive for future, negative for past.
     * @return The conversion factor.
     * @throws std::invalid_argument if discountRate < 0.
     */
    static long double toPvConversionFactor(long double discountRate, int period);

    static bool isValidBoolString(const QString& input);
    static QString getColorSmartName(const QColor &color, bool& found);
    static QString buildColorDisplayName(const QColor &color);
    static quint32 bitSet(quint32 number, quint32 n);
    static quint32 bitClear(quint32 number, quint32 n);
    static quint32 bitToggle(quint32 number, quint32 n);
    static quint32 bitCheck(quint32 number, quint32 n);

    /**
     * @brief Calculates the signed number of calendar months between two dates.
     * @details The sign of the result indicates temporal direction:
     *   - target > reference → positive period (months forward)
     *   - target < reference → negative period (months backward)
     *   - target = reference → zero
     * @param reference The reference date from which the month offset is calculated.
     * Year must be >= 0.
     * @param target The date whose month offset from reference is calculated.
     * Year must be >= 0.
     * @return Signed number of complete months from reference to target.
     *         Positive if target > reference, negative if target < reference,
     *         zero if in the same month.
     * @note Calculates month offset, not inclusive count. The day component
     *       determines whether a month is complete.
     *       Examples:
     *       - QDate(1000,1,15) to QDate(1000,3,15) → 2 months
     *       - QDate(1000,3,15) to QDate(1000,1,15) → -2 months
     *       - QDate(1000,1,31) to QDate(1000,3,1)  → 1 month (not 2)
     * @throws std::invalid_argument If either date is invalid or has year < 0.
     */
    static int noOfMonthsDifference(QDate reference , QDate target);

    /**
     * @brief Calculates the signed number of calendar years between two dates.
     * @details The sign of the result indicates temporal direction:
     *   - target > reference → positive period (years forward)
     *   - target < reference → negative period (years backward)
     *   - target = reference → zero
     * @param reference The reference date from which the year offset is calculated.
     * Year must be >= 0.
     * @param target The date whose year offset from reference is calculated.
     * Year must be >= 0.
     * @return Signed number of complete years from reference to target.
     *         Positive if target > reference, negative if target < reference,
     *         zero if in the same year.
     * @note Calculates year offset, not inclusive count. The month and day
     *       components determine whether a year is complete.
     *       Examples:
     *       - QDate(1000,6,15) to QDate(2000,6,15) → 1000 years
     *       - QDate(2000,6,15) to QDate(1000,6,15) → -1000 years
     *       - QDate(1000,1,1) to QDate(1000,12,31) → 0 years (same year)
     * @throws std::invalid_argument If either date is invalid or has year < 0.
     */
    static int noOfYearsDifference(QDate reference , QDate target);

    /**
     * @brief Adjust X-axis time range by expanding or contracting around the midpoint.
     * @param min Minimum datetime value (modified in place).
     * @param max Maximum datetime value (modified in place).
     * @param expansionFactor Factor to expand (>1.0) or contract (<1.0) the range.
     */
    static void calculateZoomXaxis( QDateTime &min, QDateTime &max, double expansionFactor);

    /**
     * @brief Adjust Y-axis numeric range by expanding or contracting around the midpoint.
     * @param min Minimum Y value (modified in place).
     * @param max Maximum Y value (modified in place).
     * @param expansionFactor Factor to expand (>1.0) or contract (<1.0) the range.
     */
    static void calculateZoomYaxis(double &min, double &max, double expansionFactor);

    static bool findMinMaxInYvalues(const QList<QPointF> ptList, double from, double to, double &min, double &max);
    static QString wordCapitalize(bool upper, QString s);

    /**
     * @brief Change the size of a font according to some parametric constraints, without affecting
     * the other properties.
     * @param font The font to be changed.
     * @param intensity Intensity of the font resizing.
     * @param decreaseSize True if font size must be decreased, false if it must be increased.
     * @param context Used when logging to explain in what context this is called.
     */
    static void changeFontSize(QFont& font, Util::FontResizeIntensity intensity,
        bool decreaseSize, const QString& context);

    /**
     * @brief Return the Locale to be used throughout the application, taking into account a
     * "system locale override" argument passed to the application at launch time.
     * @details Must be called once by main.cpp as early as possible.
     * The "system locale override" argument format must be : -locale=<L>-<T> where <L> is a 2 or 3
     * char string representing ISO 639 code and <T> is a 2 or 3 char string representing
     * ISO 3166 code. E.g. : en-US, fr-CA
     * The Default System Locale will be returned in case of any error in the "system locale
     * override" argument passed or simply if it is not passed.
     * @param arguments Arguments passed to the application when it is started.
     * @param systemLocale Indicate it the Locale returned is the Default System Locale (true) or
     * not (false).
     * @return The final QLocale to use by GBP.
     */
    static QLocale getLocale(QStringList arguments, bool& systemLocale);

    /**
     * @brief Validates whether a QString adheres to the ISO 8601 date format (YYYY-MM-DD) and is
     * a valid date.
     * @param dateStr The date string to validate.
     * @param valid Output parameter indicating whether the date is valid.
     * @return QDate object (valid if the input is a valid ISO 8601 date, invalid otherwise).
     * @details The method checks if the input string matches the ISO 8601 date format (YYYY-MM-DD),
     * with:
     * - Year: Exactly 4 digits.
     * - Month: Exactly 2 digits (01-12).
     * - Day: Exactly 2 digits (01-31, respecting month and leap year rules).
     * - Separators: Hyphens (`-`) only.
     * The date is validated to ensure it exists (e.g., "2020-02-29" is valid, but "2020-02-30"
     * is not). If validation succeeds, `result` is set to true, and a valid QDate is returned.
     * If validation fails (due to incorrect format or invalid date), `result` is set to false,
     * and an invalid QDate is returned.
     */
    static QDate isValidISO8601Date(const QString& dateStr, bool& valid);

    /**
     * @brief Red color, contrast and luminosity optimized for both dark and light background.
     * @return The optimized red color.
     */
    static QColor getOptimizedRed();

    /**
     * @brief Green color, contrast and luminosity optimized for both dark and light background.
     * @return The optimized green color.
     */
    static QColor getOptimizedGreen();

     /**
     * @brief Blue color, contrast and luminosity optimized for both dark and light background.
     * @return The optimized blue color.
     */
    static QColor getOptimizedBlue();

    /**
     * @brief Orange color, contrast and luminosity optimized for both dark and light background.
     * @return The optimized orange color.
     */
    static QColor getOptimizedOrange();

    /**
     * @brief Purple color, contrast and luminosity optimized for both dark and light background.
     * @return The optimized purple color.
     */
    static QColor getOptimizedPurple();

    /**
     * @brief Teal color, contrast and luminosity optimized for both dark and light background.
     * @return The optimized teal color.
     */
    static QColor getOptimizedTeal();

    /**
     * @brief Get the stylesheet string associated to a color, in order to set the stylesheet
     * of a widget (e.g. a button).
     * @return The string for stylesheet.
     */
    static QString getStyleSheetStringForColor(const QColor &color);

    /**
     * @brief Convert a string to a valid QUuid. Braces are NOT allowed.
     * @param s The string to convert. It is trimmed before the conversion.
     * @param success True if the convertion succeeded, false otherwise.
     * @return
     */
    static QUuid convertStringToQuuid(const QString &s, bool& success);

    /**
     * @brief Calculate statistical measures from a list of double values.
     * @details Computes mean, standard deviation (population), median, and sum from the input
     * data. If the input list is empty, all output parameters are set to 0.
     * @note Variance and standard deviation are both measures of variability in a dataset,
     * but they differ in their calculation and interpretation. Variance is the average of the
     * squared differences from the mean, while standard deviation is the square root of the
     * variance. Standard deviation is often preferred because it is expressed in the same units
     * as the original data, making it easier to interpret in the context of the data set.
     * This implementation uses population standard deviation (dividing by N).
     * @param data The input list of double values to analyze.
     * @param mean Output parameter for the arithmetic mean (average) of the data.
     * @param stdDeviation Output parameter for the population standard deviation.
     * @param median Output parameter for the median value. For odd-sized datasets, this is
     * the middle value when sorted. For even-sized datasets, this is the average of the two
     * middle values.
     * @param sum Output parameter for the sum of all values in the data.
     */
    static void calculateStats(const QList<double> data, double& mean, double& stdDeviation,
        double& median, double& sum);

    /**
     * @brief Build an HTML QString representation for display of multiple double values.
     * Formats each value with its corresponding name and optionally applies color.
     * Takes into account the locale and the currency for proper formatting.
     * @param values List of double values to format
     * @param names List of names/labels for each value. Empty name is allowed.
     * @param colors List of colors to apply to each value. If a color is invalid,
     * the corresponding value will not be colored.
     * @param currInfo Currency information for formatting the values
     * @param locale Locale for number formatting
     * @return The QString HTML representation in the format "name1 = value1 , name2 = value2 , ..."
     * where values are wrapped in colored HTML spans if valid colors are provided.
     *
     * Example:
     * @code
     * QList<double> values = {1234.56, 789.12, 445.44};
     * QList<QString> names = {"Income", "Expense", "Delta"};
     * QList<QColor> colors = {Qt::green, Qt::red, Qt::blue};
     * CurrencyInfo currInfo = {...};
     * QLocale locale = QLocale::system();
     * QString result = Util::buildStringForDoubles(values, names, colors, currInfo, locale);
     * // Returns: "Income = <span style='color: #00ff00;'>1,234.56</span> ,
     * //           Expense = <span style='color: #ff0000;'>789.12</span> ,
     * //           Delta = <span style='color: #0000ff;'>445.44</span>"
     * @endcode
     */
    static QString buildStringForDoubles(const QList<double> values, const QList<QString> names,
        const QList<QColor> colors, const CurrencyInfo& currInfo, const QLocale& locale);

    /**
     * @brief Calculate the percentage change from P (previous) to C (current).
     * @details Formula: ((C - P) / |P|) * 100. Uses |P| in the denominator so that
     * the sign of the result always reflects the direction of the change, even when
     * P is negative.
     * @param P Previous value.
     * @param C Current value.
     * @param undefinedResult Set to true if P is 0, ±inf or NaN (result is meaningless).
     *        Set to false otherwise.
     * @return The percentage change, or 0.0 if undefined.
     */
    static double percentageChange(double P, double C, bool &undefinedResult);

private:

    // QT color names
    static QStringList qtColorNames;

    /**
     * @brief Convert an enum FontResizeIntensity into string. For debugging.
     * @param intensity Intensity of the convertion.
     * @return The string representaition.
     */
    static QString getFontResizeIntensityNames(Util::FontResizeIntensity intensity);

    /**
     * @brief Calculate the new size of a font according to some constraints.
     * @param Intensity How aggressive the change will be, @see FontResizeIntensity.
     * @param decreaseSize True if the size must decrease, false if it must increase.
     * @param originalSize Original size of the font in "point". Cannot be 0.
     * @return The new size calculated. 1 is the minimum value returned.
     */
    static uint calculateFontResize(Util::FontResizeIntensity intensity, bool decreaseSize,
        uint originalSize);


private:



};

#endif // UTIL_H
