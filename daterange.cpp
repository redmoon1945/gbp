/*
 *  Copyright (C) 2024 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
 *  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "daterange.h"
#include <stdexcept>


// the most restrictive by default (empty)
DateRange::DateRange(){
    this->type = EMPTY;
    // dummy values
    this->start = QDate(1000,1,1);
    this->end = QDate(1000,12,31);
}


DateRange::DateRange(const DateRange &o)
{
    this->start = o.start;
    this->end = o.end;
    this->type = o.type;
}


// Does not make any sense to use it for BOUNDED (use the other constructor), but it works
DateRange::DateRange(Type r){
    this->type = r;
    // Dummy values
    this->start = QDate(1000,1,1);
    this->end = QDate(1000,12,31);
}


DateRange::~DateRange()
{

}


DateRange::DateRange(const QDate from, const QDate to){
    if (from.isValid()==false){
        throw std::invalid_argument("from is invalid");
    }
    if (to.isValid()==false){
        throw std::invalid_argument("to is invalid");
    }
    if (from > to){
        throw std::invalid_argument("from must not be greater than to");
    }
    if ( (to.year()-from.year()+1) > MAX_YEARS){
        throw std::invalid_argument("Range cannot span more than "+std::to_string(MAX_YEARS)+" years");
    }
    this->start = from;
    this->end = to;
    this->type = BOUNDED;
}


int DateRange::GetNoOfYearsSpanned() const {
    if (this->type == EMPTY){
        return 0;
    } else if (this->type==INFINITE){
            throw std::out_of_range("DateRange is infinite");
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
        ((this->type==INFINITE) && (o.type==INFINITE)) ||
        ((this->type==INFINITE) && (o.type==BOUNDED)) ||
        ((this->type==BOUNDED) && (o.type==INFINITE)) ) {
        return true;
    } else if ((this->type==BOUNDED) && (o.type==BOUNDED)){
        return (!((this->end<o.start) || (o.end < this->start)));
    } else {
        return false;
    }
}


DateRange DateRange::intersection(const DateRange o) const{
    if (false == intersectWith(o)){
        return DateRange();
    }
    if ( ((this->type==INFINITE) && (o.type==INFINITE))  ) {
        return DateRange(INFINITE);
    } else if ( ((this->type==INFINITE) && (o.type==BOUNDED)) ){
        return o;
    } else if ( ((this->type==BOUNDED) && (o.type==INFINITE)) ){
        return *this;
    }
    // necessarily BOUNDED and BOUNDED
    QDate from = ((start<o.start)?(o.start):(start));
    QDate to=((end>o.end)?(o.end):(end));
    return DateRange(from,to);
}


bool DateRange::includeDate(const QDate o) const{
    if (this->type==INFINITE){
        return true;
    } else if (this->type==EMPTY){
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
    if ( this->type==EMPTY){
         return list;
    } else if (this->type==INFINITE){
        throw std::out_of_range("DateRange is infinite");
    }
    list.reserve(GetNoOfYearsSpanned());
    QDate date = start;
    while (!(date>end)) {
         list.append(QDate(date));
         date = date.addDays(1);
    }
    return list;
}


// have to be internationalized
QString DateRange::toString() const{
    if (this->type==EMPTY){
        return tr("Empty");
    } else if (this->type==INFINITE){
         return tr("Infinite");
    }
    return QString::asprintf("[%04d-%02d-%02d,%04d-%02d-%02d]", start.year(), start.month(), start.day(), end.year(), end.month(), end.day());
}


QJsonObject DateRange::toJson() const
{
    QJsonObject jobject;
    jobject["Start"] = start.toString(Qt::ISODate);
    jobject["End"] = end.toString(Qt::ISODate);
    jobject["Type"] = type;
    return jobject;
}



DateRange DateRange::fromJson(const QJsonObject &jsonObject, Util::OperationResult &result)
{
    QJsonValue jsonValue;
    DateRange r;
    double d;
    int ok;

    result.success = false;
    result.errorStringUI = "";
    result.errorStringLog = "";

    // Start tag
    jsonValue = jsonObject.value("Start");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("DateRange - Cannot find Start tag");
        result.errorStringLog = "DateRange - Cannot find Start tag";
        return r;
    }
    if (jsonValue.isString()==false){
        result.errorStringUI = tr("DateRange - Start tag is not a string");
        result.errorStringLog = "DateRange - Start tag is not a string";
        return r;
    }
    QDate s = QDate::fromString(jsonValue.toString(),Qt::ISODate);
    if( !(s.isValid()) ) {
        result.errorStringUI = tr("DateRange - Start Date value %1 is not a valid ISO Date").arg(jsonValue.toString());
        result.errorStringLog = QString("DateRange - Start Date value %1 is not a valid ISO Date").arg(jsonValue.toString());
        return r;
    }
    // End tag
    jsonValue = jsonObject.value("End");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("DateRange - Cannot find End tag");
        result.errorStringLog = "DateRange - Cannot find End tag";
        return r;
    }
    if (jsonValue.isString()==false){
        result.errorStringUI = tr("DateRange - End tag is not a string");
        result.errorStringLog = "DateRange - End tag is not a string";
        return r;
    }
    QDate e = QDate::fromString(jsonValue.toString(),Qt::ISODate);
    if( !(e.isValid()) ) {
        result.errorStringUI = tr("DateRange - End Date value %1 is not a valid ISO Date").arg(jsonValue.toString());
        result.errorStringLog = QString("DateRange - End Date value %1 is not a valid ISO Date").arg(jsonValue.toString());
        return r;
    }
    // check that dates are valid

    // check that end is >= start
    if(e<s){
        result.errorStringUI = tr("DateRange - End Date value %1 is smaller than start date %2").arg(e.toString(Qt::ISODate),s.toString(Qt::ISODate));
        result.errorStringLog = QString("DateRange - End Date value %1 is smaller than start date %2").arg(e.toString(Qt::ISODate),s.toString(Qt::ISODate));
        return r;
    }

    // type
    jsonValue = jsonObject.value("Type");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("DateRange - Cannot find Type tag");
        result.errorStringLog = "DateRange - Cannot find Type tag";
        return r;
    }
    if (jsonValue.isDouble()==false){
        result.errorStringUI = tr("DateRange - Type tag is not a number");
        result.errorStringLog = "DateRange - Type tag is not a number";
        return r;
    }

    d = jsonValue.toDouble();
    qint64 t = Util::extractQint64FromDoubleWithNoFracPart(d,ok);
    if ( ok==-1 ){
        result.errorStringUI = tr("DateRange - Type tag %1 is not an integer").arg(d);
        result.errorStringLog = QString("DateRange - Type tag %1 is not an integer").arg(d);
        return r;
    }
    if ( ok==-2 ){
        result.errorStringUI = tr("DateRange - Type tag %1 is far too big").arg(d);
        result.errorStringLog = QString("DateRange - Type tag %1 is far too big").arg(d);
        return r;
    }

    Type type;
    try {
        switch(t){
        case 0:
            r = DateRange(EMPTY);
            break;
        case 1:
            r =  DateRange(s,e);
            break;
        case 2:
            r = DateRange(INFINITE);
            break;
        default:
            result.errorStringUI = tr("DateRange - Type tag %1 is unkown").arg(t);
            result.errorStringLog = QString("DateRange - Type tag %1 is unkown").arg(t);
            return r;
        }
    } catch (...) {
        std::exception_ptr p = std::current_exception();
        result.errorStringUI = tr("DateRange - An unexpected error has occured.\n\nDetails : %1").arg((p ? p.__cxa_exception_type()->name() : "null"));
        result.errorStringLog = QString("DateRange - An unexpected error has occured.\n\nDetails : %1").arg((p ? p.__cxa_exception_type()->name() : "null"));
        return r;
    }

    result.success = true;
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
        ((this->type==INFINITE) && (o.type==EMPTY)) ||
        ((this->type==INFINITE) && (o.type==BOUNDED)) ||
        ((this->type==EMPTY) && (o.type==INFINITE)) ||
        ((this->type==EMPTY) && (o.type==BOUNDED)) ||
        ((this->type==BOUNDED) && (o.type==EMPTY)) ||
        ((this->type==BOUNDED) && (o.type==INFINITE))  ) {
         return false;
    } else if (
        (this->type==INFINITE) && (o.type==INFINITE) ||
        (this->type==EMPTY) && (o.type==EMPTY)
        ){
         return true;
    } else {

         if ( (start!=o.start) || (end!=o.end)){
             return false;
         } else {
             return true;
         }
    }
}

DateRange &DateRange::operator=(const DateRange &o)
{
    this->start = o.start;
    this->end = o.end;
    this->type = o.type;
    return *this;
}


// GETTERS AND SETTERS

QDate DateRange::getStart() const
{
    return start;
}

QDate DateRange::getEnd() const
{
    return end;
}

DateRange::Type DateRange::getType() const
{
    return type;
}

