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

#include <QVariantMap>
#include <QSharedPointer>
#include <QString>
#include <QCoreApplication>
#include "daterange.h"
#include "math.h"
#include "growth.h"
#include "util.h"
#include "currencyhelper.h"


qint64 Growth::MIN_GROWTH_DECIMAL = static_cast<qint64>(Growth::MIN_GROWTH_DOUBLE*(
    pow(10,Growth::NO_OF_DECIMALS)));
qint64 Growth::MAX_GROWTH_DECIMAL = static_cast<qint64>(Growth::MAX_GROWTH_DOUBLE*(
    pow(10,Growth::NO_OF_DECIMALS)));



Growth::Growth()
{
    this->type = Type::NONE;
    this->annualVariableGrowth = {};
    this->annualConstantGrowth = 0;
    recalculateMonthlyData();
}


Growth Growth::fromConstantAnnualPercentageDouble(double annualPercentage)
{
    if (annualPercentage > MAX_GROWTH_DOUBLE){
        throw std::domain_error("Growth is too big");
    }
    if (annualPercentage < MIN_GROWTH_DOUBLE){
        throw std::domain_error("Growth is too small");
    }
    qint64 annualPercentageDecimal = fromDoubleToDecimal(annualPercentage);
    return fromConstantAnnualPercentageDecimal(annualPercentageDecimal);
}


Growth Growth::fromConstantAnnualPercentageDecimal(qint64 annualPercentage)
{
    if (annualPercentage > MAX_GROWTH_DECIMAL){
        throw std::domain_error("Growth is too big");
    }
    if (annualPercentage < MIN_GROWTH_DECIMAL){
        throw std::domain_error("Growth is too small");
    }
    Growth g;
    g.type = Type::CONSTANT;
    g.annualVariableGrowth = {};
    g.annualConstantGrowth = annualPercentage;

    g.recalculateMonthlyData();
    return g;
}


Growth Growth::fromVariableDataAnnualBasisDecimal(const QMap<QDate, qint64> &newVariableGrowth)
{
    Growth g;

    Util::ResultOfOperation result;
    g.areFactorsValid(newVariableGrowth, result);
    if ( result.status == Util::ResultOfOperationStatus::ERROR){
        QString errorString = QString("Variable growth is invalid - %1").arg(result.logErrorMessage);
        throw std::domain_error(errorString.toLocal8Bit().data());
    }
    g.type = Type::VARIABLE;
    g.annualVariableGrowth = newVariableGrowth; // shallow copy, but copy-on-write
    g.annualConstantGrowth = 0;

    g.recalculateMonthlyData();
    return g;
}


Growth Growth::fromVariableDataAnnualBasisDouble(const QMap<QDate, double> &newVariableGrowth)
{
    Growth g;

    Util::ResultOfOperation result;
    g.areFactorsValid(newVariableGrowth, result);
    if ( result.status == Util::ResultOfOperationStatus::ERROR){
        QString errorString = QString("Variable growth is invalid - %1").arg(result.logErrorMessage);
        throw std::domain_error(errorString.toLocal8Bit().data());
    }

    // convert to decimal form
    QMap<QDate, qint64> c;
    for (QMap<QDate, double>::const_iterator it = newVariableGrowth.constBegin();
         it != newVariableGrowth.constEnd(); ++it) {
        QDate date = it.key();
        double value = it.value();
        c.insert(date, fromDoubleToDecimal(value));
    }

    // Build and return
    g.type = Type::VARIABLE;
    g.annualVariableGrowth = c; // shallow copy, but copy-on-write
    g.annualConstantGrowth = 0;

    g.recalculateMonthlyData();
    return g;
}


Growth::Growth(const Growth& o)
{
    this->type = o.type;
    // persistent
    this->annualVariableGrowth = o.annualVariableGrowth; // shallow copy, but copy-on-write
    this->annualConstantGrowth = o.annualConstantGrowth;
    // transient
    this->monthlyConstantGrowth = o.monthlyConstantGrowth;
    this->monthlyVariableGrowth = o.monthlyVariableGrowth;
}



