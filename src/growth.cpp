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

#include <QVariantMap>
#include <QSharedPointer>
#include <QString>
#include <QCoreApplication>
#include "daterange.h"
#include "math.h"
#include "growth.h"
#include "util.h"
#include "currencyhelper.h"

// Precision of stored value for growth percentage, expressed as no of decimals. E.g., if 5,
// then 1% is stored as 100000 int value, which allow a precision of 0.00001 for growth percentage.
uint Growth::NO_OF_DECIMALS = 5;

// this is for ANNUAL growth

// min annual percentage, -100% per year, give also -100% a month.
double Growth::MIN_GROWTH_DOUBLE = -100;
// max annual percentage, +10,000 % a year is going from 1 to 101, give 46.901686 % a month
double Growth::MAX_GROWTH_DOUBLE = 10000;
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


// annualPercentage will be approximated in the final internal storage value
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


// no of decimal assumed = NO_OF_DECIMALS
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


Growth Growth::fromVariableDataAnnualBasisDecimal(QMap<QDate, qint64> newVariableGrowth)
{
    Growth g;

    Util::OperationResult result;
    g.isFactorsValid(newVariableGrowth, result);
    if ( result.success == false){
        QString errorString = QString("Variable growth is invalid - %1").arg(result.errorStringLog);
        throw std::domain_error(errorString.toLocal8Bit().data());
    }
    g.type = Type::VARIABLE;
    g.annualVariableGrowth = newVariableGrowth;    // shallow copy, but copy-on-write (detach shared objects)
    g.annualConstantGrowth = 0;

    g.recalculateMonthlyData();
    return g;
}


Growth::Growth(const Growth& o)
{
    this->type = o.type;
    // persistent
    this->annualVariableGrowth = o.annualVariableGrowth; // shallow copy, but copy-on-write (detach shared objects)
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
        this->annualVariableGrowth = o.annualVariableGrowth; // shallow copy, but copy-on-write (detach shared objects)
        this->annualConstantGrowth = o.annualConstantGrowth;
        // transient
        this->monthlyVariableGrowth = o.monthlyVariableGrowth; // shallow copy, but copy-on-write (detach shared objects)
        this->monthlyConstantGrowth = o.monthlyConstantGrowth;
    }
    return *this;
}


// compare just the persistent data
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


// Save only the persistent data
QJsonObject Growth::toJson() const
{
    QJsonObject jobject;
    jobject["NoOfDecimals"] = static_cast<int>(NO_OF_DECIMALS);
    jobject["Type"] = type; // CONSTANT=0  VARIABLE=1  NONE=2
    jobject["AnnualConstantGrowth"] = annualConstantGrowth;
    QJsonObject jobjectFactors;
    for (auto it = annualVariableGrowth.begin(); it != annualVariableGrowth.end(); ++it) {
        jobjectFactors[it.key().toString(Qt::ISODate)] = it.value();
    }
    jobject["AnnualVariableGrowth"] = jobjectFactors;
    return jobject;
}


