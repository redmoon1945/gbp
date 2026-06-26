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

#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <qfileinfo.h>
#include <qjsonarray.h>
#include "scenario.h"
#include "currencyhelper.h"
#include "constants.h"


// stay independant of "gbpcontroller.h"

QString Scenario::LATEST_VERSION = "2.0.0";
QString Scenario::VERSION_1 = "1.0.0";


Scenario::Scenario(const Scenario &o){
    this->version = o.version;
    this->name = o.name;
    this->description = o.description;
    this->feGenerationDuration = o.feGenerationDuration;
    this->inflation = o.inflation;
    this->currencyIsoCode = o.currencyIsoCode;
    this->incomePeriodicCsds = o.incomePeriodicCsds;
    this->incomeIrregularCsds = o.incomeIrregularCsds;
    this->expensePeriodicCsds = o.expensePeriodicCsds;
    this->expenseIrregularCsds = o.expenseIrregularCsds;
    this->tags = o.tags;
    this->tagCsdRelationships = o.tagCsdRelationships;
}


Scenario::~Scenario()
{
}


Scenario::Scenario(const QString version, const QString name, const QString description,
    const quint16 feGenerationDuration, const Growth inflation, const QString &currencyIsoCode,
    QHash<QUuid,QSharedPointer<PeriodicCsd>> incomePeriodicCsdSet,
    QHash<QUuid,QSharedPointer<IrregularCsd>> incomeIrregularCsdSet,
    QHash<QUuid,QSharedPointer<PeriodicCsd>> expensePeriodicCsdSet,
    QHash<QUuid,QSharedPointer<IrregularCsd>> expenseIrregularCsdSet,
    const Tags newTags, const TagCsdRelationships newtagCsdRelationships) :
    version(version.left(VERSION_MAX_LEN)),
    name(name.left(NAME_MAX_LEN)), description(description.left(DESC_MAX_LEN)),
    feGenerationDuration(feGenerationDuration), inflation(inflation),
    currencyIsoCode(currencyIsoCode),
    incomePeriodicCsds(incomePeriodicCsdSet), incomeIrregularCsds(incomeIrregularCsdSet),
    expensePeriodicCsds(expensePeriodicCsdSet),expenseIrregularCsds(expenseIrregularCsdSet),
    tags(newTags), tagCsdRelationships(newtagCsdRelationships)
{}


Scenario &Scenario::operator=(const Scenario &o)
{
    this->version = o.version.left(VERSION_MAX_LEN);
    this->name = o.name.left(NAME_MAX_LEN);
    this->description = o.description.left(DESC_MAX_LEN);
    this->feGenerationDuration = o.feGenerationDuration;
    this->inflation = o.inflation;
    this->currencyIsoCode = o.currencyIsoCode;
    this->incomePeriodicCsds = o.incomePeriodicCsds;
    this->incomeIrregularCsds = o.incomeIrregularCsds;
    this->expensePeriodicCsds = o.expensePeriodicCsds;
    this->expenseIrregularCsds = o.expenseIrregularCsds;
    this->tags = o.tags;
    this->tagCsdRelationships = o.tagCsdRelationships;

    return *this;
}

bool Scenario::operator==(const Scenario &o) const
{
    if ( !(this->version==o.version) ||
        !(this->name==o.name) ||
        !(this->description==o.description) ||
        !(this->feGenerationDuration==o.feGenerationDuration) ||
        !(this->inflation==o.inflation) ||
        !(this->currencyIsoCode==o.currencyIsoCode) ){
        return false;
    }

    // Csds
    if ( false == deepComparePeriodicCsdMaps(this->incomePeriodicCsds, o.incomePeriodicCsds) ) {
        return false;
    }
    if ( false == deepCompareIrregularCsdMaps(this->incomeIrregularCsds,o.incomeIrregularCsds) ){
        return false;
    }
    if ( false == deepComparePeriodicCsdMaps(this->expensePeriodicCsds,o.expensePeriodicCsds) ){
        return false;
    }
    if ( false == deepCompareIrregularCsdMaps(this->expenseIrregularCsds,o.expenseIrregularCsds) ){
        return false;
    }

    // tags and relationships
    if ( !(this->tags==o.tags) ){
        return false;
    }
    if ( !(this->tagCsdRelationships==o.tagCsdRelationships) ){
        return false;
    }
    return true;
}


