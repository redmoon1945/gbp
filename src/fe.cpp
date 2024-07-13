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

#include "fe.h"


bool Fe::operator==(const Fe& o) const {
    if( (amount != o.amount) ||
        (occurence != o.occurence) ||
        (id != o.id) ){
        return false;
    }
    return true;
}

Fe &Fe::operator=(const Fe &o)
{
    this->amount = o.amount;
    this->occurence = o.occurence;
    this->id = o.id;
    return *this;
}


QString Fe::toString() const {
    return QString("(%1,%2)").arg(amount).arg(occurence.toString());
}


// for the amount , a "loose" comparison is performed. 2 double are declared equal if
// the difference is less than the smallest unit of all the currency available (3 decimals + 1 spare for rounding)
bool FeDisplay::operator==(const FeDisplay& o) const {
    if( fabs(amount - o.amount) >= 0.0001 ){
        return false;
    }
    if( id != o.id ){
        return false;
    }
    return true;
}

FeDisplay &FeDisplay::operator=(const FeDisplay &o)
{
    this->amount = o.amount;
    this->id = o.id;
    return *this;
}



QString FeDisplay::toString(QString streamDefName, const CurrencyInfo& currInfo, const QLocale& locale) const {
    QString amountString = locale.toString(amount,'f', currInfo.noOfDecimal);
    QString s = QString("%1 : %2").arg(amountString).arg(streamDefName);
    return s;
}
