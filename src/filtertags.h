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


#ifndef FILTERTAGS_H
#define FILTERTAGS_H

#include <QUuid>
#include <QSet>
#include "qcoreapplication.h"

// Convenient container representing the filter tags used for displaying CSDs
// in the EditScenarioDialog.

class FilterTags
{
    Q_DECLARE_TR_FUNCTIONS(FilterTags);

public:

    // ALL : to be displayed, csd must be linked to ALL of the selected tags
    // ANY : to be displayed, csd must be linked to AT LEAST ONE of the selected tags (default)
    // NONE : to be displayed, csd must be linked to NONE of the selected tags
    enum Mode {ALL=0, ANY=1, NONE=2};

    FilterTags();
    FilterTags(const FilterTags& o);
    virtual ~FilterTags();

    // operators
    bool operator==(const FilterTags &o) const;
    bool operator!=(const FilterTags &o) const;
    FilterTags& operator=(const FilterTags& o) ;

    // Methods
    void clear();

    // Getters / Setters
    bool getEnableFilterByTags() const;
    void setEnableFilterByTags(bool newEnableFilterByTags);
    QSet<QUuid> getFilterTagIdSet() const;
    void setFilterTagIdSet(const QSet<QUuid> &newFilterTagIdSet);
    Mode getMode() const;
    void setMode(Mode newMode);

private:
    // If false, Csds are displayed without considering the tags they are or are not linked to
    // and in that case, values of filterTagIdSet and mode are irrelevant
    bool enableFilterByTags;

    // Set of Tag Id used for filtering.
    QSet<QUuid> filterTagIdSet;

    //  Filter Tags Mode (how selected tags are combined for filtering)
    Mode mode;
};

#endif // FILTERTAGS_H