Growth& Growth::operator=(const Growth &o)
{
    if (this != &o){                // to protect against self-assignment
        // persistent
        this->type = o.type;
        this->annualVariableGrowth = o.annualVariableGrowth; // shallow copy, but copy-on-write
        this->annualConstantGrowth = o.annualConstantGrowth;
        // transient
        this->monthlyVariableGrowth = o.monthlyVariableGrowth; // shallow copy, but copy-on-write
        this->monthlyConstantGrowth = o.monthlyConstantGrowth;
    }
    return *this;
}


bool Growth::operator==(const Growth& o) const
{
    if ( (this->type!=o.type) || (this->annualVariableGrowth!=o.annualVariableGrowth) ||
        (this->annualConstantGrowth!=o.annualConstantGrowth) ) {
        return false;
    }
    return true;
}


bool Growth::operator!=(const Growth &o) const
{
    return !(*this==o);
}


Growth::~Growth()
{
    annualVariableGrowth.clear();    // most probaly useless : TODO : check
    monthlyVariableGrowth.clear();   // most probaly useless : TODO : check
}


QJsonObject Growth::toJson() const
{
    QJsonObject jobject;
    jobject["NoOfDecimals"] = static_cast<int>(NO_OF_DECIMALS);
    jobject["Type"] = convertTypeFromEnumToInt(type);
    jobject["AnnualConstantGrowth"] = annualConstantGrowth;
    QJsonObject jobjectFactors;
    for (auto it = annualVariableGrowth.begin(); it != annualVariableGrowth.end(); ++it) {
        jobjectFactors[it.key().toString(Qt::ISODate)] = it.value();
    }
    jobject["AnnualVariableGrowth"] = jobjectFactors;
    return jobject;
}


