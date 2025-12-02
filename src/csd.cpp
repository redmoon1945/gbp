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

#include "csd.h"
#include "gbplogger.h"


// Not used explicitly in GBP
Csd::Csd()
{
    this->id = QUuid::createUuid();
    this->name = "";
    this->desc = "";
    this->type = Csd::CsdType::PERIODIC;
    this->active = false;
    this->isIncome = true;
    this->decorationColor = QColor(); // invalid color, so it means not used
}


Csd::Csd(const Csd& o )
{
    this->id = o.getId();
    this->name = o.name.left(Csd::NAME_MAX_LEN); // truncate if required
    this->desc = o.desc.left(Csd::DESC_MAX_LEN); // truncate if required
    this->type = o.type;
    this->active = o.active;
    this->isIncome = o.isIncome;
    this->decorationColor = o.decorationColor;
}


Csd::Csd(const QUuid &id, const QString &name, const QString &desc,
    Csd::CsdType type, bool active, bool isIncome,
    const QColor &decorationColor) :
    id(id),
    name(name.left(NAME_MAX_LEN)),
    desc(desc.left(DESC_MAX_LEN)),
    type(type),
    active(active),
    isIncome(isIncome),
    decorationColor(decorationColor)
{
}


Csd::~Csd()
{

}


Csd &Csd::operator=(const Csd &o)
{
    if (this != &o){    // to protect against self-assignment
        this->id = o.id;    // id is copied and will be the same !
        this->name = o.name.left(NAME_MAX_LEN);
        this->desc = o.desc.left(DESC_MAX_LEN);
        this->type = o.type;
        this->active = o.active;
        this->isIncome = o.isIncome;
        this->decorationColor = o.decorationColor;
    }
    return *this;
}


bool Csd::operator==(const Csd &o) const
{
    if ( (this->id != o.id) ||
        (this->name != o.name) ||
        (this->desc != o.desc) ||
        (this->type != o.type) ||
        (this->active != o.active) ||
        (this->isIncome != o.isIncome) ||
        (this->decorationColor != o.decorationColor) ){
        return false;
    } else{
        return true;
    }
}


bool Csd::operator!=(const Csd &o) const
{
    return !(*this==o);
}


void Csd::toJson(QJsonObject &jsonObject) const
{
    jsonObject["Id"] = id.toString(QUuid::WithoutBraces);
    jsonObject["Name"] = name;
    jsonObject["Description"] = desc;
    jsonObject["StreamType"] = convertTypeFromEnumToInt(type);
    jsonObject["Active"] = active;
    jsonObject["IsIncome"] = isIncome;
    // color (optional)
    if (decorationColor.isValid()) {
        jsonObject["DecorationColor"] = decorationColor.name(QColor::HexRgb);
    }
}


bool Csd::evaluateIfSameFeList(const Csd &o, QString& diff) const
{
    diff = "";

    if ( type != o.type ) {
        diff = QString("Base Csd %1 : type is different : %2 vs %3")
            .arg(REDACT(name)).arg(convertTypeFromEnumToInt(type))
            .arg(convertTypeFromEnumToInt(o.type));
        return false;
    }
    if ( active != o.active ) {
        diff = QString("Base Csd %1 : active status is different : %2 vs %3")
            .arg(REDACT(name)).arg(active).arg(o.active);
        return false;
    }
    if ( isIncome != o.isIncome ) {
        diff = QString("Base Csd %1 : isIncome is different : %2 vs %3")
            .arg(REDACT(name)).arg(isIncome).arg(o.isIncome);
        return false;
    }

    return true;
}


