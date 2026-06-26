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

#include "tags.h"
#include <QJsonArray>
#include <QSet>



Tags::Tags() {
    tags.clear();
}


Tags::Tags(const Tags &o)
{
    this->tags = o.tags;
}


Tags::~Tags()
{
}


bool Tags::operator==(const Tags &o) const
{
    if ( this->tags != o.tags ) {
        return false;
    }
    return true;
}


bool Tags::operator!=(const Tags &o) const
{
    return !(*this==o);
}


Tags &Tags::operator=(const Tags &o)
{
    if (this != &o){                // to protect against self-assignment
        this->tags = o.tags;
    }
    return *this;
}


bool Tags::insert(const Tag &t)
{
    // Have we reached the max no of tags ?
    if (tags.count()>= MAX_NO_TAGS) {
        return false;
    }

    // Doc : If there is already an item with the key, that item's value is replaced with value.
    tags.insert(t.getId(), t);

    return true;
}


bool Tags::remove(QUuid tagId)
{
    if (tagId.isNull()==true) {
        return false;
    }
    return tags.remove(tagId);
}


void Tags::clear()
{
    tags.clear();
}


bool Tags::containsTagId(QUuid tagId) const
{
    if (tagId.isNull()==true) {
        return false;
    }
    return tags.contains(tagId);
}


bool Tags::cleanIdList(QSet<QUuid> &idList) const
{
    bool changed = false;

    // In idList, remove tags that do not exist in this Object
    QSet<QUuid> iterationCopySet = idList;
    foreach(QUuid tagId, iterationCopySet){
        if (tagId.isNull()==true) {
            continue;
        }
        if (tags.contains(tagId)==false) {
            idList.remove(tagId);
            changed = true;
        }
    }

    return changed;
}


quint16 Tags::containsTagName(const QString &name) const
{
    quint16 no=0;
    QHash<QUuid, Tag>::const_iterator it = tags.begin();
    while (it != tags.end()) {
        Tag tag = it.value();
        if (tag.getName()==name) {
            no++;
        }
        ++it;
    }
    return no;
}


Tag Tags::getTag(QUuid tagId, bool &found) const
{
    Tag t;
    found = false;

    if (tagId.isNull()==true) {
        return t;
    }
    if ( true==tags.contains(tagId) ){
        found = true;
        return tags.value(tagId);
    } else {
        return t;
    }
}


QSet<Tag> Tags::getTags() const
{
    QList<Tag> list = tags.values();
    return QSet(list.begin(), list.end());
}


quint16 Tags::size() const
{
    return tags.size();
}


QList<QUuid> Tags::getTagIdSet()
{
    return tags.keys();
}


QSet<QUuid> Tags::getTagIdSetAsQset()
{
    QSet<QUuid> r;
    for (auto it = tags.constBegin(); it != tags.constEnd(); ++it) {
        const QUuid &key = it.key();
        r.insert(key);
    }
    return r;
}


QJsonObject Tags::toJson() const
{
    QJsonObject jsonObject;
    QJsonArray jsonArrayTags;

    QHash<QUuid, Tag>::const_iterator it = tags.begin();
    while (it != tags.end()) {
        Tag tag = it.value();
        QJsonObject tagJson = tag.toJson();
        jsonArrayTags.append(tagJson);
        ++it;
    }

    jsonObject["Set"] = jsonArrayTags;
    return jsonObject;
}


Tags Tags::fromJson(const QJsonObject &jsonObject, Util::ResultOfOperation &result)
{
    // Reset result to ERROR
    result.init();

    Tags tagSet;
    QJsonArray jsonArrayTags;
    QJsonValue jsonValue;

    jsonValue = jsonObject.value("Set");
    if (jsonValue == QJsonValue::Undefined){
        // If no Tags defined, this could be an old version of the scenario file
        // do nothing in that case
        return tagSet;
    }
    if (false==jsonValue.isArray()){
        result.logErrorMessage = QString("Tags: Set is not an array");
        return tagSet;
    }

    // Check if too many tags
    jsonArrayTags = jsonValue.toArray();
    if (jsonArrayTags.size() > Tags::MAX_NO_TAGS) {
        result.logErrorMessage = QString("Tags: Too many tags : max allowed is %1, %2 found")
            .arg(Tags::MAX_NO_TAGS).arg(jsonArrayTags.size());
        return tagSet;
    }

    for (int i = 0; i < jsonArrayTags.size(); ++i) {
        QJsonValue value = jsonArrayTags.at(i);
        if (value.isObject()) {
            QJsonObject jsonObject = value.toObject();
            // extract the tag
            Tag t = Tag::fromJson(jsonObject, result);
            if (result.status==Util::ResultOfOperationStatus::ERROR){
                tagSet.clear();
                result.logErrorMessage = "Tags->" + result.logErrorMessage;
                return tagSet;
            }
            // add the tag to the set
            tagSet.insert(t);
        } else {
            // Illegal, must be an object (a tag)
            result.init();
            result.logErrorMessage = "Tags: An element of the tag list is not a object";
            tagSet.clear();
            return tagSet;
        }
    }

    result.status = Util::ResultOfOperationStatus::SUCCESS;
    return tagSet;
}