Scenario::FileResult Scenario::saveToFile(QString fullFileName) const
{
    QJsonObject jobject;
    Scenario::FileResult result;
    QJsonDocument doc;

    // simple elements
    jobject["Version"] = version;
    jobject["Name"] = name;
    jobject["Description"] = description;
    jobject["FeGenerationDuration"] = feGenerationDuration;
    jobject["Inflation"] = inflation.toJson();
    jobject["CurrencyCode"] = currencyIsoCode;
    // Also write CountryCode for backward compatibility with GBP 1.7 and below
    jobject["CountryCode"] = CurrencyHelper::getRepresentativeCountryForCurrency(currencyIsoCode);

    try {
        // incomes - Periodic
        QJsonObject jobjectIncomesPs;
        for (auto it = incomePeriodicCsds.begin(); it != incomePeriodicCsds.end(); ++it) {
            QSharedPointer<PeriodicCsd> ps = it.value();
            jobjectIncomesPs[it.key().toString(QUuid::WithoutBraces)] = ps->toJson();
        }
        jobject["IncomesPeriodic"] = jobjectIncomesPs;
        // incomes - irregular
        QJsonObject jobjectIncomesIr;
        for (auto it = incomeIrregularCsds.begin(); it != incomeIrregularCsds.end(); ++it) {
            QSharedPointer<IrregularCsd> ir = it.value();
            jobjectIncomesIr[it.key().toString(QUuid::WithoutBraces)] = ir->toJson();
        }
        jobject["IncomesIrregular"] = jobjectIncomesIr;

        // expenses - Periodic
        QJsonObject jobjectExpensesPs;
        for (auto it = expensePeriodicCsds.begin(); it != expensePeriodicCsds.end(); ++it) {
            QSharedPointer<PeriodicCsd> ps = it.value();
            jobjectExpensesPs[it.key().toString(QUuid::WithoutBraces)] = ps->toJson();
        }
        jobject["ExpensesPeriodic"] = jobjectExpensesPs;
        // expenses - irregular
        QJsonObject jobjectExpensesIr;
        for (auto it = expenseIrregularCsds.begin(); it != expenseIrregularCsds.end(); ++it) {
            QSharedPointer<IrregularCsd> ir = it.value();
            jobjectExpensesIr[it.key().toString(QUuid::WithoutBraces)] = ir->toJson();
        }
        jobject["ExpensesIrregular"] = jobjectExpensesIr;

        // Tags
        jobject["Tags"] = tags.toJson();

        // Tags relationships
        jobject["TagRelationships"] = tagCsdRelationships.toJson();

        // Build the final JSON document
        doc = QJsonDocument(jobject);   // gather everything and create JSON document

        // validate
        if (doc.isNull()){
            // should never happen
            result.code = FileResultCode::SAVE_ERROR_JSON_CREATION;
            result.logErrorMessage = "Cannot form a valid Json Document";
            return result;
        }

    } catch(const std::runtime_error& re) {
        result.logErrorMessage = QString("Runtime error trying to form a valid Json Document: %1")
            .arg(re.what());
        result.code = FileResultCode::SAVE_ERROR_JSON_CREATION;
        return result;
    }
    catch(const std::exception& ex){
        result.logErrorMessage = QString("Error occured while trying to form a valid Json "
            "Document: %1").arg(ex.what());
        result.code = FileResultCode::SAVE_ERROR_JSON_CREATION;
        return result;
    }

    QFile file(fullFileName);
    bool fileAlreadyExist = file.exists();
    // If the file does not already exist, this function will try to create a new
    // file before opening it
    if (false==file.open(QFile::WriteOnly)){
        if (fileAlreadyExist){
            result.code = FileResultCode::SAVE_ERROR_OPENING_FILE_FOR_WRITING;
            result.logErrorMessage = QString("Cannot open existing file in write-only mode : %1")
                .arg(file.errorString());
        } else {
            result.code = FileResultCode::SAVE_ERROR_CREATING_FILE_FOR_WRITING;
            result.logErrorMessage = QString("Cannot create the file in write-only mode : %1")
                .arg(file.errorString());
        }

        return result;
    }
    if (-1==file.write(doc.toJson())){
        file.close();
        result.code = FileResultCode::SAVE_ERROR_WRITING_TO_FILE;
        result.logErrorMessage = QString("Cannot write to the file : %1").arg(file.errorString());
        return result;
    }
    file.close();

    result.code = FileResultCode::SUCCESS;
    result.logErrorMessage = "";
    return result;
}


