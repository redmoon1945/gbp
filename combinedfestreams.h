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

#ifndef COMBINEDFESTREAMS_H
#define COMBINEDFESTREAMS_H
#include <QDate>
#include <QCoreApplication>
#include <QMap>
#include "fe.h"
#include "currencyhelper.h"



// Aggregator class intended to merge several QList<Fe> produced by individual FeStreamDef in a single scenario.
// One use real currency "double" value (and not qint64 decimal representation), because no more calculationis required
// from this stage before displaying to the chart.
class CombinedFeStreams
{
    // this is to be able to use the "tr" translation function
    Q_DECLARE_TR_FUNCTIONS(CombinedFeStreams)

public:

    struct DailyInfo{
        double totalIncomes;    // already in currency unit
        double totalExpenses;   // currency unit, negative number
        double totalDelta;      // currency unit, total incomes - total expenses
        QList<FeDisplay> incomesList;
        QList<FeDisplay> expensesList;
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
    //QString toString(CurrencyInfo currInfo, QLocale locale) const;

    // getters & setters
    QMap<QDate, DailyInfo> getCombinedStreams() const;


private:
    // There could be a lot of data in there... 100 years for daily event leads to 36500 Fe.
    // For lets say 50 such FeStreams, it means roughly : 50*(36500*(24 bytes per Fe)) + (36500 * 32 bytes for the rest of DailyInfo)) = 45 MB
    // One expects however much much lower amount of data in real-life scenarios,
    QMap<QDate,DailyInfo> combinedStreams;

};

#endif // COMBINEDFESTREAMS_H
