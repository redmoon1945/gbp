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

#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <qfileinfo.h>
#include <qjsonarray.h>
#include "scenario.h"
#include "currencyhelper.h"

// stay independant of "gbpcontroller.h"

// File format version of a scenario on disk. Format released up to now are :
// 1.0.0 : gbp 1.0 to and including 1.3
// 2.0.0 : gbp 1.4
// Format 2.0.0 cannot be read by gbp 1.3 ou before.
// but format 1.0.0 can be read by gbp 1.4+ (will be converted on the fly to 2.0.0)
QString Scenario::LATEST_VERSION = "2.0.0";
QString Scenario::VERSION_1 = "1.0.0";

quint16 Scenario::NAME_MAX_LEN = 100;
quint16 Scenario::DESC_MAX_LEN = 4000;
quint16 Scenario::VERSION_MAX_LEN = 20;
quint16 Scenario::MAX_NO_STREAM_DEF = 200;
quint16 Scenario::MIN_DURATION_FE_GENERATION = 1;   // we mandate at least 1 year of data
quint16 Scenario::MAX_DURATION_FE_GENERATION = 100;   // slow search speed at maximum...
// This has a great influence on the response time after a user click on a point to display info.
// 25 provides a reasonable search time, but expect people to boost it to 50 or even 75.
// More optimization may be required.
quint16 Scenario::DEFAULT_DURATION_FE_GENERATION = 25; // provide a reasonable search time for click
// Max no of tags defined per scenario. Essentially, well over any perceived reasonable value.
quint16 Scenario::MAX_NO_TAGS = 200;


Scenario::Scenario(const Scenario &o){
    this->version = o.version;
    this->name = o.name.left(NAME_MAX_LEN); // truncate if required
    this->description = o.description;
    this->feGenerationDuration = o.feGenerationDuration;
    this->inflation = o.inflation;
    this->countryCode = o.countryCode;
    this->incomesDefPeriodic = o.incomesDefPeriodic;
    this->incomesDefIrregular = o.incomesDefIrregular;
    this->expensesDefPeriodic = o.expensesDefPeriodic;
    this->expensesDefIrregular = o.expensesDefIrregular;
    this->tags = o.tags;
    this->tagCsdRelationships = o.tagCsdRelationships;
}


Scenario::~Scenario()
{
}


Scenario::Scenario(const QString version, const QString name, const QString description,
    const quint16 feGenerationDuration, const Growth inflation, QString countryCode,
    const QMap<QUuid, PeriodicFeStreamDef> incomesDefPeriodicSet, const QMap<QUuid,
    IrregularFeStreamDef> incomesDefIrregularSet,
    const QMap<QUuid, PeriodicFeStreamDef> expensesDefPeriodicSet,
    const QMap<QUuid, IrregularFeStreamDef> expensesDefIrregularSet,
    const Tags newTags, const TagCsdRelationships newtagCsdRelationships) :
    version(version.left(VERSION_MAX_LEN)), name(name.left(NAME_MAX_LEN)),
    description(description.left(DESC_MAX_LEN)), feGenerationDuration(feGenerationDuration),
    inflation(inflation), countryCode(countryCode), incomesDefPeriodic(incomesDefPeriodicSet),
    incomesDefIrregular(incomesDefIrregularSet), expensesDefPeriodic(expensesDefPeriodicSet),
    expensesDefIrregular(expensesDefIrregularSet), tags(newTags),
    tagCsdRelationships(newtagCsdRelationships)
{}


Scenario &Scenario::operator=(const Scenario &o)
{
    this->version = o.version.left(VERSION_MAX_LEN);
    this->name = o.name.left(NAME_MAX_LEN);
    this->description = o.description.left(DESC_MAX_LEN);
    this->feGenerationDuration = o.feGenerationDuration;
    this->inflation = o.inflation;
    this->countryCode = o.countryCode;
    this->incomesDefPeriodic = o.incomesDefPeriodic;
    this->incomesDefIrregular = o.incomesDefIrregular;
    this->expensesDefPeriodic = o.expensesDefPeriodic;
    this->expensesDefIrregular = o.expensesDefIrregular;
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
    if ( !(this->tags==o.tags) ){
        return false;
    }
    if ( !(this->tagCsdRelationships==o.tagCsdRelationships) ){
        return false;
    }
    return true;
}


