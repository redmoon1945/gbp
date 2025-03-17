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

#include "tags.h"
#include <QJsonArray>
#include <QSet>


quint16 Tags::MAX_NO_TAGS = 5000; // seems more than reasonable amount...


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


// Add or replace a tag to the set. If this is a new tag (not present in the set), it will be added,
// unless the max no of tags has been reached, in which case the method returns False (true
// otherwise). If there is a tag that has the same ID, its value ("Tag") will be replaced by t.
bool Tags::insert(Tag t)
{
    // Have we already reached the max no of tags ?
    if (tags.count()>= MAX_NO_TAGS) {
        return false;
    }

    // Doc : If there is already an item with the key, that item's value is replaced with value.
    tags.insert(t.getId(), t);

    return true;
}


// Return true if an item was actually removed, false otherwise
bool Tags::remove(Tag t)
{
    return tags.remove(t.getId());
}


bool Tags::remove(QUuid tagId)
{
    return tags.remove(tagId);
}


void Tags::clear()
{
    tags.clear();
}


bool Tags::containsTagId(QUuid tagId) const
{
    return tags.contains(tagId);
}


// Return no of time "name" corresponds to the name of a atg in the set
quint16 Tags::containsTagName(QString name) const
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
    if ( true==tags.contains(tagId) ){
        found = true;
        return tags.value(tagId);
    } else {
        found = false;
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


Tags Tags::fromJson(const QJsonObject &jsonObject, Util::OperationResult &result)
{
    result.success = false;
    result.errorStringUI = "";
    result.errorStringLog = "";
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
        result.errorStringUI = tr("Set is not an array");
        result.errorStringLog = QString("Set is not an array");
        return tagSet;
    }

    // Check if too many tags
    jsonArrayTags = jsonValue.toArray();
    if (jsonArrayTags.size() > Tags::MAX_NO_TAGS) {
        result.errorStringUI = tr("Too many tags : max allowed is %1, %2 found")
            .arg(Tags::MAX_NO_TAGS).arg(jsonArrayTags.size());
        result.errorStringLog = QString("Too many tags : max allowed is %1, %2 found")
            .arg(Tags::MAX_NO_TAGS).arg(jsonArrayTags.size());
        return tagSet;
    }

    for (int i = 0; i < jsonArrayTags.size(); ++i) {
        QJsonValue value = jsonArrayTags.at(i);
        if (value.isObject()) {
            QJsonObject jsonObject = value.toObject();
            // extract the tag
            Tag t = Tag::fromJson(jsonObject, result);
            if (result.success==false){
                tagSet.clear();
                return tagSet;
            }
            // add the tag to the set
            tagSet.insert(t);
        }
    }

    result.success = true;
    return tagSet;
}