Growth Growth::fromJson(const QJsonObject &jsonObject, Util::OperationResult &result)
{
    QJsonValue jsonValue;
    double d;
    int ok;
    QString str;
    Growth g;
    result.success = false;
    result.errorStringUI = "";
    result.errorStringLog = "";

    // check that this version of Growth uses the current defined no of decimals
    jsonValue = jsonObject.value("NoOfDecimals");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("Cannot find NoOfDecimals tag");
        result.errorStringLog = "Cannot find NoOfDecimals tag";
        return g;
    }
    if (false==jsonValue.isDouble()){
        result.errorStringUI = tr("No of decimals value is not a number");
        result.errorStringLog = QString("No of decimals value is not a number");
        return g;
    }
    d = jsonValue.toDouble();
    qint64 noOfDecimals = Util::extractQint64FromDoubleWithNoFracPart(d,ok);
    if ( ok==-1 ){
        result.errorStringUI = tr("No of decimals value %1 is not an integer").arg(d);
        result.errorStringLog = QString("No of decimals value %1 is not an integer").arg(d);
        return g;
    }
    if ( ok==-2 ){
        result.errorStringUI = tr("No of decimals value %1 is invalid").arg(d);
        result.errorStringLog = QString("No of decimals value %1 is invalid").arg(d);
        return g;
    }
    if ( noOfDecimals != Growth::NO_OF_DECIMALS){
        result.errorStringUI = tr("No of decimals value %1 is incompatible with expected value %2").arg(noOfDecimals).arg(Growth::NO_OF_DECIMALS);
        result.errorStringLog = QString("No of decimals value %1 is incompatible with expected value %2").arg(noOfDecimals).arg(Growth::NO_OF_DECIMALS);
        return g;
    }
    // Constant Growth
    jsonValue = jsonObject.value("AnnualConstantGrowth");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("Cannot find AnnualConstantGrowth tag");
        result.errorStringLog = "Cannot find AnnualConstantGrowth tag";
        return g;
    }
    if (false==jsonValue.isDouble()){
        result.errorStringUI = tr("AnnualConstantGrowth value is not a number");
        result.errorStringLog = QString("AnnualConstantGrowth value is not a number");
        return g;
    }
    d = jsonValue.toDouble();
    qint64 growth = Util::extractQint64FromDoubleWithNoFracPart(d,ok);
    if ( ok==-1 ){
        result.errorStringUI = tr("AnnualConstantGrowth value %1 is not an integer").arg(d);
        result.errorStringLog = QString("AnnualConstantGrowth value %1 is not an integer").arg(d);
        return g;
    }
    if ( ok!=0 ){
        result.errorStringUI = tr("AnnualConstantGrowth value %1 is invalid for unknow reason").arg(growth);
        result.errorStringLog = QString("AnnualConstantGrowth value %1 is invalid for unknow reason").arg(growth);
        return g;
    }
    if ( growth>MAX_GROWTH_DECIMAL ){
        result.errorStringUI = tr("AnnualConstantGrowth value %1 is larger than the maximum allowed of %2").arg(growth).arg(MAX_GROWTH_DECIMAL);
        result.errorStringLog = QString("AnnualConstantGrowth value %1 is larger than the maximum allowed of %2").arg(growth).arg(MAX_GROWTH_DECIMAL);
        return g;
    }
    if ( growth<MIN_GROWTH_DECIMAL ){
        result.errorStringUI = tr("AnnualConstantGrowth value %1 is smaller than the minimum value allowed of %2").arg(growth).arg(MIN_GROWTH_DECIMAL);
        result.errorStringLog = QString("AnnualConstantGrowth value %1 is smaller than the minimum value allowed of %2").arg(growth).arg(MIN_GROWTH_DECIMAL);
        return g;
    }
    // Variable growth
    jsonValue = jsonObject.value("AnnualVariableGrowth");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("Cannot find AnnualVariableGrowth tag");
        result.errorStringLog = "Cannot find AnnualVariableGrowth tag";
        return g;
    }
    if (jsonValue.isObject()==false){
        result.errorStringUI = tr("AnnualVariableGrowth tag is not an Object");
        result.errorStringLog = "AnnualVariableGrowth tag is not an Object";
        return g;
    }
    QMap<QDate,qint64> f;
    QJsonObject factorsObject = jsonObject["AnnualVariableGrowth"].toObject();
    for (auto it = factorsObject.begin(); it != factorsObject.end(); ++it) {
        QDate key = QDate::fromString(it.key(),Qt::ISODate);
        // date
        if (key.isValid()==false){
            result.errorStringUI = tr("Entry key %1 in AnnualVariableGrowth table is not a valid ISO Date").arg(it.key());
            result.errorStringLog = QString("Entry key %1 in AnnualVariableGrowth table is not a valid ISO Date").arg(it.key());
            return g;
        }
        if (key.day() != 1){
            result.errorStringUI = tr("Entry key %1 in AnnualVariableGrowth table has Day not set to 1").arg(key.toString(Qt::ISODate));
            result.errorStringLog = QString("Entry key %1 in AnnualVariableGrowth table has Day not set to 1").arg(key.toString(Qt::ISODate));
            return g;
        }
        // growth value
        QJsonValueRef valRef = it.value();
        if ( valRef.isDouble() == false){
            result.errorStringUI = tr("Value %1 in AnnualVariableGrowth table is not a number").arg(valRef.toString());
            result.errorStringLog = QString("Value %1 in AnnualVariableGrowth table is not a number").arg(valRef.toString());
            return g;
        }
        d = valRef.toDouble();
        qint64 value = Util::extractQint64FromDoubleWithNoFracPart(d,ok);
        if ( ok==-1 ){
            result.errorStringUI = tr("AnnualVariableGrowth value %1 is not an integer").arg(d);
            result.errorStringLog = QString("AnnualVariableGrowth value %1 is not an integer").arg(d);
            return g;
        }
        if ( ok!=0 ){
            result.errorStringUI = tr("Value %1 in AnnualVariableGrowth table is invalid for unknown reason").arg(d);
            result.errorStringLog = QString("Value %1 in AnnualVariableGrowth table is invalid for unknown reason").arg(d);
            return g;
        }
        if ( value>MAX_GROWTH_DECIMAL ){
            result.errorStringUI = tr("Value %1 in AnnualVariableGrowth table is bigger than the maximum allowed of %2").arg(value).arg(MAX_GROWTH_DECIMAL);
            result.errorStringLog = QString("Value %1 in AnnualVariableGrowth table is bigger than the maximum allowed of %2").arg(value).arg(MAX_GROWTH_DECIMAL);
            return g;
        }
        if ( value<MIN_GROWTH_DECIMAL) {
            result.errorStringUI = tr("Value %1 in AnnualVariableGrowth table is smaller than the minimum value of %2").arg(value).arg(MIN_GROWTH_DECIMAL);
            result.errorStringLog = QString("Value %1 in AnnualVariableGrowth table is smaller than the minimum value of %2").arg(value).arg(MIN_GROWTH_DECIMAL);
            return g;
        }
        // commit
        f.insert(key, value);
    }
    // type
    jsonValue = jsonObject.value("Type");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("Cannot find Type tag");
        result.errorStringLog = "Cannot find Type tag";
        return g;
    }
    if ( jsonValue.isDouble() == false){
        result.errorStringUI = tr("Type tag %1 is not a number").arg(str);
        result.errorStringLog = QString("Type tag %1 is not a number").arg(str);
        return g;
    }
    d = jsonValue.toDouble();
    qint64 typeInt = Util::extractQint64FromDoubleWithNoFracPart(d,ok);
    if ( ok==-1 ){
        result.errorStringUI =  tr("Type tag %1 is not an integer").arg(d);
        result.errorStringLog = QString("Type tag %1 is not an integer").arg(d);
        return g;
    }
    if ( ok==-2 ){
        result.errorStringUI =  tr("Type tag %1 is far too big").arg(d);
        result.errorStringLog = QString("Type tag %1 is far too big").arg(d);
        return g;
    }
    switch(typeInt){
        case 0:  // CONSTANT
            g = fromConstantAnnualPercentageDecimal(growth);
            break;
        case 1:  // VARIABLE
            g = fromVariableDataAnnualBasisDecimal(f);
            break;
        case 2:  // NONE
            g = Growth();
            break;
        default:
            result.errorStringUI = tr("Type tag %1 value is unknown").arg(typeInt);
            result.errorStringLog = QString("Type tag %1 value is unknown").arg(typeInt);
            return g;
        }

    result.success = true;
    return g;
}