Scenario::FileResult Scenario::loadFromFile(QString fullFileName)
{
    QJsonValue buf;
    bool ok;
    Scenario::FileResult result;

    // Check if the file exists
    QFile file(fullFileName);
    if (!file.exists()){
        result.code = FileResultCode::LOAD_FILE_DOES_NOT_EXIST;
        result.logErrorMessage = QString("The file does not exist");
        return result;
    }

    // Check if the file is readable
    QFileInfo fileInfo(fullFileName);
    if ( fileInfo.isReadable()==false) {
        result.code = FileResultCode::LOAD_FILE_IS_NOT_READABLE;
        result.logErrorMessage = tr("The file is not \"readable\". The current user running "
            "GBP might not have the necessary read permissions.");
        return result;
    }

    // open the file
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.code = FileResultCode::LOAD_CANNOT_OPEN_FILE;
        result.logErrorMessage = QString("Cannot open file in read-only mode % 1")
            .arg(file.errorString());
        return result;
    }

    // read the whole content
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    if (error.error != QJsonParseError::NoError) {
        result.code = FileResultCode::LOAD_JSON_PARSING_ERROR;
        result.logErrorMessage = QString("JSON parsing error. Error code = %1, "
            "offset = %2, error message = %3")
            .arg(error.error).arg(error.offset).arg(error.errorString());
        return result;
    }

    // read all the bits and pieces of Scenario from the Json
    QJsonObject root = doc.object();

    // version : first thing to read, in order to check version
    // If version 1 found, convert to version 2 on the fly.
    buf = root.value("Version");
    if (buf == QJsonValue::Undefined){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.logErrorMessage = QString("Cannot find token \"Version\"");
        return result;
    }
    if (buf.isString()==false){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.logErrorMessage = QString("Version token is not a string");
        return result;
    }
    QString version = buf.toString();
    if( version.length()>VERSION_MAX_LEN ){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.logErrorMessage = QString("Version token has a length of %1, which is longer "
            "than max allowed of %2")
            .arg(version.length()).arg(VERSION_MAX_LEN);
        return result;
    }
    if( version != Scenario::LATEST_VERSION ){
        if (version == Scenario::VERSION_1) {
            result.version1found = true;    // notify that we have auto-converted V1 to latest
            version = LATEST_VERSION;       // since it is auto-converted when loaded
        } else {
            // appears to be an invalid or future version. This is an error.
            result.code = FileResultCode::LOAD_UNKNOWN_VERSION;
            QFileInfo fileInfo(fullFileName);
            result.logErrorMessage  = QString("The scenario file uses file format version %1, "
                "which is incompatible with this version of GBP that requires version %2 "
                "or older")
                .arg(version).arg(Scenario::LATEST_VERSION);
            return result;
        }
    }

    // Edited By (added in 1.7.0). We do not load this value (if present), because
    // it will anyway be overwritten by Scenario()

    // name
    buf = root.value("Name");
    if (buf == QJsonValue::Undefined){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.logErrorMessage = QString("Cannot find token \"Name\"");
        return result;
    }
    if (buf.isString()==false){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.logErrorMessage = QString("Name toen is not a string");
        return result;
    }
    QString name = buf.toString();
    if (name.length()>NAME_MAX_LEN){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.logErrorMessage = QString("Name token is too long (%1), maximum length is %2")
            .arg(name.length()).arg(NAME_MAX_LEN);
        return result;
    }

    // Description
    buf = root.value("Description");
    if (buf == QJsonValue::Undefined){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.logErrorMessage = QString("Cannot find token \"Description\"");
        return result;
    }
    if (buf.isString()==false){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.logErrorMessage = QString("Description token is not a string");
        return result;
    }
    QString desc = buf.toString();
    if (desc.length()>DESC_MAX_LEN){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.logErrorMessage = QString("Description token has a length of %1, which is greater "
            "than the maximum allowed of %2")
            .arg(desc.length()).arg(DESC_MAX_LEN);
        return result;
    }

    // feGenerationDuration
    buf = root.value("FeGenerationDuration");
    quint16 feGenDuration;
    if (buf == QJsonValue::Undefined){
        // Older versions may not have this field : give it default value
        feGenDuration = Constants::DEFAULT_DURATION_FE_GENERATION;
    } else {
        if (buf.isDouble()==false){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage = "FeGeneration token is not a number";
            return result;
        }
        int ok;
        double d = buf.toDouble();
        feGenDuration = Util::extractQuint16FromDoubleWithNoFracPart(d,
            Constants::MAX_DURATION_FE_GENERATION, ok);
        if ( ok==-1 ){
            result.logErrorMessage = QString("FeGenerationDuration: Value %1 is not an integer")
                .arg(d);
            return result;
        }
        if ( ok==-2 ){
            result.logErrorMessage = QString("FeGenerationDuration: Value %1 is too big")
                .arg(d);
            return result;
        }
    }

    // currency code (GBP 1.8+: CurrencyCode field; GBP 1.7 and below: derive from CountryCode)
    QString currencyIsoCode;
    buf = root.value("CurrencyCode");
    if (buf != QJsonValue::Undefined) {
        if (buf.isString() == false) {
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage = QString("CurrencyCode token is not a string");
            return result;
        }
        currencyIsoCode = buf.toString();
        if (!CurrencyHelper::currencyIsoCodeExists(currencyIsoCode)) {
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage = QString("CurrencyCode %1 is invalid").arg(currencyIsoCode);
            return result;
        }
    } else {
        // Legacy format: read CountryCode and derive the currency from it
        buf = root.value("CountryCode");
        if (buf == QJsonValue::Undefined) {
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage =
                QString("Cannot find token \"CurrencyCode\" or \"CountryCode\"");
            return result;
        }
        if (buf.isString() == false) {
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage = QString("CountryCode token is not a string");
            return result;
        }
        QString countryCode = buf.toString();
        if (!CurrencyHelper::countryExists(countryCode)) {
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage = QString("CountryCode %1 is invalid").arg(countryCode);
            return result;
        }
        bool found;
        CurrencyInfo ci = CurrencyHelper::getCurrencyInfoFromCountryCode(
            countryCode, QLocale::Language::English, found);
        if (!found) {
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage = QString("Cannot derive currency from CountryCode %1")
                .arg(countryCode);
            return result;
        }
        currencyIsoCode = ci.isoCode;
    }

    // inflation
    buf = root.value("Inflation");
    if (buf == QJsonValue::Undefined){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.logErrorMessage = QString("Cannot find token \"Inflation\"");
        return result;
    }
    if (buf.isObject()==false){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.logErrorMessage = QString("Inflation token is not an object");
        return result;
    }
    Util::ResultOfOperation infParsingResult;
    Growth inflation = Growth::fromJson(buf.toObject(),infParsingResult);
    if (infParsingResult.status==Util::ResultOfOperationStatus::ERROR){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.logErrorMessage = QString("Inflation value is invalid : %1")
            .arg(infParsingResult.logErrorMessage);
        return result;
    }

    // => read the 4 complex maps <=
    QHash<QUuid,QSharedPointer<PeriodicCsd>> incPsMap;
    QHash<QUuid,QSharedPointer<IrregularCsd>> incIrMap;
    QHash<QUuid,QSharedPointer<PeriodicCsd>> expPsMap;
    QHash<QUuid,QSharedPointer<IrregularCsd>> expIrMap;

    // QMap Income Periodic Csds
    {
        buf = root.value("IncomesPeriodic");
        if (buf == QJsonValue::Undefined){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage = "Cannot find token \"IncomesPeriodic\"";
            return result;
        }
        if (buf.isObject()==false){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage = "IncomesPeriodic token is not an object";
            return result;
        }
        QJsonObject incPsObject = buf.toObject();
        // check max no of Csds
        if (incPsObject.count()> MAX_NO_CSDS){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage =QString("Too many Periodic Incomes items found (%1 found, "
                "max is %2)")
                .arg(incPsObject.count()).arg(MAX_NO_CSDS);
            return result;
        }

        // Build the Hash table object
        for (auto it = incPsObject.begin(); it != incPsObject.end(); ++it) {
            QString key = it.key();             // Csds ID string representation
            QUuid id = Util::convertStringToQuuid(key,ok);  // the id itself
            if (ok==false) {
                key.truncate(38);
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.logErrorMessage = QString("Key \"%1\" for Periodic Income is not a "
                    "valid UUID").arg(key);
                return result;
            }
            // extract associated Csds
            QJsonValueRef valueRef = it.value();
            if (!valueRef.isObject()){
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.logErrorMessage = QString("Periodic Income Value for key \"%1\" is not an "
                    "Object").arg(key);
                return result;
            }
            QJsonObject valueObject = valueRef.toObject();
            Util::ResultOfOperation parsingResult;
            QSharedPointer<PeriodicCsd> value = PeriodicCsd::fromJson(valueObject,parsingResult);
            if(parsingResult.status==Util::ResultOfOperationStatus::ERROR){
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.logErrorMessage = parsingResult.logErrorMessage;
                return result;
            }
            // The CSD ID must match the key mentioned in the Hash table itself
            if (id != value->getId()) {
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.logErrorMessage = QString("IncomesPeriodic: Key \"%1\" in Hash table does"
                    " not match the ID \"%2\" in the Csd")
                    .arg(key)
                    .arg(value->getId().toString(QUuid::WithoutBraces));
                return result;
            }

            // Make sure this key is unique. Qt JSON parser should already have removed the
            // duplicate entries and kept ust the last one.
            if (incPsMap.contains(id)==true) {
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.logErrorMessage = QString("IncomesPeriodic: Duplicate Key \"%1\"")
                    .arg(key);
                return result;
            }

            incPsMap.insert(id, value);
        }
    }

    // QMap Income Irregular Csds
    {
        buf = root.value("IncomesIrregular");
        if (buf == QJsonValue::Undefined){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage = "Cannot find token \"IncomesIrregular\"";
            return result;
        }
        if (buf.isObject()==false){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage = "IncomesIrregular token is not an object";
            return result;
        }
        QJsonObject incIrrObject = buf.toObject();
        // check max no of Csds
        if (incIrrObject.count()> MAX_NO_CSDS){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage = QString("Too many Irregular Incomes items found (%1 found, "
                "max is %2)").arg(incIrrObject.count()).arg(MAX_NO_CSDS);
            return result;
        }

        // Build the Hash table object
        for (auto it = incIrrObject.begin(); it != incIrrObject.end(); ++it) {
            QString key = it.key(); // Csd ID string representation
            QUuid id = Util::convertStringToQuuid(key,ok);  // the id itself
            if (ok==false) {
                key.truncate(38);
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.logErrorMessage = QString("Key \"%1\" for Irregular Income is not "
                    "a valid UUID").arg(key);
                return result;
            }
            // extract associated Csd
            QJsonValueRef valueRef = it.value();
            if (!valueRef.isObject()){
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.logErrorMessage = QString("Irregular Income Value for key \"%1\" is not "
                    "an Object").arg(key);
                return result;
            }
            QJsonObject valueObject = valueRef.toObject();
            Util::ResultOfOperation parsingResult;
            QSharedPointer<IrregularCsd> value = IrregularCsd::fromJson(valueObject,parsingResult);
            if(parsingResult.status==Util::ResultOfOperationStatus::ERROR){
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.logErrorMessage = parsingResult.logErrorMessage;
                return result;
            }
            // The CSD ID must match the key mentioned in the Hash table itself
            if (id != value->getId()) {
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.logErrorMessage = QString("IncomesIrregular: Key \"%1\" in Hash table does "
                    "not match the ID \"%2\" in the Csd")
                    .arg(key)
                    .arg(value->getId().toString(QUuid::WithoutBraces));
                return result;
            }

            // Make sure this key is unique. Qt JSON parser should already have removed the
            // duplicate entries and kept ust the last one.
            if (incIrMap.contains(id)==true) {
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.logErrorMessage = QString("IncomesIrregular: Duplicate Key \"%1\"")
                    .arg(key);
                return result;
            }

            incIrMap.insert(id, value);
        }
    }

    // QMap Expense Periodic Csds
    {
        buf = root.value("ExpensesPeriodic");
        if (buf == QJsonValue::Undefined){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage = "Cannot find token \"ExpensesPeriodic\"";
            return result;
        }
        if (buf.isObject()==false){
            throw std::domain_error("ExpensesPeriodic token is not an object");
        }
        QJsonObject expPsObject = buf.toObject();
        // check max no of Csds
        if (expPsObject.count()> MAX_NO_CSDS){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage = QString("Too many Periodic Expenses items found (%1 found, "
                "max is %2)").arg(expPsObject.count()).arg(MAX_NO_CSDS);
            return result;
        }

        // Build the Hash table object
        for (auto it = expPsObject.begin(); it != expPsObject.end(); ++it) {
            QString key = it.key(); // Csd ID string representation
            QUuid id = QUuid::fromString(key);  // the id itself
            if (id.isNull()) {
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.logErrorMessage = QString("Periodic Expense Value for key \"%1\" is not a "
                    "valid UUID").arg(key);
                return result;
            }
            // extract associated Csds
            QJsonValueRef valueRef = it.value();
            if (!valueRef.isObject()){
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.logErrorMessage = QString("Periodic Expense Value for key \"%1\" is not "
                    "an Object").arg(key);
                return result;
            }
            QJsonObject valueObject = valueRef.toObject();
            Util::ResultOfOperation parsingResult;
            QSharedPointer<PeriodicCsd> value = PeriodicCsd::fromJson(valueObject,parsingResult);
            if(parsingResult.status==Util::ResultOfOperationStatus::ERROR){
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.logErrorMessage = parsingResult.logErrorMessage;
                return result;
            }
            // The CSD ID must match the key mentioned in the Hash table itself
            if (id != value->getId()) {
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.logErrorMessage = QString("ExpensesPeriodic: Key \"%1\" in Hash table does "
                    "not match the ID \"%2\" in the Csd")
                    .arg(key)
                    .arg(value->getId().toString(QUuid::WithoutBraces));
                return result;
            }

            // Make sure this key is unique. Qt JSON parser should already have removed the
            // duplicate entries and kept ust the last one.
            if (expPsMap.contains(id)==true) {
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.logErrorMessage = QString("ExpensesPeriodic: Duplicate Key \"%1\"")
                    .arg(key);
                return result;
            }

            expPsMap.insert(id, value);
        }
    }

    // QMap Expense Irregular Csds
    {
        buf = root.value("ExpensesIrregular");
        if (buf == QJsonValue::Undefined){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage = "Cannot find token \"ExpensesIrregular\"";
            return result;
        }
        if (buf.isObject()==false){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage = "ExpensesIrregular token is not an object";
            return result;
        }
        QJsonObject expIrrObject = buf.toObject();
        // check max no of sds
        if (expIrrObject.count()> MAX_NO_CSDS){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage = QString("Too many Irregular Expenses items found (%1 found, "
                "max is %2)").arg(expIrrObject.count()).arg(MAX_NO_CSDS);
            return result;
        }

        // Build the Hash table object
        for (auto it = expIrrObject.begin(); it != expIrrObject.end(); ++it) {
            QString key = it.key(); // Csd ID string representation
            QUuid id = QUuid::fromString(key);  // the id itself
            if (id.isNull()) {
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.logErrorMessage = QString("Irregular Expense Value for key \"%1\" is not "
                    "a valid UUID").arg(key);
                return result;
            }
            // extract associated Csd
            QJsonValueRef valueRef = it.value();
            if (!valueRef.isObject()){
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.logErrorMessage = QString("Irregular Expense Value for key \"%1\" is not "
                    "an Object").arg(key);
                return result;
            }
            QJsonObject valueObject = valueRef.toObject();
            Util::ResultOfOperation parsingResult;
            QSharedPointer<IrregularCsd> value = IrregularCsd::fromJson(valueObject,parsingResult);
            if(parsingResult.status==Util::ResultOfOperationStatus::ERROR){
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.logErrorMessage = parsingResult.logErrorMessage;
                return result;
            }
            // The CSD ID must match the key mentioned in the Hash table itself
            if (id != value->getId()) {
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.logErrorMessage = QString("ExpensesIrregular: Key \"%1\" in Hash table does"
                    " not match the ID \"%2\" in the Csd")
                    .arg(key)
                    .arg(value->getId().toString(QUuid::WithoutBraces));
                return result;
            }

            // Make sure this key is unique. Qt JSON parser should already have removed the
            // duplicate entries and kept ust the last one.
            if (expIrMap.contains(id)==true) {
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.logErrorMessage = QString("ExpensesIrregular: Duplicate Key \"%1\"")
                    .arg(key);
                return result;
            }

            expIrMap.insert(id, value);
        }
    }

    // Tags
    Tags theTags;
    buf = root.value("Tags");
    if (buf == QJsonValue::Undefined){
        // older version of scenario file do not have tags, keep the empty set then
    } else {
        if (buf.isObject()==false){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage = QString("Tags token is not an object");
            return result;
        }
        Util::ResultOfOperation parsingResult;
        theTags = Tags::fromJson(buf.toObject(), parsingResult);
        if (parsingResult.status==Util::ResultOfOperationStatus::ERROR){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage = QString("%1")
                .arg(parsingResult.logErrorMessage);
            return result;
        }
    }

    // Tag Relationships
    TagCsdRelationships rel;
    buf = root.value("TagRelationships");
    if (buf == QJsonValue::Undefined){
        // older version of scenario file do not have tags relationships, keep the empty set then
    } else {
        if (buf.isObject()==false){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage = QString("Tags relationships is not an object");
            return result;
        }
        Util::ResultOfOperation parsingResult;
        rel = TagCsdRelationships::fromJson(buf.toObject(),parsingResult);
        if (parsingResult.status==Util::ResultOfOperationStatus::ERROR){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.logErrorMessage = QString("%1")
                .arg(parsingResult.logErrorMessage);
            return result;
        }
    }


    // All data have been collected : build and return a new Scenario
    result.code = FileResultCode::SUCCESS;
    QSharedPointer<Scenario> ptr;
    try {
        ptr = QSharedPointer<Scenario>(new Scenario(version, name, desc, feGenDuration, inflation,
            currencyIsoCode, incPsMap, incIrMap, expPsMap, expIrMap, theTags, rel));
    } catch (...) {
        // should never happen
        result.code = FileResultCode::LOAD_ERROR;
        result.logErrorMessage = QString("An unexpected exception occured while trying to create "
            "the scenario object");
        return result;
    }
    result.scenarioPtr = ptr;

    // Once the scenario is built, check that all relationships have tags and fsds defined
    if (false == ptr->checkTagCsdRelationshipsIntegrity()) {
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.logErrorMessage = QString("Tags Relationships integrity is invalid");
        return result;
    }

    // Finally, if it was an older file format version, UPDATE the file to the latest format
    // without notifying the user
    if (result.version1found==true) {
        FileResult convertResult = ptr->saveToFile(fullFileName);
        if (convertResult.code != FileResultCode::SUCCESS) {
            // we have a problem, should not happen in principle
            result.code = FileResultCode::LOAD_CANNOT_UPGRADE;
            result.logErrorMessage = convertResult.logErrorMessage;
            return result;
        }
    }

    return result;
}


