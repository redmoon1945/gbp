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

#ifndef SCENARIO_H
#define SCENARIO_H

#include <QString>
#include <QSharedPointer>
#include <QCoreApplication>
#include "growth.h"
#include "combinedfestreams.h"
#include "irregularfestreamdef.h"
#include "periodicfestreamdef.h"


class Scenario
{

    Q_DECLARE_TR_FUNCTIONS(Scenario)

public:
    static QString LatestVersion ;
    static int VERSION_MAX_LEN;     // max length of the version
    static int NAME_MAX_LEN;        // max length of the scenario name
    static int DESC_MAX_LEN;        // max length of the scenario description
    static int MAX_NO_STREAM_DEF;   // max no of Stream Definition per type


    enum FileResultCode { SUCCESS=1, ERROR_OTHER=2,
                          SAVE_ERROR_CREATING_FILE_FOR_WRITING=3, SAVE_ERROR_OPENING_FILE_FOR_WRITING=4, SAVE_ERROR_WRITING_TO_FILE=5, SAVE_ERROR_INTERNAL_JSON_CREATION=6,
                          LOAD_FILE_DOES_NOT_EXIST=7, LOAD_CANNOT_OPEN_FILE=8, LOAD_JSON_PARSING_ERROR=9, LOAD_JSON_SEMANTIC_ERROR=10};
    struct FileResult{
        FileResultCode code;
        QString errorStringUI;  // for display (trnaslated in Locale language)
        QString errorStringLog; // for logging (stay in English)
        QSharedPointer<Scenario> scenarioPtr;  // for Load only, not filled if code != SUCCESS
    };



    // Constructors and destructor
    Scenario(const Scenario& o);
    Scenario(const QString version, const QString name, const QString description, const Growth inflation, QString countryCode,
             const QMap<QUuid,PeriodicFeStreamDef> incomesDefPeriodicSet,
             const QMap<QUuid,IrregularFeStreamDef> incomesDefIrregularSet,
             const QMap<QUuid,PeriodicFeStreamDef> expensesDefPeriodicSet,
             const QMap<QUuid,IrregularFeStreamDef> expensesDefIrregularSet);
    virtual ~Scenario();

    // operators
    Scenario& operator=(const Scenario &o);
    bool operator==(const Scenario &o) const;

    // Methods
    QMap<QDate,CombinedFeStreams::DailyInfo>generateFinancialEvents(QLocale systemLocale, DateRange fromto, double pvAnnualDiscountRate, QDate pvPresent, uint &saturationCount) const;
    void getStreamDefNameAndColorFromId(QUuid id,  QString& name, QColor& color, bool& found) const;
    FileResult saveToFile(QString fullFileName) const;
    static FileResult loadFromFile(QString fullFileName);
    int getNoOfPeriodicIncomes();
    int getNoOfIrregularIncomes();
    int getNoOfPeriodicExpenses();
    int getNoOfIrregularExpenses();

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

private:
    QString version;
    QString name;
    QString description;
    Growth inflation;
    QString countryCode;   // ISO 3166 alpha-2 (used to derive currency and hence no of decimals). Cannot be changed once set
    QMap<QUuid,PeriodicFeStreamDef> incomesDefPeriodic;       // key is Stream Def ID
    QMap<QUuid,IrregularFeStreamDef> incomesDefIrregular;     // key is Stream Def ID
    QMap<QUuid,PeriodicFeStreamDef> expensesDefPeriodic;      // key is Stream Def ID
    QMap<QUuid,IrregularFeStreamDef> expensesDefIrregular;    // key is Stream Def ID
};

#endif // SCENARIO_H
