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

#ifndef DATEHELPER_H
#define DATEHELPER_H

#include <QDate>

/**
 * @class DateHelper
 * @brief Contains helper methods to manipulate dates.
 */
class DateHelper
{
public:

    /**
     * @brief Time unit types for date calculations.
     * @details Defines different time intervals that can be used to advance or retreat dates.
     */
    enum class TimeUnitType {
        Day,         ///< Calendar day interval (24-hour period)
        Week,        ///< 7-day interval
        Month,       ///< Calendar month interval, preserving the day-of-month when possible
        EndOfMonth,  ///< Calendar month interval, always landing on the last day of the month
        Year         ///< 12-month interval (one calendar year)
    };

    /**
     * @brief From a given date "X", calculates the next or previous date by adding or subtracting
     * from "X" a given number "multiplier" of "Time Unit" (see TimeUnitType).
     * @param date The starting date.
     * @param multiplier Number of Time Units to add (positive) or subtract (negative).
     * @return The resulting date.
     * @throws std::invalid_argument If the date is invalid.
     */
    static QDate getNextDate(const QDate& date, TimeUnitType noOfTimeUnits, int multiplier)  ;

    /**
     * @brief Checks if a date is the last day of its month.
     * @param date The date to check.
     * @return True if the date is the last day of the month, false otherwise.
     * @throws std::invalid_argument If the date is invalid.
     */
    static bool isEndOfMonth(QDate date);

private:
    /**
     * @brief Gets the N-th end-of-month strictly after (or before) a given date.
     * @details Ends-of-month are counted starting from the given date, excluding the date itself:
     *   - multiplier > 0 : returns the multiplier-th end-of-month strictly AFTER date.
     *     -> If date is already an end-of-month, it does not count : the 1st one is the end of
     *        the next month. E.g. Jan 31 x1 = Feb 28, Jan 31 x3 = Apr 30.
     *     -> If date is mid-month, the end of its own month is still ahead and counts as the
     *        1st one. E.g. Jan 15 x1 = Jan 31, Jan 15 x3 = Mar 31 (and NOT Apr 30).
     *   - multiplier < 0 : returns the |multiplier|-th end-of-month strictly BEFORE date.
     *     E.g. Jul 15 x-1 = Jun 30, Jul 31 x-1 = Jun 30, Jul 15 x-7 = Dec 31 of previous year.
     *
     * The result is always the last day of a month, so there is no day-of-month drift : the
     * result can safely be used as the input of a next call.
     *
     * **Note for financial event generation** : this method answers "where is the N-th
     * end-of-month from here ?", not "where is the first event of a series ?".
     * @param date The reference date. Must be valid.
     * @param multiplier Number of ends-of-month to move forward (positive) or backward
     * (negative). Must not be 0.
     * @return The last day of the target month.
     * @throws std::invalid_argument If date is invalid or multiplier is 0.
     */
    static QDate getNextDateEndOfMonth(const QDate &date, int multiplier);
};

#endif // DATEHELPER_H