QSharedPointer<CombinedFeStreams> Scenario::generateFinancialEvents(QDate today,
    const QLocale &systemLocale, DateRange fromto, double pvAnnualDiscountRate,
    QDate pvPresent, uint &saturationCount) const
{
    // check input parameters
    if (pvAnnualDiscountRate < 0 ) {
        throw std::invalid_argument("PV discount rate cannot be negative");
    }
    if (pvPresent.isValid()==false) {
        throw std::invalid_argument("PV present date is invalid");
    }
    if (today.isValid()==false) {
        throw std::invalid_argument("Today date is invalid");
    }
    if (fromto.getType() != DateRange::Type::BOUNDED) {
        throw std::invalid_argument("fromtoInitial is not of type BOUNDED");
    }

    // calculate the number of max days acccording to the scenario limit
    // and create the combinedStream object
    QDate maxDate = today.addYears(feGenerationDuration); // compute max date for FeGeneration
    QDate tomorrow = today.addDays(1);
    qint64 tomorrowJulianDays = tomorrow.toJulianDay();
    int maxNoOfdays = maxDate.toJulianDay() - tomorrowJulianDays + 1;
    QSharedPointer<CombinedFeStreams> comb = QSharedPointer<CombinedFeStreams>(
        new CombinedFeStreams(maxNoOfdays) );

    // Build the shared FeStream to be reused by all the Csds.
    // Reference to the proper Csd will be set when appropriate, so now we set a null value.
    FeStream sharedFeStream(maxNoOfdays, QWeakPointer<Csd>(), tomorrow);

    uint saturationNo;
    saturationCount = 0;
    FeMinMaxInfo minMaxInfo; // we wont use it
    bool found;

    // We dont care about the currency name language here, because we are not going to use it.
    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromIsoCode(
        currencyIsoCode, QLocale::Language::English, found);
    if (!found){
        // should never happen
        return comb;
    }

    foreach(QSharedPointer<PeriodicCsd> item,incomePeriodicCsds){
        sharedFeStream.setCsdPtr(item.toWeakRef());
        item->generateEventStream(sharedFeStream, tomorrow, fromto,
            maxDate, inflation, pvAnnualDiscountRate, pvPresent, saturationNo, minMaxInfo);
        comb->addStream(sharedFeStream,currInfo);
        saturationCount += saturationNo;
    }
    foreach(QSharedPointer<PeriodicCsd> item,expensePeriodicCsds){
        sharedFeStream.setCsdPtr(item.toWeakRef());
        item->generateEventStream(sharedFeStream, tomorrow, fromto,
            maxDate, inflation, pvAnnualDiscountRate, pvPresent, saturationNo, minMaxInfo);
        comb->addStream(sharedFeStream,currInfo);
        saturationCount += saturationNo;
    }
    foreach(QSharedPointer<IrregularCsd> item,incomeIrregularCsds){
        sharedFeStream.setCsdPtr(item.toWeakRef());
        item->generateEventStream(sharedFeStream, tomorrow, fromto,
            maxDate, pvAnnualDiscountRate, pvPresent, saturationNo, minMaxInfo);
        comb->addStream(sharedFeStream,currInfo);
        saturationCount += saturationNo;
    }
    foreach(QSharedPointer<IrregularCsd> item,expenseIrregularCsds){
        sharedFeStream.setCsdPtr(item.toWeakRef());
        item->generateEventStream(sharedFeStream, tomorrow, fromto,
            maxDate, pvAnnualDiscountRate, pvPresent, saturationNo, minMaxInfo);
        comb->addStream(sharedFeStream,currInfo);
        saturationCount += saturationNo;
    }

    return comb;
}


