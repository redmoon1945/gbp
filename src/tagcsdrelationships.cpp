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
#include <QCryptographicHash>
#include <QJsonArray>


TagCsdRelationships::TagCsdRelationships() {
    tagLinks = QHash<QUuid,QSet<QUuid>>();
    csdLinks = QHash<QUuid,QSet<QUuid>>();
}


TagCsdRelationships::TagCsdRelationships(const TagCsdRelationships &o)
{
    this->tagLinks = o.tagLinks;
    this->csdLinks = o.csdLinks;
}


TagCsdRelationships::~TagCsdRelationships()
{
}


bool TagCsdRelationships::operator==(const TagCsdRelationships &o) const
{
    if ( (this->tagLinks!=o.tagLinks) || (this->csdLinks!=o.csdLinks) ) {
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
        this->tagLinks = o.tagLinks;
        this->csdLinks = o.csdLinks;
    }
    return *this;
}


bool TagCsdRelationships::tagHasRelationships(const QUuid tagId) const
{
    if (tagId.isNull()) {
        return false;
    }
    return tagLinks.contains(tagId);
}


bool TagCsdRelationships::csdHasRelationships(const QUuid csdId) const
{
    if (csdId.isNull()) {
        return false;
    }
    return csdLinks.contains(csdId);
}


bool TagCsdRelationships::relationshipExists(const QUuid tagId, const QUuid csdId) const
{
    if (tagId.isNull() || csdId.isNull()) {
        return false;
    }
    // Check just one side of the relationship
    QSet s = tagLinks.value(tagId,{});
    return s.contains(csdId);
}


QSet<QUuid> TagCsdRelationships::getRelationshipsForTag(const QUuid tagId) const
{
    if (tagId.isNull()) {
        return {};
    }
    return tagLinks.value(tagId,{});
}


QSet<QUuid> TagCsdRelationships::getRelationshipsForCsd(const QUuid csdId) const
{
    if (csdId.isNull()) {
        return {};
    }
    return csdLinks.value(csdId,{});
}


QList<QUuid> TagCsdRelationships::getAllTagsWithRelationships() const
{
    return tagLinks.keys();
}


QList<QUuid> TagCsdRelationships::getAllCsdsWithRelationships() const
{
    return csdLinks.keys();
}


uint TagCsdRelationships::noOfRelationships() const
{
    uint count = 0;
    for (const QSet<QUuid>& set : tagLinks) {
        count += set.size();
    }
    return count;
}


void TagCsdRelationships::addRelationship(const QUuid tagId, const QUuid csdId)
{
    if (tagId.isNull() || csdId.isNull()) {
        return;
    }

    // Prevent self-links
    if (tagId == csdId){
        return;
    }

    /*
     * In QHash<QUuid, QSet<QUuid>>, the operator[] behaves as follows:
     *  - If the key exists: tagLinks[tagId] returns a reference to the existing QSet<QUuid>
     *    associated with tagId.
     *  - If the key does not exist: tagLinks[tagId] creates a new, default-constructed
     *    QSet<QUuid> (which is empty), inserts it into the QHash with tagId as the key,
     *    and returns a reference to this new QSet<QUuid>.
     */
    tagLinks[tagId].insert(csdId);
    csdLinks[csdId].insert(tagId);
}


void TagCsdRelationships::deleteRelationship(const QUuid tagId, const QUuid csdId)
{
    if (tagId.isNull() || csdId.isNull()) {
        return;
    }

    QSet<QUuid>& refSetTag = tagLinks[tagId];// Unwanted empty set inserted if tagId do not exist.
    QSet<QUuid>& refSetCsd = csdLinks[csdId];// Unwanted empty set inserted if csdId do not exist.

    refSetTag.remove(csdId); // do nothing if csdId is not in the set
    refSetCsd.remove(tagId); // do nothing if tagId is not in the set

    // Remove entries if empty, because empty QSet is unwanted
    if (refSetTag.isEmpty()){
        tagLinks.remove(tagId);
    }
    if (refSetCsd.isEmpty()){
        csdLinks.remove(csdId);
    }
}


void TagCsdRelationships::deleteRelationshipsForTag(const QUuid tagId)
{
    if (tagId.isNull()) {
        return;
    }

    // Get all csds linked
    QSet<QUuid> linkedCsdIds = tagLinks.value(tagId,{});
    if (linkedCsdIds.size()==0) {
        return; // no relationship for that tagId
    }

    // Remove corresponding entries from csdLinks
    for (const QUuid& csdId : linkedCsdIds) {
        QSet<QUuid>& set = csdLinks[csdId];
        set.remove(tagId);
        if (set.isEmpty()) {
            csdLinks.remove(csdId);
        }
    }
    // Remove tagId from tagLinks
    tagLinks.remove(tagId);
}


