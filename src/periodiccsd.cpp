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

#include "periodiccsd.h"
#include "datehelper.h"
#include "currencyhelper.h"
#include "gbplogger.h"
// do not #include "gbpcontroller.h" (because this is a core class)
// do not include scenario.h because it is higher in hierarchy
#include <QUuid>

const int PeriodicCsd::PERIOD_MULTIPLIER_MAX = 365*100; // 100 years of daily occurrences
const int PeriodicCsd::PERIOD_MULTIPLIER_MIN = 1;
const int PeriodicCsd::GROWTH_APP_PERIOD_MAX = 100*12;  // once every 100 years
const int PeriodicCsd::GROWTH_APP_PERIOD_MIN = 1;
const double PeriodicCsd::MAX_INFLATION_ADJUSTMENT_FACTOR = 100;   // 100x inflation maximum
const QDate PeriodicCsd::MIN_START_DATE = QDate(2000,1,1);

PeriodicCsd::PeriodicCsd() : Csd()
{
    this->period = PeriodType(PeriodType::MONTHLY);
    this->periodMultiplier = 1;
    this->amount = 0;
    this->growth = Growth();
    this->growthStrategy = GrowthStrategy::NONE;
    this->growthApplicationPeriod = 1;
    this->startDate = QDate::currentDate();
    this->endDate = startDate.addYears(1); // dummy value
    this->useScenarioForEndDate = true; // Dynamic End date by default.
    this->inflationAdjustmentFactor = 1;
}


PeriodicCsd::PeriodicCsd(const PeriodicCsd &o) :
    Csd(o)
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


