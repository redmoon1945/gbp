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

#include "periodicfestreamdef.h"
#include "datehelper.h"
#include "currencyhelper.h"
// do not #include "gbpcontroller.h" (because this is a core class)
// do not include scenario.h because it is higher in hierarchy
#include <QUuid>

const int PeriodicFeStreamDef::PERIOD_MULTIPLIER_MAX = 365*100; // 100 years of daily occurrences
const int PeriodicFeStreamDef::PERIOD_MULTIPLIER_MIN = 1;
const int PeriodicFeStreamDef::GROWTH_APP_PERIOD_MAX = 100*12;  // once every 100 years
const int PeriodicFeStreamDef::GROWTH_APP_PERIOD_MIN = 1;
const double PeriodicFeStreamDef::MAX_INFLATION_ADJUSTMENT_FACTOR = 100;   // 100x inflation maximum


// not used explicitely in Gbp
PeriodicFeStreamDef::PeriodicFeStreamDef() : FeStreamDef()
{
    this->period = PeriodType(PeriodType::MONTHLY);
    this->periodMultiplier = 1;
    this->amount = 0;
    this->growth = Growth();
    this->growthStrategy = GrowthStrategy::NONE;
    this->growthApplicationPeriod = 1;
    this->startDate = QDate();
    this->endDate = QDate();
    this->useScenarioForEndDate = true;
    this->inflationAdjustmentFactor = 1;
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
    this->startDate = o.startDate;
    this->endDate = o.endDate;
    this->useScenarioForEndDate = o.useScenarioForEndDate;
    this->inflationAdjustmentFactor = o.inflationAdjustmentFactor;
}


PeriodicFeStreamDef::PeriodicFeStreamDef(PeriodicFeStreamDef::PeriodType periodicType,  quint16 periodMultiplier,  qint64 amount,
                                         const Growth &growth, const GrowthStrategy &growthStrategy,  quint16 growthApplicationPeriod, const QUuid &id,
                                         const QString &name, const QString &desc, bool active, bool isIncome, const QColor& decorationColor,
                                         const QDate &startDate, const QDate &endDate, bool useScenarioForEndDate, double inflationAdjustmentFactor)
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
    if (startDate.isValid()==false){
        throw std::invalid_argument("Start Date is invalid");
    }
    if (endDate.isValid()==false){
        throw std::invalid_argument("End Date is invalid");
    }
    if (inflationAdjustmentFactor<0){
        throw std::invalid_argument("inflationAdjustmentFactor must not be negative");
    }
    if (inflationAdjustmentFactor>MAX_INFLATION_ADJUSTMENT_FACTOR){
        throw std::invalid_argument("inflationAdjustmentFactor is too big");
    }
    this->period = periodicType;
    this->periodMultiplier = periodMultiplier;
    this->amount = amount;
    this->growth = growth;
    this->growthStrategy = growthStrategy;
    this->growthApplicationPeriod = growthApplicationPeriod;
    this->startDate = startDate;
    this->endDate = endDate;
    this->useScenarioForEndDate = useScenarioForEndDate;
    this->inflationAdjustmentFactor = inflationAdjustmentFactor;
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
    this->startDate = o.startDate;
    this->endDate = o.endDate;
    this->useScenarioForEndDate = o.useScenarioForEndDate;
    this->inflationAdjustmentFactor = o.inflationAdjustmentFactor;
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
        !(this->startDate == o.startDate) ||
        !(this->endDate == o.endDate) ||
        !(this->useScenarioForEndDate == o.useScenarioForEndDate) ||
        !(this->inflationAdjustmentFactor == o.inflationAdjustmentFactor) ) {
        return false;
    } else {
        return true;
    }
}


PeriodicFeStreamDef::~PeriodicFeStreamDef()
{

}


