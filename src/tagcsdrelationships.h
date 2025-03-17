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

#ifndef TAGCSDRELATIONSHIPS_H
#define TAGCSDRELATIONSHIPS_H

#include <QUuid>
#include <QSet>
#include <qjsonobject.h>
#include "util.h"


// Structure representing a relationship between a Tag and a Cash Stream Definition
// Must be made global because of qHash. Should not be used by anything else than
// TagCsdRelationships Class.
struct TagCsdRelationship{
    QUuid tagId;
    QUuid csdId;
    bool operator==(const TagCsdRelationship &o) const;
    bool operator!=(const TagCsdRelationship &o) const;
    TagCsdRelationship& operator=(const TagCsdRelationship &o) ;
};


// Hold N-N relationships between Tags and Cash Stream Definition, using their ids as keys.
// No attempt is ever made to check if tagIds and csdIds actually correspond to existing items.
// This implementation is inefficient, but simple. As there wont be a lot of
// relationships for a given scenario (for 10 tags, 50 csds, assuming full connection,
// it is 500 relationships total), the speed will be adequate.
class TagCsdRelationships
{
    Q_DECLARE_TR_FUNCTIONS(TagCsdRelationships)

public:
    static quint16 MAX_NO_RELATIONSHIPS;

    struct IntegrityReport{
        bool errorFound;
        QString problem;
    };

    TagCsdRelationships();
    TagCsdRelationships(const TagCsdRelationships& o);
    virtual ~TagCsdRelationships();

    // operators
    bool operator==(const TagCsdRelationships &o) const;
    bool operator!=(const TagCsdRelationships &o) const;
    TagCsdRelationships& operator=(const TagCsdRelationships&o) ;

    // *** Methods ***
    // query
    bool tagHasRelationships(const QUuid tagId);
    bool csdHasRelationships(const QUuid csdId);
    bool relationshipExists(const QUuid tagId, const QUuid csdId);
    QSet<QUuid> getRelationshipsForTag(const QUuid tagId);
    QSet<QUuid> getRelationshipsForCsd(const QUuid csdId);
    QSet<QUuid> getAllTagsWithRelationships();
    QSet<QUuid> getAllCsdsWithRelationships();
    uint noOfRelationships();
    // add, delete
    void addRelationship(const QUuid tagId, const QUuid csdId);
    bool deleteRelationship(const QUuid tagId, const QUuid csdId);
    void deleteRelationshipsForTag(const QUuid tagId);
    void deleteRelationshipsForCsd(const QUuid csdId);
    void clear();
    // clone
    void cloneCsdRelationshipsForTag(QUuid sourceTagId, QUuid destTagId);
    void cloneTagRelationshipsForCsd(QUuid sourceCsdId, QUuid destCsdId);
    // Json stuff
    QJsonObject toJson() const;
    static TagCsdRelationships fromJson(const QJsonObject& o, Util::OperationResult &result);

private:
    // the relationships store. Nothing is sorted. No duplicate entry ever.
    QSet<TagCsdRelationship> store;

};

#endif // TAGCSDRELATIONSHIPS_H