PeriodicCsd::PeriodicCsd(PeriodicCsd::PeriodType periodicType,  quint16 periodMultiplier,
    quint64 amount, const Growth &growth, const GrowthStrategy &growthStrategy,
    quint16 growthApplicationPeriod, const QUuid &id, const QString &name, const QString &desc,
    bool active, bool isIncome, const QColor& decorationColor, const QDate &startDate,
    const QDate &endDate, bool useScenarioForEndDate, double inflationAdjustmentFactor)
    : Csd(id, name,desc,CsdType::PERIODIC,active,isIncome, decorationColor)
{
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
        throw std::invalid_argument("Start date is invalid");
    }
    if (startDate < MIN_START_DATE){
        throw std::invalid_argument("Start date occurs before the minimum date authorized");
    }
    if (endDate.isValid()==false){
        throw std::invalid_argument("End date is invalid");
    }
    if (endDate < MIN_START_DATE.addDays(1)){
        throw std::invalid_argument("End date occurs before the minimum date authorized");
    }
    if( (useScenarioForEndDate==false) && (startDate > endDate) ){
        throw std::invalid_argument("Start date must not occur after end date when not using "
            "scenario's end date");
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


PeriodicCsd &PeriodicCsd::operator=(const PeriodicCsd &o)
{
    if (this != &o){    // to protect against self-assignment
        Csd::operator=(o);
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
    return *this;
}


bool PeriodicCsd::operator==(const PeriodicCsd& o) const
{
    if ( !(Csd::operator==(o)) ||
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


bool PeriodicCsd::operator!=(const PeriodicCsd &o) const
{
    return !(*this==o);
}


PeriodicCsd::~PeriodicCsd()
{

}


void PeriodicCsd::generateEventStream(FeStream& feStream, QDate tomorrow, DateRange fromto,
    QDate maxDateScenarioFeGeneration, const Growth &inflation, double pvDiscountRate,
    QDate pvPresent, uint &saturationCount, FeMinMaxInfo& minMaxInfo) const
{
    if (fromto.getType()!=DateRange::Type::BOUNDED) {
        throw std::invalid_argument("fromto must be of type BOUNDED");
    }

    // Reset the content of the FeStream
    feStream.reset();

    saturationCount = 0;

    // is Csd is inactive, do not generate any Fe
    if (!active)  {
        return;
    }

    // Determine the effective validity range of this Periodic Csd, which is
    // "adjusted" start and end date.
    // => Start is advanced when type is END_OF_MONTH, so that it corresponds to a real
    //    end-of-month.
    // => End is adjusted so that it does not go over max date allowed by the scenario
    //    in any circumstance.
    QDate realEndDate = getRealEndDate(maxDateScenarioFeGeneration);
    QDate realStartDate = startDate;
    if (realEndDate < realStartDate) {
        // This can happen in the case of "dynamic" end date for periodic csd,
        // if scenario's max duration is decreased a lot. This is not an error.
        return ; // nothing to generate
    }
    if (period==PeriodType::END_OF_MONTHLY) {
        if (false==DateHelper::isEndOfMonth(startDate)){
            // First event must be at end-of-month
            QDate nextStart = getNextEventDate(startDate);
            // Are we past "real end date" ?
            if (nextStart > realEndDate) {
                // so there could be no event : return
                return ;
            } else {
                realStartDate = nextStart;
            }
        }
    }
    DateRange realValidityRange = DateRange(realStartDate, realEndDate);

    // there must be some intersection
    if (false == fromto.intersectWith(realValidityRange)) {
        return ;
    }

    // ** STEP 1 : Generate the flow of dates ***
    //    So starting from realValidityRange.Start, advance till fromto.End or realValidityrange.End
    //    is met.  At this stage, we need all the occurrences from realValidityrange.start in order
    //    to calculate Growth from the very beginning. Start date can occur BEFORE tomorrow,
    //    but not smaller than MIN_START_DATE. Events before fromto.start or before
    //    tomorrow will be REMOVED in the last stage.
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

    // ** Step 2 : Correct for growth (custom or inflation) ***
    //    Growth is calculated from realValidityRange.start, even if it is before
    //    fromto.start or tomorrow. For now, keep amount positive even if this is an expense.
    //    Also transform future values into present values, if requested (pvDiscountRate > 0).
    quint64 am = this->amount;
    QList<quint64> adjustedAmounts; // this will hold the resulting FE amounts
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
                return ;
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
                return ;
            }
            break;
        case GrowthStrategy::CUSTOM:
            adjustedAmounts = growth.adjustForGrowth(am, occurrenceDates, appStrategy,
                pvDiscountRate, pvPresent,afgResult);
            saturationCount = afgResult.saturationCount;
            if(afgResult.success==false){   // should not happen
                return ;
            }
            break;
        default:
            break;
    }

    // *** Step 3 : Calculate min/max and build result ***
    // Remove dates occuring before fromto.start() or before tomorrow, now that growth
    // has been calculated
    minMaxInfo.yMin =std::numeric_limits<qint64>::max();
    minMaxInfo.yMax = std::numeric_limits<qint64>::min();
    qint64 tomorrowJulianDays = tomorrow.toJulianDay();
    QDate fromStart = (fromto.getStart()<tomorrow)?(tomorrow):(fromto.getStart());
    int s = occurrenceDates.size();
    for (int i=0; i<s; i++) {
        if( occurrenceDates[i] >= fromStart ){
            // Set value for the proper day
            feStream.set(occurrenceDates[i].toJulianDay()-tomorrowJulianDays, adjustedAmounts[i]);
            // set min max for amount
            if( adjustedAmounts[i] > minMaxInfo.yMax){
                minMaxInfo.yMax = adjustedAmounts[i];
            }
            if( adjustedAmounts[i] < minMaxInfo.yMin){
                minMaxInfo.yMin = adjustedAmounts[i];
            }
        }
    }

    return ;
}


bool PeriodicCsd::evaluateIfSameFeList(const PeriodicCsd &o, QString& diff) const
{
    diff = "";

    if (Csd::evaluateIfSameFeList(o, diff)==false){
        return false;
    }

    if ( period != o.period ){
        diff = QString("Periodic Csd %1 : Period Type are different : %2 vs %3")
            .arg(REDACT(name)).arg(periodTypeToInt(period)).arg(periodTypeToInt(o.period));
        return false;
    }

    if ( periodMultiplier != o.periodMultiplier ){
        diff = QString("Periodic Csd %1 : Period multiplier are different : %2 vs %3")
        .arg(REDACT(name)).arg(periodMultiplier).arg(o.periodMultiplier);
        return false;
    }

    if ( amount != o.amount ){
        diff = QString("Periodic Csd %1 : Amounts are different : %2 vs %3")
        .arg(REDACT(name)).arg(amount).arg(o.amount);
        return false;
    }

    if ( growthStrategy != o.growthStrategy ){
        diff = QString("Periodic Csd %1 : Growth strategies are different : %2 vs %3")
            .arg(REDACT(name)).arg(growthStrategyToInt(growthStrategy)).arg(growthStrategyToInt(o.growthStrategy));
        return false;
    }

    if ( startDate != o.startDate ) {
        diff = QString("Periodic Csd %1 : Start dates are different : %2 vs %3")
            .arg(name).arg(startDate.toString()).arg(o.startDate.toString());
        return false;
    }

    if ( useScenarioForEndDate != o.useScenarioForEndDate ) {
        diff = QString("Periodic Csd %1 : useScenarioForEndDate are different : %2 vs %3")
            .arg(REDACT(name)).arg(useScenarioForEndDate).arg(o.useScenarioForEndDate);
        return false;
    }

    if ( (growthStrategy == GrowthStrategy::CUSTOM) &&
        (growth != o.growth) ) {
        diff = QString("Periodic Csd %1 :  Custom growths are different").arg(REDACT(name));
        return false;
    }

    if ( (growthStrategy != GrowthStrategy::NONE) &&
        (growthApplicationPeriod != o.growthApplicationPeriod) ) {
        diff = QString("Periodic Csd %1 : growthApplicationPeriods are different : %2 vs %3")
            .arg(REDACT(name)).arg(growthApplicationPeriod).arg(o.growthApplicationPeriod);
        return false;
    }

    if ( (growthStrategy==GrowthStrategy::INFLATION) &&
        (inflationAdjustmentFactor != o.inflationAdjustmentFactor) )  {
        diff = QString("Periodic Csd %1 : inflationAdjustmentFactors are different : %2 vs %3")
            .arg(REDACT(name)).arg(inflationAdjustmentFactor).arg(o.inflationAdjustmentFactor);
        return false;
    }

    if ( (useScenarioForEndDate == false) &&
        (endDate != o.endDate) ) {
        diff = QString("Periodic Csd %1 : endDates are different : %2 vs %3")
            .arg(REDACT(name)).arg(endDate.toString()).arg(o.endDate.toString());
        return false;
    }

    return true;
}


QJsonObject PeriodicCsd::toJson() const
{
    QJsonObject jobject;

    // base class data
    Csd::toJson(jobject);
    // this derived class data
    jobject["PeriodType"] = periodTypeToInt(period);
    jobject["PeriodMultiplier"] = periodMultiplier;
    jobject["Amount"] = QVariant::fromValue(amount).toJsonValue();
    jobject["Growth"] = growth.toJson();
    jobject["GrowthStrategy"] = growthStrategyToInt(growthStrategy);
    jobject["GrowthApplicationPeriod"] = growthApplicationPeriod;
    jobject["StartDate"] = startDate.toString(Qt::ISODate);
    jobject["EndDate"] = endDate.toString(Qt::ISODate);
    jobject["UseScenarioForEndDate"] = useScenarioForEndDate;
    jobject["InflationAdjustmentFactor"] = inflationAdjustmentFactor;
    // return result
    return jobject;
}



QSharedPointer<PeriodicCsd> PeriodicCsd::fromJson(const QJsonObject &jsonObject,
    Util::ResultOfOperation &result)
{
    QJsonValue jsonValue;
    int convResult;
    double d;
    QSharedPointer<PeriodicCsd> nullResult; // Null by default

    // Reset result to ERROR
    result.init();

    // *** BASE CLASS DATA ***
    QUuid id;
    QString name,desc;
    bool active;
    bool isIncome;
    QColor decoColor;
    Util::ResultOfOperation resultBaseClass;
    Csd::fromJson(jsonObject, Csd::CsdType::PERIODIC, id, name, desc, active, isIncome, decoColor,
        resultBaseClass);
    if (resultBaseClass.status==Util::ResultOfOperationStatus::ERROR){
        result.logErrorMessage = QString("PeriodicCsd -> %1").arg(resultBaseClass.logErrorMessage);
        return nullResult;
    }

    // *** DERIVE CLASS DATA ***
    // * Period Type *
    jsonValue = jsonObject.value("PeriodType");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = QString("PeriodicCsd %1: Cannot find token \"PeriodType\"")
            .arg(id.toString(QUuid::WithoutBraces));
        return nullResult;
    }
    if (jsonValue.isDouble()==false){
        result.logErrorMessage = QString("PeriodicCsd %1: PeriodType token is not a number")
            .arg(id.toString(QUuid::WithoutBraces));
        return nullResult;
    }
    d = jsonValue.toDouble();
    qint64 pType = Util::extractQint64FromDoubleWithNoFractionalPart(d,convResult);
    if ( convResult !=0  ){
        result.logErrorMessage = QString("PeriodicCsd %1: PeriodType value \"%2\" is not a valid "
            "integer (code=%3)")
            .arg(id.toString(QUuid::WithoutBraces)).arg(d).arg(convResult);
        return nullResult;
    }
    PeriodType periodType;
    if (false==intToPeriodType(pType,periodType)) {
        result.logErrorMessage = QString("PeriodicCsd %1: PeriodType value \"%2\" is an illegal "
            "value")
            .arg(id.toString(QUuid::WithoutBraces)).arg(pType);
        return nullResult;
    }

    // * Period Multiplier *
    jsonValue = jsonObject.value("PeriodMultiplier");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = QString("PeriodicCsd %1: Cannot find token \"PeriodMultiplier\"")
            .arg(id.toString(QUuid::WithoutBraces));
        return nullResult;
    }
    if (jsonValue.isDouble()==false){
        result.logErrorMessage = QString("PeriodicCsd %1: PeriodMultiplier token is not a number")
            .arg(id.toString(QUuid::WithoutBraces));
        return nullResult;
    }
    d = jsonValue.toDouble();
    qint64 pm = Util::extractQint64FromDoubleWithNoFractionalPart(d,convResult);
    if ( convResult != 0 ){
        result.logErrorMessage = QString("PeriodicCsd %1: PeriodMultiplier value \"%2\" is not"
            " a valid integer (code=%3)")
            .arg(id.toString(QUuid::WithoutBraces)).arg(d).arg(convResult);
        return nullResult;
    }
    if ( pm<PERIOD_MULTIPLIER_MIN ){
        result.logErrorMessage = QString("PeriodicCsd %1: PeriodMultiplier value \"%2\" is too"
            " small").arg(id.toString(QUuid::WithoutBraces)).arg(d);
        return nullResult;
    }
    if ( pm>PERIOD_MULTIPLIER_MAX ){
        result.logErrorMessage = QString("PeriodicCsd %1: PeriodMultiplier value \"%2\" is too big")
            .arg(id.toString(QUuid::WithoutBraces)).arg(d);
        return nullResult;
    }
    quint16 periodMultiplier = static_cast<quint16>(pm);

    // * Amount *
    jsonValue = jsonObject.value("Amount");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = QString("PeriodicCsd %1: Cannot find token \"Amount\"")
            .arg(id.toString(QUuid::WithoutBraces));
        return nullResult;
    }
    if (jsonValue.isDouble()==false){
        result.logErrorMessage = QString("PeriodicCsd %1: Amount token is not a number")
            .arg(id.toString(QUuid::WithoutBraces));
        return nullResult;
    }
    d = jsonValue.toDouble();
    qint64 amountInitial = Util::extractQint64FromDoubleWithNoFractionalPart(d,convResult);
    if ( convResult != 0 ){
        result.logErrorMessage = QString("PeriodicCsd %1: Amount value \"%2\" is not a valid "
            "integer (code=%3)")
            .arg(id.toString(QUuid::WithoutBraces)).arg(d).arg(convResult);
        return nullResult;
    }
    if ( amountInitial < 0 ){
        result.logErrorMessage = QString("PeriodicCsd %1: Amount value %2 is negative")
            .arg(id.toString(QUuid::WithoutBraces)).arg(d);
        return nullResult;
    }
    quint64 amount = static_cast<quint64>(amountInitial);
    const quint64 zMax = CurrencyHelper::maxValueAllowedForAmount();
    if ( amount > zMax ){
        result.logErrorMessage = QString("PeriodicCsd %1: Amount value %2 is too big")
            .arg(id.toString(QUuid::WithoutBraces)).arg(d);
        return nullResult;
    }

    // * Growth *
    jsonValue = jsonObject.value("Growth");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = QString("PeriodicCsd %1: Cannot find token \"Growth\"")
            .arg(id.toString(QUuid::WithoutBraces));
        return nullResult;
    }
    if (jsonValue.isObject()==false){
        result.logErrorMessage = QString("PeriodicCsd %1: Growth token is not an object")
            .arg(id.toString(QUuid::WithoutBraces));
        return nullResult;
    }
    Util::ResultOfOperation growParsingResult;
    Growth growth = Growth::fromJson(jsonValue.toObject(), growParsingResult);
    if (growParsingResult.status==Util::ResultOfOperationStatus::ERROR){
        result.logErrorMessage = QString("PeriodicCsd %1 -> %2")
            .arg(id.toString(QUuid::WithoutBraces)).arg(growParsingResult.logErrorMessage);
        return nullResult;
    }

    // * Growth Strategy *
    jsonValue = jsonObject.value("GrowthStrategy");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = QString("PeriodicCsd %1: Cannot find token \"GrowthStrategy\"")
            .arg(id.toString(QUuid::WithoutBraces));
        return nullResult;
    }
    if (jsonValue.isDouble()==false){
        result.logErrorMessage = QString("PeriodicCsd %1: GrowthStrategy token %2 is not a number")
            .arg(id.toString(QUuid::WithoutBraces)).arg(jsonValue.toString());
        return nullResult;
    }
    d = jsonValue.toDouble();
    qint64 gsType = Util::extractQint64FromDoubleWithNoFractionalPart(d,convResult);
    if ( convResult != 0 ){
        result.logErrorMessage = QString("PeriodicCsd %1: GrowthStrategy value \"%2\" is not a "
            "valid integer (code=%3)")
            .arg(id.toString(QUuid::WithoutBraces)).arg(d).arg(convResult);
        return nullResult;
    }
    GrowthStrategy gs;
    if (false==intToGrowthStrategy(d,gs)) {
        result.logErrorMessage = QString("PeriodicCsd %1: Growth Strategy value \"%2\" is "
            "an illegal value")
            .arg(id.toString(QUuid::WithoutBraces)).arg(gsType);
        return nullResult;
    }

    // * GrowthApplicationPeriod *
    jsonValue = jsonObject.value("GrowthApplicationPeriod");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = QString("PeriodicCsd %1: Cannot find token"
            " \"GrowthApplicationPeriod\"").arg(id.toString(QUuid::WithoutBraces));
        return nullResult;
    }
    if (jsonValue.isDouble()==false){
        result.logErrorMessage = QString("PeriodicCsd %1: GrowthApplicationPeriod token \"%2\" is "
            "not a number").arg(id.toString(QUuid::WithoutBraces)).arg(jsonValue.toString());
        return nullResult;
    }
    d = jsonValue.toDouble();
    qint64 gapOrig = Util::extractQint64FromDoubleWithNoFractionalPart(d,convResult);
    if ( convResult != 0 ){
        result.logErrorMessage = QString("PeriodicCsd %1: GrowthApplicationPeriod value \"%2\" is"
            " not a valid integer (code=%3)")
            .arg(id.toString(QUuid::WithoutBraces)).arg(d).arg(convResult);
        return nullResult;
    }
    if ( gapOrig < GROWTH_APP_PERIOD_MIN ){
        result.logErrorMessage = QString("PeriodicCsd %1: GrowthApplicationPeriod value %2"
            " is either too small ").arg(id.toString(QUuid::WithoutBraces)).arg(d);
        return nullResult;
    }
    if (  gapOrig > GROWTH_APP_PERIOD_MAX ){
        result.logErrorMessage = QString("PeriodicCsd %1: GrowthApplicationPeriod value %2"
            " is too big").arg(id.toString(QUuid::WithoutBraces)).arg(d);
        return nullResult;
    }
    quint16 gap = static_cast<quint16>(gapOrig);

    // * Start and End date (or validity range for V1) *
    // Support version 1 of scenario file, which has the info stored as a DateRange
    // (not anymore in V2).
    // Minimum Start Date will be forced to MIN_START_DATE.
    // Minimum End Date will be forced to MIN_START_DATE.addDays(1).
    QDate sDate,eDate;
    bool useScenarioDef;
    jsonValue = jsonObject.value("StartDate"); // only V2 has this
    if (jsonValue == QJsonValue::Undefined){
        // could be because this is version 1 : lets check
        QJsonValue jsonValueV1 = jsonObject.value("ValidityRange");
        if (jsonValueV1 == QJsonValue::Undefined){
            // ok, not version 1 nor 2, so format is invalid
            result.logErrorMessage = QString("PeriodicCsd %1: Validity Range : Cannot find token"
                " for V1 or V2").arg(id.toString(QUuid::WithoutBraces));
            return nullResult;
        }
        if (jsonValueV1.isObject()==false){
            result.logErrorMessage = QString("PeriodicCsd %1: ValidityRange token is not an object")
                .arg(id.toString(QUuid::WithoutBraces));
            return nullResult;
        }
        // VERSION 1 confirmed
        Util::ResultOfOperation validityParsingResult;
        DateRange validity = DateRange::fromJson(jsonValueV1.toObject(),validityParsingResult);
        if (validityParsingResult.status==Util::ResultOfOperationStatus::ERROR){
            result.logErrorMessage =  QString("PeriodicCsd %1 -> %2")
                .arg(id.toString(QUuid::WithoutBraces))
                .arg(validityParsingResult.logErrorMessage);
            return nullResult;
        }
        // convert
        useScenarioDef = false;
        sDate = validity.getStart();
        if (sDate < MIN_START_DATE) {
            // 1.6.3 and before have no constraint for Start Date. Now we have.
            // So adjust silently the start date to MIN_START_DATE if sDate < MIN_START_DATE.
            // It is estimated that the chance user may have set a date lower than MIN_START_DATE
            // is very low.
            sDate = MIN_START_DATE;
        }
        eDate = validity.getEnd();
        if (eDate < MIN_START_DATE.addDays(1)) {
            // See comment for sDate
            eDate = MIN_START_DATE.addDays(1);
        }
    } else {
        // This is confirmed VERSION 2
        // Get Start Date
        if (jsonValue.isString()==false){
            result.logErrorMessage = QString("PeriodicCsd %1: Token StartDate is not a string")
                .arg(id.toString(QUuid::WithoutBraces));
            return nullResult;
        }
        bool validDate;
        sDate = Util::isValidISO8601Date(jsonValue.toString(), validDate);
        if( validDate==false ) {
            result.logErrorMessage = QString("PeriodicCsd %1: Start Date value \"%2\" is not"
                " a valid ISO Date or is invalid")
                .arg(id.toString(QUuid::WithoutBraces))
                .arg(jsonValue.toString());
            return nullResult;
        }
        if (sDate < MIN_START_DATE) {
            // 1.6.3 and before have no constraint for Start Date. Now we have.
            // So adjust silently the start date to MIN_START_DATE if sDate < MIN_START_DATE.
            // It is estimated that the chance user may have set a date lower than MIN_START_DATE
            // is very low.
            sDate = MIN_START_DATE;
        }
        // End Date
        jsonValue = jsonObject.value("EndDate");
        if (jsonValue == QJsonValue::Undefined){
            result.logErrorMessage = QString("PeriodicCsd %1: Token \"EndDate\" cannot be found")
                .arg(id.toString(QUuid::WithoutBraces));
            return nullResult;
        }
        if (jsonValue.isString()==false){
            result.logErrorMessage = QString("PeriodicCsd %1: EndDate token is not a string")
                .arg(id.toString(QUuid::WithoutBraces));
            return nullResult;
        }
        eDate = Util::isValidISO8601Date(jsonValue.toString(), validDate);
        if( validDate==false ) {
            result.logErrorMessage = QString("PeriodicCsd %1: EndDate value \"%2\" is not a "
                "valid ISO Date or is invalid")
                .arg(id.toString(QUuid::WithoutBraces))
                .arg(jsonValue.toString());
            return nullResult;
        }
        if (eDate < MIN_START_DATE.addDays(1)) {
            // See comment for sDate.
            eDate = MIN_START_DATE.addDays(1);
        }
        // Use Scenario Definition of End Date
        jsonValue = jsonObject.value("UseScenarioForEndDate");
        if (jsonValue == QJsonValue::Undefined){
            result.logErrorMessage = QString("PeriodicCsd %1: Cannot find token "
                "UseScenarioForEndDate").arg(id.toString(QUuid::WithoutBraces));
            return nullResult;
        }
        if (jsonValue.isBool()==false){
            result.logErrorMessage = QString("PeriodicCsd %1: UseScenarioForEndDate token is not "
                "a boolean").arg(id.toString(QUuid::WithoutBraces));
            return nullResult;
        }
        useScenarioDef = jsonValue.toBool();

        // If UseScenarioForEndDate==true, it may happen that the effective end date get smaller
        // than start date. This is ok, it is not an error. Date of Today and/or
        // feGenerationDuration may have changed a lot after the creation of this CSD.
        // However, if UseScenarioForEndDate==false, then it  simpertive than eDate>=sDate
        if( (useScenarioDef==false) && (eDate<sDate) ){
            result.logErrorMessage = QString("PeriodicCsd %1: End Date %2 occurs before Start "
                "Date %3")
            .arg(id.toString(QUuid::WithoutBraces))
            .arg(eDate.toString(Qt::ISODate))
            .arg(sDate.toString(Qt::ISODate));
            return nullResult;
        }
    }

    // Inflation Adjustment Factor : to keep compatibility with older version of config file,
    // if not found, set to 1
    double infAdjFactor = 1;
    jsonValue = jsonObject.value("InflationAdjustmentFactor");
    if (jsonValue != QJsonValue::Undefined){
        if (jsonValue.isDouble()==false){
            result.logErrorMessage = QString("PeriodicCsd %1: InflationAdjustmentFactor token is"
                " not a number").arg(id.toString(QUuid::WithoutBraces));
            return nullResult;
        }
        infAdjFactor = jsonValue.toDouble();
        if ( (infAdjFactor>PeriodicCsd::MAX_INFLATION_ADJUSTMENT_FACTOR) || (infAdjFactor<0) ){
            result.logErrorMessage = QString("PeriodicCsd %1: InflationAdjustmentFactor "
                "value \"%2\" is either smaller than 0 or too big")
                .arg(id.toString(QUuid::WithoutBraces))
                .arg(d);
            return nullResult;
        }
        jsonValue = jsonObject.value("EndDate");
    }


    // *** build and return ***
    result.status = Util::ResultOfOperationStatus::SUCCESS;
    PeriodicCsd* p = new PeriodicCsd( periodType, periodMultiplier, static_cast<quint64>(amount),
        growth, gs, gap, id,
        name, desc,active, isIncome, decoColor, sDate, eDate, useScenarioDef, infAdjFactor);
    return QSharedPointer<PeriodicCsd>(p);
}


