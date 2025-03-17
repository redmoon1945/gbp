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

#include "irregularfestreamdef.h"
#include "currencyhelper.h"



int IrregularFeStreamDef::AmountInfo::NOTES_MAX_LEN = 100;

IrregularFeStreamDef::IrregularFeStreamDef() : FeStreamDef()
{
    this->amountSet = {};
}


IrregularFeStreamDef::IrregularFeStreamDef(QMap<QDate,AmountInfo> amountSet, const QUuid &id, const QString &name, const QString &desc, bool active, bool isIncome, const QColor& decorationColor)
    : FeStreamDef(id, name,desc,FeStreamType::IRREGULAR,active,isIncome, decorationColor)
{
    // make sure amountSet is valid
    IrregularFeStreamDef::validateKeysResult r = validateKeysAndValues(amountSet);
    if (r.valid==false){
        throw std::invalid_argument(r.reasonLog.toLocal8Bit().data());
    }
    this->amountSet =  amountSet;
}


IrregularFeStreamDef::IrregularFeStreamDef(const IrregularFeStreamDef &o) :
    FeStreamDef(o)
{
    this->amountSet = o.amountSet;
}


IrregularFeStreamDef &IrregularFeStreamDef::operator=(const IrregularFeStreamDef &o)
{
    FeStreamDef::operator=(o);
    this->amountSet = o.amountSet;
    return *this;
}

bool IrregularFeStreamDef::operator==(const IrregularFeStreamDef &o) const
{
    if ( !(FeStreamDef::operator==(o)) || !(this->amountSet == o.amountSet)){
        return false;
    } else {
        return true;
    }
}


bool IrregularFeStreamDef::operator!=(const IrregularFeStreamDef &o) const
{
    return !(*this==o);
}


IrregularFeStreamDef::~IrregularFeStreamDef()
{
}



// Generate the whole suite of financial events for that Stream Definition.
// Input params :
//   DateRange fromTo : interval of time inside which the events should be generated
//   maxDateScenarioFeGeneration : Date where all FE generation must stop (derived from scenario)
//   double pvDiscountRate : ANNUAL discount rate in percentage to apply to transform the amounts
//                           to Present Value. Value of 0 means do not transform future values into
//                           present value
//   QDate pvPresent : first date (origin) to use for discounting FE amount (usually = tomorrow)
// Output params:
//   QList<Fe> : the ordered (by inc. time) list of financial events.
//   uint &saturationCount : number of times the FE amount was over the maximum allowed
//   FeMinMaxInfo& minMaxInfo : computed min/max for values (absolute value, never negative).
//                              Invalid if 0 element returned.
QList<Fe> IrregularFeStreamDef::generateEventStream(DateRange fromto,
    QDate maxDateScenarioFeGeneration, double pvAnnualDiscountRate, QDate pvPresent,
    uint &saturationCount, FeMinMaxInfo &minMaxInfo) const
{
    QList<Fe> ss;
    saturationCount = 0;
    minMaxInfo.yMin =std::numeric_limits<qint64>::max();
    minMaxInfo.yMax = std::numeric_limits<qint64>::min();

    // is FeStreamDefinition is inactive, do not generate any Fe
    if (!active)  {
        return ss;
    }
    // if no data
    if ( amountSet.size()==0 ){
        return ss;
    }

    long double monthlyDiscountRate = Util::annualToMonthlyGrowth(pvAnnualDiscountRate);

    // iterate once in the set to generate Fes and convert future values to present values
    foreach (const QDate date, amountSet.keys()) {
        // still in the validity range of this stream def ? Still in the fromto range ? Still under the max allowed ?
        if ( fromto.includeDate(date) && (date<=maxDateScenarioFeGeneration) ){
            qint64 temp = amountSet.value(date).amount;

            // *** convert to present value (applied on a monthly basis) ***
            // how many PV periods (months) have already passed before reaching the event date
            int pvPeriods = Util::noOfMonthDifference(pvPresent, date);
            // calculate FV to PV factor
            long double factor = Util::presentValueConversionFactor(monthlyDiscountRate,pvPeriods);
            // calculate the PV (in integer)
            qint64 pv = static_cast<qint64>(std::round((static_cast<long double>(temp) * factor)));
            // *********************************

            // build Fe and insert in the result list
            qint64 feAmount = (isIncome?(pv):-(pv));
            Fe fe = {.amount=feAmount,.occurrence=date, .id=this->id};
            // set min max for amount
            if( abs(feAmount) > minMaxInfo.yMax){
                minMaxInfo.yMax = abs(feAmount);
            }
            if( abs(feAmount) < minMaxInfo.yMin){
                minMaxInfo.yMin = abs(feAmount);
            }
            ss.append(fe);
        }
    }

    return ss;
}


// Compare this FeStreamDef with another one and evaluate if the list of FE generated will be
// exactly the same.
bool IrregularFeStreamDef::evaluateIfSameFeList(const IrregularFeStreamDef &o) const
{
    if (FeStreamDef::evaluateIfSameFeList(o)==false){
        return false;
    }
    if ( amountSet != o.amountSet ) {
        return false;
    }
    return true;
}


