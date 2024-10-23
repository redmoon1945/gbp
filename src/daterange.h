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

#ifndef DATERANGE_H
#define DATERANGE_H
#include <QDate>
#include <QJsonObject>
#include <QCoreApplication>
#include "util.h"


// Date interval. It has a "type" that indicates its nature, that is :
// * BOUNDED : start and end are finite value and are always inclusive. Start always <= end
// * INFINITE : start is at -infinity and end is at +infinity
// * EMPTY : no start not end
// Interval is set to not be greater than 1000 years (does not make any sense to go over that in the context of gbp)
class DateRange
{
    Q_DECLARE_TR_FUNCTIONS(DateRange)

public:

    enum Type { EMPTY,BOUNDED,INFINITE};
    static const long MAX_YEARS = 1000; // max size of a DateRange

    // Constructors and destructor
    DateRange( const QDate from, const QDate to );   // always generates a DateRange of type BOUNDED
    DateRange();                                    // always generates a DateRange of type EMPTY
    DateRange(const DateRange & o);
    DateRange(Type r);
    virtual ~DateRange();

    // operators
    bool operator==(const DateRange & o) const;
    bool operator!=(const DateRange & o) const;
    DateRange& operator=(const DateRange &o);

    // methods
    int GetNoOfYearsSpanned() const;
    bool intersectWith(const DateRange o) const;
    DateRange intersection(const DateRange o) const;
    bool includeDate(const QDate o) const;
    QList<QDate> getDateList() const;   // get all the dates in the range
    QString toString() const;
    QJsonObject toJson() const;
    static DateRange fromJson(const QJsonObject& o, Util::OperationResult &result);

    // getters and setters
    QDate getStart() const;
    QDate getEnd() const;
    Type getType() const;

private:
    QDate start;
    QDate end;
    Type type;
};

#endif // DATERANGE_H
