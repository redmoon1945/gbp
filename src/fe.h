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

#ifndef FE_H
#define FE_H

#include <QtGlobal>
#include <QDate>
#include <QUuid>
#include "currencyhelper.h"

// A Financial Event is a single amount of money (either income or expense) that occurs at a specific moment in time (date).
// It is generated through a FE Stream Definition. Amount is an int in the smaller currency unit (e.g. cents for US dollar).
struct Fe{
    qint64 amount;          // NEGATIVE number for expense
    QDate occurrence;        // Date when the financial event occured
    QUuid id;               // Reference to the StreamDefinition having generated this Fe.

    bool operator==(const Fe &o) const;
    Fe& operator=(const Fe &o) ;

    QString toString() const;
};

// subset of a Fe just for display purpose (curve-related) : occurance date is not required.
// toString is also geared toward curve display information.
struct FeDisplay{
    double amount;          // total and final amount for a specific day, negative number for expense, in currency unit
    QUuid id;               // Reference to the StreamDefinition having generated this Fe

    bool operator==(const FeDisplay& o) const;
    FeDisplay &operator=(const FeDisplay &o) ;

    QString toString(QString streamDefName, const CurrencyInfo& currInfo, const QLocale& locale) const;
};

// structure to hold absolute y values min/max of a QList<Fe>
struct FeMinMaxInfo{
    qint64 yMin;    // 0 or positive
    qint64 yMax;    // 0 or positive
};



#endif // FE_H
