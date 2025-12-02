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
#include "irregularcsd.h"
#include "periodiccsd.h"
#include "tags.h"
#include "tagcsdrelationships.h"

/**
 * @class Scenario
 * @brief A set of information representing a specific group of income and expense definitions.
 * Each of these definitions is called a "Cash Stream Definition", or Csd.
 * @details Based on its Csds, a Scenario can generate a set of financial events distributed
 * through time (see FeStream).
 */
class Scenario
{

    Q_DECLARE_TR_FUNCTIONS(Scenario)

public:

    /**
     * @brief Latest file format version of a scenario on disk. Incremented when changes prevent
     * backward compatibility with earlier GBP versions.
     * @details Format released up to now are :
     *    1.0.0 : gbp 1.0 to and including 1.3
     *    2.0.0 : gbp 1.4 and up
     *  Format 2.0.0 cannot be read by gbp 1.3 ou before. But format 1.0.0 can be read by gbp 1.4+
     *  (will be converted on the fly to 2.0.0).
     */
    static QString LATEST_VERSION ;

    static QString VERSION_1; /// Label for version 1.0.0.
    static constexpr quint16 VERSION_MAX_LEN = 20; /// max length of the version label.
    static constexpr quint16 NAME_MAX_LEN = 100; /// max length of the scenario name.
    static constexpr quint16 DESC_MAX_LEN =4000 ; /// max length of the scenario description.

    /**
     * @brief Max no of Csd for a combination of (incomes/expense, Periodic/Irregular). It means
     * a scenario can have a total maximum of 4 * MAX_NO_STREAM_DEF Csds.
     */
    static constexpr quint16 MAX_NO_CSDS= 200;

    /**
     * @brief Possibles values for success/error code when loading or saving a scenario from/to
     * a file.
     */
    enum class FileResultCode { SUCCESS, ERROR_OTHER,
        SAVE_ERROR_CREATING_FILE_FOR_WRITING, SAVE_ERROR_OPENING_FILE_FOR_WRITING,
        SAVE_ERROR_WRITING_TO_FILE, SAVE_ERROR_JSON_CREATION,
        LOAD_FILE_DOES_NOT_EXIST, LOAD_FILE_IS_NOT_READABLE, LOAD_CANNOT_OPEN_FILE,
        LOAD_JSON_PARSING_ERROR, LOAD_JSON_SEMANTIC_ERROR, LOAD_CANNOT_UPGRADE, LOAD_ERROR,
        LOAD_UNKNOWN_VERSION};

    /**
     * @struct FileResult
     * @brief Provide info on the success or failure of loading/saving operations.
     */
    struct FileResult{
        FileResultCode code; /// Error code
        QString logErrorMessage; /// Explains the error, for logging (stays in English)
        bool version1found;
        QSharedPointer<Scenario> scenarioPtr; /// for Load only, not filled if code != SUCCESS

        FileResult();

        /**
         * @brief Convert a FileResultCode to an english QString.
         * @return The resulting QString.
         */
        QString codeToString();

        void init();
    };

    // --- Constructors and destructors ---

    Scenario() = delete;
    Scenario(const Scenario& o);

    /**
     * @brief Detailed Constructor. "EditedBy" is set to current version of GBP.
     * @param version Version of the file.
     * @param name Name of the scenario. Cut to a max of NAME_MAX_LEN char.
     * @param description Description of the scenario. Cut to a max of DESC_MAX_LEN char.
     * @param feGenerationDuration Max no of years the FE can be generated, starting from
     * "tomorrow".
     * @param inflation Inflation growth pattern.
     * @param countryCode ISO country code.
     * @param incomePeriodicCsdSet The set of periodic income Csds.
     * @param incomeIrregularCsdSet The set of irregular income Csds.
     * @param expensePeriodicCsdSet The set of periodic expense Csds.
     * @param expenseIrregularCsdSet The set of irregular expense Csds.
     * @param newTags A set of tag definitions.
     * @param newTagFsdRelationships Relationship between tags and Csds.
     */
    Scenario(const QString version, const QString name, const QString description,
        const quint16 feGenerationDuration, const Growth inflation, QString countryCode,
        QHash<QUuid,QSharedPointer<PeriodicCsd>> incomePeriodicCsdSet,
        QHash<QUuid,QSharedPointer<IrregularCsd>> incomeIrregularCsdSet,
        QHash<QUuid,QSharedPointer<PeriodicCsd>> expensePeriodicCsdSet,
        QHash<QUuid,QSharedPointer<IrregularCsd>> expenseIrregularCsdSet,
        const Tags newTags, const TagCsdRelationships newTagFsdRelationships);

