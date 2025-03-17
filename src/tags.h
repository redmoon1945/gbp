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

#ifndef TAGS_H
#define TAGS_H

#include <QSet>
#include <QHash>
#include <QUuid>
#include "qcoreapplication.h"
#include "tag.h"
#include "util.h"

// Represents a set of tags, unsorted
class Tags
{
    Q_DECLARE_TR_FUNCTIONS(Tags);

public:

    static quint16 MAX_NO_TAGS;

    Tags();
    Tags(const Tags& o);
    virtual ~Tags();

    // operators
    bool operator==(const Tags &o) const;
    bool operator!=(const Tags &o) const;
    Tags& operator=(const Tags& o) ;

    // Methods
    bool insert(Tag t);
    bool remove(Tag t);
    bool remove(QUuid tagId);
    void clear();
    bool containsTagId(QUuid tagId) const;
    quint16 containsTagName(QString name) const;
    Tag getTag(QUuid tagId, bool& found) const;
    QSet<Tag> getTags() const;
    quint16 size() const;
    QJsonObject toJson() const;
    static Tags fromJson(const QJsonObject &jsonObject, Util::OperationResult &result);


private:
    // the list of tags, unsorted
    QHash<QUuid, Tag> tags; // key is unique and correspond to the Tag ID
};

#endif // TAGS_H