QString IrregularFeStreamDef::toStringForDisplay(CurrencyInfo currInfo, QLocale locale) const
{
    // order is : <amount> on <date>, {and more...}
    // E.G. :
    //     Payments for XYZ

    if (amountSet.isEmpty()){
        return tr("No event defined");
    } else {
        QString sAmount, sDate, sFinal;
        int ok;

        QDate date = amountSet.firstKey();
        AmountInfo ai = amountSet.value(date);
        sAmount = CurrencyHelper::quint64ToDoubleString(ai.amount, currInfo, locale, false, ok);
        if ( ok != 0 ){
            return "Error";  // amount or noOfCurrencyDecimals is too big, should not happen
        }
        sDate = date.toString(Qt::ISODate);
        if (amountSet.size()>1){
            sFinal = tr("%1 on %2 and %3 more...").arg(sAmount).arg(sDate).arg(amountSet.size()-1);
        } else {
            sFinal = tr("%1 on %2").arg(sAmount,sDate);
        }
        return sFinal;
    }
}


QJsonObject IrregularFeStreamDef::toJson() const
{
    QJsonObject jobject;

    // base class data
    FeStreamDef::toJson(jobject);
    // this derived class data
    QJsonObject jobjectAmountSet;
    for (auto it = amountSet.begin(); it != amountSet.end(); ++it) {
        QDate date = it.key();
        AmountInfo o = it.value();
        jobjectAmountSet[date.toString((Qt::ISODate))] = o.toJson();
    }
    jobject["AmountSet"] = jobjectAmountSet;
    // return result
    return jobject;
}



IrregularFeStreamDef IrregularFeStreamDef::fromJson(const QJsonObject &jsonObject, Util::OperationResult &result)
{
    QJsonValue jsonValue;
    result.success = false;
    result.errorStringUI = "";
    result.errorStringLog = "";
    IrregularFeStreamDef is;
    double d;
    int ok;

    // *** BASE CLASS DATA ***
    QUuid id;
    QString name,desc;
    bool active;
    bool isIncome;
    QColor decoColor;
    Util::OperationResult resultBaseClass;
    FeStreamDef::fromJson(jsonObject, IRREGULAR, id, name, desc, active, isIncome, decoColor, resultBaseClass);
    if(resultBaseClass.success==false){
        result.errorStringUI = tr("IrregularFeStreamDef - ")+resultBaseClass.errorStringUI;
        result.errorStringLog = "IrregularFeStreamDef - "+resultBaseClass.errorStringLog;
        return is;
    }

    // *** DERIVED CLASS DATA ***
    // AmountSet
    jsonValue = jsonObject.value("AmountSet");
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("IrregularFeStreamDef - Cannot find AmountSet tag");
        result.errorStringLog = "IrregularFeStreamDef - Cannot find AmountSet tag";
        return is;
    }
    if (jsonValue.isObject()==false){
        result.errorStringUI = tr("IrregularFeStreamDef - AmountSet tag is not an object");
        result.errorStringLog = "IrregularFeStreamDef - AmountSet tag is not an object";
        return is;
    }
    QMap<QDate,AmountInfo> f;
    QJsonObject amountSetObject = jsonValue.toObject();
    for (auto it = amountSetObject.begin(); it != amountSetObject.end(); ++it) {
        QString keyString = it.key();
        QDate s = QDate::fromString(keyString,Qt::ISODate);
        if( !(s.isValid()) ) {
            result.errorStringUI = tr("IrregularFeStreamDef - Key %1 is not a valid ISO Date").arg(keyString);
            result.errorStringLog = QString("IrregularFeStreamDef - Key %1 is not a valid ISO Date").arg(keyString);
            return is;
        }
        if (!it.value().isObject()){
            result.errorStringUI = tr("IrregularFeStreamDef - Value for Key %1 is not an object").arg(keyString);
            result.errorStringLog = QString("IrregularFeStreamDef - Value for Key %1 is not an object").arg(keyString);
            return is;
        }
        QJsonObject valueObject = it.value().toObject();
        Util::OperationResult aiParsingResult;
        AmountInfo ai = AmountInfo::fromJson(valueObject,aiParsingResult);
        if (aiParsingResult.success==false){
            result.errorStringUI = aiParsingResult.errorStringUI;
            result.errorStringLog = aiParsingResult.errorStringLog;
            return is;
        }
        f.insert(s, ai);
    }
    IrregularFeStreamDef::validateKeysResult r = validateKeysAndValues(f);
    if (r.valid==false){
        result.errorStringUI = tr("IrregularFeStreamDef - Map is invalid -> %1").arg(r.reasonUI);
        result.errorStringLog = QString("IrregularFeStreamDef - Map is invalid -> %1").arg(r.reasonLog);
        return is;
    }

    // build new IrregularFeStreamDef and return
    result.success=true;
    return IrregularFeStreamDef(f, id, name, desc, active, isIncome, decoColor);
}


