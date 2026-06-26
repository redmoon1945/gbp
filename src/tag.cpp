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


#include "tag.h"


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


Tag::Tag(const QString &aName, const QString &aDescription)
    : Tag(QUuid::createUuid(), aName, aDescription) {
}


Tag::Tag(QUuid anId, const QString &aName, const QString &aDescription) {
    if(anId.isNull()){
        throw std::domain_error("QUuid is invalid");
    }
    this->id = anId;;
    this->name = aName.trimmed().left(Tag::MAX_NAME_LEN);
    this->description = aDescription.trimmed().left(Tag::MAX_DESC_LEN);
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


bool Tag::isNameIdentical(const QString &aName)
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


Tag Tag::fromJson(const QJsonObject &jsonObject, Util::ResultOfOperation &result)
{
    QJsonValue jsonValue;
    int ok;
    double d;
    QUuid id;
    QString name;
    QString description;
    Tag tag;
    bool success;

    // Reset result to ERROR
    result.init();

    // ID
    jsonValue = jsonObject.value("Id");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = QString("Tag: Cannot find token \"Id\"");
        return tag;
    }
    if (jsonValue.isString()==false){
        result.logErrorMessage = QString("Tag: Id is not a string");
        return tag;
    }
    QString idString  = jsonValue.toString();
    id = Util::convertStringToQuuid(idString, success);
    if (success==false){
        idString.truncate(38);
        result.logErrorMessage = QString("Tag : Id \"%1\" is not a valid UUID").arg(idString);
        return tag;
    }
    // Name
    jsonValue = jsonObject.value("Name");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = QString("Tag %1: Cannot find token \"Name\"").arg(idString);
        return tag;
    }
    if (jsonValue.isString()==false){
        result.logErrorMessage = QString("Tag %1: Name token is not a string").arg(idString);
        return tag;
    }
    name = jsonValue.toString();
    if (name.length()>MAX_NAME_LEN){
        result.logErrorMessage = QString("Tag %1: Name is too long, max length is %2 but"
            " found %3")
            .arg(idString).arg(MAX_NAME_LEN).arg(name.length());
        return tag;
    }
    // Description
    jsonValue = jsonObject.value("Description");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = QString("Tag %1: Cannot find token \"Description\"")
            .arg(idString);
        return tag;
    }
    if (jsonValue.isString()==false){
        result.logErrorMessage = QString("Tag %1: Description tag is not a string")
            .arg(idString);
        return tag;
    }
    description = jsonValue.toString();
    if (description.length()>MAX_DESC_LEN){
        result.logErrorMessage = QString("Tag %1: Description is too long, max length is"
            " %2 but found %3")
            .arg(idString).arg(MAX_DESC_LEN).arg(description.length());
        return tag;
    }

    tag = Tag(id, name, description);

    result.status = Util::ResultOfOperationStatus::SUCCESS;

    return tag;
}



QUuid Tag::getId() const
{
    return id;
}


void Tag::setId(const QUuid &newId)
{
    if (newId.isNull()) {
        throw std::domain_error("QUuid is invalid");
    }
    id = newId;
}


QString Tag::getName() const
{
    return name;
}


void Tag::setName(const QString &newName)
{
    name = newName.trimmed().left(Tag::MAX_NAME_LEN);
}


QString Tag::getDescription() const
{
    return description;
}


void Tag::setDescription(const QString &newDescription)
{
    description = newDescription.trimmed().left(Tag::MAX_DESC_LEN);
}


size_t  qHash(const Tag &t, size_t seed ) {
    //return qHashMulti(seed, t.getId(), t.getName());
    return qHash(t.getId(), seed);
}