    virtual ~Scenario();


    // --- operators ---

    Scenario& operator=(const Scenario &o);
    bool operator==(const Scenario &o) const;


    // --- Methods ---

    /**
     * @brief Generate a whole new suite of financial events for that scenario.
     * @param today The date of "today" as defined by gbp.
     * @param systemLocale Locale used for amount formatting.
     * @param fromto Date interval inside which the events should be generated. Must be of
     * type BOUNDED.
     * @param pvAnnualDiscountRate Annual discount rate in percentage, to transform future values
     * into present values. 0 means keep future values. Cannot be negative.
     * @param pvPresent Date considered to be the "present" for convertion to PV purpose. Usually,
     * "tomorrow" is what is used.
     * @param saturationCount Number of times the FE amount calculated was over the maximum allowed.
     * @return The full set of FE for each day, packaged under a QSharedPointer.
     */
    QSharedPointer<CombinedFeStreams> generateFinancialEvents(QDate today, QLocale systemLocale,
        DateRange fromto, double pvAnnualDiscountRate, QDate pvPresent, uint &saturationCount)
        const;

    /**
     * @brief Find the name and the decoration color of a Csd using its ID.
     * @param id ID of the Csd to find.
     * @param name Name of the Csd if found.
     * @param color Decoration color of the Csd if found.
     * @param found True if a Csd has been found, false otherwise.
     */
    void getCsdNameAndColorFromId(QUuid id,  QString& name, QColor& color, bool& found) const;

    /**
     * @brief Determine if a Csd exists.
     * @param id ID of the Csd.
     * @return True if a Csd with that ID has been found, false otherwise.
     */
    bool csdIdExists(QUuid id) const;

    /**
     * @brief Compare this scenario with another one and evaluate if the FE stream generated
     * by both will be exactly the same.
     * @details The goal is to know if changes made to a scenario will with 100% certainty NOT
     * require the regeneration of data for the update of the graphics. It does so without actually
     * generating the events and is then very significantly much faster.
     * However this technic leads to a result that is not exact all the time when it is FALSE.
     * It is 100% exact when it is TRUE.
     * @param o The other scenario to compare to.
     * @param diff Describe what has been identified as a cause for non identical FeStream.
     * @return True if the FE stream generated by both is 100% guaranteed to be exactly the same,
     * false otherwise.
     */
    bool evaluateIfSameFeStream(QSharedPointer<Scenario> o, QString& diff) const;

    /**
     * @brief Save the scenario in an JSON file. If the file alreay exists, it is overwritten.
     * @param fullFileName Name of the file to save the scenario into.
     * @return Result of the save operation.
     */
    Scenario::FileResult saveToFile(QString fullFileName) const;

    /**
     * @brief Create a new Scenario object in memory from the content of a JSON scenario file on
     * disk.
     * @details If an old file format version is found, the file is automatically converted to the
     * latest format, ON THE FLY, without notifying the user.
     * @param fullFileName Name of the file to load the scenario from.
     * @return Result of the load operation.
     */
    static Scenario::FileResult loadFromFile(QString fullFileName);

    /**
     * @brief Create an empty scenario. No inflation, no Csd, no tag.
     * @param countryCode The ISO country code for the new blank scenario.
     * @return The new blank scenario.
     */
    static QSharedPointer<Scenario> createBlankScenario(QString countryCode);

    /**
     * @brief Get no of Csds of type Periodic Income.
     * @param activeOnly If true, only search through the active Csds.
     * @return No of Csds of type Periodic Income.
     */
    int getNoOfPeriodicIncomes(bool activeOnly);

    /**
     * @brief Get no of Csds of type Irregular Income.
     * @param activeOnly If true, only search through the active Csds.
     * @return No of Csds of type Irregular Income.
     */
    int getNoOfIrregularIncomes(bool activeOnly);

    /**
     * @brief Get no of Csds of type Periodic Expense.
     * @param activeOnly If true, only search through the active Csds.
     * @return No of Csds of type Periodic Expense.
     */
    int getNoOfPeriodicExpenses(bool activeOnly);

