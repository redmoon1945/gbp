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

struct GbpMonthDay{
    quint8 month;   // jan = 1
    quint8 day;
};

class DateHelper
{
public:
    DateHelper();
    static QDate getNextEventDateDaily(QDate date,quint16 multiplier)  ;
    static QDate getNextEventDateWeekly(QDate date,quint16 multiplier)  ;
    static QDate getNextEventDateMonthly(QDate date,quint16 multiplier)  ;
    static QDate getNextEventDateEndOfMonth(QDate date,quint16 multiplier)  ;
    static QDate getNextEventDateYearly(QDate date,quint16 multiplier)  ;
    static QDate getNextEndOfMonth(QDate date, quint16 multiplier) ;
    static bool isEndOfMonth(QDate date);

};

#endif // DATEHELPER_H
