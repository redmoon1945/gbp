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

#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include "scenario.h"
#include "currencyhelper.h"


QString Scenario::LatestVersion = "1.0.0";
int Scenario::NAME_MAX_LEN = 100;
int Scenario::DESC_MAX_LEN = 4000;
int Scenario::VERSION_MAX_LEN = 20;
int Scenario::MAX_NO_STREAM_DEF = 200;


Scenario::Scenario(const Scenario &o){
    this->version = o.version;
    this->name = o.name.left(NAME_MAX_LEN); // truncate if required
    this->description = o.description;
    this->inflation = o.inflation;
    this->countryCode = o.countryCode;
    this->incomesDefPeriodic = o.incomesDefPeriodic;
    this->incomesDefIrregular = o.incomesDefIrregular;
    this->expensesDefPeriodic = o.expensesDefPeriodic;
    this->expensesDefIrregular = o.expensesDefIrregular;
}


Scenario::~Scenario()
{
}


Scenario::Scenario(const QString version, const QString name, const QString description, const Growth inflation, QString countryCode,
                   const QMap<QUuid, PeriodicFeStreamDef> incomesDefPeriodicSet,
                   const QMap<QUuid, IrregularFeStreamDef> incomesDefIrregularSet,
                   const QMap<QUuid, PeriodicFeStreamDef> expensesDefPeriodicSet,
                   const QMap<QUuid, IrregularFeStreamDef> expensesDefIrregularSet) :
    version(version.left(VERSION_MAX_LEN)),
    name(name.left(NAME_MAX_LEN)),
    description(description.left(DESC_MAX_LEN)),
    inflation(inflation),
    countryCode(countryCode),
    incomesDefPeriodic(incomesDefPeriodicSet),
    incomesDefIrregular(incomesDefIrregularSet),
    expensesDefPeriodic(expensesDefPeriodicSet),
    expensesDefIrregular(expensesDefIrregularSet)
{}


Scenario &Scenario::operator=(const Scenario &o)
{
    this->version = o.version.left(VERSION_MAX_LEN);
    this->name = o.name.left(NAME_MAX_LEN);
    this->description = o.description.left(DESC_MAX_LEN);
    this->inflation = o.inflation;
    this->countryCode = o.countryCode;
    this->incomesDefPeriodic = o.incomesDefPeriodic;
    this->incomesDefIrregular = o.incomesDefIrregular;
    this->expensesDefPeriodic = o.expensesDefPeriodic;
    this->expensesDefIrregular = o.expensesDefIrregular;

    return *this;
}

bool Scenario::operator==(const Scenario &o) const
{
    if ( !(this->version==o.version) ||
        !(this->name==o.name) ||
        !(this->description==o.description) ||
        !(this->inflation==o.inflation) ||
        !(this->countryCode==o.countryCode) ){
        return false;
    }
    if ( !(this->incomesDefPeriodic==o.incomesDefPeriodic) ){
        return false;
    }
    if ( !(this->incomesDefIrregular==o.incomesDefIrregular) ){
        return false;
    }
    if ( !(this->expensesDefPeriodic==o.expensesDefPeriodic) ){
        return false;
    }
    if ( !(this->expensesDefIrregular==o.expensesDefIrregular) ){
        return false;
    }
    return true;
}


