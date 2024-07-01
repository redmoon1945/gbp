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

#include "periodicfestreamdef.h"
#include "datehelper.h"
#include "currencyhelper.h"
#include <QUuid>


// not used explicitely in Gbp
PeriodicFeStreamDef::PeriodicFeStreamDef() : FeStreamDef()
{
    this->period = PeriodType(PeriodType::MONTHLY);
    this->periodMultiplier = 1;
    this->amount = 0;
    this->growth = Growth();
    this->growthStrategy = GrowthStrategy::NONE;
    this->growthApplicationPeriod = 1;
    this->validityRange = DateRange(DateRange::EMPTY);
}


PeriodicFeStreamDef::PeriodicFeStreamDef(const PeriodicFeStreamDef &o) :
    FeStreamDef(o)
{
    this->period = o.period;
    this->periodMultiplier = o.periodMultiplier;
    this->amount = o.amount;
    this->growth = o.growth;
    this->growthStrategy = o.growthStrategy;
    this->growthApplicationPeriod = o.growthApplicationPeriod;
    this->validityRange = o.validityRange;
}


PeriodicFeStreamDef::PeriodicFeStreamDef( PeriodicFeStreamDef::PeriodType periodicType,  quint16 periodMultiplier,  qint64 amount,
                                          const Growth &growth, const GrowthStrategy &growthStrategy,  quint16 growthApplicationPeriod, const QUuid &id,
                                          const QString &name, const QString &desc, bool active, bool isIncome, const QColor& decorationColor, const DateRange &validityRange)
    : FeStreamDef(id, name,desc,FeStreamType::PERIODIC,active,isIncome, decorationColor)
{
    if (amount<0){
        throw std::invalid_argument("Amount must not be negative");
    }
    if (amount>CurrencyHelper::maxValueAllowedForAmount()){
        throw std::invalid_argument("Amount is too big");
    }
    if (growthApplicationPeriod<GROWTH_APP_PERIOD_MIN){
        throw std::invalid_argument("growthApplicationPeriod must be > 0");
    }
    if (growthApplicationPeriod>GROWTH_APP_PERIOD_MAX){
        throw std::invalid_argument("growthApplicationPeriod too big");
    }
    if (periodMultiplier<PERIOD_MULTIPLIER_MIN){
        throw std::invalid_argument("periodMultiplier too small");
    }
    if (periodMultiplier>PERIOD_MULTIPLIER_MAX){
        throw std::invalid_argument("periodMultiplier too big");
    }
    this->period = periodicType;
    this->periodMultiplier = periodMultiplier;
    this->amount = amount;
    this->growth = growth;
    this->growthStrategy = growthStrategy;
    this->growthApplicationPeriod = growthApplicationPeriod;
    this->validityRange = validityRange;
}

PeriodicFeStreamDef &PeriodicFeStreamDef::operator=(const PeriodicFeStreamDef &o)
{
    FeStreamDef::operator=(o);
    this->period = o.period;
    this->periodMultiplier = o.periodMultiplier;
    this->amount = o.amount;
    this->growth = o.growth;
    this->growthStrategy = o.growthStrategy;
    this->growthApplicationPeriod = o.growthApplicationPeriod;
    this->validityRange = o.validityRange;
    return *this;
}

bool PeriodicFeStreamDef::operator==(const PeriodicFeStreamDef& o) const
{
    if ( !(FeStreamDef::operator==(o)) ||
        !(this->period == o.period) ||
        !(this->periodMultiplier == o.periodMultiplier) ||
        !(this->amount == o.amount) ||
        !(this->growth == o.growth) ||
        !(this->growthStrategy == o.growthStrategy) ||
        !(this->growthApplicationPeriod == o.growthApplicationPeriod) ||
        !(this->validityRange == o.validityRange) ) {
        return false;
    } else {
        return true;
    }
}


PeriodicFeStreamDef::~PeriodicFeStreamDef()
{

}


