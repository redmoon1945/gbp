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

#ifndef UTIL_H
#define UTIL_H

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
    static QString elideText(QString str, int maxNoOfChar, bool elideRight);
    static qint64 quickPow10(uint n);
    static long double monthlyToAnnualGrowth(long double monthly);
    static long double annualToMonthlyGrowth(long double annual);
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
     *   -1 : fail, b has a fractional part and so does not contain an int.
     *   -2 : fail : b is not a number (NAN).
     *   -3 : fail : b is either +infinite or -infinite
     *   -4 : fail : b is too large positively to fit in a qint64
     *   -5 : fail : b is too large negatively to fit in a qint64
     * @return The converted value, if the operation was a success. If not, 0 is returned.
     */
    static qint64 extractQint64FromDoubleWithNoFractionalPart(double amount, int &result)  ;

    static quint16 extractQuint16FromDoubleWithNoFracPart(double amount, quint16 maxValue, int &result)  ;
    static QString longDoubleToQString(long double value);
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
     * @brief Calculate conversion factor to transform a future value F into a present value P,
     * that is P = F * factor, where factor = 1 / ((1 + r)^n).
     * @param discountRate Discount rate per period ("r"), in percentage. Must be >= 0.
     * @param period Number of period ("n") AFTER present period. Can be negative.
     * @return The conversion factor.
     * @throws std::invalid_argument if discountRate<0.
     */
    static long double presentValueConversionFactor(long double discountRate, int period);

    static bool isValidBoolString(const QString& input);
    static QString getColorSmartName(QColor color, bool& found);
    static QString buildColorDisplayName(QColor color);
    static quint32 bitSet(quint32 number, quint32 n);
    static quint32 bitClear(quint32 number, quint32 n);
    static quint32 bitToggle(quint32 number, quint32 n);
    static quint32 bitCheck(quint32 number, quint32 n);
    static int noOfMonthDifference(QDate from , QDate to);
    static void calculateZoomXaxis( QDateTime &min, QDateTime &max, double expansionFactor);
    static void calculateZoomYaxis(double &min, double &max, double expansionFactor);
    static bool findMinMaxInYvalues(const QList<QPointF> ptList, double from, double to, double &min, double &max);
    static QString wordCapitalize(bool upper, QString s);

    /**
     * @brief Change the size of a font according to some parametric constraints, without affecting
     * the other properties.
     * @param font The font to be changed.
     * @param intensity Intensity of the font resizing.
     * @param decreaseSize True if font size must be decreased, false if it must be increased.
     */
    static void changeFontSize(QFont& font, Util::FontResizeIntensity intensity,
        bool decreaseSize);

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
     * @brief Get the stylesheet string associated to a color, in order to set the stylesheet
     * of a widget (e.g. a button).
     * @return The string for stylesheet.
     */
    static QString getStyleSheetStringForColor(QColor color);

    /**
     * @brief Convert a string to a valid QUuid. Braces are NOT allowed.
     * @param s The string to convert. It is trimmed before the conversion.
     * @param success True if the convertion succeeded, false otherwise.
     * @return
     */
    static QUuid convertStringToQuuid(QString s, bool& success);


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
     * @param decreaseSize True if the size must increase, false if it must increase.
     * @param originalSize Original size of the font in "point". Cannot be 0.
     * @return The new size calculated. 1 is the minimum value returned.
     */
    static uint calculateFontResize(Util::FontResizeIntensity intensity, bool decreaseSize,
        uint originalSize);


private:



};

#endif // UTIL_H