// Save the scenario in an JSON file. If the file exists, it is overwritten
Scenario::FileResult Scenario::saveToFile(QString fullFileName) const
{
    QJsonObject jobject;
    Scenario::FileResult result = {.code=ERROR_OTHER, .errorStringUI="", .errorStringLog=""};
    QJsonDocument doc;

    // simple elements
    jobject["Version"] = version;
    jobject["Name"] = name;
    jobject["Description"] = description;
    jobject["Inflation"] = inflation.toJson();
    jobject["CountryCode"] = countryCode;

    try {
        // incomes - Periodic
        QJsonObject jobjectIncomesPs;
        for (auto it = incomesDefPeriodic.begin(); it != incomesDefPeriodic.end(); ++it) {
            PeriodicFeStreamDef ps = it.value();
            jobjectIncomesPs[it.key().toString(QUuid::WithoutBraces)] = ps.toJson();
        }
        jobject["IncomesPeriodic"] = jobjectIncomesPs;
        // incomes - irregular
        QJsonObject jobjectIncomesIr;
        for (auto it = incomesDefIrregular.begin(); it != incomesDefIrregular.end(); ++it) {
            IrregularFeStreamDef ir = it.value();
            jobjectIncomesIr[it.key().toString(QUuid::WithoutBraces)] = ir.toJson();
        }
        jobject["IncomesIrregular"] = jobjectIncomesIr;

        // expenses - Periodic
        QJsonObject jobjectExpensesPs;
        for (auto it = expensesDefPeriodic.begin(); it != expensesDefPeriodic.end(); ++it) {
            PeriodicFeStreamDef ps = it.value();
            jobjectExpensesPs[it.key().toString(QUuid::WithoutBraces)] = ps.toJson();
        }
        jobject["ExpensesPeriodic"] = jobjectExpensesPs;
        // expenses - irregular
        QJsonObject jobjectExpensesIr;
        for (auto it = expensesDefIrregular.begin(); it != expensesDefIrregular.end(); ++it) {
            IrregularFeStreamDef ir = it.value();
            jobjectExpensesIr[it.key().toString(QUuid::WithoutBraces)] = ir.toJson();
        }
        jobject["ExpensesIrregular"] = jobjectExpensesIr;

        doc =  QJsonDocument(jobject);   // gather everything and create JSON document

        // validate
        if (doc.isNull()){
            // should never happen
            result.code = SAVE_ERROR_INTERNAL_JSON_CREATION;
            result.errorStringUI = tr("Cannot form a valid Json Document");
            result.errorStringLog = "Cannot form a valid Json Document";
            return result;
        }

    } catch(const std::runtime_error& re) {
        result.errorStringUI = tr("Runtime error: (%1)").arg(re.what());
        result.errorStringLog = QString("Runtime error: (%1)").arg(re.what());
        result.code = SAVE_ERROR_INTERNAL_JSON_CREATION;
        return result;
    }
    catch(const std::exception& ex){
        result.errorStringUI = tr("Error: (%1)").arg(ex.what());
        result.errorStringLog = QString("Error: (%1)").arg(ex.what());
        result.code = SAVE_ERROR_INTERNAL_JSON_CREATION;
        return result;
    }
    catch (...) {
        std::exception_ptr p = std::current_exception();
        result.errorStringUI = tr("Unknown error occured (%1)").arg((p ? p.__cxa_exception_type()->name() : "null"));
        result.errorStringLog = QString("Unknown error occured (%1)").arg((p ? p.__cxa_exception_type()->name() : "null"));
        result.code = SAVE_ERROR_INTERNAL_JSON_CREATION;
        return result;
    }

    QFile file(fullFileName);
    bool fileAlreadyExist = file.exists();
    // If the file does not already exist, this function will try to create a new file before opening it
    if (false==file.open(QFile::WriteOnly)){
        if (fileAlreadyExist){
            result.code = SAVE_ERROR_OPENING_FILE_FOR_WRITING;
            result.errorStringUI = tr("Cannot open the already existing file in write-only mode");
            result.errorStringLog = "Cannot open the already existing file in write-only mode";
        } else {
            result.code = SAVE_ERROR_CREATING_FILE_FOR_WRITING;
            result.errorStringUI = tr("Cannot create the file in write-only mode");
            result.errorStringLog = "Cannot create the file in write-only mode";
        }

        return result;
    }
    if (-1==file.write(doc.toJson())){
        file.close();
        result.code = SAVE_ERROR_WRITING_TO_FILE;
        result.errorStringUI = tr("Cannot write to the file");
        result.errorStringLog = "Cannot write to the file";
        return result;
    }
    file.close();

    result.code = SUCCESS;
    result.errorStringUI = "";
    result.errorStringLog = "";
    return result;
}


