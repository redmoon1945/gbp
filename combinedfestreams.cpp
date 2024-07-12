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

#include "combinedfestreams.h"
#include "currencyhelper.h"

CombinedFeStreams::CombinedFeStreams()
{
}


CombinedFeStreams::CombinedFeStreams(const CombinedFeStreams &o)
{
    this->combinedStreams = o.combinedStreams;
}


CombinedFeStreams::CombinedFeStreams(const QMap<QDate, DailyInfo> set)
{
    this->combinedStreams = set;
}


CombinedFeStreams &CombinedFeStreams::operator=(const CombinedFeStreams &o)
{
    this->combinedStreams = o.combinedStreams;
    return *this;
}


bool CombinedFeStreams::operator==(const CombinedFeStreams &o) const
{
    if ( !(this->combinedStreams == o.combinedStreams)) {
        return false;
    } else {
        return true;
    }
}


CombinedFeStreams::~CombinedFeStreams()
{
}


void CombinedFeStreams::addStream(const QList<Fe> theStream, CurrencyInfo currInfo)
{
    if (theStream.size()==0){
        return;
    }
    foreach(Fe fe, theStream){
        auto it = combinedStreams.find(fe.occurence);   // find an existing entry for that date, if any
        CombinedFeStreams::DailyInfo di;
        if(it != combinedStreams.end()){    // there is already something for that date : add to it
            di = it.value();
        } else{                             // there is NO entry for that date : init it
            di = CombinedFeStreams::DailyInfo();
            di.totalDelta = 0;
            di.totalExpenses = 0;
            di.totalIncomes = 0;
            di.incomesList = QList<FeDisplay>();
            di.expensesList = QList<FeDisplay>();
        }
        FeDisplay fed;
        int convResult;
        double amount = CurrencyHelper::amountQint64ToDouble(fe.amount, currInfo.noOfDecimal, convResult) ;
        if (convResult != 0){
            return; // should never happen
        }
        fed.amount = amount;
        fed.id = fe.id;
        if (fe.amount<0){
            // expense
            di.expensesList.append(fed);
            di.totalExpenses += amount;
        }else{
            // income
            di.incomesList.append(fed);
            di.totalIncomes += amount;
        }
        di.totalDelta += amount;
        combinedStreams[fe.occurence] = di;     // replacement if already exist
    }
}


// for the amount , a "loose" comparison is performed. 2 double are declared equal if
// the difference is less than the smallest unit of all the currency available (3 decimals + 1 spare for rounding)
bool CombinedFeStreams::DailyInfo::operator==(const DailyInfo& o) const{
    if( fabs(totalIncomes - o.totalIncomes) >= 0.0001 )
        return false;
    if( fabs(totalExpenses - o.totalExpenses) >= 0.0001 )
        return false;
    if( fabs(totalDelta - o.totalDelta) >= 0.0001 )
        return false;
    if(incomesList != o.incomesList)
        return false;
    if(expensesList != o.expensesList)
        return false;
    return true;
}

CombinedFeStreams::DailyInfo &CombinedFeStreams::DailyInfo::operator=(const DailyInfo &o)
{
    totalIncomes = o.totalIncomes;
    totalExpenses = o.totalExpenses;
    totalDelta = o.totalDelta;
    incomesList = o.incomesList;
    expensesList = o.expensesList;
    return *this;
}


// getter & setter

QMap<QDate, CombinedFeStreams::DailyInfo> CombinedFeStreams::getCombinedStreams() const
{
    return combinedStreams;
}


