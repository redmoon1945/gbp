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

#include "fe.h"


bool Fe::operator==(const Fe& o) const {
    if( fabs(amount - o.amount) >= 0.0001 ){
        return false;
    }
    // Two QWeakPointer<T> instances point to the same shared object if they reference the same
    // control block, which is managed by the associated QSharedPointer<T>. The control block
    // contains the object pointer and reference counts.
    auto ptr1 = csdPtr.toStrongRef();
    auto ptr2 = o.csdPtr.toStrongRef();
    if ( (ptr1!=nullptr) && (ptr2!=nullptr) && (ptr1->getId()==ptr2->getId()) ){
        return true;
    }

    return false;
}


bool Fe::operator!=(const Fe &o) const
{
    return !(*this==o);
}


Fe &Fe::operator=(const Fe &o)
{
    this->amount = o.amount;
    this->csdPtr = o.csdPtr; // simple duplication of the weak pointer
    return *this;
}



QString Fe::toString(QString streamDefName, const CurrencyInfo& currInfo,
    const QLocale& locale) const {
    QString amountString = locale.toString(amount,'f', currInfo.noOfDecimal);
    QString s = QString("%1 : %2").arg(amountString).arg(streamDefName);
    return s;
}
