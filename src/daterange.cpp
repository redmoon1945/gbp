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

#include "daterange.h"
#include <stdexcept>


DateRange::DateRange(){
    this->type = Type::EMPTY;
    // dummy values, to keep compatibility with 1.6.3 and earlier
    this->start = QDate(1000,1,1);
    this->end = QDate(1000,12,31);
    this->noOfDays = 1 + this->start.daysTo(this->end);
}


DateRange::DateRange(const DateRange &o)
{
    this->start = o.start;
    this->end = o.end;
    this->type = o.type;
    this->noOfDays = o.noOfDays;
}


DateRange::DateRange(Type r){
    if (r == Type::BOUNDED) {
        throw std::invalid_argument(QString("%1: BOUNDED DateRange must be constructed with start "
            "and end dates").arg(Q_FUNC_INFO).toStdString());
    }
    this->type = r;
    // dummy values, to keep compatibility with 1.6.3 and earlier
    this->start = QDate(1000,1,1);
    this->end = QDate(1000,12,31);
    this->noOfDays = 1 + this->start.daysTo(this->end);
}


DateRange::DateRange(const QDate from, const QDate to){
    if (from.isValid()==false){
        throw std::invalid_argument(QString("%1: from is invalid").arg(Q_FUNC_INFO).toStdString());
    }
    if (to.isValid()==false){
        throw std::invalid_argument(QString("%1: to is invalid").arg(Q_FUNC_INFO).toStdString());
    }
    if (from.year() < MIN_YEAR || to.year() < MIN_YEAR) {
        QString fInfo = QString("%1").arg(Q_FUNC_INFO);
        QString s = QString("%1: Dates before year %2 are not allowed")
            .arg(fInfo).arg(std::to_string(MIN_YEAR));
        throw std::invalid_argument(s.toStdString());
    }
    if (from > to){
        throw std::invalid_argument(QString("%1: from must not be greater than to")
            .arg(Q_FUNC_INFO).toStdString());
    }
    if ( (to.year()-from.year()+1) > MAX_YEARS){
        QString fInfo = QString("%1").arg(Q_FUNC_INFO);
        QString s = QString("%1: Range cannot span more than %2 years")
            .arg(fInfo).arg(std::to_string(MAX_YEARS));
        throw std::invalid_argument(s.toStdString());
    }
    this->start = from;
    this->end = to;
    this->type = Type::BOUNDED;
    this->noOfDays = 1 + this->start.daysTo(this->end);
}


DateRange::~DateRange()
{

}


DateRange &DateRange::operator=(const DateRange &o)
{
    if (this != &o){// to protect against self-assignment
        this->start = o.start;
        this->end = o.end;
        this->type = o.type;
        this->noOfDays = o.noOfDays;
    }
    return *this;
}


int DateRange::GetNoOfYearsSpanned() const {
    if (this->type == Type::EMPTY){
        return 0;
    } else if (this->type==Type::INFINITE){
        throw std::out_of_range(QString("%1: DateRange is infinite")
            .arg(Q_FUNC_INFO).toStdString());
    }
    return (end.year()-start.year()+1);
}


//  this        o       result
//  ---------------------------
//  INFINITE  EMPTY     false
//  INFINITE  INFINITE  true
//  INFINITE  BOUNDED   true
//  EMPTY     EMPTY     false
//  EMPTY     INFINITE  false
//  EMPTY     BOUNDED   false
//  BOUNDED   EMPTY     false
//  BOUNDED   INFINITE  true
//  BOUNDED   BOUNDED   depends
bool DateRange::intersectWith(const DateRange o) const{
    if (
        ((this->type==Type::INFINITE) && (o.type==Type::INFINITE)) ||
        ((this->type==Type::INFINITE) && (o.type==Type::BOUNDED)) ||
        ((this->type==Type::BOUNDED) && (o.type==Type::INFINITE)) ) {
        return true;
    } else if ((this->type==Type::BOUNDED) && (o.type==Type::BOUNDED)){
        return (!((this->end<o.start) || (o.end < this->start)));
    } else {
        return false;
    }
}