QList<Fe> PeriodicFeStreamDef::generateEventStream(DateRange fromto, const Growth &inflation, uint &saturationCount) const
{
    QList<Fe> ss;
    saturationCount = 0;

    // is FeStreamDefinition is inactive, do not generate any Fe
    if (!active)  {
        return ss;
    }
    // if fromto is outside validity range of this FeStreamDef, nothing to do
    if ( !(fromto.intersectWith(validityRange))){
        return ss;
    }

    // ** STEP 1 : Generate the flow of dates ***
    //    So starting from ValidityRange.Start, advance till fromto.End or validityrange.End is met.
    //    As soon as we enter fromto range, start generating occurence dates
    //    At this stage, we need all the occurences from validity.start in order to calculate Growth later
    QList<QDate> occurenceDates;
    QDate aDate = this->validityRange.getStart();
    while(true){
        if (!validityRange.includeDate(aDate) ){     // check if still in the validity range
            break;
        }
        if (aDate > fromto.getEnd()){                // check if we have gone over the fromTo End
            break;
        }
        occurenceDates.append(aDate);
        aDate = getNextEventDate(aDate);              // advance to the next event
    }

    // ** Step 2 : Correct for growth or inflation (not both) ***
    //    We need occurence from start of validation, to calculate growth, even if it is before fromto.start
    //    For now, Keep amount positive even if this is an expense
    qint64 am = this->amount;
    QMap<QDate,qint64> adjustedAmounts;
    Growth::ApplicationStrategy appStrategy = {.noOfMonths=growthApplicationPeriod};
    Growth::AdjustForGrowthResult afgResult;
    Growth noGrowth = Growth();
    switch (growthStrategy) {
        case GrowthStrategy::NONE:
            adjustedAmounts = noGrowth.adjustForGrowth(am, occurenceDates, appStrategy,afgResult);
            saturationCount = afgResult.saturationCount;
            if(afgResult.success==false){   // should not happen
                return ss;
            }
            break;
        case GrowthStrategy::INFLATION:
            adjustedAmounts = inflation.adjustForGrowth(am, occurenceDates, appStrategy,afgResult);
            saturationCount = afgResult.saturationCount;
            if(afgResult.success==false){   // should not happen
                return ss;
            }
            break;
        case GrowthStrategy::CUSTOM:
            adjustedAmounts = growth.adjustForGrowth(am, occurenceDates, appStrategy,afgResult);
            saturationCount = afgResult.saturationCount;
            if(afgResult.success==false){   // should not happen
                return ss;
            }
            break;
        default:
            break;
    }

    // *** Step 3 : build QList<Fe> result ***
    // remove dates occuring before fromto.start(), now that growth has been calculated
    // Make amount negative if this is an expense
    foreach(QDate aDate, adjustedAmounts.keys()){
        if( aDate >= fromto.getStart() ){
            qint64 am = (isIncome?(adjustedAmounts.value(aDate)):-(adjustedAmounts.value(aDate)));
            Fe fe = {.amount=am, .occurence=aDate,.id=this->getId()};
            ss.append(fe);
        }
    }

    return ss;
}


QJsonObject PeriodicFeStreamDef::toJson() const
{
    QJsonObject jobject;

    // base class data
    FeStreamDef::toJson(jobject);
    // this derived class data
    jobject["PeriodType"] = period;
    jobject["PeriodMultiplier"] = periodMultiplier;
    jobject["Amount"] = QVariant::fromValue(amount).toJsonValue();
    jobject["Growth"] = growth.toJson();
    jobject["GrowthStrategy"] = growthStrategy;
    jobject["GrowthApplicationPeriod"] = growthApplicationPeriod;
    jobject["ValidityRange"] = validityRange.toJson();
    // return result
    return jobject;
}