QSharedPointer<PeriodicCsd> PeriodicCsd::duplicate(bool keepSameId, bool keepSameName) const
{
    QSharedPointer<PeriodicCsd> ps = QSharedPointer<PeriodicCsd>( new PeriodicCsd(*this) );
    if (keepSameName==false) {
        QString newName = QString("%1 %2").arg(tr("Copy of")).arg(name);
        newName.truncate(Csd::NAME_MAX_LEN);
        ps->setName(newName);
    }
    if (keepSameId==false) {
        ps->setId(QUuid::createUuid());
    }

    return ps;
}


QHash<QUuid, QSharedPointer<PeriodicCsd>> PeriodicCsd::deepCopyHashmap(
    const QHash<QUuid, QSharedPointer<PeriodicCsd> > &in)
{
    QHash<QUuid, QSharedPointer<PeriodicCsd>> out;

    if(in.size()==0){
        return out;
    }

    // Reserve space for better performance
    out.reserve(in.size());

    for (auto it = in.constBegin(); it != in.constEnd(); ++it) {
        const QUuid& key = it.key();
        const QSharedPointer<PeriodicCsd>& csd = it.value();

        if (csd.isNull() == false) {
            QSharedPointer<PeriodicCsd> newCsd(new PeriodicCsd(*csd));
            out.insert(key, newCsd);
        } else {
            out.insert(key, QSharedPointer<PeriodicCsd>());
        }
    }

    return out;
}


