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


#include "tag.h"

// We try to limit the length, because tags are just labels used to sort things out.
// For display prupose, it is cumbersome if they are too long
uint Tag::MAX_NAME_LEN = 50;
// Max Description Length
uint Tag::MAX_DESC_LEN = 1000;


Tag::Tag() {
    this->id = QUuid::createUuid();
    this->name = "";
    this->description = "";
}


Tag::Tag(const Tag& o) {
    this->id = o.id;
    this->name = o.name;
    this->description = o.description;
}


Tag::Tag(QString aName, QString aDescription) : Tag(QUuid::createUuid(),aName, aDescription) {
}



Tag::Tag(QUuid anId, QString aName, QString aDescription) {
    if(anId.isNull()){
        throw std::domain_error("QUuid is invalid");
    }
    this->id = anId;
    // check if name is too long
    if( aName.length() > Tag::MAX_NAME_LEN){
        throw std::domain_error("Name is too long");
    }
    this->name = aName;
    // check if description is too long before assignment
    if( aDescription.length() > Tag::MAX_DESC_LEN){
        throw std::domain_error("Description is too long");
    }
    this->description = aDescription;
}


Tag::~Tag()
{
}


bool Tag::operator==(const Tag &o) const
{
    if ( (this->id!=o.id) || (this->name!=o.name) || (this->description!=o.description) ) {
        return false;
    }
    return true;
}


bool Tag::operator!=(const Tag &o) const
{
    return !(*this==o);
}


Tag &Tag::operator=(const Tag &o)
{
    if (this != &o){                // to protect against self-assignment
        this->id = o.id;
        this->name = o.name;
        this->description = o.description;
    }
    return *this;
}


bool Tag::isNameIdentical(QString aName)
{
    return (this->name==aName);
}


QJsonObject Tag::toJson() const
{
    QJsonObject jsonObject;
    jsonObject["Id"] = id.toString(QUuid::WithoutBraces);
    jsonObject["Name"] = name;
    jsonObject["Description"] = description;
    return jsonObject;
}



// From JsonObject, get the data required to build later an object of this class
Tag Tag::fromJson(const QJsonObject &jsonObject, Util::OperationResult &result)
{
    QJsonValue jsonValue;
    int ok;
    double d;
    QUuid id;
    QString name;
    QString description;
    Tag tag;

    result.success = false;
    result.errorStringUI = "";
    result.errorStringLog = "";

    // ID
    jsonValue = jsonObject.value("Id");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("Cannot find Id tag");
        result.errorStringLog = QString("Cannot find Id tag");
        return tag;
    }
    if (jsonValue.isString()==false){
        result.errorStringUI = tr("Id tag is not a string");
        result.errorStringLog = QString("Id tag is not a string");
        return tag;
    }
    QString idString  = jsonValue.toString();
    if (idString.length()>100){
        // arbitray , because I am not sure this is 36 char on ANY platform & Qt version
        result.errorStringUI = tr("Id is too long");
        result.errorStringLog = QString("Id is too long");
        return tag;
    }
    id = QUuid::fromString(idString);
    if (id.isNull()){
        result.errorStringUI = tr("Id is not a valid UUID");
        result.errorStringLog = QString("Id is not a valid UUID");
        return tag;
    }
    // Name
    jsonValue = jsonObject.value("Name");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("Cannot find Name tag");
        result.errorStringLog = QString("Cannot find Name tag");
        return tag;
    }
    if (jsonValue.isString()==false){
        result.errorStringUI = tr("Name tag is not a string");
        result.errorStringLog = QString("Name tag is not a string");
        return tag;
    }
    name = jsonValue.toString();
    if (name.length()>MAX_NAME_LEN){
        result.errorStringUI = tr("Name is too long : max length is %1, found %2")
            .arg(MAX_NAME_LEN).arg(name.length());
        result.errorStringLog = QString("Name is too long : max length is %1, found %2")
            .arg(MAX_NAME_LEN).arg(name.length());
        return tag;
    }
    // Description
    jsonValue = jsonObject.value("Description");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("Cannot find Description tag");
        result.errorStringLog = QString("Cannot find Description tag");
        return tag;
    }
    if (jsonValue.isString()==false){
        result.errorStringUI = tr("Description tag is not a string");
        result.errorStringLog = QString("Description tag is not a string");
        return tag;
    }
    description = jsonValue.toString();
    if (description.length()>MAX_DESC_LEN){
        result.errorStringUI = tr("Description is too long : max length is %1, found %2")
        .arg(MAX_DESC_LEN).arg(description.length());
        result.errorStringLog = QString("Description is too long : max length is %1, found %2")
                                    .arg(MAX_DESC_LEN).arg(description.length());
        return tag;
    }

    tag = Tag(id, name, description);

    result.success = true;
    result.errorStringUI = "";
    result.errorStringLog = "";

    return tag;
}


// Getters - Setters

QUuid Tag::getId() const
{
    return id;
}

void Tag::setId(const QUuid &newId)
{
    id = newId;
}

QString Tag::getName() const
{
    return name;
}

void Tag::setName(const QString &newName)
{
    name = newName;
}

QString Tag::getDescription() const
{
    return description;
}

void Tag::setDescription(const QString &newDescription)
{
    description = newDescription;
}



// This is a global function. Create a Hash value for Tag, required to use QSet with key = Tag.
// Id is enough to guarantee uniqueness.
// Qt doc says : "there must also be a GLOBAL qHash()
// function that returns a hash value for an argument of the key's type."
size_t  qHash(const Tag &t, size_t seed ) {
    //return qHashMulti(seed, t.getId(), t.getName());
    return qHash(t.getId(), seed);
}