PeriodicFeStreamDef PeriodicFeStreamDef::fromJson(const QJsonObject &jsonObject, Util::OperationResult &result)
{
    QJsonValue jsonValue;
    int ok;
    double d;
    PeriodicFeStreamDef ps; // dummy

    result.success = false;
    result.errorStringUI = "";
    result.errorStringLog = "";

    // *** BASE CLASS DATA ***
    QUuid id;
    QString name,desc;
    bool active;
    bool isIncome;
    QColor decoColor;
    Util::OperationResult resultBaseClass;
    FeStreamDef::fromJson(jsonObject, PERIODIC, id, name, desc, active, isIncome, decoColor, resultBaseClass);
    if (resultBaseClass.success==false){
        result.success = false;
        result.errorStringUI = tr("PeriodicFeStreamDef - ") + resultBaseClass.errorStringUI;
        result.errorStringLog = "PeriodicFeStreamDef - " + resultBaseClass.errorStringLog;
        return ps;
    }

    // *** DERIVE CLASS DATA ***
    // Period Type
    jsonValue = jsonObject.value("PeriodType");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("PeriodicFeStreamDef - Cannot find PeriodType tag");
        result.errorStringLog = "PeriodicFeStreamDef - Cannot find PeriodType tag";
        return ps;
    }
    if (jsonValue.isDouble()==false){
        result.errorStringUI = tr("PeriodicFeStreamDef - PeriodType tag is not a number");
        result.errorStringLog = "PeriodicFeStreamDef - PeriodType tag is not a number";
        return ps;
    }
    d = jsonValue.toDouble();
    qint64 pType = Util::extractQint64FromDoubleWithNoFracPart(d,ok);
    if ( ok==-1 ){
        result.errorStringUI = tr("PeriodicFeStreamDef - PeriodType value %1 is not an integer").arg(d);
        result.errorStringLog = QString("PeriodicFeStreamDef - PeriodType value %1 is not an integer").arg(d);
        return ps;
    }
    if ( ok==-2){
        result.errorStringUI = tr("PeriodicFeStreamDef - PeriodType value %1 is either too small or too big").arg(d);
        result.errorStringLog = QString("PeriodicFeStreamDef - PeriodType value %1 is either too small or too big").arg(d);
        return ps;
    }
    PeriodType periodType;
    switch(pType){
    case 0:
        periodType = DAILY;
        break;
    case 1:
        periodType = WEEKLY;
        break;
    case 2:
        periodType = MONTHLY;
        break;
    case 3:
        periodType = END_OF_MONTHLY;
        break;
    case 4:
        periodType = YEARLY;
        break;
    default:
        result.errorStringUI = tr("PeriodicFeStreamDef - Unknown Period Type %1").arg(pType);
        result.errorStringLog = QString("PeriodicFeStreamDef - Unknown Period Type %1").arg(pType);
        return ps;
    }

    // Period Multiplier
    jsonValue = jsonObject.value("PeriodMultiplier");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("PeriodicFeStreamDef - Cannot find PeriodMultiplier tag");
        result.errorStringLog = "PeriodicFeStreamDef - Cannot find PeriodMultiplier tag";
        return ps;
    }
    if (jsonValue.isDouble()==false){
        result.errorStringUI = tr("PeriodicFeStreamDef - PeriodMultiplier tag is not a number");
        result.errorStringLog = "PeriodicFeStreamDef - PeriodMultiplier tag is not a number";
        return ps;
    }
    d = jsonValue.toDouble();
    qint64 pm = Util::extractQint64FromDoubleWithNoFracPart(d,ok);
    if ( ok==-1 ){
        result.errorStringUI = tr("PeriodicFeStreamDef - PeriodMultiplier value %1 is not an integer").arg(d);
        result.errorStringLog = QString("PeriodicFeStreamDef - PeriodMultiplier value %1 is not an integer").arg(d);
        return ps;
    }
    if ( (ok==-2) || ((pm<PERIOD_MULTIPLIER_MIN) || (pm>PERIOD_MULTIPLIER_MAX)) ){
        result.errorStringUI = tr("PeriodicFeStreamDef - PeriodMultiplier value %1 is either too small or too big").arg(d);
        result.errorStringLog = QString("PeriodicFeStreamDef - PeriodMultiplier value %1 is either too small or too big").arg(d);
        return ps;
    }
    quint16 periodMultiplier = static_cast<quint16>(pm);

    // Amount
    jsonValue = jsonObject.value("Amount");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("PeriodicFeStreamDef - Cannot find Amount tag");
        result.errorStringLog = "PeriodicFeStreamDef - Cannot find Amount tag";
        return ps;
    }
    if (jsonValue.isDouble()==false){
        result.errorStringUI = tr("PeriodicFeStreamDef - Amount tag is not a number");
        result.errorStringLog = "PeriodicFeStreamDef - Amount tag is not a number";
        return ps;
    }
    d = jsonValue.toDouble();
    qint64 amount = Util::extractQint64FromDoubleWithNoFracPart(d,ok);
    if ( ok==-1 ){
        result.errorStringUI =  tr("PeriodicFeStreamDef - Amount value %1 is not an integer").arg(d);
        result.errorStringLog = QString("PeriodicFeStreamDef - Amount value %1 is not an integer").arg(d);
        return ps;
    }
    if ( (ok==-2) || (amount>CurrencyHelper::maxValueAllowedForAmount()) || (amount<0) ){
        result.errorStringUI = tr("PeriodicFeStreamDef - Amount value %1 is either smaller than 0 or too big").arg(d);
        result.errorStringLog = QString("PeriodicFeStreamDef - Amount value %1 is either smaller than 0 or too big").arg(d);
        return ps;
    }

    // Growth
    jsonValue = jsonObject.value("Growth");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("PeriodicFeStreamDef - Cannot find Growth tag");
        result.errorStringLog = "PeriodicFeStreamDef - Cannot find Growth tag";
        return ps;
    }
    if (jsonValue.isObject()==false){
        result.errorStringUI = tr("PeriodicFeStreamDef - Growth tag is not an object");
        result.errorStringLog = "PeriodicFeStreamDef - Growth tag is not an object";
        return ps;
    }
    Util::OperationResult growParsingResult;
    Growth growth = Growth::fromJson(jsonValue.toObject(), growParsingResult);
    if (growParsingResult.success==false){
        result.errorStringUI = tr("PeriodicFeStreamDef - ")+growParsingResult.errorStringUI;
        result.errorStringLog = "PeriodicFeStreamDef - "+growParsingResult.errorStringLog;
        return ps;
    }


    // Growth Strategy
    jsonValue = jsonObject.value("GrowthStrategy");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("PeriodicFeStreamDef - Cannot find GrowthStrategy tag");
        result.errorStringLog = "PeriodicFeStreamDef - Cannot find GrowthStrategy tag";
        return ps;
    }
    if (jsonValue.isDouble()==false){
        result.errorStringUI =  tr("PeriodicFeStreamDef - GrowthStrategy tag %1 is not a number").arg(jsonValue.toString());
        result.errorStringLog = QString("PeriodicFeStreamDef - GrowthStrategy tag %1 is not a number").arg(jsonValue.toString());
        return ps;
    }
    d = jsonValue.toDouble();
    qint64 gsType = Util::extractQint64FromDoubleWithNoFracPart(d,ok);
    if ( ok==-1 ){
        result.errorStringUI = tr("PeriodicFeStreamDef - GrowthStrategy value %1 is not an integer").arg(d);
        result.errorStringLog = QString("PeriodicFeStreamDef - GrowthStrategy value %1 is not an integer").arg(d);
        return ps;
    }
    if ( ok==-2 ){
        result.errorStringUI = tr("PeriodicFeStreamDef - GrowthStrategy value %1 is either too small or too big").arg(d);
        result.errorStringLog = QString("PeriodicFeStreamDef - GrowthStrategy value %1 is either too small or too big").arg(d);
        return ps;
    }
    GrowthStrategy gs;
    switch(gsType){
    case 0:
        gs = GrowthStrategy::NONE;
        break;
    case 1:
        gs = GrowthStrategy::INFLATION;
        break;
    case 2:
        gs = GrowthStrategy::CUSTOM;
        break;
    default:
        result.errorStringUI = tr("PeriodicFeStreamDef - Unknown Growth Strategy value %1").arg(gsType);
        result.errorStringLog = QString("PeriodicFeStreamDef - Unknown Growth Strategy value %1").arg(gsType);
        return ps;
    }

    // GrowthApplicationPeriod
    jsonValue = jsonObject.value("GrowthApplicationPeriod");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("PeriodicFeStreamDef - Cannot find GrowthApplicationPeriod tag");
        result.errorStringLog = "PeriodicFeStreamDef - Cannot find GrowthApplicationPeriod tag";
        return ps;
    }
    if (jsonValue.isDouble()==false){
        result.errorStringUI = tr("PeriodicFeStreamDef - GrowthApplicationPeriod tag %1 is not a number").arg(jsonValue.toString());
        result.errorStringLog = QString("PeriodicFeStreamDef - GrowthApplicationPeriod tag %1 is not a number").arg(jsonValue.toString());        return ps;

    }
    d = jsonValue.toDouble();
    qint64 gapOrig = Util::extractQint64FromDoubleWithNoFracPart(d,ok);
    if ( ok==-1 ){
        result.errorStringUI = tr("PeriodicFeStreamDef - GrowthApplicationPeriod value %1 is not an integer").arg(d);
        result.errorStringLog = QString("PeriodicFeStreamDef - GrowthApplicationPeriod value %1 is not an integer").arg(d);
        return ps;
    }
    if (  (ok==-2) || (gapOrig<GROWTH_APP_PERIOD_MIN) || (gapOrig>GROWTH_APP_PERIOD_MAX) ){
        result.errorStringUI = tr("PeriodicFeStreamDef - GrowthApplicationPeriod value %1 is either too small or too big").arg(d);
        result.errorStringLog = QString("PeriodicFeStreamDef - GrowthApplicationPeriod value %1 is either too small or too big").arg(d);
        return ps;
    }
    quint16 gap = static_cast<quint16>(gapOrig);

    // validity range
    jsonValue = jsonObject.value("ValidityRange");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("PeriodicFeStreamDef - Cannot find ValidityRange tag");
        result.errorStringLog = "PeriodicFeStreamDef - Cannot find ValidityRange tag";
        return ps;
    }
    if (jsonValue.isObject()==false){
        result.errorStringUI = tr("PeriodicFeStreamDef - ValidityRange tag is not an object");
        result.errorStringLog = "PeriodicFeStreamDef - ValidityRange tag is not an object";
        return ps;
    }
    Util::OperationResult validityParsingResult;
    DateRange validity = DateRange::fromJson(jsonValue.toObject(),validityParsingResult);
    if (validityParsingResult.success==false){
        result.errorStringUI =  tr("PeriodicFeStreamDef - ")+validityParsingResult.errorStringUI;
        result.errorStringLog =  "PeriodicFeStreamDef - "+validityParsingResult.errorStringLog;
        return ps;
    }

    // *** build and return ***
    result.success = true;
    return PeriodicFeStreamDef(
        periodType, periodMultiplier, amount, growth, gs, gap,id, name, desc,active, isIncome, decoColor, validity);
}


