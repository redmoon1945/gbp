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

#ifndef COMBINEDFESTREAMS_H
#define COMBINEDFESTREAMS_H
#include <QDate>
#include <QCoreApplication>
#include <QMap>
#include "fe.h"
#include "currencyhelper.h"



// Aggregator class intended to merge several QList<Fe> produced by individual FeStreamDef in a
// single scenario. One use real currency "double" value (and not qint64 decimal representation),
// because no more calculationis required from this stage before displaying to the chart.
class CombinedFeStreams
{
    // this is to be able to use the "tr" translation function
    Q_DECLARE_TR_FUNCTIONS(CombinedFeStreams)

public:

    // In currency unit. Pack all daily information produced by calculation.
    struct DailyInfo{
        double totalIncomes;    // sum of all incomes occurring during that day
        double totalExpenses;   // negative number. Sum of all expenses occurring during that day
        double totalDelta;      // total incomes - total expenses
        QList<FeDisplay> incomesList;   // List of all incomes occurring during that day
        QList<FeDisplay> expensesList;  // List of all expenses occurring during that day
        bool operator==(const DailyInfo& o) const;
        DailyInfo& operator=(const DailyInfo& o);
        };

    // Constructors and destructor
    CombinedFeStreams();
    CombinedFeStreams(const CombinedFeStreams& o);
    CombinedFeStreams(const QMap<QDate,DailyInfo> set);
    virtual ~CombinedFeStreams();

    // operators
    bool operator==(const CombinedFeStreams &o) const;
    CombinedFeStreams& operator=(const CombinedFeStreams &o);

    // Methods
    void addStream(const QList<Fe> theStream, CurrencyInfo currInfo);

    // getters & setters
    QMap<QDate, DailyInfo> getCombinedStreams() const;


private:
    // There could be a lot of data in there... 100 years for daily event leads to 36500 Fe.
    // For lets say 50 such FeStreams, it means roughly : 50*(36500*(24 bytes per Fe)) + (36500 * 32 bytes for the rest of DailyInfo)) = 45 MB
    // One expects however much much lower amount of data in real-life scenarios,
    QMap<QDate,DailyInfo> combinedStreams;

};

#endif // COMBINEDFESTREAMS_H
