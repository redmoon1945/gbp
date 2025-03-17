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

#ifndef SCENARIO_H
#define SCENARIO_H

#include <QString>
#include <QSharedPointer>
#include <QCoreApplication>
#include "growth.h"
#include "combinedfestreams.h"
#include "irregularfestreamdef.h"
#include "periodicfestreamdef.h"
#include "tag.h"
#include "tags.h"
#include "tagcsdrelationships.h"

// Set of declarative statements representing incomes, expenses and different contextual information.
// It is used to create a flow of financial events (called "flow data") in time.
class Scenario
{

    Q_DECLARE_TR_FUNCTIONS(Scenario)

public:
    static QString LATEST_VERSION ;
    static QString VERSION_1;

    // max length of the version
    static quint16 VERSION_MAX_LEN;
    // max length of the scenario name
    static quint16 NAME_MAX_LEN;
    // max length of the scenario description
    static quint16 DESC_MAX_LEN;
    // max no of Stream Definition per type
    static quint16 MAX_NO_STREAM_DEF;
    // no Financial Events can ever be generated past this date. Min value in years from "tomorrow"
    static quint16 MIN_DURATION_FE_GENERATION;
    // no Financial Events can ever be generated past this date. Max value in years from "tomorrow"
    static quint16 MAX_DURATION_FE_GENERATION;
    // default value for computation duration when creating a new scenario
    static quint16 DEFAULT_DURATION_FE_GENERATION;
    // max no of tag
    static quint16 MAX_NO_TAGS;

    enum FileResultCode { SUCCESS, ERROR_OTHER,
        SAVE_ERROR_CREATING_FILE_FOR_WRITING, SAVE_ERROR_OPENING_FILE_FOR_WRITING,
        SAVE_ERROR_WRITING_TO_FILE, SAVE_ERROR_INTERNAL_JSON_CREATION,
        LOAD_FILE_DOES_NOT_EXIST, LOAD_CANNOT_OPEN_FILE, LOAD_JSON_PARSING_ERROR,
        LOAD_JSON_SEMANTIC_ERROR, LOAD_CANNOT_UPGRADE};
    struct FileResult{
        // Error code
        FileResultCode code;
        // for display (translated in Locale language)
        QString errorStringUI;
        // for logging (stay in English)
        QString errorStringLog;
        // Load only : version 1 of scenario file format found. If true, it has been
        // converted anyway on the fly to v 2
        bool version1found;
        // for Load only, not filled if code != SUCCESS
        QSharedPointer<Scenario> scenarioPtr;
    };

    // Constructors and destructors
    Scenario(const Scenario& o);
    Scenario(const QString version, const QString name, const QString description,
        const quint16 feGenerationDuration, const Growth inflation, QString countryCode,
        const QMap<QUuid,PeriodicFeStreamDef> incomesDefPeriodicSet,
        const QMap<QUuid,IrregularFeStreamDef> incomesDefIrregularSet,
        const QMap<QUuid,PeriodicFeStreamDef> expensesDefPeriodicSet,
        const QMap<QUuid,IrregularFeStreamDef> expensesDefIrregularSet,
        const Tags newTags, const TagCsdRelationships newTagFsdRelationships);
    virtual ~Scenario();

    // operators
    Scenario& operator=(const Scenario &o);
    bool operator==(const Scenario &o) const;

    // Methods
    QMap<QDate,CombinedFeStreams::DailyInfo>generateFinancialEvents(QDate today,
        QLocale systemLocale, DateRange fromto, double pvAnnualDiscountRate, QDate pvPresent,
        uint &saturationCount) const;
    void getStreamDefNameAndColorFromId(QUuid id,  QString& name, QColor& color, bool& found) const;
    bool fsdIdExists(QUuid id) const;
    bool evaluateIfSameFlowData(QSharedPointer<Scenario> o) const;
    FileResult saveToFile(QString fullFileName) const;
    static FileResult loadFromFile(QString fullFileName);
    static QSharedPointer<Scenario> createBlankScenario(QString countryCode);
    int getNoOfPeriodicIncomes(bool activeOnly);
    int getNoOfIrregularIncomes(bool activeOnly);
    int getNoOfPeriodicExpenses(bool activeOnly);
    int getNoOfIrregularExpenses(bool activeOnly);

    // Getters and setters
    QString getVersion() const;
    void setVersion(const QString &newVersion);
    QString getName() const;
    void setName(const QString &newName);
    QString getDescription() const;
    void setDescription(const QString &newDescription);
    Growth getInflation() const;
    void setInflation(const Growth &newInflation);
    QString getCountryCode() const;
    void setCountryCode(const QString &newCountryCode);
    QMap<QUuid, PeriodicFeStreamDef> getIncomesDefPeriodic() const;
    void setIncomesDefPeriodic(const QMap<QUuid, PeriodicFeStreamDef> &newIncomesDefPeriodic);
    QMap<QUuid, IrregularFeStreamDef> getIncomesDefIrregular() const;
    void setIncomesDefIrregular(const QMap<QUuid, IrregularFeStreamDef> &newIncomesDefIrregular);
    QMap<QUuid, PeriodicFeStreamDef> getExpensesDefPeriodic() const;
    void setExpensesDefPeriodic(const QMap<QUuid, PeriodicFeStreamDef> &newExpensesDefPeriodic);
    QMap<QUuid, IrregularFeStreamDef> getExpensesDefIrregular() const;
    void setExpensesDefIrregular(const QMap<QUuid, IrregularFeStreamDef> &newExpensesDefIrregular);
    quint16 getFeGenerationDuration() const;
    void setFeGenerationDuration(quint16 newFeGenerationDuration);
    Tags getTags() const;
    void setTags(const Tags &newTags);
    TagCsdRelationships getTagCsdRelationships() const;
    void setTagCsdRelationships(const TagCsdRelationships &newTagCsdRelationships);

private:
    QString version;
    QString name;
    QString description;
    // Max no of years after today for which a financial event is allowed to occur
    quint16 feGenerationDuration;
    // Scenario's inflation
    Growth inflation;
    // ISO 3166 alpha-2 (used to derive currency and hence no of decimals).
    // Cannot be changed once set
    QString countryCode;
    // The Financial Stream Definitions
    QMap<QUuid,PeriodicFeStreamDef> incomesDefPeriodic;       // key is Stream Def ID
    QMap<QUuid,IrregularFeStreamDef> incomesDefIrregular;     // key is Stream Def ID
    QMap<QUuid,PeriodicFeStreamDef> expensesDefPeriodic;      // key is Stream Def ID
    QMap<QUuid,IrregularFeStreamDef> expensesDefIrregular;    // key is Stream Def ID
    // Set of tags defined in the scenario
    Tags tags;
    // Relationships between tags and Cash Stream Definition for this scenario (N-N)
    TagCsdRelationships tagCsdRelationships;

    // Methods
    bool checkTagCsdRelationshipsIntegrity();

};

#endif // SCENARIO_H
