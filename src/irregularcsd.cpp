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

#include "irregularcsd.h"
#include "currencyhelper.h"
#include "festream.h"
#include "gbplogger.h"



IrregularCsd::IrregularCsd() : Csd()
{
    this->amountSet = {};
}


IrregularCsd::IrregularCsd(const QMap<QDate,AmountInfo> &amountSet, const QUuid &id,
    const QString &name,
    const QString &desc, bool active, bool isIncome, const QColor& decorationColor)
    : Csd(id, name,desc,CsdType::IRREGULAR,active,isIncome, decorationColor)
{
    // make sure amountSet is valid
    IrregularCsd::validateKeysResult r = validateKeysAndValues(amountSet);
    if (r.valid==false){
        throw std::invalid_argument(r.reasonUI.toLocal8Bit().data());
    }
    this->amountSet =  amountSet;
}


IrregularCsd::IrregularCsd(const IrregularCsd &o) :
    Csd(o)
{
    this->amountSet = o.amountSet;
}


IrregularCsd &IrregularCsd::operator=(const IrregularCsd &o)
{
    if (this != &o){    // to protect against self-assignment
        Csd::operator=(o);
        this->amountSet = o.amountSet;
    }

    return *this;
}

bool IrregularCsd::operator==(const IrregularCsd &o) const
{
    if ( !(Csd::operator==(o)) || !(this->amountSet == o.amountSet)){
        return false;
    } else {
        return true;
    }
}


bool IrregularCsd::operator!=(const IrregularCsd &o) const
{
    return !(*this==o);
}


IrregularCsd::~IrregularCsd()
{
}


void IrregularCsd::generateEventStream(FeStream& feStream, QDate tomorrow,
    DateRange fromto, QDate maxDateScenarioFeGeneration, double pvAnnualDiscountRate,
    QDate pvPresent, uint &saturationCount, FeMinMaxInfo &minMaxInfo) const
{
    // must be BOUNDED
    if (fromto.getType()!=DateRange::Type::BOUNDED) {
        throw std::invalid_argument("fromto must be of type BOUNDED");
    }

    // Reset the content of the FeStream
    feStream.reset();

    saturationCount = 0;

    // If irregular Csd is inactive, do not generate any Fe
    if (!active)  {
        return ;
    }
    // if no data
    if ( amountSet.size()==0 ){
        return ;
    }

    long double monthlyDiscountRate = Util::annualToMonthlyGrowth(pvAnnualDiscountRate);

    // iterate once in the set to generate Fes and convert values to present values. Values
    // can occur before tomorrow, equal or after.
    minMaxInfo.yMin =std::numeric_limits<qint64>::max();
    minMaxInfo.yMax = std::numeric_limits<qint64>::min();
    QList<QDate> zeKeys = amountSet.keys();
    qint64 tomorrowJulianDays = tomorrow.toJulianDay();
    foreach (const QDate date, zeKeys) {
        // still in the validity range of this stream def ?
        // Still in the fromto range ? Still under the max allowed ?
        if ( fromto.includeDate(date) && (date<=maxDateScenarioFeGeneration) ){
            quint64 temp = amountSet.value(date).amount;

            // *** convert to present value (applied on a monthly basis) ***
            // how many PV periods (months) have already passed before reaching the event date
            int pvPeriods = Util::noOfMonthsDifference(pvPresent, date);
            // calculate FV to PV factor
            long double factor = Util::toPvConversionFactor(monthlyDiscountRate,pvPeriods);
            // calculate the PV (in integer). Never a negative value.
            quint64 pv = static_cast<quint64>(std::round((static_cast<long double>(temp)
                * factor)));
            // *********************************

            // build Fe and insert in the result list
            feStream.set(date.toJulianDay()-tomorrowJulianDays, pv);
            // set min max for amount
            if( pv > minMaxInfo.yMax){
                minMaxInfo.yMax = pv;
            }
            if( pv < minMaxInfo.yMin){
                minMaxInfo.yMin = pv;
            }
        }
    }

    return ;
}


bool IrregularCsd::evaluateIfSameFeList(const IrregularCsd &o, QString& diff) const
{
    if (Csd::evaluateIfSameFeList(o, diff)==false){
        return false;
    }
    if ( amountSet != o.amountSet ) {
        diff = QString("Irregular Csd %1 : Amount sets are different").arg(REDACT(name));
        return false;
    }
    return true;
}


QString IrregularCsd::toStringForDisplay(CurrencyInfo currInfo, QLocale locale) const
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


