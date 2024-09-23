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

#include "daterange.h"
#include <stdexcept>


DateRange::DateRange(){
    this->type = EMPTY;
    this->start = QDate(1000,1,1);  // Dummy value, so that the date is valid
    this->end = QDate(1000,12,31);  // Dummy value, so that the date is valid
}


DateRange::DateRange(const QDate from){
    if (from.isValid()==false){
        throw std::invalid_argument("from is invalid");
    }
    this->start = from;
    this->end = QDate(1000,12,31); // dummy value, so that the date is valid
    this->type = BOUNDED_START_INFINITE_END;
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


DateRange DateRange::getInfiniteRange()
{
    DateRange dr;
    dr.type = INFINITE;
    dr.start = QDate(1000,1,1);  // Dummy value, so that the date is valid
    dr.end = QDate(1000,12,31);  // Dummy value, so that the date is valid
    return  dr;
}


DateRange::DateRange(const DateRange &o)
{
    this->start = o.start;
    this->end = o.end;
    this->type = o.type;
}


DateRange::~DateRange()
{

}


int DateRange::GetNoOfYearsSpanned() const {
    if (this->type == EMPTY){
        return 0;
    } else if ( (this->type==INFINITE) || (this->type==BOUNDED_START_INFINITE_END)){
            throw std::out_of_range("DateRange is infinite");
    }
    return (end.year()-start.year()+1);
}


// Tell if 2 DateRange intersects
//
//  this                            o                               result
//  -----------------------------------------------------------------------
//  INFINITE                        EMPTY                           false
//  INFINITE                        INFINITE                        true
//  INFINITE                        BOUNDED_START_INFINITE_END      true
//  INFINITE                        BOUNDED                         true
//  EMPTY                           EMPTY                           false
//  EMPTY                           INFINITE                        false
//  EMPTY                           BOUNDED_START_INFINITE_END      false
//  EMPTY                           BOUNDED                         false
//  BOUNDED                         EMPTY                           false
//  BOUNDED                         INFINITE                        true
//  BOUNDED                         BOUNDED_START_INFINITE_END      depends
//  BOUNDED                         BOUNDED                         depends
//  BOUNDED_START_INFINITE_END      EMPTY                           false
//  BOUNDED_START_INFINITE_END      INFINITE                        true
//  BOUNDED_START_INFINITE_END      BOUNDED_START_INFINITE_END      true
//  BOUNDED_START_INFINITE_END      BOUNDED                         depends
bool DateRange::intersectWith(const DateRange o) const{
    // TRUE (6 cases)
    if (
        ((this->type==INFINITE) && (o.type==INFINITE)) ||
        ((this->type==INFINITE) && (o.type==BOUNDED_START_INFINITE_END)) ||
        ((this->type==INFINITE) && (o.type==BOUNDED)) ||
        ((this->type==BOUNDED) && (o.type==INFINITE)) ||
        ((this->type==BOUNDED_START_INFINITE_END) && (o.type==INFINITE)) ||
        ((this->type==BOUNDED_START_INFINITE_END) && (o.type==BOUNDED_START_INFINITE_END)) ) {
        return true;

    // DEPENDS (3 cases)
    } else if ((this->type==BOUNDED) && (o.type==BOUNDED_START_INFINITE_END)){
        return (o.start <= this->end);
    } else if ((this->type==BOUNDED) && (o.type==BOUNDED)){
        return (!((this->end<o.start) || (o.end < this->start)));
    } else if ((this->type==BOUNDED_START_INFINITE_END) && (o.type==BOUNDED)){
        return (this->start <= o.end);

    // FALSE (7 cases)
    } else {
        return false;
    }
}

// Return the intersection range of 2 DateRange
//
//  this                            o                               result
//  -----------------------------------------------------------------------
//  INFINITE                        EMPTY                           EMPTY
//  INFINITE                        INFINITE                        INFINITE
//  INFINITE                        BOUNDED_START_INFINITE_END      BOUNDED_START_INFINITE_END
//  INFINITE                        BOUNDED                         BOUNDED
//  EMPTY                           EMPTY                           EMPTY
//  EMPTY                           INFINITE                        EMPTY
//  EMPTY                           BOUNDED_START_INFINITE_END      EMPTY
//  EMPTY                           BOUNDED                         EMPTY
//  BOUNDED                         EMPTY                           EMPTY
//  BOUNDED                         INFINITE                        BOUNDED
//  BOUNDED                         BOUNDED_START_INFINITE_END      depends
//  BOUNDED                         BOUNDED                         depends
//  BOUNDED_START_INFINITE_END      EMPTY                           EMPTY
//  BOUNDED_START_INFINITE_END      INFINITE                        BOUNDED_START_INFINITE_END
//  BOUNDED_START_INFINITE_END      BOUNDED_START_INFINITE_END      BOUNDED_START_INFINITE_END
//  BOUNDED_START_INFINITE_END      BOUNDED                         depends
DateRange DateRange::intersection(const DateRange o) const{
    // INFINITE (1 cases)
    if ( ((this->type==INFINITE) && (o.type==INFINITE)) ){
        return DateRange::getInfiniteRange();

    // BOUNDED_START_INFINITE_END (3 cases)
    } else if(((this->type==INFINITE) && (o.type==BOUNDED_START_INFINITE_END))){
        return o;
    } else if(((this->type==BOUNDED_START_INFINITE_END) && (o.type==INFINITE))){
        return *this;
    } else if ( ((this->type==BOUNDED_START_INFINITE_END) && (o.type==BOUNDED_START_INFINITE_END)) ){
        DateRange dr;
        dr.type = BOUNDED;
        dr.end = QDate(1000,1,1);  // Dummy value, so that the date is valid
        if ( this->start <= o.start ){
            dr.start = this->start;
            return dr;
        } else {
            dr.start = o.start;
            return dr;
        }

    // BOUNDED (2 cases)
    } else if ( ((this->type==INFINITE) && (o.type==BOUNDED)) ){
        return o;
    } else if ( ((this->type==BOUNDED) && (o.type==INFINITE)) ){
        return *this;

    // Depends (3 cases)
    } else if ( ((this->type==BOUNDED) && (o.type==BOUNDED_START_INFINITE_END)) ){
        if ( o.start <= this->end ){
            DateRange dr;
            dr.type = BOUNDED;
            dr.start = o.start;
            dr.end = this->end;
            return dr;
        } else {
            return DateRange(); // empty
        }
    } else if ( ((this->type==BOUNDED) && (o.type==BOUNDED)) ){
        if ( intersectWith(o)==false){
            return DateRange();
        } else {
            QDate from = ((start<o.start)?(o.start):(start));
            QDate to=((end>o.end)?(o.end):(end));
            return DateRange(from,to);
        }
    } else if ( ((this->type==BOUNDED_START_INFINITE_END) && (o.type==BOUNDED)) ){
        if ( this->start <= o.end ){
            DateRange dr;
            dr.type = BOUNDED;
            dr.start = this->start;
            dr.end = o.end;
            return dr;
        } else {
            return DateRange(); // empty
        }

    // EMPTY (7 cases)
    } else{
        return DateRange();
    }
}


bool DateRange::includeDate(const QDate o) const{
    if (this->type==INFINITE){
        return true;
    } else if (this->type==BOUNDED_START_INFINITE_END){
        if (o <= start) {
            return false;
        } else {
            return true;
        }
    } else if (this->type==BOUNDED) {
        if ( (o<start) || (o>end)){
            return false;
        } else {
            return true;
        }
    } else {
         return false;
    }
}



QList<QDate> DateRange::getDateList() const{
    QList<QDate> list;
    if ( this->type==EMPTY){
         return list;
    } else if ( (this->type==INFINITE) || (this->type==BOUNDED_START_INFINITE_END) ){
        throw std::out_of_range("DateRange is infinite");
    }
    // BOUNDED
    list.reserve(GetNoOfYearsSpanned());
    QDate date = start;
    while (!(date>end)) {
         list.append(QDate(date));
         date = date.addDays(1);
    }
    return list;
}

// stringForEndSideIfInfinite : if type==BOUNDED_START_INFINITE_END and stringForEndSideIfInfinite!="", replace "infinite" with this custom string
QString DateRange::toString(QString stringForEndSideIfInfinite) const{
    if (this->type==EMPTY){
        return tr("Empty");
    } else if (this->type==INFINITE){
         return tr("Infinite");
    } else if (this->type==BOUNDED_START_INFINITE_END){
        QString s = QString::asprintf("[%04d-%02d-%02d,", start.year(), start.month(), start.day());
        QString infiniteString;
        if (stringForEndSideIfInfinite=="") {
            infiniteString = tr("Infinite");
        } else {
            infiniteString = stringForEndSideIfInfinite;
        }
        QString s2 = QString("%1]").arg(s).arg(infiniteString);
        return s2;
    } else {
        return QString::asprintf("[%04d-%02d-%02d,%04d-%02d-%02d]", start.year(), start.month(), start.day(), end.year(), end.month(), end.day());
    }
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
    qint64 t = Util::extractQint64FromDoubleWithNoFracPart(d,ok); // int value of the enum Type
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

    // for BOUNDED, check that end is >= start
    if( (t==1) && (e<s)){
        result.errorStringUI = tr("DateRange - End Date value %1 is smaller than start date %2").arg(e.toString(Qt::ISODate),s.toString(Qt::ISODate));
        result.errorStringLog = QString("DateRange - End Date value %1 is smaller than start date %2").arg(e.toString(Qt::ISODate),s.toString(Qt::ISODate));
        return r;
    }

    // Build the DateRange object
    Type type;
    try {
        switch(t){
        case 0:
            r = DateRange();
            break;
        case 1:
            r =  DateRange(s,e);
            break;
        case 2:
            r = DateRange::getInfiniteRange();
            break;
        case 3:
            r = DateRange(s);
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


//  this                            o                           result
//  ------------------------------------------------------------------
//  INFINITE                        EMPTY                       false
//  INFINITE                        INFINITE                    true
//  INFINITE                        BOUNDED                     false
//  INFINITE                        BOUNDED_START_INFINITE_END  false
//  EMPTY                           EMPTY                       true
//  EMPTY                           INFINITE                    false
//  EMPTY                           BOUNDED                     false
//  EMPTY                           BOUNDED_START_INFINITE_END  false
//  BOUNDED                         EMPTY                       false
//  BOUNDED                         INFINITE                    false
//  BOUNDED                         BOUNDED                     depends
//  BOUNDED                         BOUNDED_START_INFINITE_END  false
//  BOUNDED_START_INFINITE_END      EMPTY                       false
//  BOUNDED_START_INFINITE_END      INFINITE                    false
//  BOUNDED_START_INFINITE_END      BOUNDED                     false
//  BOUNDED_START_INFINITE_END      BOUNDED_START_INFINITE_END  depends

bool DateRange::operator==(const DateRange &o) const{
    if (
        // FALSE (12 cases)
        ((this->type==INFINITE) && (o.type==EMPTY)) ||
        ((this->type==INFINITE) && (o.type==BOUNDED)) ||
        ((this->type==INFINITE) && (o.type==BOUNDED_START_INFINITE_END)) ||
        ((this->type==EMPTY) && (o.type==INFINITE)) ||
        ((this->type==EMPTY) && (o.type==BOUNDED)) ||
        ((this->type==EMPTY) && (o.type==BOUNDED_START_INFINITE_END)) ||
        ((this->type==BOUNDED) && (o.type==EMPTY)) ||
        ((this->type==BOUNDED) && (o.type==BOUNDED_START_INFINITE_END)) ||
        ((this->type==BOUNDED) && (o.type==INFINITE)) ||
        ((this->type==BOUNDED_START_INFINITE_END) && (o.type==EMPTY)) ||
        ((this->type==BOUNDED_START_INFINITE_END) && (o.type==BOUNDED)) ||
        ((this->type==BOUNDED_START_INFINITE_END) && (o.type==INFINITE)) ) {
         return false;
    } else if (
        // TRUE (2 cases)
        (this->type==INFINITE) && (o.type==INFINITE) ||
        (this->type==EMPTY) && (o.type==EMPTY)
        ){
         return true;
    } else {
        // DEPENDS (2 cases)
        if ( (this->type==BOUNDED) && (o.type==BOUNDED) ) {
            if ( (start!=o.start) || (end!=o.end)){
                return false;
            } else {
                return true;
            }
        } else if( (this->type==BOUNDED_START_INFINITE_END) && (o.type==BOUNDED_START_INFINITE_END) ) {
            if ( start!=o.start){
                return false;
            } else {
                return true;
            }
        } else {
            throw std::invalid_argument("operator= : unknown depends case");
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