DateRange DateRange::intersection(const DateRange o) const{
    if (false == intersectWith(o)){
        return DateRange();
    }
    if ( ((this->type==Type::INFINITE) && (o.type==Type::INFINITE))  ) {
        return DateRange(Type::INFINITE);
    } else if ( ((this->type==Type::INFINITE) && (o.type==Type::BOUNDED)) ){
        return o;
    } else if ( ((this->type==Type::BOUNDED) && (o.type==Type::INFINITE)) ){
        return *this;
    }
    // necessarily BOUNDED and BOUNDED
    QDate from = ((start<o.start)?(o.start):(start));
    QDate to=((end>o.end)?(o.end):(end));
    return DateRange(from,to);
}


bool DateRange::includeDate(const QDate o) const{
    if (!o.isValid()) {
        return false;
    }
    if (this->type==Type::INFINITE){
        return true;
    } else if (this->type==Type::EMPTY){
        return false;
    }
    if ( (o<start) || (o>end)){
         return false;
    } else {
         return true;
    }
}



QList<QDate> DateRange::getDateList() const{
    QList<QDate> list;
    if ( this->type==Type::EMPTY){
         return list;
    } else if (this->type==Type::INFINITE){
        throw std::out_of_range(QString("%1: DateRange is infinite")
            .arg(Q_FUNC_INFO).toStdString());
    }
    QDate date = start;
    while (!(date>end)) {
        list.append(date);
        date = date.addDays(1);
    }
    return list;
}


// have to be internationalized
QString DateRange::toString() const{
    if (this->type==Type::EMPTY){
        return tr("Empty");
    } else if (this->type==Type::INFINITE){
         return tr("Infinite");
    }
    return QString::asprintf("[%04d-%02d-%02d,%04d-%02d-%02d]", start.year(), start.month(),
        start.day(), end.year(), end.month(), end.day());
}


QJsonObject DateRange::toJson() const
{
    QJsonObject jobject;
    QString s1 = start.toString(Qt::ISODate);
    jobject["Start"] = s1;
    QString s2 = end.toString(Qt::ISODate);;
    jobject["End"] = s2;
    jobject["Type"] = convertTypeFromEnumToInt(type);
    return jobject;
}


DateRange DateRange::fromJson(const QJsonObject &jsonObject, Util::ResultOfOperation &result)
{
    QJsonValue jsonValue;
    DateRange r;
    double d;

    // Reset result to ERROR
    result.init();

    // Start tag. Date should be always valid, even when type=EMPTY or INFINITE.
    jsonValue = jsonObject.value("Start");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = "DateRange: Cannot find token \"Start\"";
        return r;
    }
    if (jsonValue.isString()==false){
        result.logErrorMessage = "DateRange: Start token is not a string";
        return r;
    }
    // Because of a bug in Qt 6.9.1 QDate::fromString, we check ourself if
    // date is strict ISO 8601.
    bool validStartDate;
    QDate s = Util::isValidISO8601Date(jsonValue.toString(), validStartDate);
    if (validStartDate == false){
        result.logErrorMessage = QString("DateRange: Start Date value %1 is not a "
            "valid ISO Date or is invalid").arg(jsonValue.toString());
        return r;
    }
    // Date must be < MIN_YEAR
    if (s.year() < MIN_YEAR) {
        result.logErrorMessage = QString("DateRange: Start Date value %1 has year before %2")
            .arg(jsonValue.toString()).arg(MIN_YEAR);
        return r;
    }

    // End tag. Date should be always valid, even when type=EMPTY or INFINITE.
    jsonValue = jsonObject.value("End");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = "DateRange: Cannot find token \"End\"";
        return r;
    }
    if (jsonValue.isString()==false){
        result.logErrorMessage = "DateRange: End token is not a string";
        return r;
    }
    bool validEndDate;
    QDate e = Util::isValidISO8601Date(jsonValue.toString(), validEndDate);
    if(validEndDate==false){
        result.logErrorMessage = QString("DateRange: End Date value %1 is not a valid ISO Date"
            "or is invalid").arg(jsonValue.toString());
        return r;
    }
    // Date must be < MIN_YEAR
    if (e.year() < MIN_YEAR) {
        result.logErrorMessage = QString("DateRange: End Date value %1 has year before %2")
            .arg(jsonValue.toString()).arg(MIN_YEAR);
        return r;
    }

    // check that end is >= start
    if(e<s){
        result.logErrorMessage = QString(
            "DateRange: End Date value %1 is smaller than start date %2")
            .arg(e.toString(Qt::ISODate),s.toString(Qt::ISODate));
        return r;
    }

    // Check type
    jsonValue = jsonObject.value("Type");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = "DateRange: Cannot find token \"Type\"";
        return r;
    }
    if (jsonValue.isDouble()==false){
        result.logErrorMessage = "DateRange: Type token is not a number";
        return r;
    }
    d = jsonValue.toDouble();
    int convResult;
    qint64 tInt = Util::extractQint64FromDoubleWithNoFractionalPart(d,convResult);
    if ( convResult != 0 ){
        result.logErrorMessage = QString("DateRange: Type token %1 is not a valid integer"
            " (code=%2)").arg(d).arg(convResult);
        return r;
    }

    bool convSuccess;
    Type t = convertTypeFromIntToEnum(tInt,convSuccess);
    if (convSuccess==false) {
        result.logErrorMessage = QString("DateRange: Invalid type %1").arg(tInt);
        return r;
    }

    // Check that year difference does not exceed MAX_YEARS for BOUNDED ranges
    if (t == Type::BOUNDED ) {
        if (e.year() - s.year() > MAX_YEARS) {
            result.logErrorMessage = QString("DateRange: Year range from %1 to %2 exceeds "
                "maximum of %3 years")
                .arg(s.toString(Qt::ISODate), e.toString(Qt::ISODate)).arg(MAX_YEARS);
            return r;
        }
    }

    // Create DateRange
    switch(t){
        case Type::EMPTY:
            r = DateRange(Type::EMPTY);
            break;
        case Type::BOUNDED:
            r = DateRange(s,e);
            break;
        case Type::INFINITE:
            r = DateRange(Type::INFINITE);
            break;
        default:
            // will never happen
            result.logErrorMessage = QString("DateRange: Type token %1 is unkown").arg(tInt);
            return r;
    }

    result.status = Util::ResultOfOperationStatus::SUCCESS;
    return r;
}