PeriodicFeStreamDef PeriodicFeStreamDef::duplicate() const
{
    PeriodicFeStreamDef ps = *this;
    QString newName = QString("%1 %2").arg(tr("Copy of")).arg(name);
    newName.truncate(FeStreamDef::NAME_MAX_LEN);
    ps.setId(QUuid::createUuid());
    ps.setName(newName);
    return ps;
}


// Provide textual info on the FE Stream
QString PeriodicFeStreamDef::toStringForDisplay(CurrencyInfo currInfo, QLocale locale) const
{
    QStringList l;
    QString s;

    // order is : Every N <period> in [<start>,<end>] - Growth: <type>
    // E.G. :
    //     Occurs every 6 months in [2023-12-01,2030-05-13]
    //     Occurs every 1 month in [2023-12-01,2030-05-13]
    Util::PeriodType utilPeriodType = convertPeriodTypeToUtil(period);
    QString periodName = Util::getPeriodName(utilPeriodType, false, periodMultiplier>1);
    l.append(tr("Every %1 %2 in %3").arg(periodMultiplier).arg(periodName).arg(validityRange.toString()));
    switch (growthStrategy) {
        case NONE:
            s = tr("Growth: None");
            break;
        case INFLATION:
            s = tr("Growth: Inflation");
            break;
        case CUSTOM:
            s = tr("Growth: Custom");
            break;
        default:
            break;
    }
    l.append(s);
    return l.join(" - ");
}