IrregularFeStreamDef IrregularFeStreamDef::duplicate() const
{
    IrregularFeStreamDef is = *this;
    QString newName = QString("%1 %2").arg(tr("Copy of")).arg(name);
    newName.truncate(FeStreamDef::NAME_MAX_LEN);
    is.setId(QUuid::createUuid());
    is.setName(newName);
    return is;
}


IrregularFeStreamDef::validateKeysResult IrregularFeStreamDef::validateKeysAndValues(const QMap<QDate, AmountInfo> infoSet)
{
    validateKeysResult result={.valid=false,.reasonUI="", .reasonLog=""};

    foreach (const QDate key, infoSet.keys()) {
        if (!(key.isValid()) ){
            result.reasonUI = tr("Date %1 is invalid").arg(key.toString());
            result.reasonLog = QString("Date %1 is invalid").arg(key.toString().toLocal8Bit().data());
            return result;
        }
        // make sure values are not negative or too big
        AmountInfo ai = infoSet.value(key);
        if (ai.amount<0){
            result.reasonUI = tr("Amount %1 for date %2 cannot be negative (set isIncome to false instead)").arg(ai.amount).arg(key.toString());
            result.reasonLog = QString("Amount %1 for date %2 cannot be negative (set isIncome to false instead)").arg(ai.amount).arg(key.toString().toLocal8Bit().data());
            return result;
        }
        if (ai.amount>CurrencyHelper::maxValueAllowedForAmount()){
            result.reasonUI = tr("Amount %1 for date %2 is too big").arg(ai.amount).arg(key.toString());
            result.reasonLog = QString("Amount %1 for date %2 is too big").arg(ai.amount).arg(key.toString().toLocal8Bit().data());
            return result;
        }
    }
    result.valid = true;
    return result;
}


bool IrregularFeStreamDef::AmountInfo::operator==(const AmountInfo &o) const
{
    if ( (amount!=o.amount) || (notes!=o.notes) ){
        return false;
    }else{
        return true;
    }
}

QJsonObject IrregularFeStreamDef::AmountInfo::toJson() const
{
    QJsonObject jobject;
    jobject["Amount"] = QVariant::fromValue(amount).toJsonValue();
    jobject["Notes"] = notes.left(NOTES_MAX_LEN);
    return jobject;
}


// can throw "std::domain_error" if invalid format
IrregularFeStreamDef::AmountInfo IrregularFeStreamDef::AmountInfo::fromJson(const QJsonObject &jsonObject, Util::OperationResult &result)
{
    IrregularFeStreamDef::AmountInfo ai;
    QJsonValue jsonValue;
    double d;
    int ok;

    result.success = false;
    result.errorStringUI = "";
    result.errorStringLog = "";

    // Amount
    jsonValue = jsonObject["Amount"];
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("Cannot find Amount tag","IrregularFeStreamDef");
        result.errorStringLog = "Cannot find Amount tag";
        return ai;
    }
    if (jsonValue.isDouble()==false){
        result.errorStringUI = tr("Amount tag is not a number");
        result.errorStringLog ="Amount tag is not a number";
        return ai;
    }
    d = jsonValue.toDouble();
    qint64 amount = Util::extractQint64FromDoubleWithNoFracPart(d,ok);
    if ( ok==-1 ){
        result.errorStringUI = tr("Amount value %1 is not an integer").arg(d);
        result.errorStringLog = QString("Amount value %1 is not an integer").arg(d);
        return ai;
    }
    if (  (ok==-2) || (abs(amount)>CurrencyHelper::maxValueAllowedForAmount()) ){
        result.errorStringUI = tr("Amount value %1 is either too small or too big").arg(d);
        result.errorStringLog = QString("Amount value %1 is either too small or too big").arg(d);
        return ai;
    }

    // notes
    jsonValue = jsonObject["Notes"];
    if (jsonValue == QJsonValue::Undefined){
        result.errorStringUI = tr("Cannot find Notes tag");
        result.errorStringLog = "Cannot find Notes tag";
        return ai;
    }
    if (jsonValue.isString()==false){
        result.errorStringUI = tr("Notes tag is not a string");
        result.errorStringLog = "Notes tag is not a string";
        return ai;
    }
    QString notes = jsonValue.toString();
    if (notes.length()>NOTES_MAX_LEN){
        result.errorStringUI = tr("Notes length is %1, which is bigger than maximum allowed of %2").arg(notes.length()).arg(NOTES_MAX_LEN);
        result.errorStringLog = QString("Notes length is %1, which is bigger than maximum allowed of %2").arg(notes.length()).arg(NOTES_MAX_LEN);
        return ai;
    }

    // create struct and return
    result.success = true;
    AmountInfo finalResult ={amount,notes};
    return finalResult;
}


// GETTERS & SETTERS


QMap<QDate, IrregularFeStreamDef::AmountInfo> IrregularFeStreamDef::getAmountSet() const
{
    return amountSet;
}