// Save the scenario in an JSON file. If the file exists, it is overwritten.
Scenario::FileResult Scenario::saveToFile(QString fullFileName) const
{
    QJsonObject jobject;
    Scenario::FileResult result = {.code=ERROR_OTHER, .errorStringUI="", .errorStringLog=""};
    QJsonDocument doc;

    // simple elements
    jobject["Version"] = version;
    jobject["Name"] = name;
    jobject["Description"] = description;
    jobject["FeGenerationDuration"] = feGenerationDuration;
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

        // Tags
        jobject["Tags"] = tags.toJson();

        // Tags relationships
        jobject["TagRelationships"] = tagCsdRelationships.toJson();

        // Build the final JSON document
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

    QFile file(fullFileName);
    bool fileAlreadyExist = file.exists();
    // If the file does not already exist, this function will try to create a new
    // file before opening it
    if (false==file.open(QFile::WriteOnly)){
        if (fileAlreadyExist){
            result.code = SAVE_ERROR_OPENING_FILE_FOR_WRITING;
            result.errorStringUI = tr("Cannot overwrite the scenario file "
                ": check permissions");
            result.errorStringLog = "Cannot open the file in write-only mode";
        } else {
            result.code = SAVE_ERROR_CREATING_FILE_FOR_WRITING;
            result.errorStringUI = tr("Cannot create the scenario file : check permissions");
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


// Create a new Scenario object in memory from the content of a JSON scenario file on disk.
// If an old file format version is found, the file is automatically converted to the latest format,
// ON THE FLY, without notifying the user.
Scenario::FileResult Scenario::loadFromFile(QString fullFileName)
{
    QJsonValue buf;
    bool ok;
    Scenario::FileResult result = {.code=ERROR_OTHER, .errorStringUI="", .errorStringLog="",
        .version1found=false};

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
        result.errorStringUI = tr("File %1 is not a GBP scenario file.\n\nDetails : "
                                  "Error code = %2 ,offset = %3, error message = %4").arg(fullFileName).arg(error.error)
                                   .arg(error.offset).arg(error.errorString());
        result.errorStringLog = QString("File %1 is not a GBP scenario file.\n\nDetails : "
                                        "Error code = %2 ,offset = %3, error message = %4").arg(fullFileName).arg(error.error)
                                    .arg(error.offset).arg(error.errorString());
        return result;
    }

    // read all the bits and pieces of Scenario from the Json
    QJsonObject root = doc.object();

    // version : first thing to read, in order to check version
    // If version 1 found, convert to version 2 on the fly
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
        result.errorStringUI = tr("Version tag has a length %1, which is longer "
            "than max allowed of %2").arg(version.length()).arg(VERSION_MAX_LEN);
        result.errorStringLog = QString("Version tag has a length %1, which is longer "
            "than max allowed of %2").arg(version.length()).arg(VERSION_MAX_LEN);
        return result;
    }
    if( version != Scenario::LATEST_VERSION ){
        if (version == Scenario::VERSION_1) {
            result.version1found = true;    // notify that we have auto-converted V1 to latest
            version = LATEST_VERSION;       // since it is auto-converted when loaded
        } else {
            // appears to be an invalid version. This is an error.
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            QFileInfo fileInfo(fullFileName);
            result.errorStringUI  = tr("Scenario file %1 is of version %2, which is incompatible "
                "with version %3 used by this version of the application").arg(fileInfo.fileName())
                .arg(version).arg(Scenario::LATEST_VERSION);
            result.errorStringLog  = QString("Scenario file %1 is of version %2, which is "
                "incompatible with  version %3 used by this version of the application")
                .arg(fileInfo.fileName()).arg(version).arg(Scenario::LATEST_VERSION);
            return result;
        }
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
        result.errorStringUI = tr("Description tag has a length of %1, which is greater than "
                                  "the maximum allowed of %2").arg(desc.length()).arg(DESC_MAX_LEN);
        result.errorStringLog = QString("Description tag has a length of %1, which is greater "
                                        "than the maximum allowed of %2").arg(desc.length()).arg(DESC_MAX_LEN);
        return result;
    }

    // feGenerationDuration
    buf = root.value("FeGenerationDuration");
    quint16 feGenDuration;
    if (buf == QJsonValue::Undefined){
        // Older versions may not have this field : give it default value
        feGenDuration = Scenario::DEFAULT_DURATION_FE_GENERATION;
    } else {
        if (buf.isDouble()==false){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.errorStringUI = tr("FeGeneration tag is not a number");
            result.errorStringLog = "FeGeneration tag is not a number";
            return result;
        }
        int ok;
        double d = buf.toDouble();
        feGenDuration = Util::extractQuint16FromDoubleWithNoFracPart(d,
                                                                     Scenario::MAX_DURATION_FE_GENERATION, ok);
        if ( ok==-1 ){
            result.errorStringUI = tr("FeGenerationDuration - Value %1 is not an integer").arg(d);
            result.errorStringLog = QString("FeGenerationDuration - Value %1 is not an integer")
                                        .arg(d);
            return result;
        }
        if ( ok==-2 ){
            result.errorStringUI = tr("FeGenerationDuration - Value %1 is too big").arg(d);
            result.errorStringLog = QString("FeGenerationDuration - Value %1 is too big").arg(d);
            return result;
        }
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
        result.errorStringUI = tr("Inflation value is invalid : %1").arg(
            infParsingResult.errorStringUI);
        result.errorStringLog = QString("Inflation value is invalid : %1")
                                    .arg(infParsingResult.errorStringLog);
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
            result.errorStringUI =tr("Too many Periodic Incomes items found (%1 found, max "
                                      "is %2)").arg(incPsObject.count()).arg(MAX_NO_STREAM_DEF);
            result.errorStringLog =QString("Too many Periodic Incomes items found (%1 found, "
                                            "max is %2)").arg(incPsObject.count()).arg(MAX_NO_STREAM_DEF);
            return result;
        }
        for (auto it = incPsObject.begin(); it != incPsObject.end(); ++it) {
            QString key = it.key();             // Stream Def ID string representation
            QUuid id = QUuid::fromString(key);  // the id itself
            if (id.isNull()) {
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.errorStringUI = tr("Periodic Income - Value for key %1 is not a valid "
                                          "UUID").arg(key);
                result.errorStringLog = QString("Periodic Income - Value for key %1 is not a "
                                                "valid UUID").arg(key);
                return result;
            }
            // extract associated Stream Def
            QJsonValueRef valueRef = it.value();
            if (!valueRef.isObject()){
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.errorStringUI = tr("Periodic Income - Value for key %1 is not an "
                                          "Object").arg(key);
                result.errorStringLog = QString("Periodic Income - Value for key %1 is not an "
                                                "Object").arg(key);
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
            result.errorStringUI = tr("Too many Irregular Incomes items found (%1 found, max "
                                      "is %2)").arg(incIrrObject.count()).arg(MAX_NO_STREAM_DEF);
            result.errorStringLog = QString("Too many Irregular Incomes items found (%1 found, "
                                            "max is %2)").arg(incIrrObject.count()).arg(MAX_NO_STREAM_DEF);
            return result;
        }
        for (auto it = incIrrObject.begin(); it != incIrrObject.end(); ++it) {
            QString key = it.key(); // Stream Def ID string representation
            QUuid id = QUuid::fromString(key);  // the id itself
            if (id.isNull()) {
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.errorStringUI = tr("Irregular Income - Value for key %1 is not a "
                                          "valid UUID").arg(key);
                result.errorStringLog = QString("Irregular Income - Value for key %1 is not "
                                                "a valid UUID").arg(key);
                return result;
            }
            // extract associated Stream Def
            QJsonValueRef valueRef = it.value();
            if (!valueRef.isObject()){
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.errorStringUI = tr("Irregular Income - Value for key %1 is not an "
                                          "Object").arg(key);
                result.errorStringLog = QString("Irregular Income - Value for key %1 is not "
                                                "an Object").arg(key);
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
            result.errorStringUI = tr("Too many Periodic Expenses items found (%1 found, "
                                      "max is %2)").arg(expPsObject.count()).arg(MAX_NO_STREAM_DEF);
            result.errorStringLog = QString("Too many Periodic Expenses items found (%1 found, "
                                            "max is %2)").arg(expPsObject.count()).arg(MAX_NO_STREAM_DEF);
            return result;
        }
        for (auto it = expPsObject.begin(); it != expPsObject.end(); ++it) {
            QString key = it.key(); // Stream Def ID string representation
            QUuid id = QUuid::fromString(key);  // the id itself
            if (id.isNull()) {
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.errorStringUI = tr("Periodic Expense - Value for key %1 is not a "
                                          "valid UUID").arg(key);
                result.errorStringLog = QString("Periodic Expense - Value for key %1 is not a "
                                                "valid UUID").arg(key);
                return result;
            }
            // extract associated Stream Def
            QJsonValueRef valueRef = it.value();
            if (!valueRef.isObject()){
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.errorStringUI = tr("Periodic Expense - Value for key %1 is not an "
                                          "Object").arg(key);
                result.errorStringLog = QString("Periodic Expense - Value for key %1 is not "
                                                "an Object").arg(key);
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
            result.errorStringUI = tr("Too many Irregular Expenses items found (%1 found, "
                                      "max is %2)").arg(expIrrObject.count()).arg(MAX_NO_STREAM_DEF);
            result.errorStringLog = QString("Too many Irregular Expenses items found (%1 found, "
                                            "max is %2)").arg(expIrrObject.count()).arg(MAX_NO_STREAM_DEF);
            return result;
        }
        for (auto it = expIrrObject.begin(); it != expIrrObject.end(); ++it) {
            QString key = it.key(); // Stream Def ID string representation
            QUuid id = QUuid::fromString(key);  // the id itself
            if (id.isNull()) {
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.errorStringUI = tr("Irregular Expense - Value for key %1 is not a "
                                          "valid UUID").arg(key);
                result.errorStringLog = QString("Irregular Expense - Value for key %1 is not "
                                                "a valid UUID").arg(key);
                return result;
            }
            // extract associated Stream Def
            QJsonValueRef valueRef = it.value();
            if (!valueRef.isObject()){
                result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
                result.errorStringUI = tr("Irregular Expense - Value for key %1 is not an "
                                          "Object").arg(key);
                result.errorStringLog = QString("Irregular Expense - Value for key %1 is not "
                                                "an Object").arg(key);
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

    // Tags
    Tags theTags;
    buf = root.value("Tags");
    if (buf == QJsonValue::Undefined){
        // older version of scenario file do not have tags, keep the empty set then
    } else {
        if (buf.isObject()==false){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.errorStringUI = tr("Tags tag is not an object");
            result.errorStringLog = QString("Tags tag is not an object");
            return result;
        }
        Util::OperationResult parsingResult;
        theTags = Tags::fromJson(buf.toObject(), parsingResult);
        if (parsingResult.success==false){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.errorStringUI = tr("Tags value is invalid : %1").arg(
                parsingResult.errorStringUI);
            result.errorStringLog = QString("Tags value is invalid : %1")
                .arg(parsingResult.errorStringLog);
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
            result.errorStringUI = tr("Tag relationships is not an object");
            result.errorStringLog = QString("Tag relationships is not an object");
            return result;
        }
        Util::OperationResult parsingResult;
        rel = TagCsdRelationships::fromJson(buf.toObject(),parsingResult);
        if (parsingResult.success==false){
            result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
            result.errorStringUI = tr("Tag Relationships value is invalid : %1").arg(
                parsingResult.errorStringUI);
            result.errorStringLog = QString("Tag Relationships value is invalid : %1")
                                        .arg(parsingResult.errorStringLog);
            return result;
        }
    }


    // All data have been collected : build and return a new Scenario
    result.code = FileResultCode::SUCCESS;
    QSharedPointer<Scenario> ptr(new Scenario(version, name, desc, feGenDuration, inflation,
        countryCode, incPsMap, incIrMap, expPsMap, expIrMap, theTags, rel));
    result.scenarioPtr = ptr;

    // Once the scenario is built, check that all relationships have tags and fsds defined
    if (false == ptr->checkTagCsdRelationshipsIntegrity()) {
        result.code = FileResultCode::LOAD_JSON_SEMANTIC_ERROR;
        result.errorStringUI = tr("Tag Relationships integrity is invalid");
        result.errorStringLog = QString("Tag Relationships integrity is invalid");
        return result;
    }

    // Finally, if it was an older file format version, UPDATE the file to the latest format
    // without notifying the user
    if (result.version1found==true) {
        FileResult convertResult = ptr->saveToFile(fullFileName);
        if (convertResult.code != FileResultCode::SUCCESS) {
            // we have a problem, should not happen in principle
            result.code = FileResultCode::LOAD_CANNOT_UPGRADE;
            result.errorStringUI = convertResult.errorStringUI;
            result.errorStringLog = convertResult.errorStringLog;
            return result;
        }
    }

    return result;
}


// Generate the whole suite of financial events for that scenario
// Input params :
//   today : today as defined by gbp
//   systemLocale : Locale used for amount formatting
//   fromTo : interval of time inside which the events should be generated. Must be of type BOUNDED.
//   pvAnnualDiscountRate : annual discount rate in percentage, to transform future into present
//                          value. 0 means keep future values. Cannot be negative.
//   pvPresent : date considered to be the "present" for convertion to PV purpose. Usually,
//               "tomorrow" is what is required
// Output params:
//   saturationCount : number of times the FE amount was over the maximum allowed
QMap<QDate, CombinedFeStreams::DailyInfo> Scenario::generateFinancialEvents(QDate today,
    QLocale systemLocale, DateRange fromto, double pvAnnualDiscountRate, QDate pvPresent,
    uint &saturationCount) const
{
    CombinedFeStreams comb;
    uint saturationNo;
    saturationCount = 0;
    FeMinMaxInfo minMaxInfo; // we wont use it
    bool found;

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
    if (fromto.getType() != DateRange::BOUNDED) {
        throw std::invalid_argument("fromtoInitial is not of type BOUNDED");
    }

    CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromCountryCode(systemLocale,
        countryCode, found);
    if (!found){
        // should never happen
        return comb.getCombinedStreams();
    }

    // compute max date for FeGeneration
    QDate maxDate = today.addYears(feGenerationDuration);

    foreach(PeriodicFeStreamDef item,incomesDefPeriodic){
        QList<Fe> stream = item.generateEventStream(fromto, maxDate, inflation,
            pvAnnualDiscountRate, pvPresent, saturationNo, minMaxInfo);
        comb.addStream(stream,currInfo);
        saturationCount += saturationNo;
    }
    foreach(PeriodicFeStreamDef item,expensesDefPeriodic){
        QList<Fe> stream = item.generateEventStream(fromto, maxDate, inflation,
            pvAnnualDiscountRate, pvPresent, saturationNo, minMaxInfo);
        comb.addStream(stream,currInfo);
        saturationCount += saturationNo;
    }
    foreach(IrregularFeStreamDef item,incomesDefIrregular){
        QList<Fe> stream = item.generateEventStream(fromto, maxDate, pvAnnualDiscountRate,
            pvPresent, saturationNo, minMaxInfo);
        comb.addStream(stream,currInfo);
        saturationCount += saturationNo;
    }
    foreach(IrregularFeStreamDef item,expensesDefIrregular){
        QList<Fe> stream = item.generateEventStream(fromto, maxDate, pvAnnualDiscountRate,
            pvPresent, saturationNo, minMaxInfo);
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


// Does the Financial Stream Definition identified by id exists ?
bool Scenario::fsdIdExists(QUuid id) const
{
    foreach(PeriodicFeStreamDef item,incomesDefPeriodic){
        if (item.getId()==id){
            return true;
        }
    }
    foreach(PeriodicFeStreamDef item,expensesDefPeriodic){
        if (item.getId()==id){
            return true;
        }
    }
    foreach(IrregularFeStreamDef item,incomesDefIrregular){
        if (item.getId()==id){
            return true;
        }
    }
    foreach(IrregularFeStreamDef item,expensesDefIrregular){
        if (item.getId()==id){
            return true;
        }
    }
    return false;
}


// Compare this scenario with another one and evaluate if the cenario flow data generated by both
// will be exactly the same. It does so without actually generating the events and is then very
// significantly much faster, though not exact all the time. As a matter of fact, there are some
// more complex cases where False is returned but the flow data is still the same.
bool Scenario::evaluateIfSameFlowData(QSharedPointer<Scenario> o) const
{
    if ( (feGenerationDuration!=o->feGenerationDuration) ||
        (inflation!=o->inflation) ){
        return false;
    }

    if ( (incomesDefPeriodic.size() != o->incomesDefPeriodic.size()) ||
        (expensesDefPeriodic.size() != o->expensesDefPeriodic.size()) ||
        (incomesDefIrregular.size() != o->incomesDefIrregular.size()) ||
        (expensesDefIrregular.size() != o->expensesDefIrregular.size())
        ){
        return false;
    }

    // Incomes Periodic
    {
        QList<PeriodicFeStreamDef> valuesPer = incomesDefPeriodic.values();
        QList<PeriodicFeStreamDef> otherValuesPer = o->incomesDefPeriodic.values();
        foreach(PeriodicFeStreamDef value, valuesPer){
            bool found = false;
            for(int i=0;i<otherValuesPer.size();i++){
                PeriodicFeStreamDef otherValuePer = otherValuesPer.at(i);
                if( true == value.evaluateIfSameFeList(otherValuePer) ){
                    // These 2 items will generate the same FE List, lets go to the other "value"
                    found = true;
                    // remove the otherItem for the next loop iteration
                    otherValuesPer.removeAt(i);
                    break;
                }
            }
            if(found==false){
                // we cant find an item that will generate the same FE List.
                return false;
            }
        }
    }

    // Expenses Periodic
    {
        QList<PeriodicFeStreamDef> valuesPer = expensesDefPeriodic.values();
        QList<PeriodicFeStreamDef> otherValuesPer = o->expensesDefPeriodic.values();
        foreach(PeriodicFeStreamDef value, valuesPer){
            bool found = false;
            for(int i=0;i<otherValuesPer.size();i++){
                PeriodicFeStreamDef otherValuePer = otherValuesPer.at(i);
                if( true == value.evaluateIfSameFeList(otherValuePer) ){
                    // These 2 items will generate the same FE List, lets go to the other "value"
                    found = true;
                    // remove the otherItem for the next loop iteration
                    otherValuesPer.removeAt(i);
                    break;
                }
            }
            if(found==false){
                // we cant find an item that will generate the same FE List.
                return false;
            }
        }
    }


    // Incomes Irregular
    {
        QList<IrregularFeStreamDef> valuesIrr = incomesDefIrregular.values();
        QList<IrregularFeStreamDef> otherValuesIrr = o->incomesDefIrregular.values();
        foreach(IrregularFeStreamDef valuesIrr, valuesIrr){
            bool found = false;
            for(int i=0;i<otherValuesIrr.size();i++){
                IrregularFeStreamDef otherValueIrr = otherValuesIrr.at(i);
                if( true == valuesIrr.evaluateIfSameFeList(otherValueIrr) ){
                    // These 2 items will generate the same FE List, lets go to the other "value"
                    found = true;
                    // remove the otherItem for the next loop iteration
                    otherValuesIrr.removeAt(i);
                    break;
                }
            }
            if(found==false){
                // we cant find an item that will generate the same FE List.
                return false;
            }
        }
    }


    // Expenses Irregular
    {
        QList<IrregularFeStreamDef> valuesIrr = expensesDefIrregular.values();
        QList<IrregularFeStreamDef> otherValuesIrr = o->expensesDefIrregular.values();
        foreach(IrregularFeStreamDef valueIrr, valuesIrr){
            bool found = false;
            for(int i=0;i<otherValuesIrr.size();i++){
                IrregularFeStreamDef otherValueIrr = otherValuesIrr.at(i);
                if( true == valueIrr.evaluateIfSameFeList(otherValueIrr) ){
                    // These 2 items will generate the same FE List, lets go to the other "value"
                    found = true;
                    // remove the otherItem for the next loop iteration
                    otherValuesIrr.removeAt(i);
                    break;
                }
            }
            if(found==false){
                // we cant find an item that will generate the same FE List.
                return false;
            }
        }
    }


    return true;
}


// Create an empty scenario. No inflation, no Csd, no tag
QSharedPointer<Scenario> Scenario::createBlankScenario(QString countryCode)
{
    QSharedPointer<Scenario> newScenario;
    newScenario = QSharedPointer<Scenario>(new Scenario(
        Scenario::LATEST_VERSION, tr("No name"), "", Scenario::DEFAULT_DURATION_FE_GENERATION,
        Growth::fromConstantAnnualPercentageDouble(0), countryCode, {},{},{},{},
        Tags(), TagCsdRelationships()));
    return newScenario;
}


int Scenario::getNoOfPeriodicIncomes(bool activeOnly)
{
    if (activeOnly==true) {
        int no = 0;
        foreach (PeriodicFeStreamDef ps, incomesDefPeriodic) {
            if (ps.getActive()==true){
                no++;
            }
        }
        return no;
    } else {
        return incomesDefPeriodic.size();
    }
}


int Scenario::getNoOfIrregularIncomes(bool activeOnly)
{
    if (activeOnly==true) {
        int no = 0;
        foreach (IrregularFeStreamDef is, incomesDefIrregular) {
            if (is.getActive()==true){
                no++;
            }
        }
        return no;
    } else {
        return incomesDefIrregular.size();
    }

}


int Scenario::getNoOfPeriodicExpenses(bool activeOnly)
{
    if (activeOnly==true) {
        int no = 0;
        foreach (PeriodicFeStreamDef ps, expensesDefPeriodic) {
            if (ps.getActive()==true){
                no++;
            }
        }
        return no;
    } else {
        return expensesDefPeriodic.size();
    }
}


int Scenario::getNoOfIrregularExpenses(bool activeOnly)
{
    if (activeOnly==true) {
        int no = 0;
        foreach (IrregularFeStreamDef is, expensesDefIrregular) {
            if (is.getActive()==true){
                no++;
            }
        }
        return no;
    } else {
        return expensesDefIrregular.size();
    }
}


// Check if Tag IDs and Fsds ID referenced by tagCsdRelationships exists.
// Return true if it is the case, false otherwise
bool Scenario::checkTagCsdRelationshipsIntegrity()
{
    QSet<QUuid> theTagsRel = tagCsdRelationships.getAllTagsWithRelationships();
    QSet<QUuid> theCsdsRel = tagCsdRelationships.getAllCsdsWithRelationships();

    // Check Tag ID exists
    foreach (QUuid tId, theTagsRel) {
        bool found = tags.containsTagId(tId);
        if (found==false) {
            return false;
        }
    }
    foreach (QUuid fId, theCsdsRel) {
        bool found = fsdIdExists(fId);
        if (found==false) {
            return false;
        }
    }
    return true;
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

void Scenario::setIncomesDefIrregular(const QMap<QUuid,
    IrregularFeStreamDef> &newIncomesDefIrregular)
{
    incomesDefIrregular = newIncomesDefIrregular;
}

QMap<QUuid, PeriodicFeStreamDef> Scenario::getExpensesDefPeriodic() const
{
    return expensesDefPeriodic;
}

void Scenario::setExpensesDefPeriodic(const QMap<QUuid,
    PeriodicFeStreamDef> &newExpensesDefPeriodic)
{
    expensesDefPeriodic = newExpensesDefPeriodic;
}

QMap<QUuid, IrregularFeStreamDef> Scenario::getExpensesDefIrregular() const
{
    return expensesDefIrregular;
}

void Scenario::setExpensesDefIrregular(const QMap<QUuid,
    IrregularFeStreamDef> &newExpensesDefIrregular)
{
    expensesDefIrregular = newExpensesDefIrregular;
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