void Scenario::getCsdNameAndColorFromId(QUuid id, QString& name, QColor& color,
    bool &found) const
{
    found = true;

    QSharedPointer<PeriodicCsd> ptrPeriodic = incomePeriodicCsds.value(id);
    if (ptrPeriodic != nullptr){
        name = ptrPeriodic->getName();
        color = ptrPeriodic->getDecorationColor();
        return;
    }
    ptrPeriodic = expensePeriodicCsds.value(id);
    if (ptrPeriodic != nullptr){
        name = ptrPeriodic->getName();
        color = ptrPeriodic->getDecorationColor();
        return;
    }
    QSharedPointer<IrregularCsd> ptrIrregular = incomeIrregularCsds.value(id);
    if (ptrIrregular != nullptr){
        name = ptrIrregular->getName();
        color = ptrIrregular->getDecorationColor();
        return;
    }
    ptrIrregular = expenseIrregularCsds.value(id);
    if (ptrIrregular != nullptr){
        name = ptrIrregular->getName();
        color = ptrIrregular->getDecorationColor();
        return;
    }

    found = false;
    name = "";
    color = QColor();
    return ;
}


bool Scenario::csdIdExists(QUuid id) const
{
    if ( incomePeriodicCsds.contains(id) || expensePeriodicCsds.contains(id) ||
        incomeIrregularCsds.contains(id) || expenseIrregularCsds.contains(id) ){
        return true;
    } else{
        return false;
    }
}