QDate PeriodicCsd::getRealEndDate(const QDate maxDateScenario) const
{
    QDate realEndDate = endDate;
    if (useScenarioForEndDate==true) {
        realEndDate = maxDateScenario; // this can be smaller than start date !
    } else {
        realEndDate = ( (endDate<maxDateScenario)?(endDate):(maxDateScenario) );
    }
    return realEndDate;
}


QString PeriodicCsd::toStringForDisplay(CurrencyInfo currInfo, QLocale locale) const
{
    // *** order is : Every N <period> in [<start>,<end>] - Growth: <type>
    // *** E.G. :
    // ***     Occurs every 6 months in [2023-12-01,2030-05-13]
    // ***     Occurs every 1 month in [2023-12-01,2030-05-13]
    QStringList l;
    QString s;
    QString periodName = getPeriodName(period, false, periodMultiplier>1);
    QString valRangeString;
    if (useScenarioForEndDate==true) {
        QString endString = tr("Scenario defined");
        valRangeString = tr("Every %1 %2 in [%3,%4]").arg(periodMultiplier).arg(periodName)
            .arg(startDate.toString(Qt::ISODate)).arg(endString);
    } else {
        valRangeString = tr("Every %1 %2 in [%3,%4]").arg(periodMultiplier).arg(periodName)
            .arg(startDate.toString(Qt::ISODate)).arg(endDate.toString(Qt::ISODate));
    }
    l.append(valRangeString);

    switch (growthStrategy) {
        case GrowthStrategy::NONE:
            s = tr("Growth: None");
            break;
        case GrowthStrategy::INFLATION:
            s = tr("Growth: %1x Inflation").arg(inflationAdjustmentFactor);
            break;
        case GrowthStrategy::CUSTOM:
            s = tr("Growth: Custom");
            break;
        default:
            break;
    }
    l.append(s);
    return l.join(" - ");
}





