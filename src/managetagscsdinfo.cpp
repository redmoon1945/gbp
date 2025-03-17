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


#include "managetagscsdinfo.h"



// This is a global function. Create a Hash value for managetags::CsdItem, required to use
// QSet with key = managetags::CsdItem
// Qt doc says : "there must also be a GLOBAL qHash()
// function that returns a hash value for an argument of the key's type."
size_t managetags::qHash(const managetags::CsdItem &t, size_t seed ) {
    return qHashMulti(seed, t.id, t.name);
}

bool managetags::CsdItem::operator==(const managetags::CsdItem &o) const
{
    if ( (this->id!=o.id) || (this->name!=o.name) || (this->isIncome!=o.isIncome) ||
        (this->color!=o.color) ) {
        return false;
    } else {
        return true;
    }
}