void TagCsdRelationships::deleteRelationshipsForCsd(const QUuid csdId)
{
    if (csdId.isNull()) {
        return;
    }

    // Get all tags linked. Empty set if tag does not exist.
    QSet<QUuid> linkedTagIds = csdLinks.value(csdId,{});
    if (linkedTagIds.size()==0) {
        return; // no relationship for that csdId
    }

    // Remove corresponding entries from tagLinks
    for (const QUuid& tagId : linkedTagIds) {
        QSet<QUuid>& set = tagLinks[tagId];
        set.remove(csdId);
        if (set.isEmpty()) {
            tagLinks.remove(tagId);
        }
    }
    // Remove csdId from csdLinks
    csdLinks.remove(csdId);
}


void TagCsdRelationships::clear()
{
    tagLinks.clear();
    csdLinks.clear();
}


void TagCsdRelationships::cloneCsdRelationshipsForTag(QUuid sourceTagId, QUuid destTagId)
{
    if (sourceTagId.isNull() || destTagId.isNull()) {
        return;
    }

    // both id must be different (no self-copy)
    if (sourceTagId==destTagId) {
        return;
    }

    // Check if we have enough place to proceed with the duplication
    qsizetype destSize = getRelationshipsForTag(destTagId).size();
    qsizetype srcSize = getRelationshipsForTag(sourceTagId).size();
    if ( (noOfRelationships() - destSize + srcSize) > MAX_NO_RELATIONSHIPS) {
        return;
    }

    // First, delete all relationships for destTagId. If destCsdId does not exist, nothing happen.
    deleteRelationshipsForTag(destTagId);

    // Then check if sourceTagId has any relationship
    QSet<QUuid> linkedCsdIds = tagLinks.value(sourceTagId,{});
    if (linkedCsdIds.size()==0) {
        return;
    }

    // Finally, copy
    for (const QUuid& csdId : linkedCsdIds) {
        addRelationship(destTagId, csdId);
    }
}


void TagCsdRelationships::cloneTagRelationshipsForCsd(QUuid sourceCsdId, QUuid destCsdId)
{
    if (sourceCsdId.isNull() || destCsdId.isNull()) {
        return;
    }

    // both id must be different (no self-copy)
    if (sourceCsdId==destCsdId) {
        return;
    }

    // Check if we have enough place to proceed with the duplication
    qsizetype destSize = getRelationshipsForCsd(destCsdId).size();
    qsizetype srcSize = getRelationshipsForCsd(sourceCsdId).size();
    if ( (noOfRelationships() - destSize + srcSize) > MAX_NO_RELATIONSHIPS) {
        return;
    }

    // First, delete all relationships for destCsdId. If destCsdId does not exist, nothing happen.
    deleteRelationshipsForCsd(destCsdId);

    // Then check if sourceCsdId has any relationship
    QSet<QUuid> linkedTagIds = csdLinks.value(sourceCsdId,{});
    if (linkedTagIds.size()==0) {
        return;
    }

    // Finally, copy
    for (const QUuid& tagId : linkedTagIds) {
        addRelationship(tagId, destCsdId);
    }
}


QJsonObject TagCsdRelationships::toJson() const
{
    QJsonArray jsonArray;
    for (auto tagIt = tagLinks.constBegin(); tagIt != tagLinks.constEnd(); ++tagIt) {
        const QUuid& tagId = tagIt.key();
        const QSet<QUuid>& csdSet = tagIt.value();
        for (const QUuid& csdId : csdSet) {
            QJsonObject entryObject;
            entryObject["TagId"] = tagId.toString(QUuid::WithoutBraces);
            entryObject["CsdId"] = csdId.toString(QUuid::WithoutBraces);
            jsonArray.append(entryObject);
        }
    }
    QJsonObject jobject;
    jobject["Set"] = jsonArray;
    return jobject;
}