Growth Growth::fromJson(const QJsonObject &jsonObject, Util::ResultOfOperation &result)
{
    QJsonValue jsonValue;
    double d;
    int convResult;
    bool success;
    QString str;
    Growth g;

    // Reset result to ERROR
    result.init();

    // check that this version of Growth uses the current defined no of decimals
    jsonValue = jsonObject.value("NoOfDecimals");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = "Growth: Cannot find token \"NoOfDecimals\"";
        return g;
    }
    if (false==jsonValue.isDouble()){
        result.logErrorMessage = QString("Growth: No of decimals value is not a number");
        return g;
    }
    d = jsonValue.toDouble();
    qint64 noOfDecimals = Util::extractQint64FromDoubleWithNoFractionalPart(d,convResult);
    if ( convResult != 0 ){
        result.logErrorMessage = QString("Growth: No of decimals value \"%1\" is not a valid "
            "integer (code=%2)").arg(d).arg(convResult);
        return g;
    }

    if ( noOfDecimals != Growth::NO_OF_DECIMALS){
        result.logErrorMessage = QString("Growth: No of decimals value \"%1\" is incompatible with "
            "expected value \"%2\"").arg(noOfDecimals).arg(Growth::NO_OF_DECIMALS);
        return g;
    }
    // Constant Growth
    jsonValue = jsonObject.value("AnnualConstantGrowth");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = "Growth: Cannot find token \"AnnualConstantGrowth\"";
        return g;
    }
    if (false==jsonValue.isDouble()){
        result.logErrorMessage = QString("Growth: AnnualConstantGrowth value is not a number");
        return g;
    }
    d = jsonValue.toDouble();
    qint64 growth = Util::extractQint64FromDoubleWithNoFractionalPart(d,convResult);
    if ( convResult != 0 ){
        result.logErrorMessage = QString("Growth: AnnualConstantGrowth value \"%1\" is not a "
            "valid integer (code=%2)").arg(d).arg(convResult);
        return g;
    }
    if ( growth>MAX_GROWTH_DECIMAL ){
        result.logErrorMessage = QString("Growth: AnnualConstantGrowth value %1 is larger than"
            " the maximum allowed of %2").arg(growth).arg(MAX_GROWTH_DECIMAL);
        return g;
    }
    if ( growth<MIN_GROWTH_DECIMAL ){
        result.logErrorMessage = QString("Growth: AnnualConstantGrowth value %1 is smaller "
            "than the minimum value allowed of %2").arg(growth).arg(MIN_GROWTH_DECIMAL);
        return g;
    }
    // Variable growth
    jsonValue = jsonObject.value("AnnualVariableGrowth");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = "Growth: Cannot find token \"AnnualVariableGrowth\"";
        return g;
    }
    if (jsonValue.isObject()==false){
        result.logErrorMessage = "Growth: AnnualVariableGrowth token is not an Object";
        return g;
    }
    QMap<QDate,qint64> f;
    QJsonObject factorsObject = jsonObject["AnnualVariableGrowth"].toObject();
    for (auto it = factorsObject.begin(); it != factorsObject.end(); ++it) {
        bool validDate;
        QDate key = Util::isValidISO8601Date(it.key(), validDate);
        // date
        if (validDate==false){
            result.logErrorMessage = QString("Growth: Entry key \"%1\" in AnnualVariableGrowth"
                " table is not a valid ISO Date").arg(it.key());
            return g;
        }
        if (key.day() != 1){
            result.logErrorMessage = QString("Growth: Entry key %1 in AnnualVariableGrowth table"
                " has not \"Month Day\" set to 01").arg(key.toString(Qt::ISODate));
            return g;
        }
        // growth value
        QJsonValueRef valRef = it.value();
        if ( valRef.isDouble() == false){
            result.logErrorMessage = QString("Growth: Value \"%1\" in AnnualVariableGrowth table"
                " is not a number").arg(valRef.toString());
            return g;
        }
        d = valRef.toDouble();
        qint64 value = Util::extractQint64FromDoubleWithNoFractionalPart(d,convResult);
        if ( convResult != 0 ){
            result.logErrorMessage = QString("Growth: AnnualVariableGrowth value \"%1\" is not a "
                "valid integer (code=%2)").arg(d).arg(convResult);
            return g;
        }
        if ( value>MAX_GROWTH_DECIMAL ){
            result.logErrorMessage = QString("Growth: Value %1 in AnnualVariableGrowth table "
                "is bigger than the maximum allowed of %2").arg(value).arg(MAX_GROWTH_DECIMAL);
            return g;
        }
        if ( value<MIN_GROWTH_DECIMAL) {
            result.logErrorMessage = QString("Growth: Value %1 in AnnualVariableGrowth table "
                "is smaller than the minimum value of %2").arg(value).arg(MIN_GROWTH_DECIMAL);
            return g;
        }
        // commit
        f.insert(key, value);
    }
    // type
    jsonValue = jsonObject.value("Type");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = "Growth: Cannot find token \"Type\"";
        return g;
    }
    if ( jsonValue.isDouble() == false){
        result.logErrorMessage = QString("Growth: Type token \"%1\" is not a number").arg(str);
        return g;
    }
    d = jsonValue.toDouble();
    qint64 typeInt = Util::extractQint64FromDoubleWithNoFractionalPart(d,convResult);
    if ( convResult != 0 ){
        result.logErrorMessage = QString("Growth: Type token \"%1\" is not a valid integer "
            "(code=%2)").arg(d).arg(convResult);
        return g;
    }

    Type finalType;
    if ( false==convertTypeFromIntToEnum(typeInt, finalType) ){
        result.logErrorMessage = QString("Growth: Type token %1 value is unknown").arg(typeInt);
        return g;
    }
    switch(finalType){
        case Type::CONSTANT:
            g = fromConstantAnnualPercentageDecimal(growth);
            break;
        case Type::VARIABLE:
            g = fromVariableDataAnnualBasisDecimal(f);
            break;
        case Type::NONE:
            g = Growth();
            break;
        default:
            // should never happen
            throw std::invalid_argument("Growth: Unexpected growth type");
        }

    result.status = Util::ResultOfOperationStatus::SUCCESS;
    return g;
}


void Growth::changeByFactor(double factor, bool& capped)
{
    if (factor < 0){
        throw std::domain_error("Factor cannot be negative");
    }

    long double ld;
    capped = false;
    switch (type) {
        case Type::NONE:
            // nothing to do
            break;
        case Type::CONSTANT:
            ld = std::round(annualConstantGrowth * factor);
            if( ld > MAX_GROWTH_DECIMAL){
                ld = MAX_GROWTH_DECIMAL;
                capped = true;
            }
            annualConstantGrowth = static_cast<qint64>(ld);
            break;
        case Type::VARIABLE:
            foreach(QDate date, annualVariableGrowth.keys()){
                qint64 value = annualVariableGrowth.value(date);
                ld = std::round(value * factor);
                if( ld > MAX_GROWTH_DECIMAL){
                    ld = MAX_GROWTH_DECIMAL;
                    capped = true;
                }
                value = static_cast<qint64>(ld);
                annualVariableGrowth.insert(date, value);
            }
            break;
        default:
            throw std::domain_error("Unknown type");
            break;
    }

    recalculateMonthlyData();
}