QJsonObject IrregularCsd::toJson() const
{
    QJsonObject jobject;

    // base class data
    Csd::toJson(jobject);
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


QSharedPointer<IrregularCsd> IrregularCsd::fromJson(const QJsonObject &jsonObject,
    Util::ResultOfOperation &result)
{
    QJsonValue jsonValue;
    QSharedPointer<IrregularCsd> nullResult; // Null by default
    IrregularCsd is;
    double d;
    bool success;
    int ok;

    // Reset Result to ERROR
    result.init();

    // *** BASE CLASS DATA ***
    QUuid id;
    QString name,desc;
    bool active;
    bool isIncome;
    QColor decoColor;
    Util::ResultOfOperation resultBaseClass;
    Csd::fromJson(jsonObject, Csd::CsdType::IRREGULAR, id, name, desc, active,
        isIncome, decoColor, resultBaseClass);
    if(resultBaseClass.status == Util::ResultOfOperationStatus::ERROR){
        result.logErrorMessage = QString("IrregularCsd -> %1").arg(resultBaseClass.logErrorMessage);
        return nullResult;
    }

    // *** DERIVED CLASS DATA ***
    // AmountSet
    jsonValue = jsonObject.value("AmountSet");
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = QString("IrregularCsd %1: Cannot find token \"AmountSet\"")
            .arg(id.toString(QUuid::WithoutBraces));
        return nullResult;
    }
    if (jsonValue.isObject()==false){
        result.logErrorMessage = QString("IrregularCsd %1: AmountSet tag is not an object")
            .arg(id.toString(QUuid::WithoutBraces));
        return nullResult;
    }
    QMap<QDate,AmountInfo> f;
    QJsonObject amountSetObject = jsonValue.toObject();
    for (auto it = amountSetObject.begin(); it != amountSetObject.end(); ++it) {
        QString keyString = it.key();
        bool validDate;
        QDate s = Util::isValidISO8601Date(keyString, validDate);
        if( validDate==false ) {
            result.logErrorMessage = QString("IrregularCsd %1: Key \"%2\" is not a valid ISO Date"
                " or is invlaid")
                .arg(id.toString(QUuid::WithoutBraces)).arg(keyString);
            return nullResult;
        }
        if (!it.value().isObject()){
            result.logErrorMessage = QString("IrregularCsd %1: Value for Key %2 is not an object")
                .arg(id.toString(QUuid::WithoutBraces)).arg(keyString);
            return nullResult;
        }
        QJsonObject valueObject = it.value().toObject();
        Util::ResultOfOperation aiParsingResult;
        AmountInfo ai = AmountInfo::fromJson(valueObject,aiParsingResult);
        if (aiParsingResult.status==Util::ResultOfOperationStatus::ERROR){
            result.userErrorMessage = aiParsingResult.userErrorMessage;
            result.logErrorMessage = aiParsingResult.logErrorMessage;
            return nullResult;
        }
        f.insert(s, ai);
    }
    IrregularCsd::validateKeysResult r = validateKeysAndValues(f);
    if (r.valid==false){
        result.logErrorMessage = QString("IrregularCsd %1: Map is invalid (%2)")
            .arg(id.toString(QUuid::WithoutBraces)).arg(r.reasonLog);
        return nullResult;
    }

    // build new IrregularCsd and return
    result.status=Util::ResultOfOperationStatus::SUCCESS;
    IrregularCsd* p = new IrregularCsd(f, id, name, desc, active, isIncome, decoColor);
    return QSharedPointer<IrregularCsd>(p);
}


QSharedPointer<IrregularCsd> IrregularCsd::duplicate(bool keepSameId, bool keepSameName) const
{
    QSharedPointer<IrregularCsd> is = QSharedPointer<IrregularCsd>( new IrregularCsd(*this) );
    if (keepSameName==false) {
        QString newName = QString("%1 %2").arg(tr("Copy of")).arg(name);
        newName.truncate(Csd::NAME_MAX_LEN);
        is->setName(newName);
    }
    if (keepSameId==false) {
        is->setId(QUuid::createUuid());
    }

    return is;
}