bool Scenario::evaluateIfSameFeStream(QSharedPointer<Scenario> o, QString& diff) const
{
    diff = "";

    if ( feGenerationDuration != o->feGenerationDuration ){
        diff = QString("Scenario duration is different (%1 vs %2)")
            .arg(feGenerationDuration).arg(o->feGenerationDuration);
        return false;
    }

    if( inflation != o->inflation){
        diff = QString("Scenario inflation is different");
        return false;
    }

    // All Csd lists must have the same size
    if ( incomePeriodicCsds.size() != o->incomePeriodicCsds.size()){
        diff = QString("Income Periodic set have different size (%1 vs %2)")
            .arg(incomePeriodicCsds.size()).arg(o->incomePeriodicCsds.size());
        return false;
    }
    if ( expensePeriodicCsds.size() != o->expensePeriodicCsds.size()){
        diff = QString("Expense Periodic set have different size (%1 vs %2)")
            .arg(expensePeriodicCsds.size()).arg(o->expensePeriodicCsds.size());
        return false;
    }
    if ( incomeIrregularCsds.size() != o->incomeIrregularCsds.size()){
        diff = QString("Income Irregular set have different size (%1 vs %2)")
            .arg(incomeIrregularCsds.size()).arg(o->incomeIrregularCsds.size());
        return false;
    }
    if ( expenseIrregularCsds.size() != o->expenseIrregularCsds.size()){
        diff = QString("Expense Irregular set have different size (%1 vs %2)")
            .arg(expenseIrregularCsds.size()).arg(o->expenseIrregularCsds.size());
        return false;
    }

    // Must have the same lists of ID. Otherwise a CSD has been removed and another one added,
    // which possibly leads to different FE list (not 100% sure). We'll check later if the
    // relevant contents are identical
    for (auto it = incomePeriodicCsds.constBegin(); it != incomePeriodicCsds.constEnd(); ++it) {
        if (o->incomePeriodicCsds.contains(it.key())==false) {
            diff = QString("Income Periodic : one UUID is not in both Hashmap : %1")
                .arg(it.key().toString(QUuid::WithoutBraces));
            return false;
        }
    }
    for (auto it = expensePeriodicCsds.constBegin(); it != expensePeriodicCsds.constEnd(); ++it) {
        if (o->expensePeriodicCsds.contains(it.key())==false) {
            diff = QString("Expense Periodic : one UUID is not in both Hashmap : %1")
                .arg(it.key().toString(QUuid::WithoutBraces));
            return false;
        }
    }
    for (auto it = incomeIrregularCsds.constBegin(); it != incomeIrregularCsds.constEnd(); ++it) {
        if (o->incomeIrregularCsds.contains(it.key())==false) {
            diff = QString("Income Irregular : one UUID is not in both Hashmap : %1")
                .arg(it.key().toString(QUuid::WithoutBraces));
            return false;
        }
    }
    for (auto it = expenseIrregularCsds.constBegin(); it != expenseIrregularCsds.constEnd(); ++it) {
        if (o->expenseIrregularCsds.contains(it.key())==false) {
            diff = QString("Expense Irregular : one UUID is not in both Hashmap : %1")
                .arg(it.key().toString(QUuid::WithoutBraces));
            return false;
        }
    }


    // Incomes Periodic : check that each Csd for a given key have guaranteed identical Fe Stream
    for (auto it = incomePeriodicCsds.constBegin(); it != incomePeriodicCsds.constEnd(); ++it) {
        const QUuid& key = it.key();
        const QSharedPointer<PeriodicCsd>& ptr = it.value();
        const QSharedPointer<PeriodicCsd> ptr2 = o->incomePeriodicCsds.value(key);
        // compare
        if ((ptr2.isNull()) || (ptr.isNull())) {
            return false; // should never happen
        }
        if( false == ptr->evaluateIfSameFeList(*ptr2, diff) ){
            return false;
        }
    }

    // Expenses Periodic : check that each Csd for a given key have guaranteed identical Fe Stream
    for (auto it = expensePeriodicCsds.constBegin(); it != expensePeriodicCsds.constEnd(); ++it) {
        const QUuid& key = it.key();
        const QSharedPointer<PeriodicCsd>& ptr = it.value();
        const QSharedPointer<PeriodicCsd> ptr2 = o->expensePeriodicCsds.value(key);
        // compare
        if ((ptr2.isNull()) || (ptr.isNull())) {
            return false; // should never happen
        }
        if( false == ptr->evaluateIfSameFeList(*ptr2, diff) ){
            return false;
        }
    }

    // Incomes Irregular : check that each Csd for a given key have guaranteed identical Fe Stream
    for (auto it = incomeIrregularCsds.constBegin(); it != incomeIrregularCsds.constEnd(); ++it) {
        const QUuid& key = it.key();
        const QSharedPointer<IrregularCsd>& ptr = it.value();
        const QSharedPointer<IrregularCsd> ptr2 = o->incomeIrregularCsds.value(key);
        // compare
        if ((ptr2.isNull()) || (ptr.isNull())) {
            return false; // should never happen
        }
        if( false == ptr->evaluateIfSameFeList(*ptr2, diff) ){
            return false;
        }
    }

    // Expenses Irregular : check that each Csd for a given key have guaranteed identical Fe Stream
    for (auto it = expenseIrregularCsds.constBegin(); it != expenseIrregularCsds.constEnd(); ++it) {
        const QUuid& key = it.key();
        const QSharedPointer<IrregularCsd>& ptr = it.value();
        const QSharedPointer<IrregularCsd> ptr2 = o->expenseIrregularCsds.value(key);
        // compare
        if ((ptr2.isNull()) || (ptr.isNull())) {
            return false; // should never happen
        }
        if( false == ptr->evaluateIfSameFeList(*ptr2, diff) ){
            return false;
        }
    }

    return true;
}