// adjust all the growth values by multiplying by the factor, which must not be negative.
// Resulting growth value(s) are capped to the max allowed (MAX_GROWTH_DECIMAL) and
// if this happens at least once, capped is set to true.
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


// Convert a double representing annual growth percentage into the decimal form equivalent.
// Double precision is truncated to no of decimals defined in Growth. Loss of precision can occur.
// Value of d is assumed to be in the validity range.
qint64 Growth::fromDoubleToDecimal(long double d)
{
    qint64 v = static_cast<qint64>(round(d*Util::quickPow10(NO_OF_DECIMALS)));
    return v;
}


// Convert a decimal representing annual growth percentage into the double form equivalent (direct percentage).
// Loss of precision can occur.
// Value of i is assumed to be in the validity range.
long double Growth::fromDecimalToDouble(qint64 i)
{
    double d = static_cast<double>(i)/Util::quickPow10(NO_OF_DECIMALS);
    return d;
}


// Given a series of occurrences in time for a given initially fixed amount, adjust each amount for each
// occurrence date to take into account the growth pattern defined by this object. Also convert the amounts
// to present value if requested.
//
// If a calculated amount goes over the max defined in CurrencyHelper, it is set to this max value
// and the no of times it occured is returned as a warning (this is called "saturation").
// Growth is 0 before occurrenceDates.first. Growth is never applied for the first date in occurrenceDates,
// which is the reference date for that amount (first occurrence).
//
// Input parameters :
//   amount : the initial fixed amount. Must be >= 0.
//   occurrenceDates : the list of event occurrence. Must be sorted in order of increase date values.
//   appStrategy : Specify how growth is applied on the amount.
//   pvDiscountRate : annual discount rate used in PV calculation (percentage). Must be >=0, updated once a month on the 1st
//   pvPresent : "present date" used in the Present Value calculation.
// Output parameters :
//   QMap<QDate, qint64> : the occurrence dates with the final associated calculated amounts
//   ok : result of the method, with details if error occured
QMap<QDate, qint64> Growth::adjustForGrowth(quint64 amount,  QList<QDate> occurrenceDates, ApplicationStrategy appStrategy, double pvDiscountRate,
                                            QDate pvPresent, AdjustForGrowthResult &ok) const
{
    QMap<QDate, qint64> result = {};
    ok.success = false;
    ok.saturationCount = 0;
    ok.errorMessageUI = "";
    ok.errorMessageLog = "";

    // arguments validity check
    if (occurrenceDates.size()==0){
        ok.success = true;
        return result;  // no occurrence, so empty set returned
    }
    for (int i = 0; i < (occurrenceDates.size() - 1); i++) {
        if (occurrenceDates[i] > occurrenceDates[i + 1]) {
            ok.errorMessageUI = tr("%1 : OccurrenceDates are not sorted properly").arg(__func__);
            ok.errorMessageLog = QString("%1 : OccurrenceDates are not sorted properly").arg(__func__);
            return result;
        }
    }
    if ( (appStrategy.noOfMonths<1) ){
        ok.errorMessageUI = tr("%1 : AppStrategy.noOfMonth is invalid").arg(__func__);
        ok.errorMessageLog = QString("%1 : AppStrategy.noOfMonth is invalid").arg(__func__);
        return result;
    }
    if ( amount > CurrencyHelper::maxValueAllowedForAmount()){
        ok.errorMessageUI = tr("%1 : Amount is too big ").arg(__func__);
        ok.errorMessageLog = QString("%1 : Amount is too big ").arg(__func__);
        return result;
    }
    if ( pvDiscountRate<0 ){
        ok.errorMessageUI = tr("%1 : Present Value annual discount rate smaller than 0").arg(__func__);
        ok.errorMessageLog =QString("%1 : Present Value annual discount rate is smaller than 0").arg(__func__);
        return result;
    }
    if ( pvPresent.isValid()==false ){
        ok.errorMessageUI = tr("%1 : PV Present Date is invalid").arg(__func__);
        ok.errorMessageLog = QString("%1 : PV Present Date is invalid").arg(__func__);
        return result;
    }
    // if ( pvPresent > occurrenceDates.first() ){
    //     ok.errorMessageUI = tr("%1 : PV Present Date is more recent than first occurrence date").arg(__func__);
    //     ok.errorMessageLog = QString("%1 : PV Present Date is more recent than first occurrence date").arg(__func__);
    //     return result;
    // }


    // *** preparation for calculation ***

    uint noOfMonthsCycle = appStrategy.noOfMonths;
    uint occurrenceCounter = 0;                  // used to know when to apply the growth.

    // GROWTH : build monthy cummulative growth multiplier vector.
    // From first occurrence to last, this will provide a cummulative growth factor
    // we can use to multiply the originally fix amount to get the growth-adjusted amount
    int noOfMonthCovered = 1 + Util::noOfMonthDifference(occurrenceDates.first(), occurrenceDates.last()); // No of month spanned in the occurrenceVector : 1 to infinity
    QSharedPointer<long double> multiplierVector = buildMonthlyMultiplierVector(noOfMonthCovered,occurrenceDates.first());  // Index 0 is first month of occurrence
    long double* data = multiplierVector.data();

    // PRESENT VALUE : build monthly Present Value multiplier.
    // Computed from "Present", but applied from first occurrence to last, this will provide a
    // "future to present value" factor we can use to multiply the originally fix amount
    int pvNoOfMonthCovered = 1 + Util::noOfMonthDifference(occurrenceDates.first(), occurrenceDates.last());
    QSharedPointer<long double> pvMultiplierVector = buildPvMonthlyMultiplierVector(pvDiscountRate,pvNoOfMonthCovered, occurrenceDates.first(),pvPresent);  // Index 0 is first month of occurrence
    long double* dataPv = pvMultiplierVector.data();

    // *** calculation ***

    long double growthMultiplier=1;
    foreach(QDate date, occurrenceDates){
        occurrenceCounter++;

        // what month are we ?
        int multiplierVectorIndex = Util::noOfMonthDifference(occurrenceDates.first(),date);

        // Update growth only every N occurrences
        if ( ((occurrenceCounter-1) % noOfMonthsCycle) == 0 ){
            growthMultiplier  =  data[multiplierVectorIndex];
        }

        // the calculated amount can be outside the allowed range defined in CurrencyHelper.
        // If it happens, it is called "saturation". We just cap the value to the min/max and continue processing
        long double t =  std::round(static_cast<long double>(amount) * growthMultiplier * dataPv[multiplierVectorIndex]);
        if ( t > static_cast<long double>(CurrencyHelper::maxValueAllowedForAmount()) ){
            t = static_cast<long double>(CurrencyHelper::maxValueAllowedForAmount());
            ok.saturationCount++;
        }

        result.insert(date,static_cast<qint64>(t));
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


// Build a QT-wrapped long double vector containing, for each month in a given date interval, a "multiplier"
// (the CGF or Cummulative Growth Factor) to be applied against an initial and constant amount in order to give
// the "cummulative growth adjusted" value for this amount. noOfMonth must be > 0
// "From" is the date of the first occurrence of the amount and is always assigned CGF of "1"
// (no growth, as it is the reference value).
QSharedPointer<long double> Growth::buildMonthlyMultiplierVector(uint noOfMonths, QDate from) const {
    if (noOfMonths==0){
        throw std::domain_error("noOfMonth must be > 0");
    }
    if (from.isValid()==false){
        throw std::domain_error("Date is invalid");
    }

    QSharedPointer<long double> multiplierVector (new long double[noOfMonths] ); // resulting vector
    long double* data = multiplierVector.data();  // easy alias
    long double cgf = 1;    // CGF : long double to maximize no of significant digits
    // init multiplier vector to all CGF=1 (no growth at all)
    for(uint i=0; i < noOfMonths; i++){
        data[i] = cgf;
    }

    if( (type==NONE) || (noOfMonths==1) ){
        return multiplierVector;
    }

    if (type==CONSTANT){
        // *** CONSTANT ***
        for(uint i=1; i < noOfMonths; i++){
            cgf = cgf * ( 1 + monthlyConstantGrowth/100.0L);
            data[i] = cgf;
        }
    } else {
        // *** VARIABLE ***

        if( monthlyVariableGrowth.size()!=0 ){

            long double currentMonthlyGrowth = 0; // current monthly growth in effect, NOT inpercentage (e.g. 0.1 , -0.15)
            uint index = 1; // position of insertion in multiplier vector : skip first one
            QDate indexDate = from;
            indexDate.setDate(from.year(), from.month(),1); // reset Day to 1 to prevent problem (e.g. 29 feb)
            indexDate = indexDate.addMonths(1);
            DateRange transitionSpace = DateRange(monthlyVariableGrowth.firstKey(),monthlyVariableGrowth.lastKey());

            // get the latest growth value defined before SECOND date
            if ( transitionSpace.includeDate(indexDate) && (monthlyVariableGrowth.contains(indexDate)==false) ){
                // we have to find the closest growth defined in the past
                for (QMap<QDate, long double>::const_iterator it = monthlyVariableGrowth.cbegin(), end = monthlyVariableGrowth.cend(); it != end; ++it) {
                    if(it.key() >= indexDate){
                        break;
                    }
                    currentMonthlyGrowth = it.value()/100.0L;
                }
            } else if (transitionSpace.getEnd() < indexDate) {
                currentMonthlyGrowth = monthlyVariableGrowth.last()/100.0L; // get last growth defined
            }

            while( index < noOfMonths ){
                // any new growth defined for that date ?
                if(monthlyVariableGrowth.contains(indexDate)==true){
                    // set the new growth value
                    currentMonthlyGrowth = monthlyVariableGrowth.value(indexDate)/100.0L;
                }
                cgf = cgf * (1.0 + currentMonthlyGrowth);
                // set result entry
                data[index] = cgf;
                // go to next item
                indexDate = indexDate.addMonths(1);
                index ++;
            }
        }

    }

    return multiplierVector;
}


// annualDiscountrate : in percentage
// Be careful : first occurrence date will probably be different from PV present date (before or after).
// It means we have to find the first value of the PV factor to be assigned to the first value of the vector
QSharedPointer<long double> Growth::buildPvMonthlyMultiplierVector(double annualDiscountrate, uint noOfMonths, QDate firstOccurrence, QDate pvPresent) const
{
    // check inout parameters
    if (noOfMonths==0){
        throw std::domain_error("noOfMonth must be > 0");
    }
    if (pvPresent.isValid()==false){
        throw std::domain_error("PV date is invalid");
    }
    if (firstOccurrence.isValid()==false){
        throw std::domain_error("First occurrence date is invalid");
    }

    long double monthlyDiscountRate = Util::annualToMonthlyGrowth(annualDiscountrate); // in percentage
    QSharedPointer<long double> multiplierVector (new long double[noOfMonths] ); // resulting vector
    long double* data = multiplierVector.data();  // easy alias

    // how many PV periods are already passed before reaching the first occurrence
    int pvPeriodOffset = Util::noOfMonthDifference(pvPresent, firstOccurrence);

    for(uint i=0; i < noOfMonths; i++){
        long double temp = Util::presentValueConversionFactor(monthlyDiscountRate,pvPeriodOffset+i);
        data[i] = temp; // temp is to ease debugging
    }

    return multiplierVector;
}


void Growth::isFactorsValid(QMap<QDate, qint64> factorsToBeChecked, Util::OperationResult &result)
{
    result.success = false;
    result.errorStringUI = "";
    result.errorStringLog = "";

    // be sure change date is set to Day 1, value must not be < 100
    foreach(QDate date, factorsToBeChecked.keys()){
        if (date.isValid()==false){
            result.errorStringUI = tr("Date %1 is invalid").arg(date.toString());
            result.errorStringLog = QString("Date %1 is invalid").arg(date.toString());
            return ;
        }
        if (date.day() != 1){
            result.errorStringUI = tr("Date %1 is invalid because Day is not set to 1").arg(date.toString());
            result.errorStringLog = QString("Date %1 is invalid because Day is not set to 1").arg(date.toString());
            return ;
        }
    }
    // be sure growth is in the right range
    foreach(double val, factorsToBeChecked.values()){
        if ( (val < MIN_GROWTH_DECIMAL) ){
            result.errorStringUI = tr("Growth %1 is smaller than the minimum allowed of %2").arg(val,Growth::MIN_GROWTH_DECIMAL);
            result.errorStringLog = QString("Growth %1 is smaller than the minimum allowed of %2").arg(val,Growth::MIN_GROWTH_DECIMAL);
            return ;
        }
        if ( (val>MAX_GROWTH_DECIMAL) ){
            result.errorStringUI = tr("Growth %1 is bigger than the maximum allowed of %2").arg(val, Growth::MAX_GROWTH_DECIMAL);
            result.errorStringLog = QString("Growth %1 is bigger than the maximum allowed of %2").arg(val, Growth::MAX_GROWTH_DECIMAL);
            return ;
        }    }
    // all is well
    result.success = true;
    return ;
}


// Calculate the new value of an amount when growth is applied each month, for date interval [from,to].
// First month has no growth applied (reference value).
// The same growth must absolutely apply for all these months, that is [from, to]
// monthlyGrowth is expressed in percentage (double)
long double Growth::calculateNewAmountConstantGrowth(QDate from, QDate to, long double originalAmount,
                                                     long double monthlyGrowth) const
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
    int noOfMonth = Util::noOfMonthDifference(from, to);
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




