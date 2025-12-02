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

    enum class TimeUnitType {Day, Week, Month, EndOfMonth, Year};

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
    static QDate getNextDateEndOfMonth(const QDate &date, int multiplier);
};

#endif // DATEHELPER_H
