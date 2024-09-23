/*
 *  Copyright (C) 2024 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
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

#include "festreamdef.h"


int FeStreamDef::NAME_MAX_LEN = 100;
int FeStreamDef::DESC_MAX_LEN = 4000;


// Not used explicitely in GBP
FeStreamDef::FeStreamDef()
{
    this->id = QUuid::createUuid();
    this->name = "";
    this->desc = "";
    this->streamType = PERIODIC;
    this->active = false;
    this->isIncome = true;
    this->decorationColor = QColor(); // invalid color, so it means not used
}

FeStreamDef::FeStreamDef(const FeStreamDef& o )
{
    this->id = o.getId();
    this->name = o.name.left(FeStreamDef::NAME_MAX_LEN); // truncate if required
    this->desc = o.desc.left(FeStreamDef::DESC_MAX_LEN); // truncate if required
    this->streamType = o.streamType;
    this->active = o.active;
    this->isIncome = o.isIncome;
    this->decorationColor = o.decorationColor;
}


FeStreamDef::FeStreamDef(const QUuid &id, const QString &name, const QString &desc, FeStreamDef::FeStreamType streamType, bool active, bool isIncome, const QColor &decorationColor) :
    id(id),
    name(name.left(NAME_MAX_LEN)),
    desc(desc.left(DESC_MAX_LEN)),
    streamType(streamType),
    active(active),
    isIncome(isIncome),
    decorationColor(decorationColor)
{
}


FeStreamDef::~FeStreamDef()
{

}


FeStreamDef &FeStreamDef::operator=(const FeStreamDef &o)
{
    this->id = o.id;    // id is copied and will be the same !
    this->name = o.name.left(NAME_MAX_LEN);
    this->desc = o.desc.left(DESC_MAX_LEN);
    this->streamType = o.streamType;
    this->active = o.active;
    this->isIncome = o.isIncome;
    this->decorationColor = o.decorationColor;
    return *this;
}


bool FeStreamDef::operator==(const FeStreamDef &o) const
{
    if ( (this->id != o.id) ||
        (this->name != o.name) ||
        (this->desc != o.desc) ||
        (this->streamType != o.streamType) ||
        (this->active != o.active) ||
        (this->isIncome != o.isIncome) ||
        (this->decorationColor != o.decorationColor) ){
        return false;
    } else{
        return true;
    }
}


void FeStreamDef::toJson(QJsonObject &jsonObject) const
{
    jsonObject["Id"] = id.toString(QUuid::WithoutBraces);
    jsonObject["Name"] = name;
    jsonObject["Description"] = desc;
    jsonObject["StreamType"] = streamType;
    jsonObject["Active"] = active;
    jsonObject["IsIncome"] = isIncome;
    // color (optional)
    if (decorationColor.isValid()) {
        jsonObject["DecorationColor"] = decorationColor.name(QColor::HexRgb);
    }

}


// From JsonObject, get the data required to build later an object of this class
void FeStreamDef::fromJson(const QJsonObject &jsonObject, FeStreamType expectedStreamType, QUuid &id, QString &name, QString &desc,
                           bool &active, bool &isIncome, QColor& decorationColor, Util::OperationResult &result)
{
    QJsonValue jsonValue;
    FeStreamType streamType;
    int ok;
    double d;

    result.success = false;
    result.errorStringUI = "";
    result.errorStringLog = "";

    // ID
    jsonValue = jsonObject.value("Id");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("Cannot find Id tag");
        result.errorStringLog = QString("Cannot find Id tag");
        return;
    }
    if (jsonValue.isString()==false){
        result.errorStringUI = tr("Id tag is not a string");
        result.errorStringLog = QString("Id tag is not a string");
        return;
    }
    QString idString  = jsonValue.toString();
    if (idString.length()>100){ // arbitray , because I am not sure this is 36 char on ANY platform & Qt version
        result.errorStringUI = tr("Id is too long");
        result.errorStringLog = QString("Id is too long");
        return;
    }
    id = QUuid::fromString(idString);
    if (id.isNull()){
        result.errorStringUI = tr("Id is not a valid UUID");
        result.errorStringLog = QString("Id is not a valid UUID");
        return;
    }
    // Name
    jsonValue = jsonObject.value("Name");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("Cannot find Name tag");
        result.errorStringLog = QString("Cannot find Name tag");
        return;
    }
    if (jsonValue.isString()==false){
        result.errorStringUI = tr("Name tag is not a string");
        result.errorStringLog = QString("Name tag is not a string");
        return;
    }
    name = jsonValue.toString();
    if (name.length()>NAME_MAX_LEN){
        result.errorStringUI = tr("Name is too long (max length is %1)").arg(NAME_MAX_LEN);
        result.errorStringLog = QString("Name is too long (max length is %1)").arg(NAME_MAX_LEN);
        return;
    }
    // Description
    jsonValue = jsonObject.value("Description");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("Cannot find Description tag");
        result.errorStringLog = QString("Cannot find Description tag");
        return;
    }
    if (jsonValue.isString()==false){
        result.errorStringUI = tr("Description tag is not a string");
        result.errorStringLog = QString("Description tag is not a string");
        return;
    }
    desc = jsonValue.toString();
    if(desc.length()>DESC_MAX_LEN){
        result.errorStringUI = tr("Description is too long (max length is %1)").arg(NAME_MAX_LEN);
        result.errorStringLog = QString("Description is too long (max length is %1)").arg(NAME_MAX_LEN);
        return;
    }
    // Stream Type
    jsonValue = jsonObject.value("StreamType");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("Cannot find StreamType tag");
        result.errorStringLog = QString("Cannot find StreamType tag");
        return;
    }
    if (jsonValue.isDouble()==false){
        result.errorStringUI = tr("StreamType tag is not a number");
        result.errorStringLog = QString("StreamType tag is not a number");
        return;
    }
    d = jsonValue.toDouble();
    qint64 sType = Util::extractQint64FromDoubleWithNoFracPart(d,ok);
    if ( ok==-1 ){
        result.errorStringUI = tr("StreamType tag %1 is not an integer").arg(d);
        result.errorStringLog = QString("StreamType tag %1 is not an integer").arg(d);
        return ;
    }
    if ( ok==-2 ){
        result.errorStringUI = tr("StreamType tag %1 is far too big").arg(d);
        result.errorStringLog = QString("StreamType tag %1 is far too big").arg(d);
        return ;
    }
    switch(sType){
    case 0: // PERIODIC_SIMPLE
        streamType = PERIODIC;
        if (streamType != expectedStreamType){
            result.errorStringUI = tr("Incorrect Stream, should be type=%1").arg(expectedStreamType);
            result.errorStringLog = QString("Incorrect Stream, should be type=%1").arg(expectedStreamType);
            return;
        }
        break;
    case 1: // IRREGULAR
        streamType = IRREGULAR;
        if (streamType != expectedStreamType){
            result.errorStringUI = tr("Incorrect Stream, should be type=%1").arg(expectedStreamType);
            result.errorStringLog = QString("Incorrect Stream, should be type=%1").arg(expectedStreamType);
            return;
        }
        break;
    default:
        result.errorStringUI = tr("Unknown Stream Type %1").arg(sType);
        result.errorStringLog = QString("Unknown Stream Type %1").arg(sType);
        return;
    }
    // Active
    jsonValue = jsonObject.value("Active");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("Cannot find Active tag");
        result.errorStringLog = QString("Cannot find Active tag");
        return;
    }
    if (jsonValue.isBool()==false){
        result.errorStringUI = tr("Active tag is not a boolean");
        result.errorStringLog = QString("Active tag is not a boolean");
        return;
    }
    active = jsonValue.toBool();
    // isIncome
    jsonValue = jsonObject.value("IsIncome");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("Cannot find IsIncome tag");
        result.errorStringLog = QString("Cannot find IsIncome tag");
        return;
    }
    if (jsonValue.isBool()==false){
        result.errorStringUI = tr("IsIncome tag is not a boolean");
        result.errorStringLog = QString("IsIncome tag is not a boolean");
        return;
    }
    isIncome = jsonValue.toBool();
    // decoration color (optional)
    jsonValue = jsonObject.value("DecorationColor");
    if (jsonValue == QJsonValue::Undefined){
        // that is perfectly fine : define "not used" decoration color (= invalid color)
        decorationColor = QColor();
    } else{
        if (jsonValue.isString()==false){
            result.errorStringUI = tr("DecorationColor tag is not a string");
            result.errorStringLog = QString("DecorationColor tag is not a string");
            return;
        }
        decorationColor = QColor(jsonValue.toString());
        if (decorationColor.isValid()==false) {
            decorationColor = QColor();
        }
    }

    result.success = true;
    result.errorStringUI = "";
    result.errorStringLog = "";
}





// Getters / setters

QUuid FeStreamDef::getId() const
{
    return id;
}

void FeStreamDef::setId(const QUuid &newId)
{
    id = newId;
}

QString FeStreamDef::getName() const
{
    return name;
}

void FeStreamDef::setName(const QString &newName)
{
    name = newName;
}

QString FeStreamDef::getDesc() const
{
    return desc;
}

void FeStreamDef::setDesc(const QString &newDesc)
{
    desc = newDesc;
}

FeStreamDef::FeStreamType FeStreamDef::getStreamType() const
{
    return streamType;
}

void FeStreamDef::setStreamType(FeStreamType newStreamType)
{
    streamType = newStreamType;
}

bool FeStreamDef::getActive() const
{
    return active;
}

void FeStreamDef::setActive(bool newActive)
{
    active = newActive;
}

bool FeStreamDef::getIsIncome() const
{
    return isIncome;
}

void FeStreamDef::setIsIncome(bool newIsIncome)
{
    isIncome = newIsIncome;
}

QColor FeStreamDef::getDecorationColor() const
{
    return decorationColor;
}

void FeStreamDef::setDecorationColor(const QColor &newDecorationColor)
{
    decorationColor = newDecorationColor;
}



