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


#include "tagcsdrelationships.h"
#include <QMutableListIterator>
#include <QCryptographicHash>
#include <QMap>
#include <QHash>
#include <QJsonArray>


quint16 TagCsdRelationships::MAX_NO_RELATIONSHIPS = 10000; // 200 tags x 500 csds


TagCsdRelationships::TagCsdRelationships() {
    store = QSet<TagCsdRelationship>();
}


TagCsdRelationships::TagCsdRelationships(const TagCsdRelationships &o)
{
    this->store = o.store;
}


TagCsdRelationships::~TagCsdRelationships()
{
}


bool TagCsdRelationships::operator==(const TagCsdRelationships &o) const
{
    if ( this->store!=o.store ) {
        return false;
    }
    return true;
}


bool TagCsdRelationships::operator!=(const TagCsdRelationships &o) const
{
    return !(*this==o);
}


TagCsdRelationships &TagCsdRelationships::operator=(const TagCsdRelationships &o)
{
    if (this != &o){                // to protect against self-assignment
        this->store = o.store;
    }
    return *this;
}


// Return true if tagId has at least one relationship with a csdId
bool TagCsdRelationships::tagHasRelationships(const QUuid tagId)
{
    for (QSet<TagCsdRelationship>::const_iterator it = store.constBegin();
        it != store.constEnd(); ++it) {
        QUuid aTagId = it->tagId;
        if(aTagId==tagId){
            return true;
        }
    }
    return false;
}


bool TagCsdRelationships::csdHasRelationships(const QUuid csdId)
{
    for (QSet<TagCsdRelationship>::const_iterator it = store.constBegin();
        it != store.constEnd(); ++it) {
        if(it->csdId==csdId){
            return true;
        }
    }
    return false;
}


bool TagCsdRelationships::relationshipExists(const QUuid tagId, const QUuid csdId)
{
    return store.contains({.tagId=tagId, .csdId=csdId});
}


// Return all the csdId that have a relationship with that tagId
QSet<QUuid> TagCsdRelationships::getRelationshipsForTag(const QUuid tagId)
{
    QSet<QUuid> result = {};
    for (QSet<TagCsdRelationship>::const_iterator it = store.constBegin();
        it != store.constEnd(); ++it) {
        if( it->tagId==tagId ){
            result.insert(it->csdId); // remove duplicate
        }
    }
    return result;
}


// Return all the tagId that have a relationship with that csdId
QSet<QUuid> TagCsdRelationships::getRelationshipsForCsd(const QUuid csdId)
{
    QSet<QUuid> result = {};
    for (QSet<TagCsdRelationship>::const_iterator it = store.constBegin(); it != store.constEnd();
        ++it) {
        if( it->csdId==csdId ){
            result.insert(it->tagId); // remove duplicate
        }
    }
    return result;
}


// Return all the tagIds that have one or more relationsship with any csdId
QSet<QUuid> TagCsdRelationships::getAllTagsWithRelationships()
{
    QSet<QUuid> result = {};
    for (QSet<TagCsdRelationship>::const_iterator it = store.constBegin(); it != store.constEnd();
        ++it) {
         result.insert(it->tagId) ; // remove duplicate
    }
    return result;
}


// Return all the csdIds that have one or more relationsship with any tagId
QSet<QUuid> TagCsdRelationships::getAllCsdsWithRelationships()
{
    QSet<QUuid> result = {};
    for (QSet<TagCsdRelationship>::const_iterator it = store.constBegin(); it != store.constEnd();
        ++it) {
        result.insert(it->csdId) ; // remove duplicate
    }
    return result;
}


uint TagCsdRelationships::noOfRelationships()
{
    return store.size();
}


void TagCsdRelationships::addRelationship(const QUuid tagId, const QUuid csdId)
{
    if (store.size() >= MAX_NO_RELATIONSHIPS) {
        return;
    }
    store.insert({.tagId=tagId, .csdId=csdId}); // add only if not already there
}


// Returns true if an item was actually removed; otherwise returns false.
bool TagCsdRelationships::deleteRelationship(const QUuid tagId, const QUuid csdId)
{
    return store.remove({.tagId=tagId, .csdId=csdId});
}


// Delete all the relationships involving this specific tagId
void TagCsdRelationships::deleteRelationshipsForTag(const QUuid tagId)
{
    QList<TagCsdRelationship> toDelete; // List to hold entries to delete
    // Iterate through the QSet
    for (const TagCsdRelationship &e : store) {
        if (e.tagId==tagId) {
            toDelete.append(e); // Mark for deletion
        }
    }
    // Now remove the marked entries
    for (const TagCsdRelationship &entry : toDelete) {
        store.remove(entry);
    }
}


// Delete all the relationships involving this specific csdId
void TagCsdRelationships::deleteRelationshipsForCsd(const QUuid csdId)
{
    QList<TagCsdRelationship> toDelete; // List to hold entries to delete
    // Iterate through the QSet
    for (const TagCsdRelationship &e : store) {
        if (e.csdId==csdId) {
            toDelete.append(e); // Mark for deletion
        }
    }
    // Now remove the marked entries
    for (const TagCsdRelationship &entry : toDelete) {
        store.remove(entry);
    }
}


// Delete all relationships
void TagCsdRelationships::clear()
{
    store.clear();
}