QSharedPointer<Scenario> Scenario::createBlankScenario(QString currencyIsoCode)
{
    QSharedPointer<Scenario> newScenario = QSharedPointer<Scenario>(new Scenario(
        Scenario::LATEST_VERSION, tr("No name"), "", Constants::DEFAULT_DURATION_FE_GENERATION,
        Growth::fromConstantAnnualPercentageDouble(0), currencyIsoCode, {},{},{},{},
        Tags(), TagCsdRelationships()));
    return newScenario;
}


int Scenario::getNoOfPeriodicIncomes(bool activeOnly)
{
    if (activeOnly==true) {
        int no = 0;
        foreach (QSharedPointer<PeriodicCsd> ps, incomePeriodicCsds) {
            if (ps->getActive()==true){
                no++;
            }
        }
        return no;
    } else {
        return incomePeriodicCsds.size();
    }
}


int Scenario::getNoOfIrregularIncomes(bool activeOnly)
{
    if (activeOnly==true) {
        int no = 0;
        foreach (QSharedPointer<IrregularCsd> is, incomeIrregularCsds) {
            if (is->getActive()==true){
                no++;
            }
        }
        return no;
    } else {
        return incomeIrregularCsds.size();
    }

}


int Scenario::getNoOfPeriodicExpenses(bool activeOnly)
{
    if (activeOnly==true) {
        int no = 0;
        foreach (QSharedPointer<PeriodicCsd> ps, expensePeriodicCsds) {
            if (ps->getActive()==true){
                no++;
            }
        }
        return no;
    } else {
        return expensePeriodicCsds.size();
    }
}


int Scenario::getNoOfIrregularExpenses(bool activeOnly)
{
    if (activeOnly==true) {
        int no = 0;
        foreach (QSharedPointer<IrregularCsd> is, expenseIrregularCsds) {
            if (is->getActive()==true){
                no++;
            }
        }
        return no;
    } else {
        return expenseIrregularCsds.size();
    }
}


QString Scenario::FileResult::codeToString()
{
        switch (code) {
            case FileResultCode::SUCCESS:
                return "SUCCESS";
            case FileResultCode::ERROR_OTHER:
                return "MISC ERROR";
            case FileResultCode::SAVE_ERROR_CREATING_FILE_FOR_WRITING:
                return "FILE CREATION FAILED IN WRITE MODE";
            case FileResultCode::SAVE_ERROR_OPENING_FILE_FOR_WRITING:
                return "FILE OPENING FAILED IN WRITE MODE";
            case FileResultCode::SAVE_ERROR_WRITING_TO_FILE:
                return "ERROR WRITING TO FILE";
            case FileResultCode::SAVE_ERROR_JSON_CREATION:
                return "JSON CREATION FAILED";
            case FileResultCode::LOAD_FILE_DOES_NOT_EXIST:
                return "FILE DOES NOT EXIST";
            case FileResultCode::LOAD_FILE_IS_NOT_READABLE:
                return "FILE IS NOT READABLE";
            case FileResultCode::LOAD_CANNOT_OPEN_FILE:
                return "CANNOT OPEN FILE";
            case FileResultCode::LOAD_JSON_PARSING_ERROR:
                return "JSON PARSING ERROR";
            case FileResultCode::LOAD_JSON_SEMANTIC_ERROR:
                return "JSON SEMANTIC ERROR";
            case FileResultCode::LOAD_CANNOT_UPGRADE:
                return "CANNOT UPGRADE";
            case FileResultCode::LOAD_ERROR:
                return "LOAD ERROR";
            case FileResultCode::LOAD_UNKNOWN_VERSION:
                return "UNKNOWN FILE VERSION";
            default:
                return "UNKNOWN";
    }
}