qint64 Growth::fromDoubleToDecimal(long double d)
{
    qint64 v = static_cast<qint64>(round(d*Util::quickPow10(NO_OF_DECIMALS)));
    return v;
}


long double Growth::fromDecimalToDouble(qint64 i)
{
    double d = static_cast<double>(i)/Util::quickPow10(NO_OF_DECIMALS);
    return d;
}


QList<quint64> Growth::adjustForGrowth(quint64 amount, const QList<QDate> &occurrenceDates,
    ApplicationStrategy appStrategy, double pvDiscountRate,
    QDate pvPresent, AdjustForGrowthResult &ok) const
{
    QList<quint64> result;

    ok.success = false;
    ok.saturationCount = 0;
    ok.errorMessageUI = "";
    ok.errorMessageLog = "";

    // *** arguments validity check ***

    if (occurrenceDates.size()==0){
        ok.success = true;
        return result;  // no occurrence, so empty set returned
    }
    for (int i = 0; i < (occurrenceDates.size() - 1); i++) {
        if (occurrenceDates[i] > occurrenceDates[i + 1]) {
            ok.errorMessageUI = tr("%1 : OccurrenceDates are not sorted properly")
                .arg(__func__);
            ok.errorMessageLog = QString("%1 : OccurrenceDates are not sorted properly")
                .arg(__func__);
            return result;
        }
    }
    if ( (appStrategy.noOfMonths<1) ){
        ok.errorMessageUI = tr("%1 : AppStrategy.noOfMonth is invalid").arg(__func__);
        ok.errorMessageLog = QString("%1 : AppStrategy.noOfMonth is invalid").arg(__func__);
        return result;
    }
    if ( amount > static_cast<quint64>(CurrencyHelper::maxValueAllowedForAmount())){
        ok.errorMessageUI = tr("%1 : Amount is too big ").arg(__func__);
        ok.errorMessageLog = QString("%1 : Amount is too big ").arg(__func__);
        return result;
    }
    if ( pvDiscountRate<0 ){
        ok.errorMessageUI = tr("%1 : Present Value annual discount rate smaller than 0")
            .arg(__func__);
        ok.errorMessageLog =QString("%1 : Present Value annual discount rate is smaller than 0")
            .arg(__func__);
        return result;
    }
    if ( pvPresent.isValid()==false ){
        ok.errorMessageUI = tr("%1 : PV Present Date is invalid").arg(__func__);
        ok.errorMessageLog = QString("%1 : PV Present Date is invalid").arg(__func__);
        return result;
    }
    // All arguments are good, result can be non empty now
    result.resize(occurrenceDates.size(),0);


    // *** preparation for calculation ***

    uint noOfMonthsCycle = appStrategy.noOfMonths;

    // GROWTH : build monthly cumulative growth multiplier vector.
    // From first occurrence to last, this will provide a cumulative growth factor
    // we can use to multiply the originally fix amount to get the growth-adjusted amount
    int noOfMonthCovered = 1 + Util::noOfMonthsDifference(occurrenceDates.first(),
        occurrenceDates.last()); // No of month spanned in the occurrenceVector : 1 to infinity
    QList<long double> multiplierVector = buildMonthlyMultiplierVector(
        noOfMonthCovered,occurrenceDates.first());  // Index 0 is first month of occurrence

    // PRESENT VALUE : build monthly Present Value multiplier.
    // Computed from "Present", but applied from first occurrence to last, this will provide a
    // "future to present value" factor we can use to multiply the originally fix amount
    int pvNoOfMonthCovered = 1 + Util::noOfMonthsDifference(occurrenceDates.first(),
        occurrenceDates.last());
    QList<long double> pvMultiplierVector = buildPvMonthlyMultiplierVector(
        pvDiscountRate, pvNoOfMonthCovered, occurrenceDates.first(),pvPresent);

    // *** calculation ***

    qint64 maxLimitDecimal = CurrencyHelper::maxValueAllowedForAmount();
    long double maxLimitDecimalAsDouble = static_cast<long double>(maxLimitDecimal);
    long double growthMultiplier=1;
    uint occurrenceCounter = 0;
    for(const QDate& date : occurrenceDates){
        occurrenceCounter++;
        int multiplierVectorIndex = Util::noOfMonthsDifference(occurrenceDates.first(), date);

        // Incorrect solution proposed by AI...Calculate number of application periods
        // int applicationPeriods = multiplierVectorIndex / noOfMonthsCycle; // frac part dropped
        // int adjustedIndex = applicationPeriods * noOfMonthsCycle; // Snap to application period
        // long double growthMultiplier = multiplierVector[adjustedIndex];

        // Apply CFG every noOfMonthsCycle only, otherwise keep the preious CFG
        if ( ((occurrenceCounter-1) % noOfMonthsCycle) == 0 ){
            growthMultiplier  =  multiplierVector[multiplierVectorIndex];
        }

        // the calculated amount can be outside the allowed range defined in CurrencyHelper.
        // If it happens, it is called "saturation". We just cap the value to the min/max and
        // continue processing
        long double t = std::round(static_cast<long double>(amount) * growthMultiplier *
            pvMultiplierVector[multiplierVectorIndex]);
        if ( t > maxLimitDecimalAsDouble ){
            t = maxLimitDecimalAsDouble;
            ok.saturationCount++;
        }

        result[occurrenceCounter-1] = static_cast<quint64>(t);
    }

    ok.success = true;
    return result;
}