// For a given sourceTag, copy all its Csd relationships to destTagId. If sourceTag has no
// relationship, the method does nothing. All existing Csd relationships of destTagId are
// first deleted.
void TagCsdRelationships::cloneCsdRelationshipsForTag(QUuid sourceTagId, QUuid destTagId)
{
    if ( false == tagHasRelationships(sourceTagId)){
        return;
    }
    deleteRelationshipsForTag(destTagId);
    QSet<QUuid> oldList = getRelationshipsForTag(sourceTagId);
    foreach(QUuid csdId, oldList) {
        store.insert({destTagId,csdId});
    }
}


// For a given sourceCsd, copy all its Tag relationships to destCsdId. If sourceCsd has no
// relationship, the method does nothing. All existing Tag relationships of destCsdId are
// first deleted.
void TagCsdRelationships::cloneTagRelationshipsForCsd(QUuid sourceCsdId, QUuid destCsdId)
{
    if ( false == csdHasRelationships(sourceCsdId)){
        return;
    }
    deleteRelationshipsForCsd(destCsdId);
    QSet<QUuid> oldList = getRelationshipsForCsd(sourceCsdId);
    foreach(QUuid tagId, oldList) {
        store.insert({tagId, destCsdId});
    }
}


QJsonObject TagCsdRelationships::toJson() const
{
    QJsonArray jsonArray;
    for (const TagCsdRelationship& entry : store) {
        QJsonObject entryObject;
        entryObject["TagId"] = entry.tagId.toString(QUuid::WithoutBraces);
        entryObject["CsdId"] = entry.csdId.toString(QUuid::WithoutBraces);
        jsonArray.append(entryObject);
    }
    QJsonObject jobject;
    jobject["Set"] = jsonArray;
    return jobject;
}


TagCsdRelationships TagCsdRelationships::fromJson(const QJsonObject &o,
    Util::OperationResult &result)
{
    QJsonValue jsonValue;
    result.success = false;
    result.errorStringUI = "";
    result.errorStringLog = "";
    TagCsdRelationships r;
    QUuid tagId;
    QUuid csdId;
    int ok;

    jsonValue = o.value("Set");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("TagRelationship - Cannot find Set tag");
        result.errorStringLog = "TagRelationship - Cannot find Set tag";
        return r;
    }
    if (jsonValue.isArray()==false){
        result.errorStringUI = tr("TagRelationship - Set tag is not an array");
        result.errorStringLog = "TagRelationship - Set tag is not an array";
        return r;
    }
    QJsonArray arr = o["Set"].toArray();

    if (arr.size() > MAX_NO_RELATIONSHIPS) {
        result.errorStringUI = tr("TagRelationship - Too many entries");
        result.errorStringLog = "TagRelationship - Too many entries";
        return r;
    }

    for (int i = 0; i < arr.size(); ++i) {
        QJsonValue value = arr.at(i);
        if (value.isObject()) {
            QJsonObject jsonObject = value.toObject();
            TagCsdRelationship entry;

            // Tag ID
            bool found = jsonObject.contains("TagId");
            if (found==false){
                result.errorStringUI = tr("TagRelationship - TagId not found");
                result.errorStringLog = "TagRelationship - TagId not found";
                return r;
            }
            QJsonValue tagIdValue = jsonObject["TagId"];
            if (tagIdValue.isString()) {
                QUuid uuid (tagIdValue.toString());
                if (uuid.isNull()) {
                    // invalid UUID
                    result.errorStringUI = tr("TagRelationship - TagId is not a valid UUID");
                    result.errorStringLog = "TagRelationship - TagId is not a valid UUID";
                    return r;
                } else {
                    // all is well
                    tagId = uuid;
                }
            } else {
                // Handle the case where TagId is not a string
                result.errorStringUI = tr("TagRelationship - TagId is not a string");
                result.errorStringLog = "TagRelationship - TagId is not a string";
                return r;
            }

            // Csd ID
            found = jsonObject.contains("CsdId");
            if (found==false){
                result.errorStringUI = tr("TagRelationship - CsdId not found");
                result.errorStringLog = "TagRelationship - CsdId not found";
                return r;
            }
            QJsonValue csdIdValue = jsonObject["CsdId"];
            if (csdIdValue.isString()) {
                QUuid uuid(csdIdValue.toString());
                if (uuid.isNull()) {
                    // invalid UUID
                    result.errorStringUI = tr("TagRelationship - CsdId is not a valid UUID");
                    result.errorStringLog = "TagRelationship - CsdId is not a valid UUID";
                    return r;
                } else {
                    // all is well
                    csdId = uuid;
                }
            } else {
                // Handle the case where csdId is not a string
                result.errorStringUI = tr("TagRelationship - CsdId is not a string");
                result.errorStringLog = "TagRelationship - CsdId is not a string";
                return r;
            }

            r.addRelationship(tagId, csdId);
        }
    }

    result.success = true;
    return r;
}


// Qt doc says about the key : the type must provide operator==()
bool TagCsdRelationship::operator==(const TagCsdRelationship &o) const
{
    if( (tagId != o.tagId) ||
        (csdId != o.csdId) ){
        return false;
    }
    return true;
}


bool TagCsdRelationship::operator!=(const TagCsdRelationship &o) const
{
    return !(*this==o);
}


TagCsdRelationship &TagCsdRelationship::operator=(const TagCsdRelationship &o)
{
    this->tagId = o.tagId;
    this->csdId = o.csdId;
    return *this;
}


// Create a Hash value for TagCsdRelationships::Entry, required to use QSet with
// key = TagCsdRelationships::Entry
// Qt doc says : "there must also be a GLOBAL qHash()
// function that returns a hash value for an argument of the key's type."
size_t  qHash(const TagCsdRelationship &entry, size_t seed = 0) {
    return qHashMulti(seed, entry.tagId, entry.csdId);
}