int PeriodicCsd::periodTypeToInt(PeriodType pType)
{
    switch (pType) {
        case PeriodType::DAILY:
            return 0;
        case PeriodType::WEEKLY:
            return 1;
        case PeriodType::MONTHLY:
            return 2;
        case PeriodType::END_OF_MONTHLY:
            return 3;
        case PeriodType::YEARLY:
            return 4;
        default:
            throw std::invalid_argument("unknown period type");// Should never happen
    }
}


bool PeriodicCsd::intToPeriodType(int value,
    PeriodicCsd::PeriodType& convertedValue)
{
    switch (value) {
        case 0:
            convertedValue = PeriodType::DAILY;
            return true;
        case 1:
            convertedValue = PeriodType::WEEKLY;
            return true;
        case 2:
            convertedValue = PeriodType::MONTHLY;
            return true;
        case 3:
            convertedValue = PeriodType::END_OF_MONTHLY;
            return true;
        case 4:
            convertedValue = PeriodType::YEARLY;
            return true;
        default:
            return false;// illegal int value
    }
}


QString PeriodicCsd::getPeriodName(PeriodType period, bool capitalizeFirstLetter, bool plural)
{
    if ( !plural ){
        if (capitalizeFirstLetter){
            if (period==PeriodType::DAILY) {
                return tr("Day");
            } else if (period==PeriodType::WEEKLY){
                return tr("Week");
            } else if (period==PeriodType::MONTHLY){
                return tr("Month");
            } else if (period==PeriodType::END_OF_MONTHLY){
                return tr("End-of-Month");
            } else {
                return tr("Year");
            }
        } else {
            if (period==PeriodType::DAILY) {
                return tr("day");
            } else if (period==PeriodType::WEEKLY){
                return tr("week");
            } else if (period==PeriodType::MONTHLY){
                return tr("month");
            } else if (period==PeriodType::END_OF_MONTHLY){
                return tr("end-of-month");
            } else {
                return tr("year");
            }
        }

    } else {
        if (capitalizeFirstLetter){
            if (period==PeriodType::DAILY) {
                return tr("Days");
            } else if (period==PeriodType::WEEKLY){
                return tr("Weeks");
            } else if (period==PeriodType::MONTHLY){
                return tr("Months");
            } else if (period==PeriodType::END_OF_MONTHLY){
                return tr("Ends-of-Month");
            } else {
                return tr("Years");
            }
        } else {
            if (period==PeriodType::DAILY) {
                return tr("days");
            } else if (period==PeriodType::WEEKLY){
                return tr("weeks");
            } else if (period==PeriodType::MONTHLY){
                return tr("months");
            } else if (period==PeriodType::END_OF_MONTHLY){
                return tr("ends-of-month");
            } else {
                return tr("years");
            }
        }
    }
}