Util::PeriodType PeriodicFeStreamDef::convertPeriodTypeToUtil(PeriodicFeStreamDef::PeriodType periodType) const
{
    switch (periodType) {
        case PeriodicFeStreamDef::PeriodType::DAILY:
            return Util::PeriodType::DAILY;
            break;
        case PeriodicFeStreamDef::PeriodType::WEEKLY:
            return Util::PeriodType::WEEKLY;
            break;
        case PeriodicFeStreamDef::PeriodType::MONTHLY:
            return Util::PeriodType::MONTHLY;
            break;
        case PeriodicFeStreamDef::PeriodType::END_OF_MONTHLY:
            return Util::PeriodType::END_OF_MONTHLY;
            break;
        case PeriodicFeStreamDef::PeriodType::YEARLY:
            return Util::PeriodType::YEARLY;
            break;
        default:
            throw std::invalid_argument("Unnown PeriodicFeStreamDef::PeriodType"); // shold never happen
            break;
    }
}


QDate PeriodicFeStreamDef::getNextEventDate(QDate date) const
{
    QDate nextDate;
    switch (this->period) {
    case PeriodType::DAILY:
        return DateHelper::getNextEventDateDaily(date,this->periodMultiplier);
        break;
    case PeriodType::WEEKLY:
        return DateHelper::getNextEventDateWeekly(date,this->periodMultiplier);
        break;
    case PeriodType::MONTHLY:
        return DateHelper::getNextEventDateMonthly(date,this->periodMultiplier);
        break;
    case PeriodType::END_OF_MONTHLY:
        return DateHelper::getNextEventDateEndOfMonth(date,this->periodMultiplier);
        break;
    case PeriodType::YEARLY:
        return DateHelper::getNextEventDateYearly(date,this->periodMultiplier);
        break;
    default:
        throw std::invalid_argument("PeriodicFeStreamDef type does not exist");
        break;
    }
    return nextDate; // dummy
}



// *** GETTERS and SETTERS ***

PeriodicFeStreamDef::PeriodType PeriodicFeStreamDef::getPeriod() const
{
    return period;
}

quint16 PeriodicFeStreamDef::getPeriodMultiplier() const
{
    return periodMultiplier;
}

qint64 PeriodicFeStreamDef::getAmount() const
{
    return amount;
}

Growth PeriodicFeStreamDef::getGrowth() const
{
    return growth;
}

PeriodicFeStreamDef::GrowthStrategy PeriodicFeStreamDef::getGrowthStrategy() const
{
    return growthStrategy;
}

quint16 PeriodicFeStreamDef::getGrowthApplicationPeriod() const
{
    return growthApplicationPeriod;
}

DateRange PeriodicFeStreamDef::getValidityRange() const
{
    return validityRange;
}