TagCsdRelationships TagCsdRelationships::fromJson(const QJsonObject &o,
    Util::ResultOfOperation &result)
{
    bool success;
    TagCsdRelationships r;

    // Reset result to ERROR
    result.init();

    // Check if "Set" key exists
    QJsonValue jsonValue = o.value("Set");
    if (jsonValue == QJsonValue::Undefined) {
        result.logErrorMessage = "TagRelationship: Cannot find token \"Set\"";
        return r;
    }

    // Check if "Set" is an array
    if (!jsonValue.isArray()) {
        result.logErrorMessage = "TagRelationship: Set token is not an array";
        return r;
    }

    QJsonArray arr = jsonValue.toArray();

    // Check array size against MAX_NO_RELATIONSHIPS
    if (arr.size() > MAX_NO_RELATIONSHIPS) {
        result.logErrorMessage = QString("TagRelationship: Too many entries, found %2 but "
            "maximum is %3").arg(arr.size()).arg(MAX_NO_RELATIONSHIPS);
        return r;
    }

    // Iterate through the array
    for (int i = 0; i < arr.size(); ++i) {
        QJsonValue value = arr.at(i);
        if (!value.isObject()) {
            result.logErrorMessage = "TagRelationship: An array element is not an object";
            return r;
        }

        QJsonObject jsonObject = value.toObject();

        // Tag ID
        if (!jsonObject.contains("TagId")) {
            result.logErrorMessage = "TagRelationship: Token \"TagId\" not found";
            return r;
        }
        QJsonValue tagIdValue = jsonObject["TagId"];
        if (!tagIdValue.isString()) {
            result.logErrorMessage = "TagRelationship: TagId is not a string";
            return r;
        }
        QString idString = tagIdValue.toString();
        QUuid tagId  = Util::convertStringToQuuid(idString, success);
        if (success==false) {
            idString.truncate(38);
            result.logErrorMessage = QString("TagRelationship: TagId \"%1\" is not a valid UUID")
                .arg(idString);
            return r;
        }

        // Csd ID
        if (!jsonObject.contains("CsdId")) {
            result.logErrorMessage = "TagRelationship: Token \"CsdId\" not found";
            return r;
        }
        QJsonValue csdIdValue = jsonObject["CsdId"];
        if (!csdIdValue.isString()) {
            result.logErrorMessage = "TagRelationship: CsdId is not a string";
            return r;
        }
        idString = csdIdValue.toString();
        QUuid csdId  = Util::convertStringToQuuid(idString, success);
        if (success==false) {
            result.logErrorMessage = QString("TagRelationship: CsdId \"%1\" is not a valid UUID")
                .arg(idString);;
            return r;
        }

        // Both IDs must be different
        if (csdId==tagId) {
            result.logErrorMessage = QString("TagRelationship: CsdId %1 is the same as TagId")
                .arg(tagId.toString(QUuid::WithoutBraces));
            return r;
        }

        // Add to tagLinks and csdLinks
        r.tagLinks[tagId].insert(csdId);
        r.csdLinks[csdId].insert(tagId);
    }

    result.status = Util::ResultOfOperationStatus::SUCCESS;
    return r;
}


TagCsdRelationships::IntegrityReport TagCsdRelationships::checkIntegrity() const {

    IntegrityReport report{false, QString()};

    // Check tagLinks -> csdLinks consistency and self-links
    for (auto it = tagLinks.constBegin(); it != tagLinks.constEnd(); ++it) {
        const QUuid& tagId = it.key();
        const QSet<QUuid>& csdIds = it.value();

        // Check for empty sets
        if (csdIds.isEmpty()) {
            report.errorFound = true;
            report.problem += QString("Empty set found in tagLinks for tagId %1\n")
                .arg(tagId.toString());
            continue;
        }

        // Check for self-links
        if (csdIds.contains(tagId)) {
            report.errorFound = true;
            report.problem += QString("Self-link found in tagLinks for tagId %1\n")
                .arg(tagId.toString());
            continue;
        }

        // Verify each csdId in tagLinks[tagId] has tagId in csdLinks[csdId]
        for (const QUuid& csdId : csdIds) {
            if (!csdLinks.contains(csdId) || !csdLinks.value(csdId).contains(tagId)) {
                report.errorFound = true;
                report.problem += QString("tagId %1 links to csdId %2 but csdId does not link"
                "back to tagId in csdLinks\n").arg(tagId.toString(), csdId.toString());
            }
        }
    }

    // Check csdLinks -> tagLinks consistency and self-links
    for (auto it = csdLinks.constBegin(); it != csdLinks.constEnd(); ++it) {
        const QUuid& csdId = it.key();
        const QSet<QUuid>& tagIds = it.value();

        // Check for empty sets
        if (tagIds.isEmpty()) {
            report.errorFound = true;
            report.problem += QString("Empty set found in csdLinks for csdId %1\n")
                .arg(csdId.toString());
            continue;
        }

        // Check for self-links
        if (tagIds.contains(csdId)) {
            report.errorFound = true;
            report.problem += QString("Self-link found in csdLinks for csdId %1\n")
                .arg(csdId.toString());
            continue;
        }

        // Verify each tagId in csdLinks[csdId] has csdId in tagLinks[tagId]
        for (const QUuid& tagId : tagIds) {
            if (!tagLinks.contains(tagId) || !tagLinks.value(tagId).contains(csdId)) {
                report.errorFound = true;
                report.problem += QString("csdId %1 links to tagId %2 but tagId does not link"
                " back to csdId in tagLinks\n").arg(csdId.toString(), tagId.toString());
            }
        }
    }

    return report;
}


