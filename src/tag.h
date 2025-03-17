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


// A Tag is a set of words, like "Health Care", that can be linked (which means associated) to
// Cash Stream Definitions (a.k.a. incomes/expenses), in order to enable searching or browsing.
// It is also a powerful way to implement categories of incomes and expenses. Each scenario has
// its own set of tags. A particular Tag can be linked to zero, one or many Cash Stream Definitions.
// Likewise, a particular Cash Stream Definition can be linked to zero, one or many tags.
//
// 2 tags may have the same name, but id is absolutely unique.
class Tag
{
       Q_DECLARE_TR_FUNCTIONS(Tag)
public:
    static uint MAX_NAME_LEN; // max length of a name
    static uint MAX_DESC_LEN; // max length of a description

    // Constructors/destructors
    Tag();
    Tag(const Tag& o);
    Tag(QString aName, QString aDescription="");
    Tag(QUuid anId, QString aName, QString aDescription="");
    virtual ~Tag();


    // operators
    bool operator==(const Tag &o) const;
    bool operator!=(const Tag &o) const;
    Tag& operator=(const Tag&o) ;

    // Methods
    bool isNameIdentical(QString aName);
    QJsonObject toJson() const;
    static Tag fromJson(const QJsonObject &jsonObject, Util::OperationResult &result);

    // getters/setters
    QUuid getId() const;
    void setId(const QUuid &newId);
    QString getName() const;
    void setName(const QString &newName);
    QString getDescription() const;
    void setDescription(const QString &newDescription);

private:
    QUuid id;       // unique
    QString name;   // can be anything, including empty string
    QString description;
};



// Hash function for cases where Tag is used as a key for QSet or QHash.
size_t  qHash(const Tag &t, size_t seed = 0);


#endif // TAG_H