void Csd::fromJson(const QJsonObject &jsonObject, CsdType expectedStreamType,
    QUuid &id, QString &name, QString &desc, bool &active, bool &isIncome, QColor& decorationColor,
    Util::ResultOfOperation &result)
{
    QJsonValue jsonValue;
    CsdType streamType;
    double d;
    bool success;

    // Reset result to ERROR
    result.init();

    // ID
    jsonValue = jsonObject.value("Id");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = QString("Csd: Cannot find token \"Id\"");
        return;
    }
    if (jsonValue.isString()==false){
        result.logErrorMessage = QString("Csd: Id token is not a string");
        return;
    }
    QString idString  = jsonValue.toString();
    id = Util::convertStringToQuuid(idString, success);
    if (success == false){
        idString.truncate(38);
        result.logErrorMessage = QString("Csd : Id \"%1\" is not a valid UUID").arg(idString);
        return;
    }

    // Name
    jsonValue = jsonObject.value("Name");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = QString("Csd %1: Cannot find token \"Name\"").arg(idString);
        return;
    }
    if (jsonValue.isString()==false){
        result.logErrorMessage = QString("Csd %1: Name token is not a string").arg(idString);
        return;
    }
    name = jsonValue.toString();
    if (name.length()>NAME_MAX_LEN){
        result.logErrorMessage = QString("Csd %1: Name is too long (max length is %2)")
            .arg(idString).arg(NAME_MAX_LEN);
        return;
    }

    // Description
    jsonValue = jsonObject.value("Description");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = QString("Csd %1: Cannot find token \"Description\"").arg(idString);
        return;
    }
    if (jsonValue.isString()==false){
        result.logErrorMessage = QString("Csd %1: Description token is not a string").arg(idString);
        return;
    }
    desc = jsonValue.toString();
    if(desc.length()>DESC_MAX_LEN){
        result.logErrorMessage = QString("Csd %1: Description is too long (max length is %2)")
            .arg(idString).arg(NAME_MAX_LEN);
        return;
    }

    // Stream Type
    jsonValue = jsonObject.value("StreamType");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = QString("Csd %1: Cannot find token \"StreamType\"").arg(idString);
        return;
    }
    if (jsonValue.isDouble()==false){
        result.logErrorMessage = QString("Csd %1: StreamType token is not a number").arg(idString);
        return;
    }
    d = jsonValue.toDouble();
    int convResult;
    qint64 sType = Util::extractQint64FromDoubleWithNoFractionalPart(d,convResult);
    if ( convResult != 0 ){
        result.logErrorMessage = QString("Csd %1: StreamType token %2 is not a valid integer "
            "(code=%3)").arg(idString).arg(d).arg(convResult);
        return ;
    }

    bool streamTypeConversionSucceeded;
    Csd::CsdType readType;
    streamTypeConversionSucceeded = Csd::convertTypeFromIntToEnum(sType, readType);
    if (streamTypeConversionSucceeded==false) {
        result.logErrorMessage = QString("Csd %1: Invalid Stream Type value \"%2\"")
            .arg(idString).arg(sType);
        return;
    }

    // Active
    jsonValue = jsonObject.value("Active");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = QString("Csd %1: Cannot find token \"Active\"").arg(idString);
        return;
    }
    if (jsonValue.isBool()==false){
        result.logErrorMessage = QString("Csd %1: Active token is not a boolean").arg(idString);
        return;
    }
    active = jsonValue.toBool();

    // isIncome
    jsonValue = jsonObject.value("IsIncome");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = QString("Csd %1: Cannot find token \"IsIncome\"").arg(idString);
        return;
    }
    if (jsonValue.isBool()==false){
        result.logErrorMessage = QString("Csd %1: IsIncome token is not a boolean").arg(idString);
        return;
    }
    isIncome = jsonValue.toBool();

    // decoration color (optional)
    jsonValue = jsonObject.value("DecorationColor");
    if (jsonValue == QJsonValue::Undefined){
        // that is perfectly fine if not present : then set it to "not used", which is
        // implemented as "invalid color" or QColor()
        decorationColor = QColor();
    } else{
        if (jsonValue.isString()==false){
            result.logErrorMessage = QString("Csd %1: DecorationColor token is not a string")
                .arg(idString);
            return;
        }
        // If present, it must be valid though
        decorationColor = QColor(jsonValue.toString());
        if (decorationColor.isValid()==false) {
            //decorationColor = QColor();
            result.logErrorMessage = QString("Csd %1: DecorationColor token \"%2\" is invalid")
                .arg(idString).arg(jsonValue.toString());
            return;
        }
    }

    result.status = Util::ResultOfOperationStatus::SUCCESS;
}





// Getters / setters

QUuid Csd::getId() const
{
    return id;
}

void Csd::setId(const QUuid &newId)
{
    id = newId;
}

QString Csd::getName() const
{
    return name;
}

void Csd::setName(const QString &newName)
{
    name = newName.left(NAME_MAX_LEN);
}

QString Csd::getDesc() const
{
    return desc;
}

void Csd::setDesc(const QString &newDesc)
{
    desc = newDesc.left(DESC_MAX_LEN);
}

Csd::CsdType Csd::getType() const
{
    return type;
}

void Csd::setType(CsdType newType)
{
    type = newType;
}

bool Csd::getActive() const
{
    return active;
}

void Csd::setActive(bool newActive)
{
    active = newActive;
}

bool Csd::getIsIncome() const
{
    return isIncome;
}

void Csd::setIsIncome(bool newIsIncome)
{
    isIncome = newIsIncome;
}

QColor Csd::getDecorationColor() const
{
    return decorationColor;
}

void Csd::setDecorationColor(const QColor &newDecorationColor)
{
    decorationColor = newDecorationColor;
}


bool Csd::convertTypeFromIntToEnum(int value, CsdType &result)
{
    if (value==0) {
        result = Csd::CsdType::PERIODIC;
        return true;
    } else if (value==1){
        result = Csd::CsdType::IRREGULAR;
        return true;
    } else{
        return false;
    }
}


int Csd::convertTypeFromEnumToInt(CsdType type)
{
    if (type==Csd::CsdType::PERIODIC) {
        return 0;
    } else if (type==Csd::CsdType::IRREGULAR){
        return 1;
    } else{
        // should never happen
        throw std::invalid_argument("Invalid CsdType");
    }
}





