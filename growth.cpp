/*
 *  Copyright (C) 2024 Claude Dumas <claudedumas63@protonmail.com>
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

#include <QVariantMap>
#include <QSharedPointer>
#include <QString>
#include <QCoreApplication>
#include "math.h"
#include "growth.h"
#include "util.h"
#include "currencyhelper.h"


uint Growth::NO_OF_DECIMALS = 5;

// this is for ANNUAL growth
double Growth::MIN_GROWTH_DOUBLE = -100;    // min annual percentage, -100% per year, give also -100% a month.
double Growth::MAX_GROWTH_DOUBLE = 10000;   // max annual percentage, +10,000 % a year is going from 1 to 101, give 46.901686 % a month
qint64 Growth::MIN_GROWTH_DECIMAL = static_cast<qint64>(Growth::MIN_GROWTH_DOUBLE*(pow(10,Growth::NO_OF_DECIMALS)));
qint64 Growth::MAX_GROWTH_DECIMAL = static_cast<qint64>(Growth::MAX_GROWTH_DOUBLE*(pow(10,Growth::NO_OF_DECIMALS)));



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


// Given a series of occurence in time for a given amount, adjust each amount of each date to take
// into account the growth pattern defined by this object.
// If calculated amount goes over the max defined in CurrencyHelper, it is set to these max values and
// the no of times it occurs is returned as a warning (this is called "saturation").
// Amount must be >= 0.
// occurenceDates must be sorted in order of increase date values.
// Growth is 0 before occurenceDates.first.
// Growth is never applied for the first date in occurenceDates, which is the reference date for that amount (first occurence).
//
QMap<QDate, qint64> Growth::adjustForGrowth(qint64 amount,  QList<QDate> occurenceDates, ApplicationStrategy appStrategy, AdjustForGrowthResult &ok) const
{
    QMap<QDate, qint64> result = {};
    ok.success = false;
    ok.saturationCount = 0;
    ok.errorMessageUI = "";
    ok.errorMessageLog = "";

    // arguments validity check
    if (occurenceDates.size()==0){
        ok.success = true;
        return result;  // no occurence, so empty set returned
    }
    for (int i = 0; i < (occurenceDates.size() - 1); i++) {
        if (occurenceDates[i] > occurenceDates[i + 1]) {
            ok.errorMessageUI = tr("OccurenceDates is not sorted properly");
            ok.errorMessageLog = "OccurenceDates is not sorted properly";
            return result;
        }
    }
    if ( (appStrategy.noOfMonths<1) ){
        ok.errorMessageUI = tr("AppStrategy.noOfMonth is invalid");
        ok.errorMessageLog = "AppStrategy.noOfMonth is invalid";
        return result;
    }
    if ( amount > CurrencyHelper::maxValueAllowedForAmount()){
        ok.errorMessageUI = tr("Amount is too big ");
        ok.errorMessageLog = "Amount is too big ";
        return result;
    }
    if ( amount < 0){
        ok.errorMessageUI = tr("Amount is smaller than 0");
        ok.errorMessageLog = "Amount is smaller than 0";
        return result;
    }

    // do nothing if no growth
    if(type==NONE){
        foreach(QDate date, occurenceDates){
            result.insert(date,amount);
        }
        ok.success = true;
        return result;
    }

    // prepare
    //qint64 currentAmount = amount;              // calculated amount at every occurence,
    qint64 lastAmountInserted = amount;         // last amount inserted in the result
    //QDate lastDate = occurenceDates.first();    // insure first occurence has no growth applied to
    uint noOfMonthsCycle = appStrategy.noOfMonths;
    uint occurenceCounter = 0;                  // used to know when to apply new growth.

    // build monthy multiplier vector (cummulative)
    int noOfMonthCovered = noOfMonthSpanned(occurenceDates.first(), occurenceDates.last()); // No of month spanned in the occurenceVector : 1 to infinity
    QSharedPointer<long double> multiplierVector = buildMonthlyMultiplierVector(noOfMonthCovered,occurenceDates.first());  // Index 0 is first month of occurence
    long double* data = multiplierVector.data();

    // calculation
    foreach(QDate date, occurenceDates){
        occurenceCounter++;
        if ( ((occurenceCounter-1) % noOfMonthsCycle) == 0 ){
            int multiplierVectorIndex = noOfMonthSpanned(occurenceDates.first(),date)-1;

            // the calculated amount can be outside the allowed range defined in CurrencyHelper.
            // If it happens, it is called "saturation". We just cap the value to the min/max and continue processing
            long double t =  std::round(static_cast<long double>(amount) * data[multiplierVectorIndex]);
            if ( t > static_cast<long double>(CurrencyHelper::maxValueAllowedForAmount()) ){
                t = static_cast<long double>(CurrencyHelper::maxValueAllowedForAmount());
                ok.saturationCount++;
            }

            lastAmountInserted = static_cast<qint64>(t);
        }
        result.insert(date,lastAmountInserted);
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
// to be applied against an initial and constant amount in order to give the "cummulative growth adjusted" value for this amount.
// noOfMonth must be > 0
// "From" is the date of the first occurence and is always assigned multiplier "1" (no growth, as it is the reference value).
QSharedPointer<long double> Growth::buildMonthlyMultiplierVector(uint noOfMonths, QDate from) const {
    if (noOfMonths==0){
        throw std::domain_error("noOfMonth must be > 0");
    }
    if (from.isValid()==false){
        throw std::domain_error("Date is invalid");
    }

    QSharedPointer<long double> multiplierVector (new long double[noOfMonths] );
    long double* data = multiplierVector.data();  // easy alias
    long double currentMultiplier = 1;      // long double to max no of significant digits
    data[0] = 1; // first multiplier is always 1, for CONSTANT or VARIABLE

    if (type==CONSTANT){
        // *** CONSTANT ***
        if (noOfMonths>1){
            for(uint i=1; i < noOfMonths; i++){
                currentMultiplier = currentMultiplier + (currentMultiplier * monthlyConstantGrowth/100.0L);
                data[i] = currentMultiplier;
            }
        }
    } else {
        // *** VARIABLE ***
        long double currentGrowthFactor = 0; // monthly growth
        if (noOfMonths>1){
            QDate indexDate = from; // Date corresponding to each index in Data array
            indexDate.setDate(from.year(), from.month(),1); // reset Day to 1 to prevent problem (e.g. 29 feb)
            for(uint i=1; i < noOfMonths; i++){
                // date for this index
                indexDate = indexDate.addMonths(1);
                // any new growth value ? if yes, update the multiplier
                if(monthlyVariableGrowth.contains(indexDate)){
                    currentGrowthFactor = monthlyVariableGrowth.value(indexDate)/100.0L;
                }
                currentMultiplier = currentMultiplier + (currentMultiplier * (currentGrowthFactor));
                data[i] = currentMultiplier;
            }
        }
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
    uint noOfMonth = noOfMonthSpanned(from, to) - 1;
    return originalAmount*pow((long double)(1+(monthlyGrowth/100.0)), noOfMonth);
}


// Calculate the no of months covered by [from,to].
// 2 dates inside the same month produce a result of 1.
uint Growth::noOfMonthSpanned(QDate from , QDate to) const{
    if (from.isValid()==false){
        throw std::invalid_argument("from is an invalid date");
    }
    if (to.isValid()==false){
        throw std::invalid_argument("to is an invalid date");
    }
    if(to<from){
        throw std::invalid_argument("to is before from");
    }
    return 1 + ( (12*to.year())+to.month()) - ( (12*from.year())+from.month()) ;
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