int PeriodicCsd::growthStrategyToInt(GrowthStrategy gs)
{
    switch (gs) {
        case GrowthStrategy::NONE:
            return 0;
        case GrowthStrategy::INFLATION:
            return 1;
        case GrowthStrategy::CUSTOM:
            return 2;
        default:
            throw std::invalid_argument("unknown period type");// Should never happen
    }
}


bool PeriodicCsd::intToGrowthStrategy(int value, GrowthStrategy &convertedValue)
{
    switch (value) {
        case 0:
            convertedValue = GrowthStrategy::NONE;
            return true;
        case 1:
            convertedValue = GrowthStrategy::INFLATION;
            return true;
        case 2:
            convertedValue = GrowthStrategy::CUSTOM;
            return true;
        default:
            return false;// illegal int value
    }
}


QDate PeriodicCsd::getNextEventDate(QDate date) const
{
    QDate nextDate;
    switch (this->period) {
        case PeriodType::DAILY:
            return DateHelper::getNextDate(date, DateHelper::TimeUnitType::Day,
                this->periodMultiplier);
        case PeriodType::WEEKLY:
            return DateHelper::getNextDate(date, DateHelper::TimeUnitType::Week,
                this->periodMultiplier);
        case PeriodType::MONTHLY:
            return DateHelper::getNextDate(date, DateHelper::TimeUnitType::Month,
                this->periodMultiplier);
        case PeriodType::END_OF_MONTHLY:
            return DateHelper::getNextDate(date, DateHelper::TimeUnitType::EndOfMonth,
                this->periodMultiplier);
        case PeriodType::YEARLY:
            return DateHelper::getNextDate(date, DateHelper::TimeUnitType::Year,
                this->periodMultiplier);
        default:
            throw std::invalid_argument("PeriodicCsd type does not exist");
    }
    return nextDate; // dummy
}


PeriodicCsd::PeriodType PeriodicCsd::getPeriod() const
{
    return period;
}


quint16 PeriodicCsd::getPeriodMultiplier() const
{
    return periodMultiplier;
}


quint64 PeriodicCsd::getAmount() const
{
    return amount;
}


void PeriodicCsd::setAmount(quint64 newAmount)
{
    amount = newAmount;
}


Growth PeriodicCsd::getGrowth() const
{
    return growth;
}


PeriodicCsd::GrowthStrategy PeriodicCsd::getGrowthStrategy() const
{
    return growthStrategy;
}


quint16 PeriodicCsd::getGrowthApplicationPeriod() const
{
    return growthApplicationPeriod;
}


double PeriodicCsd::getInflationAdjustmentFactor() const
{
    return inflationAdjustmentFactor;
}


QDate PeriodicCsd::getStartDate() const
{
    return startDate;
}


QDate PeriodicCsd::getEndDate() const
{
    return endDate;
}


bool PeriodicCsd::getUseScenarioForEndDate() const
{
    return useScenarioForEndDate;
}