//
// *** PRIVATE ***
//



void Growth::recalculateMonthlyData()
{
    // Constant
    long double ld = fromDecimalToDouble(annualConstantGrowth);
    monthlyConstantGrowth = Util::annualToMonthlyGrowth(ld);

    // Variable
    monthlyVariableGrowth = QMap<QDate,long double>();
    foreach(QDate date, annualVariableGrowth.keys()){
        qint64 value = annualVariableGrowth.value(date);
        long double ld = fromDecimalToDouble(value);
        long double d = Util::annualToMonthlyGrowth(ld);
        monthlyVariableGrowth.insert(date, d);
    }
}


QList<long double> Growth::buildMonthlyMultiplierVector(uint noOfMonths,
    QDate from) const {
    if (noOfMonths==0){
        throw std::domain_error("noOfMonth must be > 0");
    }
    if (from.isValid()==false){
        throw std::domain_error("Date is invalid");
    }

    QList<long double> multiplierVector;
    multiplierVector.resize(noOfMonths, 1);
    long double cgf = 1;    // CGF : long double to maximize no of significant digits

    if( (type==Type::NONE) || (noOfMonths==1) ){
        return multiplierVector;
    }

    if (type==Type::CONSTANT){
        // *** CONSTANT ***
        // First month of occurrence always have multiplierVector = 1 (no growth)
        for(uint i=1; i < noOfMonths; i++){
            cgf = cgf * ( 1 + monthlyConstantGrowth/100.0L);
            multiplierVector[i] = cgf;
        }
    } else {
        // *** VARIABLE ***
        // First month of occurrence always have multiplierVector = 1 (no growth)
        if( monthlyVariableGrowth.size()!=0 ){

            // current monthly growth in effect, NOT inpercentage (e.g. 0.1 , -0.15)
            long double currentMonthlyGrowth = 0;
            uint index = 1; // position of insertion in multiplier vector : skip first one
            QDate indexDate = from;
            // reset Day to 1 to prevent problem (e.g. 29 feb)
            indexDate.setDate(from.year(), from.month(),1);
            indexDate = indexDate.addMonths(1);
            DateRange transitionSpace = DateRange(monthlyVariableGrowth.firstKey(),
                monthlyVariableGrowth.lastKey());

            // get the latest growth value defined before SECOND date
            if ( transitionSpace.includeDate(indexDate) &&
                (monthlyVariableGrowth.contains(indexDate)==false) ){
                // we have to find the closest growth defined in the past
                for (QMap<QDate, long double>::const_iterator it = monthlyVariableGrowth.cbegin(),
                    end = monthlyVariableGrowth.cend(); it != end; ++it) {
                    if(it.key() >= indexDate){
                        break;
                    }
                    currentMonthlyGrowth = it.value()/100.0L;
                }
            } else if (transitionSpace.getEnd() < indexDate) {
                // get last growth defined
                currentMonthlyGrowth = monthlyVariableGrowth.last()/100.0L;
            }

            while( index < noOfMonths ){
                // any new growth defined for that date ?
                if(monthlyVariableGrowth.contains(indexDate)==true){
                    // set the new growth value
                    currentMonthlyGrowth = monthlyVariableGrowth.value(indexDate)/100.0L;
                }
                cgf = cgf * (1.0 + currentMonthlyGrowth);
                // set result entry
                multiplierVector[index] = cgf;
                // go to next item
                indexDate = indexDate.addMonths(1);
                index ++;
            }
        }

    }

    return multiplierVector;
}