// Generate the whole suite of financial events for that scenario
// Input params :
//   systemLocale : Locale used for amount formatting
//   fromTo : interval of time inside which the events should be generated
//   pvAnnualDiscountRate : annual discount rate in percentage, to transform future into present value.
//       0 means keep future values. Cannot be negative.
//   pvPresent : date considered the "present" for convertion to PV purpose
// Output params:
//   saturationCount : number of times the FE amount was over the maximum allowed
QMap<QDate, CombinedFeStreams::DailyInfo> Scenario::generateFinancialEvents(QLocale systemLocale, DateRange fromto,
                                                                            double pvAnnualDiscountRate, QDate pvPresent,
                                                                            uint &saturationCount) const
{
    CombinedFeStreams comb;
    uint saturationNo;
    saturationCount = 0;
    bool found;

    // check input parameters
    if (pvAnnualDiscountRate < 0 ) {
        throw std::invalid_argument("PV discount rate cannot be negative");
    }
    if (pvPresent.isValid()==false) {
        throw std::invalid_argument("PV present date is invalid");
    }

    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromCountryCode(systemLocale, countryCode, found);
    if (!found){
        // should never happen
        return comb.getCombinedStreams();
    }

    foreach(PeriodicFeStreamDef item,incomesDefPeriodic){
        QList<Fe> stream = item.generateEventStream(fromto, inflation, pvAnnualDiscountRate, pvPresent, saturationNo);
        comb.addStream(stream,currInfo);
        saturationCount += saturationNo;
    }
    foreach(PeriodicFeStreamDef item,expensesDefPeriodic){
        QList<Fe> stream = item.generateEventStream(fromto, inflation, pvAnnualDiscountRate, pvPresent, saturationNo);
        comb.addStream(stream,currInfo);
        saturationCount += saturationNo;
    }
    foreach(IrregularFeStreamDef item,incomesDefIrregular){
        QList<Fe> stream = item.generateEventStream(fromto, pvAnnualDiscountRate, pvPresent, saturationNo);
        comb.addStream(stream,currInfo);
        saturationCount += saturationNo;
    }
    foreach(IrregularFeStreamDef item,expensesDefIrregular){
        QList<Fe> stream = item.generateEventStream(fromto, pvAnnualDiscountRate, pvPresent, saturationNo);
        comb.addStream(stream,currInfo);
        saturationCount += saturationNo;
    }

    return comb.getCombinedStreams();
}


void Scenario::getStreamDefNameAndColorFromId(QUuid id, QString& name, QColor& color, bool &found) const
{
    found = true;

    foreach(PeriodicFeStreamDef item,incomesDefPeriodic){
        if (item.getId()==id){
            name = item.getName();
            color = item.getDecorationColor();
            return;
        }
    }
    foreach(PeriodicFeStreamDef item,expensesDefPeriodic){
        if (item.getId()==id){
            name = item.getName();
            color = item.getDecorationColor();
            return;
        }
    }
    foreach(IrregularFeStreamDef item,incomesDefIrregular){
        if (item.getId()==id){
            name = item.getName();
            color = item.getDecorationColor();
            return;
        }
    }
    foreach(IrregularFeStreamDef item,expensesDefIrregular){
        if (item.getId()==id){
            name = item.getName();
            color = item.getDecorationColor();
            return;
        }
    }

    found = false;
    return ;

}


