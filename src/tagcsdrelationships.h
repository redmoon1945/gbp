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

#ifndef TAGFSDRELATIONSHIPS_H
#define TAGFSDRELATIONSHIPS_H

#include "qjsonobject.h"
#include <quuid.h>
#include <QHash>
#include <QSet>
#include "util.h"


/**
 * @brief Hold N-N relationships between Tags and CashStreamDefs (a.k.a. csd),
 * using their ids only. This implementation is optimized for speed of access (query).
 */
class TagCsdRelationships
{
        Q_DECLARE_TR_FUNCTIONS(TagCsdRelationships)
public:
    /**
     * @brief MAX_NO_RELATIONSHIPS Max no of relationships authorized.
     */
    static const quint16 MAX_NO_RELATIONSHIPS = 10000;

    /**
     * @brief Used by checkIntegrity to report problems.
     */
    struct IntegrityReport{
        bool errorFound;
        QString problem;
    };

    // *** constructors and destructor ***
    TagCsdRelationships();
    TagCsdRelationships(const TagCsdRelationships& o);
    virtual ~TagCsdRelationships();

    // *** operators ***
    bool operator==(const TagCsdRelationships &o) const;
    bool operator!=(const TagCsdRelationships &o) const;
    TagCsdRelationships& operator=(const TagCsdRelationships&o) ;

    // *** Methods ***

    // query

    /**
     * @brief Check if the tag id "tagId" has at least one link to a csd id.
     * @param tagId The tag id. Must not be null otherwise False is returned.
     * @return True if it has, false otherwise.
     */
    bool tagHasRelationships(const QUuid tagId) const;

    /**
     * @brief Check if the csd id "csdId" has at least one link to a tag id.
     * @param csdId The csd id. Must not be null otherwise False is returned.
     * @return True if it has, false otherwise.
     */
    bool csdHasRelationships(const QUuid csdId) const;

    /**
     * @brief Check if there is a link between the tag id "tagId" and the csd id "csdId".
     * @param tagId The tag id. Must not be null otherwise False is returned.
     * @param csdId The csd id. Must not be null otherwise False is returned.
     * @return True if the link exists, false otherwise.
     */
    bool relationshipExists(const QUuid tagId, const QUuid csdId) const;

    /**
     * @brief For a given tag id, get the list of all the csd ids it is linked to. Return an empty
     * list if tagId has no relationship.
     * @param tagId The tag id. Must not be null otherwise empty set is returned.
     * @return The list of all the csd ids.
     */
    QSet<QUuid> getRelationshipsForTag(const QUuid tagId) const;

    /**
     * @brief For a given csd id, get the list of all the tag ids it is linked to. Return an empty
     * list if csdId has no relationship.
     * @param csdId The csd id. Must not be null otherwise empty set is returned.
     * @return The list of all the tag ids.
     */
    QSet<QUuid> getRelationshipsForCsd(const QUuid csdId) const;

    /**
     * @brief Get the list of all the tag ids that have at least one link to a csd.
     * @return List of all the tag ids.
     */
    QList<QUuid> getAllTagsWithRelationships() const;

    /**
     * @brief Get the list of all the csd ids that have at least one link to a tag.
     * @return List of all the csd ids.
     */
    QList<QUuid> getAllCsdsWithRelationships() const;

    /**
     * @brief Get total no of relationships defined, that is of pair (tag id, csd id).
     * @return Total relationships.
     */
    uint noOfRelationships() const;

    // add, delete

    /**
     * @brief Create a relationship between a specific tag id and csd id.
     * If the relationship already exists, nothing occurs. Cannot create over MAX_NO_RELATIONSHIPS
     * relationships.
     * @details Tag id must be different from csd id.
     * @param tagId The tag id. Must not be null otherwise nothing is changed.
     * @param csdId The csd id. Must not be null otherwise nothing is changed.
     */
    void addRelationship(const QUuid tagId, const QUuid csdId);

    /**
     * @brief Delete a relationship between specific tag id and csd id.
     * If the relationship does not exists, nothing occurs.
     * @param tagId The tag id. Must not be null otherwise nothing is changed.
     * @param csdId The csd id. Must not be null otherwise nothing is changed.
     */
    void deleteRelationship(const QUuid tagId, const QUuid csdId);

    /**
     * @brief Delete all the relationships related to a specific tag id. If this tag id
     * does not exist, nothing happen.
     * @param tagId The Tag id for which all relationships must be deleted. Must not be
     * null otherwise nothing is changed.
     */
    void deleteRelationshipsForTag(const QUuid tagId);