QHash<QUuid, QSharedPointer<IrregularCsd> > IrregularCsd::deepCopyHashmap(
    const QHash<QUuid, QSharedPointer<IrregularCsd> > &in)
{
    QHash<QUuid, QSharedPointer<IrregularCsd>> out;

    if(in.size()==0){
        return out;
    }

    // Reserve space for better performance
    out.reserve(in.size());

    for (auto it = in.constBegin(); it != in.constEnd(); ++it) {
        const QUuid& key = it.key();
        const QSharedPointer<IrregularCsd>& csd = it.value();

        if (csd.isNull() == false) {
            QSharedPointer<IrregularCsd> newCsd(new IrregularCsd(*csd));
            out.insert(key, newCsd);
        } else {
            out.insert(key, QSharedPointer<IrregularCsd>());
        }
    }

    return out;
}


IrregularCsd::validateKeysResult IrregularCsd::validateKeysAndValues(
    const QMap<QDate, AmountInfo> infoSet)
{
    validateKeysResult result={.valid=false,.reasonUI="", .reasonLog=""};

    foreach (const QDate key, infoSet.keys()) {
        if (!(key.isValid()) ){
            result.reasonUI = tr("A date is invalid"); // cannot have a string representation
            result.reasonLog = QString("Date %1 is invalid")
                .arg(key.toString().toLocal8Bit().data());
            return result;
        }
        // make sure values are not too big
        AmountInfo ai = infoSet.value(key);
        if (ai.amount>CurrencyHelper::maxValueAllowedForAmount()){
            result.reasonUI = tr("Amount %1 for date %2 is too big").arg(ai.amount).arg(key.toString());
            result.reasonLog = QString("Amount %1 for date %2 is too big").arg(ai.amount).arg(key.toString().toLocal8Bit().data());
            return result;
        }
    }
    result.valid = true;
    return result;
}


bool IrregularCsd::AmountInfo::operator==(const AmountInfo &o) const
{
    if ( (amount!=o.amount) || (notes!=o.notes) ){
        return false;
    }else{
        return true;
    }
}

QJsonObject IrregularCsd::AmountInfo::toJson() const
{
    QJsonObject jobject;
    jobject["Amount"] = QVariant::fromValue(amount).toJsonValue();
    jobject["Notes"] = notes.left(NOTES_MAX_LEN);
    return jobject;
}


IrregularCsd::AmountInfo IrregularCsd::AmountInfo::fromJson(const QJsonObject &jsonObject,
    Util::ResultOfOperation &result)
{
    IrregularCsd::AmountInfo ai;
    QJsonValue jsonValue;
    int convResult;

    // Reset result to ERROR
    result.init();

    // Amount
    jsonValue = jsonObject["Amount"];
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = "AmountInfo: Cannot find token Amount";
        return ai;
    }
    if (jsonValue.isDouble()==false){
        result.logErrorMessage ="AmountInfo: Amount token is not a number";
        return ai;
    }
    double d = jsonValue.toDouble();
    qint64 amount = Util::extractQint64FromDoubleWithNoFractionalPart(d,convResult);
    if ( convResult != 0 ){
        result.logErrorMessage = QString("AmountInfo: Amount value %1 is not a valid integer "
            "(code=%2)").arg(d).arg(convResult);
        return ai;
    }
    if (  amount > CurrencyHelper::maxValueAllowedForAmount() ){
        result.logErrorMessage = QString("AmountInfo: Amount value %1 is too big")
            .arg(QString::number(d,'f',0));
        return ai;
    }
    if (  amount < 0 ){
        result.logErrorMessage = QString("AmountInfo: Amount value %1 is negative").arg(d);
        return ai;
    }
    // notes
    jsonValue = jsonObject["Notes"];
    if (jsonValue == QJsonValue::Undefined){
        result.logErrorMessage = "AmountInfo: Cannot find Notes tag";
        return ai;
    }
    if (jsonValue.isString()==false){
        result.logErrorMessage = "AmountInfo: Notes tag is not a string";
        return ai;
    }
    QString notes = jsonValue.toString();
    if (notes.length()>NOTES_MAX_LEN){
        result.logErrorMessage = QString("AmountInfo: Notes length is %1, which is bigger than maximum allowed of %2").arg(notes.length()).arg(NOTES_MAX_LEN);
        return ai;
    }

    // create struct and return
    result.status = Util::ResultOfOperationStatus::SUCCESS;
    AmountInfo finalResult ={static_cast<quint64>(amount),notes};
    return finalResult;
}


// GETTERS & SETTERS


QMap<QDate, IrregularCsd::AmountInfo> IrregularCsd::getAmountSet() const
{
    return amountSet;
}


void IrregularCsd::setAmountSet(const QMap<QDate, AmountInfo> &newAmountSet)
{
    amountSet = newAmountSet;
}