// create a new Scenario from content of a JSON file
Scenario::FileResult Scenario::loadFromFile(QString fullFileName)
{
    QJsonValue buf;
    bool ok;
    Scenario::FileResult result = {.code=ERROR_OTHER, .errorStringUI="", .errorStringLog=""};


    // open the file
    QFile file(fullFileName);
    if (!file.exists()){
        result.code = FileResultCode::LOAD_FILE_DOES_NOT_EXIST;
        result.errorStringUI = tr("File %1 does not exist").arg(fullFileName);
        result.errorStringLog = QString("File %1 does not exist").arg(fullFileName);
        return result;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.code = FileResultCode::LOAD_CANNOT_OPEN_FILE;
        result.errorStringUI = tr("Cannot open file %1 in read-only mode").arg(fullFileName);
        result.errorStringLog = QString("Cannot open file %1 in read-only mode").arg(fullFileName);
        return result;
    }

    // read the whole content
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    if (error.error != QJsonParseError::NoError) {
        result.code = FileResultCode::LOAD_JSON_PARSING_ERROR;
        result.errorStringUI = tr("File %1 is not a GBP scenario file.\n\nDetails : Error code = %2 ,offset = %3, error message = %4").arg(fullFileName).arg(error.error).arg(error.offset).arg(error.errorString());
        result.errorStringLog = QString("File %1 is not a GBP scenario file.\n\nDetails : Error code = %2 ,offset = %3, error message = %4").arg(fullFileName).arg(error.error).arg(error.offset).arg(error.errorString());
        return result;
    }

    // read all the bits and pieces of Scenario from the Json
    QJsonObject root = doc.object();

    // version
    buf = root.value("Version");
    if (buf == QJsonValue::Undefined){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.errorStringUI = tr("Cannot find Version tag");
        result.errorStringLog = QString("Cannot find Version tag");
        return result;
    }
    if (buf.isString()==false){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.errorStringUI = tr("Version tag is not a string");
        result.errorStringLog = QString("Version tag is not a string");
        return result;
    }
    QString version = buf.toString();
    if( version.length()>VERSION_MAX_LEN ){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.errorStringUI = tr("Version tag has a length %1, which is longer than max allowed of %2").arg(version.length()).arg(VERSION_MAX_LEN);
        result.errorStringLog = QString("Version tag has a length %1, which is longer than max allowed of %2").arg(version.length()).arg(VERSION_MAX_LEN);
        return result;
    }
    if( version != Scenario::LatestVersion ){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.errorStringUI  = tr("File %1 is of version %2, which is incompatible with current version %3 (Scenario)").arg(fullFileName).arg(version).arg(Scenario::LatestVersion);
        result.errorStringLog  = QString("File %1 is of version %2, which is incompatible with current version %3 (Scenario)").arg(fullFileName).arg(version).arg(Scenario::LatestVersion);
        return result;
    }

    // name
    buf = root.value("Name");
    if (buf == QJsonValue::Undefined){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.errorStringUI = tr("Cannot find Name tag");
        result.errorStringLog = QString("Cannot find Name tag");
        return result;
    }
    if (buf.isString()==false){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.errorStringUI = tr("Name tag is not a string");
        result.errorStringLog = QString("Name tag is not a string");
        return result;
    }
    QString name = buf.toString();
    if (name.length()>NAME_MAX_LEN){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.errorStringUI = tr("Name tag is too long");
        result.errorStringLog = QString("Name tag is too long");
        return result;
    }

    // Description
    buf = root.value("Description");
    if (buf == QJsonValue::Undefined){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.errorStringUI = tr("Cannot find Description tag");
        result.errorStringLog = QString("Cannot find Description tag");
        return result;
    }
    if (buf.isString()==false){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.errorStringUI = tr("Description tag is not a string");
        result.errorStringLog = QString("Description tag is not a string");
        return result;
    }
    QString desc = buf.toString();
    if (desc.length()>DESC_MAX_LEN){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.errorStringUI = tr("Description tag has a length of %1, which is greater than the maximum allowed of %2").arg(desc.length()).arg(DESC_MAX_LEN);
        result.errorStringLog = QString("Description tag has a length of %1, which is greater than the maximum allowed of %2").arg(desc.length()).arg(DESC_MAX_LEN);
        return result;
    }

    // country code
    buf = root.value("CountryCode");
    if (buf == QJsonValue::Undefined){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.errorStringUI = tr("Cannot find CountryCode tag");
        result.errorStringLog = QString("Cannot find CountryCode tag");
        return result;
    }
    if (buf.isString()==false){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.errorStringUI = tr("CountryCode tag is not a string");
        result.errorStringLog = QString("CountryCode tag is not a string");
        return result;
    }
    QString countryCode = buf.toString();
    if ( !(CurrencyHelper::countryExists(countryCode)) ){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.errorStringUI = tr("Country code %1 is invalid").arg(countryCode);
        result.errorStringLog = QString("Country code %1 is invalid").arg(countryCode);
        return result;
    }

    // inflation
    buf = root.value("Inflation");
    if (buf == QJsonValue::Undefined){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.errorStringUI = tr("Cannot find Inflation tag");
        result.errorStringLog = QString("Cannot find Inflation tag");
        return result;
    }
    if (buf.isObject()==false){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.errorStringUI = tr("Inflation tag is not an object");
        result.errorStringLog = QString("Inflation tag is not an object");
        return result;
    }
    Util::OperationResult infParsingResult;
    Growth inflation = Growth::fromJson(buf.toObject(),infParsingResult);
    if (infParsingResult.success==false){
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.errorStringUI = tr("Inflation value is invalid : %1").arg(infParsingResult.errorStringUI);
        result.errorStringLog = QString("Inflation value is invalid : %1").arg(infParsingResult.errorStringLog);
        return result;
    }

    // => read the 4 complex maps <=
    QMap<QUuid,PeriodicFeStreamDef> incPsMap;
    QMap<QUuid,IrregularFeStreamDef> incIrMap;
    QMap<QUuid,PeriodicFeStreamDef> expPsMap;
    QMap<QUuid,IrregularFeStreamDef> expIrMap;

    // QMap Income Periodic
    {
        buf = root.value("IncomesPeriodic");
        if (buf == QJsonValue::Undefined){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.errorStringUI = tr("Cannot find IncomesPeriodic tag");
            result.errorStringLog = "Cannot find IncomesPeriodic tag";
            return result;
        }
        if (buf.isObject()==false){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.errorStringUI = tr("IncomesPeriodic tag is not an object");
            result.errorStringLog = "IncomesPeriodic tag is not an object";
            return result;
        }
        QJsonObject incPsObject = buf.toObject();
        // check max no of Stream Def
        if (incPsObject.count()> MAX_NO_STREAM_DEF){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.errorStringUI =tr("Too many Periodic Incomes items found (%1 found, max is %2)").arg(incPsObject.count()).arg(MAX_NO_STREAM_DEF);
            result.errorStringLog =QString("Too many Periodic Incomes items found (%1 found, max is %2)").arg(incPsObject.count()).arg(MAX_NO_STREAM_DEF);
            return result;
        }
        for (auto it = incPsObject.begin(); it != incPsObject.end(); ++it) {
            QString key = it.key();             // Stream Def ID string representation
            QUuid id = QUuid::fromString(key);  // the id itself
            if (id.isNull()) {
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.errorStringUI = tr("Periodic Income - Value for key %1 is not a valid UUID").arg(key);
                result.errorStringLog = QString("Periodic Income - Value for key %1 is not a valid UUID").arg(key);
                return result;
            }
            // extract associated Stream Def
            QJsonValueRef valueRef = it.value();
            if (!valueRef.isObject()){
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.errorStringUI = tr("Periodic Income - Value for key %1 is not an Object").arg(key);
                result.errorStringLog = QString("Periodic Income - Value for key %1 is not an Object").arg(key);
                return result;
            }
            QJsonObject valueObject = valueRef.toObject();
            Util::OperationResult parsingResult;
            PeriodicFeStreamDef value = PeriodicFeStreamDef::fromJson(valueObject,parsingResult);
            if(parsingResult.success==false){
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.errorStringUI = parsingResult.errorStringUI;
                result.errorStringLog = parsingResult.errorStringLog;
                return result;
            }
            incPsMap.insert(id, value);
        }
    }

    // QMap Income Irregular
    {
        buf = root.value("IncomesIrregular");
        if (buf == QJsonValue::Undefined){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.errorStringUI = tr("Cannot find IncomesIrregular tag");
            result.errorStringLog = "Cannot find IncomesIrregular tag";
            return result;
        }
        if (buf.isObject()==false){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.errorStringUI = tr("IncomesIrregular tag is not an object");
            result.errorStringLog = "IncomesIrregular tag is not an object";
            return result;
        }
        QJsonObject incIrrObject = buf.toObject();
        // check max no of Stream Def
        if (incIrrObject.count()> MAX_NO_STREAM_DEF){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.errorStringUI = tr("Too many Irregular Incomes items found (%1 found, max is %2)").arg(incIrrObject.count()).arg(MAX_NO_STREAM_DEF);
            result.errorStringLog = QString("Too many Irregular Incomes items found (%1 found, max is %2)").arg(incIrrObject.count()).arg(MAX_NO_STREAM_DEF);
            return result;
        }
        for (auto it = incIrrObject.begin(); it != incIrrObject.end(); ++it) {
            QString key = it.key(); // Stream Def ID string representation
            QUuid id = QUuid::fromString(key);  // the id itself
            if (id.isNull()) {
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.errorStringUI = tr("Irregular Income - Value for key %1 is not a valid UUID").arg(key);
                result.errorStringLog = QString("Irregular Income - Value for key %1 is not a valid UUID").arg(key);
                return result;
            }
            // extract associated Stream Def
            QJsonValueRef valueRef = it.value();
            if (!valueRef.isObject()){
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.errorStringUI = tr("Irregular Income - Value for key %1 is not an Object").arg(key);
                result.errorStringLog = QString("Irregular Income - Value for key %1 is not an Object").arg(key);
                return result;
            }
            QJsonObject valueObject = valueRef.toObject();
            Util::OperationResult parsingResult;
            IrregularFeStreamDef value = IrregularFeStreamDef::fromJson(valueObject,parsingResult);
            if(parsingResult.success==false){
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.errorStringUI = parsingResult.errorStringUI;
                result.errorStringLog = parsingResult.errorStringLog;
                return result;
            }
            incIrMap.insert(id, value);
        }
    }

    // QMap Expense Periodic
    {
        buf = root.value("ExpensesPeriodic");
        if (buf == QJsonValue::Undefined){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.errorStringUI = tr("Cannot find ExpensesPeriodic tag");
            result.errorStringLog = "Cannot find ExpensesPeriodic tag";
            return result;
        }
        if (buf.isObject()==false){
            throw std::domain_error("ExpensesPeriodic tag is not an object");
        }
        QJsonObject expPsObject = buf.toObject();
        // check max no of Stream Def
        if (expPsObject.count()> MAX_NO_STREAM_DEF){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.errorStringUI = tr("Too many Periodic Expenses items found (%1 found, max is %2)").arg(expPsObject.count()).arg(MAX_NO_STREAM_DEF);
            result.errorStringLog = QString("Too many Periodic Expenses items found (%1 found, max is %2)").arg(expPsObject.count()).arg(MAX_NO_STREAM_DEF);
            return result;
        }
        for (auto it = expPsObject.begin(); it != expPsObject.end(); ++it) {
            QString key = it.key(); // Stream Def ID string representation
            QUuid id = QUuid::fromString(key);  // the id itself
            if (id.isNull()) {
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.errorStringUI = tr("Periodic Expense - Value for key %1 is not a valid UUID").arg(key);
                result.errorStringLog = QString("Periodic Expense - Value for key %1 is not a valid UUID").arg(key);
                return result;
            }
            // extract associated Stream Def
            QJsonValueRef valueRef = it.value();
            if (!valueRef.isObject()){
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.errorStringUI = tr("Periodic Expense - Value for key %1 is not an Object").arg(key);
                result.errorStringLog = QString("Periodic Expense - Value for key %1 is not an Object").arg(key);
                return result;
            }
            QJsonObject valueObject = valueRef.toObject();
            Util::OperationResult parsingResult;
            PeriodicFeStreamDef value = PeriodicFeStreamDef::fromJson(valueObject,parsingResult);
            if(parsingResult.success==false){
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.errorStringUI = parsingResult.errorStringUI;
                result.errorStringLog = parsingResult.errorStringLog;
                return result;
            }
            expPsMap.insert(id, value);
        }
    }

    // QMap Expense Irregular
    {
        buf = root.value("ExpensesIrregular");
        if (buf == QJsonValue::Undefined){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.errorStringUI = tr("Cannot find ExpensesIrregular tag");
            result.errorStringLog = "Cannot find ExpensesIrregular tag";
            return result;
        }
        if (buf.isObject()==false){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.errorStringUI = tr("ExpensesIrregular tag is not an object");
            result.errorStringLog = "ExpensesIrregular tag is not an object";
            return result;
        }
        QJsonObject expIrrObject = buf.toObject();
        // check max no of Stream Def
        if (expIrrObject.count()> MAX_NO_STREAM_DEF){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.errorStringUI = tr("Too many Irregular Expenses items found (%1 found, max is %2)").arg(expIrrObject.count()).arg(MAX_NO_STREAM_DEF);
            result.errorStringLog = QString("Too many Irregular Expenses items found (%1 found, max is %2)").arg(expIrrObject.count()).arg(MAX_NO_STREAM_DEF);
            return result;
        }
        for (auto it = expIrrObject.begin(); it != expIrrObject.end(); ++it) {
            QString key = it.key(); // Stream Def ID string representation
            QUuid id = QUuid::fromString(key);  // the id itself
            if (id.isNull()) {
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.errorStringUI = tr("Irregular Expense - Value for key %1 is not a valid UUID").arg(key);
                result.errorStringLog = QString("Irregular Expense - Value for key %1 is not a valid UUID").arg(key);
                return result;
            }
            // extract associated Stream Def
            QJsonValueRef valueRef = it.value();
            if (!valueRef.isObject()){
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.errorStringUI = tr("Irregular Expense - Value for key %1 is not an Object").arg(key);
                result.errorStringLog = QString("Irregular Expense - Value for key %1 is not an Object").arg(key);
                return result;
            }
            QJsonObject valueObject = valueRef.toObject();
            Util::OperationResult parsingResult;
            IrregularFeStreamDef value = IrregularFeStreamDef::fromJson(valueObject,parsingResult);
            if(parsingResult.success==false){
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.errorStringUI = parsingResult.errorStringUI;
                result.errorStringLog = parsingResult.errorStringLog;
                return result;
            }
            expIrMap.insert(id, value);
        }
    }

    // build and return a new Scenario
    result.code = FileResultCode::SUCCESS;
    QSharedPointer<Scenario> ptr(new Scenario(version, name, desc, inflation, countryCode, incPsMap, incIrMap, expPsMap, expIrMap));
    result.scenarioPtr = ptr;
    return result;
}


