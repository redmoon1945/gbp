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

#ifndef TAG_H
#define TAG_H

#include <QJsonObject>
#include <QUuid>
#include <QCoreApplication>
#include "util.h"


/**
 * @class Tag
 * @brief Define a Tag, which is a phrase that can be linked (which means associated) to
 * Cash Stream Definitions, in order to enable searching or categorizing.
 * @details Tag is a powerful way to implement categories of incomes and expenses. Each scenario has
 * its own set of tags. A particular tag can be linked to zero, one or many Cash Stream Definitions.
 * Likewise, a particular Cash Stream Definition can be linked to zero, one or many tags.
 * 2 tags may have the same name, but respective ids is absolutely unique.
 */
class Tag
{

    Q_DECLARE_TR_FUNCTIONS(Tag)

public:

    /**
     * @brief Max name length of a tag.
     * @details We try to limit the name length, because tags are just labels used
     * to sort things out. For display purpose, it is cumbersome if they are too long.
     */
    static constexpr uint MAX_NAME_LEN = 50;

    /**
     *  @brief Max Description length of a tag.
     */
    static constexpr uint MAX_DESC_LEN = 1000;

    // *** Constructors/destructors ***

    /**
     * @brief Default constructor. A unique ID will be automatically generated.
     */
    Tag();

    /**
     * @brief Copy constructor.
     * @param o The object to copy from.
     */
    Tag(const Tag& o);

    /**
     * @brief Simplified constructor. A unique ID will be automatically generated.
     * @param aName Name if the tag (trimmed and then cut to MAX_NAME_LEN max char).
     * @param aDescription Description of the tag (trimmed and then cut to MAX_DESC_LEN max char).
     */
    Tag(QString aName, QString aDescription="");

    /**
     * @brief Full detailed constructor.
     * @param anId The ID of the tag. Must be a valid QUuid.
     * @param aName Name of the tag (trimmed and then cut to MAX_NAME_LEN max char).
     * @param aDescription Description of the tag (trimmed and then cut to MAX_DESC_LEN max char).
     */
    Tag(QUuid anId, QString aName, QString aDescription="");


    virtual ~Tag();


    // *** operators ***
    bool operator==(const Tag &o) const;
    bool operator!=(const Tag &o) const;
    Tag& operator=(const Tag&o) ;

    // *** Methods ***

    /**
     * @brief Does the name of this object equals aName.
     * @param aName The name to compare to.
     * @return true if the name of this object equals aName, false otherwise.
     */
    bool isNameIdentical(QString aName);

    /**
     * @brief toJson Convert this object into a JSON object.
     * @return The JSON object representing this object.
     */
    QJsonObject toJson() const;

    /**
     * @brief From a JsonObject, build an object of this class.
     * @param jsonObject the source JSON object.
     * @param result Indicates if the operation succeeded. Only log message is provided.
     * @return The new Tag object build from jsonObject.
     */
    static Tag fromJson(const QJsonObject &jsonObject, Util::ResultOfOperation &result);

    QUuid getId() const;
    void setId(const QUuid &newId);
    QString getName() const;
    void setName(const QString &newName);
    QString getDescription() const;
    void setDescription(const QString &newDescription);

private:
    QUuid id;       ///< Unique ID of this tag.
    /**
     * @brief Name of this tag, not necessarly unique. Can be anything, including empty string.
     */
    QString name;
    QString description; ///< Description of this tag. Can be empty.
};


/**
 * @brief Hash function for cases where Tag is used as a key for QSet or QHash.
 * @details This is a global function. Create a Hash value for Tag, required to use QSet with
 * key = Tag. Id is enough to guarantee uniqueness. Qt doc says : "there must also be a
 * GLOBAL qHash() function that returns a hash value for an argument of the key's type.".
 * @param t The tag to hash.
 * @param seed The seed for hashing (0 by default).
 * @return The hash produced.
 */
size_t  qHash(const Tag &t, size_t seed = 0);


#endif // TAG_H