// Generate the whole suite of financial events for that Stream Definition.
// Input params :
//   fromTo : interval of time inside which the events should be generated. Must be of type BOUNDED.
//   maxDateScenarioFeGeneration : Date where all FE generation must stop (derived from scenario)
//   inflation : scenario's inflation to apply to the events. Internally corrected with the
//               Inflation Adjustment Factor.
//   pvDiscountRate : ANNUAL discount rate in percentage to apply to transform the amounts to
//                    Present Value. Value of 0 means do not transform future values into present
//                    value
//   pvPresent : Define what is the "present" as far as PV conversation is concerned.
//                Note that this date can be afer the first occurrence of events.
// Output params:
//   QList<Fe> : the ordered (by inc. time) list of financial events.
//   uint &saturationCount : number of times the FE amount was over the maximum allowed
//   FeMinMaxInfo& minMaxInfo : computed min/max for values (absolute value, never negative).
//                              Invalid if 0 element returned.
QList<Fe> PeriodicFeStreamDef::generateEventStream(DateRange fromto,
    QDate maxDateScenarioFeGeneration, const Growth &inflation, double pvDiscountRate,
    QDate pvPresent, uint &saturationCount, FeMinMaxInfo& minMaxInfo) const
{
    QList<Fe> ss;
    saturationCount = 0;
    minMaxInfo.yMin =std::numeric_limits<qint64>::max();
    minMaxInfo.yMax = std::numeric_limits<qint64>::min();

    if (fromto.getType()!=DateRange::BOUNDED) {
        throw std::invalid_argument("fromto must be of type BOUNDED");
    }

    // is FeStreamDefinition is inactive, do not generate any Fe
    if (!active)  {
        return ss;
    }

    // Determine the real validity range of this Periodic StreamDef. For the special case where type
    // is END_OF_MONTH, set real start date so that it corresponds to a real end-of-month (since
    // there can be nothing before). For End Date, adjust it so that it does not go over max date
    // allowed by the scenario in any circumstance.
    QDate realEndDate = endDate;
    if (useScenarioForEndDate==true) {
        realEndDate = maxDateScenarioFeGeneration;
    } else {
        realEndDate =
            ( (endDate<maxDateScenarioFeGeneration)?(endDate):(maxDateScenarioFeGeneration) );
    }
    QDate realStartDate = startDate;
    if (period==END_OF_MONTHLY) {
        if (false==DateHelper::isEndOfMonth(startDate)){
            // First event must be at end-of-month
            QDate nextStart = getNextEventDate(startDate);
            // Are we past "real end date" ?
            if (nextStart > realEndDate) {
                // so there could be no event : return
                return ss;
            } else if (nextStart > maxDateScenarioFeGeneration) {
                    // we are already past the max scenario date
                return ss;
            } else {
                realStartDate = nextStart;
            }
        }
    }
    if (realEndDate < realStartDate) {
        return ss; // nothing to generate
    }
    DateRange realValidityRange = DateRange(realStartDate, realEndDate);

    // there must be some intersection
    if (false == fromto.intersectWith(realValidityRange)) {
        return ss;
    }

    // ** STEP 1 : Generate the flow of dates ***
    //    So starting from realValidityRange.Start, advance till fromto.End or realValidityrange.End
    //    is met.  At this stage, we need all the occurrences from realValidityrange.start in order
    //    to calculate Growth in the next stage. Events before fromto.start will be removed in the
    //    last stage.
    QList<QDate> occurrenceDates;
    QDate aDate = realValidityRange.getStart();
    while(true){
        if (!realValidityRange.includeDate(aDate) ){ // check if still in the validity range
            break;
        }
        if (aDate > fromto.getEnd()){                // check if we have gone over the fromTo End
            break;
        }
        occurrenceDates.append(aDate);
        aDate = getNextEventDate(aDate);             // advance to the next event
    }

    // ** Step 2 : Correct for growth or inflation (not both) ***
    //    We need occurrence from start of validation, to calculate growth, even if it is before
    //    fromto.start. For now, keep amount positive even if this is an expense.
    //    Also transform future values into present values, if requested (pvDiscountRate > 0).
    qint64 am = this->amount;
    QMap<QDate,qint64> adjustedAmounts; // this will hold the resulting FE amounts
    Growth::ApplicationStrategy appStrategy = {.noOfMonths=growthApplicationPeriod};
    Growth::AdjustForGrowthResult afgResult;
    Growth noGrowth = Growth();
    Growth adjustedInflation = inflation;
        switch (growthStrategy) {
        case GrowthStrategy::NONE:
            adjustedAmounts = noGrowth.adjustForGrowth(am, occurrenceDates, appStrategy,
                pvDiscountRate, pvPresent, afgResult);
            saturationCount = afgResult.saturationCount;
            if(afgResult.success==false){   // should not happen
                return ss;
            }
            break;
        case GrowthStrategy::INFLATION:
            // adjust inflation value
            bool capped;
            if(inflationAdjustmentFactor != 1){
                adjustedInflation.changeByFactor(inflationAdjustmentFactor,capped);
            }
            // calculate
            adjustedAmounts = adjustedInflation.adjustForGrowth(am, occurrenceDates, appStrategy,
                pvDiscountRate, pvPresent,afgResult);
            saturationCount = afgResult.saturationCount;
            if(afgResult.success==false){   // should not happen
                return ss;
            }
            break;
        case GrowthStrategy::CUSTOM:
            adjustedAmounts = growth.adjustForGrowth(am, occurrenceDates, appStrategy,
                pvDiscountRate, pvPresent,afgResult);
            saturationCount = afgResult.saturationCount;
            if(afgResult.success==false){   // should not happen
                return ss;
            }
            break;
        default:
            break;
    }

    // *** Step 3 : build QList<Fe> result ***
    // Remove dates occuring before fromto.start(), now that growth has been calculated
    // Make amount negative if this is an expense.
    // Compute min/max values.
    foreach(QDate aDate, adjustedAmounts.keys()){
        if( aDate >= fromto.getStart() ){
            qint64 am = (isIncome?(adjustedAmounts.value(aDate)):-(adjustedAmounts.value(aDate)));
            Fe fe = {.amount=am, .occurrence=aDate,.id=this->getId()};
            ss.append(fe);
            // set min max for amount
            if( abs(am) > minMaxInfo.yMax){
                minMaxInfo.yMax = abs(am);
            }
            if( abs(am) < minMaxInfo.yMin){
                minMaxInfo.yMin = abs(am);
            }
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
    jobject["StartDate"] = startDate.toString(Qt::ISODate);
    jobject["EndDate"] = endDate.toString(Qt::ISODate);
    jobject["UseScenarioForEndDate"] = useScenarioForEndDate;
    jobject["InflationAdjustmentFactor"] = inflationAdjustmentFactor;
    // return result
    return jobject;
}



PeriodicFeStreamDef PeriodicFeStreamDef::fromJson(const QJsonObject &jsonObject,
    Util::OperationResult &result)
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
    FeStreamDef::fromJson(jsonObject, PERIODIC, id, name, desc, active, isIncome, decoColor,
        resultBaseClass);
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
        result.errorStringUI = tr("PeriodicFeStreamDef - PeriodType value %1 is not an integer").
            arg(d);
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

    // validity range. Support version 1, which has the info stored as a DateRange (not anymore in V2).
    QDate sDate,eDate;
    bool useScenarioDef;
    jsonValue = jsonObject.value("StartDate");
    if (jsonValue == QJsonValue::Undefined){
        // could be because this is version 1 : lets check
        QJsonValue jsonValueV1 = jsonObject.value("ValidityRange");
        if (jsonValueV1 == QJsonValue::Undefined){
            // ok, format is invalid
            result.errorStringUI = tr("PeriodicFeStreamDef - Validity Range : Cannot find tag for V1 or V2");
            result.errorStringLog = "PeriodicFeStreamDef - Validity Range : Cannot find tag for V1 or V2";
            return ps;
        }
        if (jsonValueV1.isObject()==false){
            result.errorStringUI = tr("PeriodicFeStreamDef - ValidityRange tag is not an object");
            result.errorStringLog = "PeriodicFeStreamDef - ValidityRange tag is not an object";
            return ps;
        }
        Util::OperationResult validityParsingResult;
        DateRange validity = DateRange::fromJson(jsonValueV1.toObject(),validityParsingResult);
        if (validityParsingResult.success==false){
            result.errorStringUI =  tr("PeriodicFeStreamDef - ")+validityParsingResult.errorStringUI;
            result.errorStringLog =  "PeriodicFeStreamDef - "+validityParsingResult.errorStringLog;
            return ps;
        }
        // convert
        useScenarioDef = false;
        sDate = validity.getStart();
        eDate = validity.getEnd();
    } else {
        // This is VERSION 2
        // Start Date
        if (jsonValue.isString()==false){
            result.errorStringUI = tr("StartDate - tag is not a string");
            result.errorStringLog = "DateRange - tag is not a string";
            return ps;
        }
        sDate = QDate::fromString(jsonValue.toString(),Qt::ISODate);
        if( !(sDate.isValid()) ) {
            result.errorStringUI = tr("Start Date - value %1 is not a valid ISO Date").arg(jsonValue.toString());
            result.errorStringLog = QString("Start Date - value %1 is not a valid ISO Date").arg(jsonValue.toString());
            return ps;
        }
        // End Date
        jsonValue = jsonObject.value("EndDate");
        if (jsonValue == QJsonValue::Undefined){
            result.errorStringUI = tr("EndDate - Cannot find tag");
            result.errorStringLog = "EndDate - Cannot find tag";
            return ps;
        }
        if (jsonValue.isString()==false){
            result.errorStringUI = tr("EndDate - tag is not a string");
            result.errorStringLog = "EndDate - tag is not a string";
            return ps;
        }
        eDate = QDate::fromString(jsonValue.toString(),Qt::ISODate);
        if( !(eDate.isValid()) ) {
            result.errorStringUI = tr("EndDate - value %1 is not a valid ISO Date").arg(jsonValue.toString());
            result.errorStringLog = QString("EndDate - value %1 is not a valid ISO Date").arg(jsonValue.toString());
            return ps;
        }
        // Use Scenario Definition of End Date
        jsonValue = jsonObject.value("UseScenarioForEndDate");
        if (jsonValue == QJsonValue::Undefined){
            result.errorStringUI = tr("Cannot find UseScenarioForEndDate tag");
            result.errorStringLog = QString("Cannot find UseScenarioForEndDate tag");
            return ps;
        }
        if (jsonValue.isBool()==false){
            result.errorStringUI = tr("UseScenarioForEndDate tag is not a boolean");
            result.errorStringLog = QString("UseScenarioForEndDate tag is not a boolean");
            return ps;
        }
        useScenarioDef = jsonValue.toBool();
        // finally, if UseScenarioForEndDate==false, maeke sure to is not smaller than from
        if( (useScenarioDef==false) && (eDate<sDate) ){
            result.errorStringUI = tr("End Date occur before Start Date");
            result.errorStringLog = QString("End Date occur before Start Date");
            return ps;
        }
    }

    // Inflation Adjustment Factor : to keep compatibility with older version of config file,
    // if not found, set to 1
    double infAdjFactor = 1;
    jsonValue = jsonObject.value("InflationAdjustmentFactor");
    if (jsonValue != QJsonValue::Undefined){
        if (jsonValue.isDouble()==false){
            result.errorStringUI = tr("PeriodicFeStreamDef - InflationAdjustmentFactor tag is not a number");
            result.errorStringLog = "PeriodicFeStreamDef - InflationAdjustmentFactor tag is not a number";
            return ps;
        }
        infAdjFactor = jsonValue.toDouble();
        if ( (infAdjFactor>PeriodicFeStreamDef::MAX_INFLATION_ADJUSTMENT_FACTOR) || (infAdjFactor<0) ){
            result.errorStringUI = tr("PeriodicFeStreamDef - InflationAdjustmentFactor value %1 is either smaller than 0 or too big").arg(d);
            result.errorStringLog = QString("PeriodicFeStreamDef - InflationAdjustmentFactor value %1 is either smaller than 0 or too big").arg(d);
            return ps;
        }
        jsonValue = jsonObject.value("EndDate");


    }


    // *** build and return ***
    result.success = true;
    return PeriodicFeStreamDef(
        periodType, periodMultiplier, amount, growth, gs, gap,id, name, desc,active, isIncome, decoColor, sDate, eDate, useScenarioDef, infAdjFactor);
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
    // *** order is : Every N <period> in [<start>,<end>] - Growth: <type>
    // *** E.G. :
    // ***     Occurs every 6 months in [2023-12-01,2030-05-13]
    // ***     Occurs every 1 month in [2023-12-01,2030-05-13]
    QStringList l;
    QString s;
    Util::PeriodType utilPeriodType = convertPeriodTypeToUtil(period);
    QString periodName = Util::getPeriodName(utilPeriodType, false, periodMultiplier>1);
    QString valRangeString;
    if (useScenarioForEndDate==true) {
        QString endString = tr("Scenario Defined");
        valRangeString = tr("Every %1 %2 in [%3,%4]").arg(periodMultiplier).arg(periodName).arg(startDate.toString(Qt::ISODate)).arg(endString);
    } else {
        valRangeString = tr("Every %1 %2 in [%3,%4]").arg(periodMultiplier).arg(periodName).arg(startDate.toString(Qt::ISODate)).arg(endDate.toString(Qt::ISODate));
    }
    l.append(valRangeString);

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

double PeriodicFeStreamDef::getInflationAdjustmentFactor() const
{
    return inflationAdjustmentFactor;
}

QDate PeriodicFeStreamDef::getStartDate() const
{
    return startDate;
}

QDate PeriodicFeStreamDef::getEndDate() const
{
    return endDate;
}

bool PeriodicFeStreamDef::getUseScenarioForEndDate() const
{
    return useScenarioForEndDate;
}


