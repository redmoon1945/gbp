/*
 *  Copyright (C) 2024-2026 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
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

/** @brief Convenient container representing a set of tag IDs used as filter.
 *  @details Only the Tags Ids are collected in this object. This class optionally supports a
 *  "combination mode" for tags, that is if tags should be used
 *  in "AND", "OR", "NOT".
 */
class FilterTags
{
    Q_DECLARE_TR_FUNCTIONS(FilterTags);

public:

    /**
     * @brief Represent the possible tags combination mode
     */
    enum Mode {
        ALL=0,      ///< AND mode : all tags must be linked to the CSD.
        ANY=1,      ///< OR mode : at least one tag must be linked the CSD.
        NOT=2       ///< NOT mode : none of the tag must be linked to the CSD.
    };

    FilterTags();
    FilterTags(const FilterTags& o);
    virtual ~FilterTags();

    // operators
    bool operator==(const FilterTags &o) const;
    bool operator!=(const FilterTags &o) const;
    FilterTags& operator=(const FilterTags& o) ;

    // Methods

    /**
     * @brief Remove all tag ids contained in this object. Combination mode is NOT affected.
     */
    void clear();

    /**
     * @brief Re-initialize the whole object, emptying the tags list and setting
     *  combination mode to Mode::ANY.
     */
    void reset();

    /**
     * @brief Get the number of elements (tag's ID) in this object.
     * @return The number of elements in this object.
     */
    qsizetype size();
    /**
     * @brief Check if this tag id is contained in this object.
     * @param tagId The Tag id to check.
     * @return True if this tag id is contained in this object, else false.
     */
    bool containsTagId(const QUuid tagId );

    QSet<QUuid> getFilterTagIdSet() const;
    void setFilterTagIdSet(const QSet<QUuid> &newFilterTagIdSet);
    Mode getMode() const;
    void setMode(Mode newMode);

private:

    QSet<QUuid> filterTagIdSet;///< Set of Tag Id used for filtering. Unordered.
    Mode mode;///< Filter Tags Mode (how selected tags are combined for filtering).
};

#endif // FILTERTAGS_H