    /**
     * @brief Get no of Csds of type Irregular Expense.
     * @param activeOnly If true, only search through the active Csds.
     * @return No of Csds of type Irregular Expense.
     */
    int getNoOfIrregularExpenses(bool activeOnly);


    // --- Getters and setters ---

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
    QHash<QUuid, QSharedPointer<PeriodicCsd>> getIncomePeriodicCsds() const;
    void setIncomePeriodicCsds(const QHash<QUuid,
        QSharedPointer<PeriodicCsd>> &newIncomesDefPeriodic);
    QHash<QUuid, QSharedPointer<IrregularCsd>> getIncomeIrregularCsds() const;
    void setIncomeIrregularCsds(const QHash<QUuid,
        QSharedPointer<IrregularCsd>> &newIncomesDefIrregular);
    QHash<QUuid, QSharedPointer<PeriodicCsd>> getExpensePeriodicCsds() const;
    void setExpensePeriodicCsds(const QHash<QUuid,
        QSharedPointer<PeriodicCsd>> &newExpensesDefPeriodic);
    QHash<QUuid, QSharedPointer<IrregularCsd>> getExpenseIrregularCsds() const;
    void setExpenseIrregularCsds(const QHash<QUuid,
        QSharedPointer<IrregularCsd>> &newExpensesDefIrregular);
    quint16 getFeGenerationDuration() const;
    void setFeGenerationDuration(quint16 newFeGenerationDuration);
    Tags getTags() const;
    void setTags(const Tags &newTags);
    TagCsdRelationships getTagCsdRelationships() const;
    void setTagCsdRelationships(const TagCsdRelationships &newTagCsdRelationships);


private:

    QString version;     /// Format version
    QString name;        /// Name of the scenario
    QString description; /// Some description

    /**
     * @brief Max no of years after today for which a financial event is allowed to occur.
     */
    quint16 feGenerationDuration;

    /**
     * @brief Scenario's inflation growth pattern
     */
    Growth inflation;

    /**
     * @brief ISO 3166 alpha-2 (used to derive currency and hence no of decimals).
     * Cannot be changed once set
     */
    QString countryCode;

    /**
     * @brief Periodic Income Csds. We use QSharedPointer, because Csds are referenced elsewhere.
     * Key is Csd ID
     */
    QHash<QUuid,QSharedPointer<PeriodicCsd>> incomePeriodicCsds;

    /**
     * @brief Irregular Income Csds. We use QSharedPointer, because Csds are referenced elsewhere.
     * Key is Csd ID
     */
    QHash<QUuid,QSharedPointer<IrregularCsd>> incomeIrregularCsds;

    /**
     * @brief Periodic Expense Csds. We use QSharedPointer, because Csds are referenced elsewhere.
     * Key is Csd ID
     */
    QHash<QUuid,QSharedPointer<PeriodicCsd>> expensePeriodicCsds;

    /**
     * @brief Irregular Expense Csds. We use QSharedPointer, because Csds are referenced elsewhere.
     * Key is Csd ID
     */
    QHash<QUuid,QSharedPointer<IrregularCsd>> expenseIrregularCsds;

    Tags tags; /// Set of tags defined in the scenario

    /**
     * @brief Relationships between tags and Csds for this scenario (N-N).
     */
    TagCsdRelationships tagCsdRelationships;



    // -- Methods ---


    /**
     * @brief Check if Tag IDs and Csds ID referenced by tagCsdRelationships exists.
     * @return Return true if it is the case, false otherwise.
     */
    bool checkTagCsdRelationshipsIntegrity();

    /**
     * @brief Deep compare for 2 QHash<QUuid,QSharedPointer<PeriodicCsd>>.
     * @param map1 First map to compare.
     * @param map2 Second map to compare.
     * @return True if equal, false otherwise.
     */
    bool deepComparePeriodicCsdMaps(const QHash<QUuid, QSharedPointer<PeriodicCsd>>& map1,
        const QHash<QUuid, QSharedPointer<PeriodicCsd>>& map2) const;

    /**
     * @brief Deep compare for 2 QHash<QUuid,QSharedPointer<IrregularCsd>>.
     * @param map1 First map to compare.
     * @param map2 Second map to compare.
     * @return True if equal, false otherwise.
     */
    bool deepCompareIrregularCsdMaps(const QHash<QUuid, QSharedPointer<IrregularCsd>>& map1,
        const QHash<QUuid, QSharedPointer<IrregularCsd>>& map2) const;

};

#endif // SCENARIO_H