bool Scenario::checkTagCsdRelationshipsIntegrity()
{
    QList<QUuid> theTagsRel = tagCsdRelationships.getAllTagsWithRelationships();
    QList<QUuid> theCsdsRel = tagCsdRelationships.getAllCsdsWithRelationships();

    // Check Tag ID exists
    foreach (QUuid tId, theTagsRel) {
        bool found = tags.containsTagId(tId);
        if (found==false) {
            return false;
        }
    }
    foreach (QUuid fId, theCsdsRel) {
        bool found = csdIdExists(fId);
        if (found==false) {
            return false;
        }
    }
    return true;
}


bool Scenario::deepComparePeriodicCsdMaps(const QHash<QUuid, QSharedPointer<PeriodicCsd> > &map1,
    const QHash<QUuid, QSharedPointer<PeriodicCsd> > &map2) const
{
    // Check if the sizes are different
    if (map1.size() != map2.size()) {
        return false;
    }

    // Iterate through the first map
    for (auto it = map1.cbegin(); it != map1.cend(); ++it) {
        const QUuid& key = it.key();
        const QSharedPointer<PeriodicCsd>& value1 = it.value();

        // Check if the key exists in the second map
        if (!map2.contains(key)) {
            return false;
        }

        const QSharedPointer<PeriodicCsd>& value2 = map2.value(key);

        // Check if both values are null or if they are equal
        if (value1 == nullptr && value2 == nullptr) {
            continue; // Both are null, considered equal
        }
        if (value1 == nullptr || value2 == nullptr || (*value1 != *value2) ) {
            return false; // One is null or they are not equal
        }
    }

    return true; // All keys and values are equal
}


bool Scenario::deepCompareIrregularCsdMaps(const QHash<QUuid, QSharedPointer<IrregularCsd> > &map1,
    const QHash<QUuid, QSharedPointer<IrregularCsd> > &map2) const
{
    // Check if the sizes are different
    if (map1.size() != map2.size()) {
        return false;
    }

    // Iterate through the first map
    for (auto it = map1.cbegin(); it != map1.cend(); ++it) {
        const QUuid& key = it.key();
        const QSharedPointer<IrregularCsd>& value1 = it.value();

        // Check if the key exists in the second map
        if (!map2.contains(key)) {
            return false;
        }

        const QSharedPointer<IrregularCsd>& value2 = map2.value(key);

        // Check if both values are null or if they are equal
        if (value1 == nullptr && value2 == nullptr) {
            continue; // Both are null, considered equal
        }
        if (value1 == nullptr || value2 == nullptr || (*value1 != *value2) ) {
            return false; // One is null or they are not equal
        }
    }

    return true; // All keys and values are equal
}


Scenario::FileResult::FileResult()
{
    init();
}


void Scenario::FileResult::init()
{
    code = FileResultCode::ERROR_OTHER;
    logErrorMessage = "";
    version1found = false;
    scenarioPtr = QSharedPointer<Scenario>( scenarioPtr); // null
}


// Getters and setters

QString Scenario::getVersion() const
{
    return version;
}

void Scenario::setVersion(const QString &newVersion)
{
    version = newVersion;
}

QString Scenario::getName() const
{
    return name;
}

void Scenario::setName(const QString &newName)
{
    name = newName;
}

QString Scenario::getDescription() const
{
    return description;
}

void Scenario::setDescription(const QString &newDescription)
{
    description = newDescription;
}

Growth Scenario::getInflation() const
{
    return inflation;
}

void Scenario::setInflation(const Growth &newInflation)
{
    inflation = newInflation;
}

QString Scenario::getCurrencyIsoCode() const
{
    return currencyIsoCode;
}

void Scenario::setCurrencyIsoCode(const QString &newIsoCode)
{
    currencyIsoCode = newIsoCode;
}

QHash<QUuid, QSharedPointer<PeriodicCsd>> Scenario::getIncomePeriodicCsds() const
{
    return incomePeriodicCsds;
}

void Scenario::setIncomePeriodicCsds(const QHash<QUuid, QSharedPointer<PeriodicCsd>> &newincomePeriodicCsds)
{
    incomePeriodicCsds = newincomePeriodicCsds;
}

QHash<QUuid, QSharedPointer<IrregularCsd>> Scenario::getIncomeIrregularCsds() const
{
    return incomeIrregularCsds;
}

void Scenario::setIncomeIrregularCsds(const QHash<QUuid,
    QSharedPointer<IrregularCsd>> &newincomeIrregularCsds)
{
    incomeIrregularCsds = newincomeIrregularCsds;
}

QHash<QUuid, QSharedPointer<PeriodicCsd>> Scenario::getExpensePeriodicCsds() const
{
    return expensePeriodicCsds;
}

void Scenario::setExpensePeriodicCsds(const QHash<QUuid,
    QSharedPointer<PeriodicCsd>> &newexpensePeriodicCsds)
{
    expensePeriodicCsds = newexpensePeriodicCsds;
}

QHash<QUuid, QSharedPointer<IrregularCsd>> Scenario::getExpenseIrregularCsds() const
{
    return expenseIrregularCsds;
}

void Scenario::setExpenseIrregularCsds(const QHash<QUuid,
    QSharedPointer<IrregularCsd>> &newexpenseIrregularCsds)
{
    expenseIrregularCsds = newexpenseIrregularCsds;
}

quint16 Scenario::getFeGenerationDuration() const
{
    return feGenerationDuration;
}

void Scenario::setFeGenerationDuration(quint16 newFeGenerationDuration)
{
    feGenerationDuration = newFeGenerationDuration;
}

Tags Scenario::getTags() const
{
    return tags;
}

void Scenario::setTags(const Tags &newTags)
{
    tags = newTags;
}

TagCsdRelationships Scenario::getTagCsdRelationships() const
{
    return tagCsdRelationships;
}

void Scenario::setTagCsdRelationships(const TagCsdRelationships &newtagCsdRelationships)
{
    tagCsdRelationships = newtagCsdRelationships;
}