// // empty scenario (test only)
// QSharedPointer<Scenario> Scenario::createNewEmptyScenario()
// {
//     Growth inflation = Growth();
//     QMap<QUuid,PeriodicFeStreamDef> incomesDefPeriodic = {};
//     QMap<QUuid,IrregularFeStreamDef> incomesDefIrregular = {};
//     QMap<QUuid,PeriodicFeStreamDef> expensesDefPeriodic = {};
//     QMap<QUuid,IrregularFeStreamDef> expensesDefIrregular = {};

//     QSharedPointer<Scenario> scenario = QSharedPointer<Scenario>(new Scenario(
//         LatestVersion, tr("Unnamed","Scenario"), "", inflation,  "CA", incomesDefPeriodic, incomesDefIrregular, expensesDefPeriodic, expensesDefIrregular));
//     return scenario;
// }




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

QString Scenario::getCountryCode() const
{
    return countryCode;
}

void Scenario::setCountryCode(const QString &newCountryCode)
{
    countryCode = newCountryCode;
}

QMap<QUuid, PeriodicFeStreamDef> Scenario::getIncomesDefPeriodic() const
{
    return incomesDefPeriodic;
}

void Scenario::setIncomesDefPeriodic(const QMap<QUuid, PeriodicFeStreamDef> &newIncomesDefPeriodic)
{
    incomesDefPeriodic = newIncomesDefPeriodic;
}

QMap<QUuid, IrregularFeStreamDef> Scenario::getIncomesDefIrregular() const
{
    return incomesDefIrregular;
}

void Scenario::setIncomesDefIrregular(const QMap<QUuid, IrregularFeStreamDef> &newIncomesDefIrregular)
{
    incomesDefIrregular = newIncomesDefIrregular;
}

QMap<QUuid, PeriodicFeStreamDef> Scenario::getExpensesDefPeriodic() const
{
    return expensesDefPeriodic;
}

void Scenario::setExpensesDefPeriodic(const QMap<QUuid, PeriodicFeStreamDef> &newExpensesDefPeriodic)
{
    expensesDefPeriodic = newExpensesDefPeriodic;
}

QMap<QUuid, IrregularFeStreamDef> Scenario::getExpensesDefIrregular() const
{
    return expensesDefIrregular;
}

void Scenario::setExpensesDefIrregular(const QMap<QUuid, IrregularFeStreamDef> &newExpensesDefIrregular)
{
    expensesDefIrregular = newExpensesDefIrregular;
}




