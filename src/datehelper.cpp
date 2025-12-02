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

#include "datehelper.h"
#include <stdexcept>

QDate DateHelper::getNextDate(const QDate& date, TimeUnitType noOfTimeUnits, int multiplier)
{
    if ( !date.isValid() ) {
        throw std::invalid_argument("Invalid date");
    }
    if ( multiplier==0 ) {
        throw std::invalid_argument("Invalid multiplier");
    }

    switch (noOfTimeUnits) {
        case TimeUnitType::Day:
            return date.addDays(multiplier);
        case TimeUnitType::Week:
            return date.addDays(7*multiplier);
        case TimeUnitType::Month:
            return date.addMonths(multiplier);
        case TimeUnitType::Year:
            return date.addYears(multiplier);
        case TimeUnitType::EndOfMonth:
            return getNextDateEndOfMonth(date, multiplier);
        default: // should not happen
            return date;
    }
}


QDate DateHelper::getNextDateEndOfMonth(const QDate& date, int multiplier)
{
    if ( (!date.isValid()) || (multiplier==0)) {
        throw std::invalid_argument("Invalid input arguments");
    }

    QDate d = date;
    if ( (multiplier<0) || (date.day() == date.daysInMonth()) ) {
        d = date.addMonths(multiplier) ;
    } else {
        // multiplier > 0 and date is not at the end of a month
        d = date.addMonths(-1).addMonths(multiplier) ;
    }
    return QDate(d.year(), d.month(), d.daysInMonth());
}


// Check if a date correspond to the end of the month
bool DateHelper::isEndOfMonth(QDate date)
{
    if (!date.isValid()) {
        throw std::invalid_argument("Invalid date: " + date.toString().toStdString());
    }
    return (date.day()==date.daysInMonth());
}


