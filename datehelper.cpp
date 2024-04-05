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

#include "datehelper.h"

DateHelper::DateHelper()
{

}


QDate DateHelper::getNextEventDateDaily(QDate date,quint16 multiplier)
{
    if (multiplier==0){
        throw std::invalid_argument("multiplier must be greater than 0");
    }
    if (!date.isValid()){
        throw std::invalid_argument("date is invalid");
    }
    return date.addDays(1*multiplier);
}


QDate DateHelper::getNextEventDateWeekly(QDate date,quint16 multiplier)
{
    if (multiplier==0){
        throw std::invalid_argument("multiplier must be greater than 0");
    }
    if (!date.isValid()){
        throw std::invalid_argument("date is invalid");
    }
    return date.addDays(7*multiplier);
}


QDate DateHelper::getNextEventDateMonthly(QDate date,quint16 multiplier)
{
    if (multiplier==0){
        throw std::invalid_argument("multiplier must be greater than 0");
    }
    if (!date.isValid()){
        throw std::invalid_argument("date is invalid");
    }
    return date.addMonths(1*multiplier);
}


QDate DateHelper::getNextEventDateEndOfMonth(QDate date,quint16 multiplier)
{
    if (multiplier==0){
        throw std::invalid_argument("multiplier must be greater than 0");
    }
    if (!date.isValid()){
        throw std::invalid_argument("date is invalid");
    }
    return getNextEndOfMonth(date,multiplier);
}


QDate DateHelper::getNextEventDateYearly(QDate date,quint16 multiplier)
{
    if (multiplier==0){
        throw std::invalid_argument("multiplier must be greater than 0");
    }
    if (!date.isValid()){
        throw std::invalid_argument("date is invalid");
    }
    return date.addYears(1*multiplier);
}


// multiplier must be greater than 0
QDate DateHelper::getNextEndOfMonth(QDate date, quint16 multiplier)
{
    if (multiplier==0){
        throw std::invalid_argument("multiplier must be greater than 0");
    }
    if (!date.isValid()){
        throw std::invalid_argument("date is invalid");
    }
    QDate nextDay = date;
    QDate result = date;
    for (int i = 0; i < multiplier; ++i) {
        nextDay = result.addDays(1);
        result = QDate(nextDay.year(), nextDay.month(),nextDay.daysInMonth());
    }
    return result;
}

