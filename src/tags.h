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

#ifndef TAGS_H
#define TAGS_H

#include <QSet>
#include <QHash>
#include <QUuid>
#include "qcoreapplication.h"
#include "tag.h"
#include "util.h"


/**
 * @class Tags
 * @brief Represents a set of Tag objects, unsorted.
 */
class Tags
{
    Q_DECLARE_TR_FUNCTIONS(Tags);

public:

    static const quint16 MAX_NO_TAGS = 500; ///< @brief Maximum no of tags this object can contain.

    Tags();
    Tags(const Tags& o);
    virtual ~Tags();

    // *** operators ***

    bool operator==(const Tags &o) const;
    bool operator!=(const Tags &o) const;
    Tags& operator=(const Tags& o) ;

    // *** Methods ***

    /**
     * @brief Add the tag into the set or replace its value if it already exist.
     * @details If this is a new tag (not present in the set), it will be added, unless the max no
     * of tags has been reached. If there is a tag that has the same ID, its value ("Tag") will be
     * replaced by t.
     * @param t The tag to be inserted/replaced.
     * @return False if max no of tags has been reached and prevents insertion. True otherwise.
     */
    bool insert(const Tag &t);

    /**
     * @brief Remove the tag from the set if the tag key already exists.
     * @param tagId The tag id of the tag to be removed. Must not be invalid, otherwise
     * nothing happens.
     * @return True if an item was actually removed, false otherwise.
     */
    bool remove(QUuid tagId);

    /**
     * @brief Remove all the tags contained in this object.
     */
    void clear();

    /**
     * @brief Check if this object contains a tag with the id equals to "tagId".
     * @param tagId Tag ID to search. Must not be invalid, otherwise
     * nothing happens.
     * @return True if it does, false otherwise.
     */
    bool containsTagId(QUuid tagId) const;

    /**
     * @brief Change the QSet<QUuid> passed in argument by removing any Ids that are not in this
     * object.
     * @param idList The QSet<QUuid> to be inspected and potentially modified. Must not contain
     * invalid QUuids, otherwise they are skipped.
     * @return True if idList has been modified, false otherwise.
     */
    bool cleanIdList(QSet<QUuid>& idList) const;

    /**
     * @brief Count no of time "name" corresponds to the name of a tag in this object.
     * @param name Name of a tag
     * @return No of time "name" is used as a name for a tag in the set.
     */
    quint16 containsTagName(const QString &name) const;

    /**
     * @brief Get the number of Tags contained in this object.
     * @return The number of Tags contained in this object
     */
    quint16 size() const;

    /**
     * @brief Get a list of all tag id stored in this object, as a QList.
     * @details A QList is returned because this is the fastest implementation offred by QHash.
     * @return The list of tag id.
     */
    QList<QUuid> getTagIdSet();

    /**
     * @brief Get a list of all tag id stored in this object, as a QSet.
     * @return The list of tag id.
     */
    QSet<QUuid> getTagIdSetAsQset();

    /**
     * @brief Serialize this object to a JSon object.
     * @return The serialized object in JSon.
     */
    QJsonObject toJson() const;

    /**
     * @brief Build a new Tags object from a serialized JSon representation.
     * @param jsonObject The JSon objet.
     * @param result Indicates if the operation was a success or an error occurred.
     * @return The new Tags object.
     */
    static Tags fromJson(const QJsonObject &jsonObject, Util::ResultOfOperation &result);

    /**
     * @brief Return a Tag contained in this set, using its id as search key.
     * @param tagId Tag ID to search. Must not be invalid, otherwise
     * an empty tag is returned and found is set to false.
     * @param found Set to True if the tag has been found, false otherwise.
     * @return The Tag found.
     */
    Tag getTag(QUuid tagId, bool& found) const;


    // Getters

    QSet<Tag> getTags() const;


private:
    /**
     * @brief List of tags contained in this object, unsorted. Key is unique and
     * correspond to the Tag ID
     */
    QHash<QUuid, Tag> tags;
};

#endif // TAGS_H