QList<long double> Growth::buildPvMonthlyMultiplierVector(double annualDiscountrate,
    uint noOfMonths, QDate firstOccurrence, QDate pvPresent) const
{
    if (noOfMonths==0){
        throw std::domain_error(QString("%1: noOfMonth must be > 0")
            .arg(Q_FUNC_INFO).toStdString());
    }
    if (pvPresent.isValid()==false){
        throw std::domain_error(QString("%1: PV date is invalid").arg(Q_FUNC_INFO).toStdString());
    }
    if (firstOccurrence.isValid()==false){
        throw std::domain_error(QString("%1: First occurrence date is invalid")
            .arg(Q_FUNC_INFO).toStdString());
    }

    long double monthlyDiscountRate = Util::annualToMonthlyGrowth(annualDiscountrate); // in %
    QList<long double> multiplierVector;
    multiplierVector.resize(noOfMonths,1);

    // how many PV periods are already passed before reaching the first occurrence.
    // Return a negative period if pvPresent > firstOccurrence
    int pvPeriodOffset = Util::noOfMonthsDifference(pvPresent, firstOccurrence);

    for(uint i=0; i < noOfMonths; i++){
        long double temp = Util::toPvConversionFactor(monthlyDiscountRate,pvPeriodOffset+i);
        multiplierVector[i] = temp; // temp is to ease debugging
    }

    return multiplierVector;
}


void Growth::areFactorsValid(QMap<QDate, double> factorsToBeChecked,
    Util::ResultOfOperation &result)
{
    // Reset to ERROR status
    result.init();

    // be sure change date is set to Day 1
    foreach(QDate date, factorsToBeChecked.keys()){
        if (date.isValid()==false){
            result.userErrorMessage = tr("Date %1 is invalid").arg(date.toString());
            result.logErrorMessage = QString("Date %1 is invalid").arg(date.toString());
            return ;
        }
        if (date.day() != 1){
            result.userErrorMessage = tr("Date %1 is invalid because Day is not set to 1")
            .arg(date.toString());
            result.logErrorMessage = QString("Date %1 is invalid because Day is not set to 1")
                .arg(date.toString());
            return ;
        }
    }
    // be sure growth value is in the right range
    foreach(double val, factorsToBeChecked.values()){
        if ( val < MIN_GROWTH_DOUBLE ){
            result.userErrorMessage = tr("Growth %1 is smaller than the minimum allowed of %2")
            .arg(val,Growth::MIN_GROWTH_DECIMAL);
            result.logErrorMessage = QString("Growth %1 is smaller than the minimum allowed of %2")
                .arg(val,Growth::MIN_GROWTH_DOUBLE);
            return ;
        }
        if ( val>MAX_GROWTH_DOUBLE ){
            result.userErrorMessage = tr("Growth %1 is bigger than the maximum allowed of %2")
            .arg(val, Growth::MAX_GROWTH_DECIMAL);
            result.logErrorMessage = QString("Growth %1 is bigger than the maximum allowed of %2")
                .arg(val, Growth::MAX_GROWTH_DOUBLE);
            return ;
        }    }

    // all is well
    result.status = Util::ResultOfOperationStatus::SUCCESS;
    return ;
}