//  this        o       result
//  ---------------------------
//  INFINITE  EMPTY     false
//  INFINITE  INFINITE  true
//  INFINITE  BOUNDED   false
//  EMPTY     EMPTY     true
//  EMPTY     INFINITE  false
//  EMPTY     BOUNDED   false
//  BOUNDED   EMPTY     false
//  BOUNDED   INFINITE  false
//  BOUNDED   BOUNDED   depends
bool DateRange::operator==(const DateRange &o) const{
    if (
        ((this->type==Type::INFINITE) && (o.type==Type::EMPTY)) ||
        ((this->type==Type::INFINITE) && (o.type==Type::BOUNDED)) ||
        ((this->type==Type::EMPTY) && (o.type==Type::INFINITE)) ||
        ((this->type==Type::EMPTY) && (o.type==Type::BOUNDED)) ||
        ((this->type==Type::BOUNDED) && (o.type==Type::EMPTY)) ||
        ((this->type==Type::BOUNDED) && (o.type==Type::INFINITE))  ) {
         return false;
    } else if (
        ((this->type==Type::INFINITE) && (o.type==Type::INFINITE)) ||
        ((this->type==Type::EMPTY) && (o.type==Type::EMPTY))
        ){
         return true;
    } else {
         return ( (start == o.start) && (end == o.end));
    }
}


bool DateRange::operator!=(const DateRange &o) const
{
    return !(*this==o);
}


int DateRange::convertTypeFromEnumToInt(Type t)
{
    switch (t) {
        case Type::EMPTY:
            return 0;
        case Type::BOUNDED:
            return 1;
        case Type::INFINITE:
            return 2;
        default:
            throw std::invalid_argument(QString("%1: Unknown DateRange::Type value")
                .arg(Q_FUNC_INFO).toStdString());
    }
}


DateRange::Type DateRange::convertTypeFromIntToEnum(int value, bool &success)
{
    success = true;
    switch (value) {
        case 0:
            return Type::EMPTY;
        case 1:
            return Type::BOUNDED;
        case 2:
            return Type::INFINITE;
        default:
            success = false;
            return Type::EMPTY;
    }
}


uint DateRange::getDayIndex(const QDate &date) const
{
    if (type != Type::BOUNDED) {
        return 0;
    }
    return this->start.daysTo(date);
}



// GETTERS


DateRange::Type DateRange::getType() const
{
    return type;
}


QDate DateRange::getStart() const
{
    return start;
}


QDate DateRange::getEnd() const
{
    return end;
}


uint DateRange::getNoOfDays() const
{
    return noOfDays;
}