    /**
     * @brief Delete all the relationships related to a specific csd id. If this csd id
     * does not exist, nothing happen.
     * @param csdId The sdc id for which all relationships must be deleted. Must not be
     * null otherwise nothing is changed.
     */
    void deleteRelationshipsForCsd(const QUuid csdId);

    /**
     * @brief Delete all the relationships.
     */
    void clear();

    // clone

    /**
     * @brief For a given tag id "sourceTagId", copy all its Csd relationships to
     * another tag id "destTagId".
     * @details
     * => If sourceTagId or destTagId is NULL (that is invalid), nothing happens.
     * => If sourceTagId == destTagId, nothing happens.
     * => If sourceTagId has no relationship
     *     All relationships of destTagId are destroyed.
     * => If sourceTagId has relationships,
     *     If destTagId already has relationships
     *         They are first destroyed, then the relationships of sourceTagId are copied.
     *     If destTagId does not have relationships initially
     *         The relationships of sourceTagId are copied to destTagId
     * @param sourceTagId The source Tag ID for which Csd relationships must be cloned.
     * @param destTagId The destination Tag ID where all Csd relationships will be copied.
     */
    void cloneCsdRelationshipsForTag(QUuid sourceTagId, QUuid destTagId);

    /**
     * @brief For a given csd id "sourceCsdId", copy all its Tag relationships to
     * another tag id "destCsdId".
     * @details
     * => If sourceCsdId or destCsdId is NULL (that is invalid), nothing happens.
     * => If sourceCsdId == destCsdId, nothing happens.
     * => If sourceCsd has no relationship
     *     All relationships of destCsd are destroyed.
     * => If sourceCsd has relationships,
     *     If destCsdId already has relationships
     *         They are first destroyed, then the relationships of sourceCsdId are copied.
     *     If destCsdId does not have relationships initially
     *         The relationships of sourceCsdId are copied to destTagId
     * @param sourceCsdId The source Csd ID for which Tag relationships must be cloned.
     * @param destCsdId The destination Csd ID where all Tag relationships will be copied.
     */
    void cloneTagRelationshipsForCsd(QUuid sourceCsdId, QUuid destCsdId);

    // Json stuff

    /**
     * @brief Convert this object to a JSon object.
     * @details This implementation is 100% compatible with JSON file produced by
     * GBP version 1.6.3 and before, where private data of TagCsdRelationship was differently
     * engineered. It produces exactly the same JSON.
     * @return The JSon object.
     */
    QJsonObject toJson() const;

    /**
     * @brief Return a new object of this class by decoding a JSon object representation.
     * @details This implementation is 100% compatible with JSON file produced by
     * version 1.6.3 and before, where private data of TagCsdRelationship was differently
     * engineered. It reads the same JSON.
     * @param o The JSon object representation.
     * @param result Indicates if the operation was a success or not.
     * @return The new TagCsdRelationships object built.
     */
    static TagCsdRelationships fromJson(const QJsonObject& o, Util::ResultOfOperation &result);

    // Integrity checking

    /**
     * @brief Check the integrity of the internal data.
     * @details We need to ensure the bidirectional consistency of the mappings. The class maintains
     * a bidirectional relationship where tagLinks[tagId].contains(csdId) if and only if
     * csdLinks[csdId].contains(tagId). Additionally, since the addRelationship method prevents
     * self-links (tagId == csdId), we must verify that no self-links exist in either map.
     * The integrity check should also confirm that empty sets are absent, as the class uses
     * isEmpty() checks to remove them in misc methods.
     * @return A report indicating if problems have been detected and what they are.
     */
    IntegrityReport checkIntegrity() const;

private:

    // We implement bidirectional lookup, to maximize retrieval/search speed (at the cost of higher
    // internal complexity and slower addition/removal)

    /**
     * @brief tagLinks List of tag ids, with, for each, the list of linked csd ids.
     * @details Key = tag id, value = list of associated csd keys. For a given key,
     * the associated QSet is never empty.
     */
    QHash<QUuid,QSet<QUuid>> tagLinks;

    /**
     * @brief csdLinks List of csd ids, with, for each, the list of linked tag ids.
     * @details Key = csd id, value = list of associated tag keys. For a given key,
     * the associated QSet is never empty.
     */
    QHash<QUuid,QSet<QUuid>> csdLinks;


};

#endif // TAGFSDRELATIONSHIPS_H