void Growth::areFactorsValid(QMap<QDate, qint64> factorsToBeChecked,
    Util::ResultOfOperation &result)
{
    // Reset to ERROR status
    result.init();

    // empty map is valid
    if (factorsToBeChecked.size()==0) {
        result.status = Util::ResultOfOperationStatus::SUCCESS;
        return;
    }

    // make sure factorsToBeChecked map has no more than VARIABLE_MAX_NO_OF_ENTRIES. Check this
    // before entering the loop later (in case no of entries is insane due to mistake)
    if (factorsToBeChecked.size() > VARIABLE_MAX_NO_OF_ENTRIES ) {
        result.userErrorMessage = tr("%1 : Too many entries for variable growth").arg(__func__);
        result.logErrorMessage = QString("%1 : Too many entries for variable growth").arg(__func__);
        return ;
    }

    // Check all dates : be sure date is valid and set to Day 1
    foreach(QDate date, factorsToBeChecked.keys()){
        if (date.isValid()==false){
            result.userErrorMessage = tr("Date %1 is invalid").arg(date.toString());
            result.logErrorMessage = QString("Date %1 is invalid").arg(date.toString());
            return ;
        }
        if (date.day() != 1){
            result.userErrorMessage = tr("Date %1 is invalid because Day is not set to 1")
                .arg(date.toString());
            result.logErrorMessage = QString("Date %1 is invalid because Day is not set to 1")
                .arg(date.toString());
            return ;
        }
    }
    // be sure growth value is in the right range
    foreach(qint64 val, factorsToBeChecked.values()){
        if ( val < MIN_GROWTH_DECIMAL ){
            result.userErrorMessage = tr("Growth %1 is smaller than the minimum allowed of %2")
                .arg(val,Growth::MIN_GROWTH_DECIMAL);
            result.logErrorMessage = QString("Growth %1 is smaller than the minimum allowed of %2")
                .arg(val,Growth::MIN_GROWTH_DECIMAL);
            return ;
        }
        if ( val>MAX_GROWTH_DECIMAL ){
            result.userErrorMessage = tr("Growth %1 is bigger than the maximum allowed of %2")
                .arg(val, Growth::MAX_GROWTH_DECIMAL);
            result.logErrorMessage = QString("Growth %1 is bigger than the maximum allowed of %2")
                .arg(val, Growth::MAX_GROWTH_DECIMAL);
            return ;
        }    }

    // all is well
    result.status = Util::ResultOfOperationStatus::SUCCESS;
    return ;
}


int Growth::convertTypeFromEnumToInt(Growth::Type theType)
{
    if (theType==Growth::Type::CONSTANT) {
        return 0;
    } else if (theType==Growth::Type::NONE){
        return 2;
    } else if (theType==Growth::Type::VARIABLE){
        return 1;
    } else{
        // should never happen
        throw std::invalid_argument("Invalid Growth Type");
    }
}


bool Growth::convertTypeFromIntToEnum(int value, Type &result)
{
    if (value==0) {
        result = Growth::Type::CONSTANT;
        return true;
    } else if (value==1){
        result = Growth::Type::VARIABLE;
        return true;
    } else if (value==2){
        result = Growth::Type::NONE;
        return true;
    } else {
        return false;
    }
}


long double Growth::calculateNewAmountConstantGrowth(QDate from, QDate to,
    long double originalAmount, long double monthlyGrowth) const
{
    if (from.isValid()==false){
        throw std::invalid_argument("from is an invalid date");
    }
    if (to.isValid()==false){
        throw std::invalid_argument("to is an invalid date");
    }
    if(to<from){
        throw std::invalid_argument("to is before from");
    }
    int noOfMonth = Util::noOfMonthsDifference(from, to);
    return originalAmount*pow((long double)(1+(monthlyGrowth/100.0)), noOfMonth);
}







// *** Getters / setters ***

Growth::Type Growth::getType() const
{
    return type;
}

qint64 Growth::getAnnualConstantGrowth() const
{
    return annualConstantGrowth;
}

QMap<QDate, qint64> Growth::getAnnualVariableGrowth() const
{
    return annualVariableGrowth;
}

long double Growth::getMonthlyConstantGrowth() const
{
    return monthlyConstantGrowth;
}

QMap<QDate, long double> Growth::getMonthlyVariableGrowth() const
{
    return monthlyVariableGrowth;
}




