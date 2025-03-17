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


#include "filtertags.h"


FilterTags::FilterTags() {
    clear();
}


FilterTags::FilterTags(const FilterTags &o)
{
    this->enableFilterByTags = o.enableFilterByTags;
    this->filterTagIdSet = o.filterTagIdSet;
    this->mode = o.mode;
}


FilterTags::~FilterTags()
{
}


bool FilterTags::operator==(const FilterTags &o) const
{
    if ( (this->enableFilterByTags != o.enableFilterByTags) ||
        (this->filterTagIdSet != o.filterTagIdSet) || (this->mode != o.mode) ) {
        return false;
    }
    return true;
}


bool FilterTags::operator!=(const FilterTags &o) const
{
    return !(*this==o);
}


FilterTags &FilterTags::operator=(const FilterTags &o)
{
    this->enableFilterByTags = o.enableFilterByTags;
    this->filterTagIdSet = o.filterTagIdSet;
    this->mode = o.mode;
    return *this;
}


void FilterTags::clear()
{
    enableFilterByTags = false;
    filterTagIdSet = {};
    mode = Mode::ANY;
}


// Getters / Setters



QSet<QUuid> FilterTags::getFilterTagIdSet() const
{
    return filterTagIdSet;
}

void FilterTags::setFilterTagIdSet(const QSet<QUuid> &newFilterTagIdSet)
{
    filterTagIdSet = newFilterTagIdSet;
}

FilterTags::Mode FilterTags::getMode() const
{
    return mode;
}

void FilterTags::setMode(FilterTags::Mode newMode)
{
    mode = newMode;
}

bool FilterTags::getEnableFilterByTags() const
{
    return enableFilterByTags;
}

void FilterTags::setEnableFilterByTags(bool newEnableFilterByTags)
{
    enableFilterByTags = newEnableFilterByTags;
}
