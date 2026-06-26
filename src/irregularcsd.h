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

#ifndef IRREGULARCSD_H
#define IRREGULARCSD_H

#include "csd.h"
#include "currencyhelper.h"
#include <quuid.h>
#include <QCoreApplication>
#include "daterange.h"
#include "fe.h"
#include "festream.h"


/**
 * @class IrregularCsd
 * @brief Definition of an Irregular Csd.
 * @details This type of Csd defines a set of amounts specified over an unconstrained date range.
 * Each amount occurs once and is not repeated. Amounts themselved cannot be negative, but the
 * Csd type may specify an income or an expense. There is no growth or inflation adjustment
 * applicable, as amounts are presumed to already include any type of growth.
 * Amounts are considered future values, with optional conversion to present value.
 */
class IrregularCsd : public Csd
{

    Q_DECLARE_TR_FUNCTIONS(IrregularCsd)

public:

    /**
     * @struct AmountInfo
     * @brief Structure to hold amount and associated notes for an irregular Csd.
     */
    struct AmountInfo{
        /// @brief Amount in decimal format. Cannot be negative.
        quint64 amount;
        /// @brief Notes associated with the amount (length limited).
        QString notes;
        /// @brief Maximum length for notes.
        static const int NOTES_MAX_LEN = 100;
        /**
         * @brief Equality operator for AmountInfo.
         * @param o The AmountInfo object to compare.
         * @return True if equal, false otherwise.
         */
        bool operator==(const AmountInfo &o) const;
        /**
         * @brief Converts AmountInfo to a JSON object.
         * @return JSON representation of AmountInfo.
         */
        QJsonObject toJson() const;
        /**
         * @brief Creates an AmountInfo from a JSON object.
         * @param jsonObject JSON object containing AmountInfo data.
         * @param result Operation result indicating success or failure.
         * @return Constructed AmountInfo object.
         */
        static AmountInfo fromJson(const QJsonObject& jsonObject, Util::ResultOfOperation &result);
    };

    // *** constructors and destructor ***

    /**
     * @brief Default constructor.
     * @details Used only for certain QMap operations.
     */
    IrregularCsd();

    /**
     * @brief Copy constructor.
     * @param o The IrregularCsd object to copy.
     */
    IrregularCsd(const IrregularCsd& o);

    /**
     * @brief Constructor with full initialization.
     * @param amountSet Map of dates to AmountInfo objects.
     * @param id Unique identifier.
     * @param name Name of the IrregularCsd.
     * @param desc Description of the IrregularCsd.
     * @param active Whether the IrregularCsd is active.
     * @param isIncome Whether the IrregularCsd represents income.
     * @param decorationColor Color for UI decoration.
     */
    IrregularCsd(const QMap<QDate,AmountInfo> &amountSet, const QUuid &id,
        const QString &name, const QString &desc, bool active, bool isIncome,
        const QColor& decorationColor);
    virtual ~IrregularCsd();


    // *** operators ***

    /**
     * @brief Assignment operator.
     * @param o The IrregularCsd object to assign.
     * @return Reference to this object.
     */
    IrregularCsd& operator=(const IrregularCsd &o);

    /**
     * @brief Equality operator.
     * @param o The IrregularCsd object to compare.
     * @return True if equal, false otherwise.
     */
    bool operator==(const IrregularCsd &o) const;

    /**
     * @brief Inequality operator.
     * @param o The IrregularCsd object to compare.
     * @return True if not equal, false otherwise.
     */
    bool operator!=(const IrregularCsd &o) const;


    // *** methods ***

    /**
     * @brief Generates a list of financial events from the properties of that IrregularCsd.
     * @param feStream FeStream to put the result in. It is reset before use. It is up
     * to the caller of this method to set the QWeakPointer to the proper Csd.
     * @param tomorrow The date of tomorrow.
     * @param fromto Interval of time inside which the events should be generated. Must be BOUNDED.
     * @param maxDateScenarioFeGeneration Maximum date for event generation.
     * @param pvAnnualDiscountRate Annual discount rate for present value calculation (percentage).
     * Value of 0 means do not transform future values into present value.
     * @param pvPresent Date defining the "present" for present value calculation.
     * @param saturationCount Number of times the financial event amount exceeded the maximum.
     * @param minMaxInfo Computed min/max values for events. Absolute value, never negative.
     * Invalid if 0 element returned.
     */
    void generateEventStream(FeStream& feStream, QDate tomorrow, DateRange fromto,
        QDate maxDateScenarioFeGeneration, double pvAnnualDiscountRate, QDate pvPresent,
        uint &saturationCount, FeMinMaxInfo& minMaxInfo) const;

    /**
     * @brief Evaluates if two IrregularCsd objects generate the same financial event list.
     * @details If true is returned, this is always exact. If false
     * is returned, sometimes it may be actually true.
     * @param o The IrregularCsd object to compare.
     * @param diff QString describing what has been identified as a cause for non identical.
     * @return True if the financial event lists are identical, false otherwise.
     */
    bool evaluateIfSameFeList(const IrregularCsd& o, QString& diff) const;

    /**
     * @brief Converts the IrregularCsd to a display string.
     * @param currInfo Currency information for formatting.
     * @param locale Locale for formatting.
     * @return Formatted string for display.
     */
    QString toStringForDisplay(CurrencyInfo currInfo, QLocale locale) const;

    /**
     * @brief Converts the IrregularCsd to a JSON object.
     * @return JSON representation of the IrregularCsd.
     */
    QJsonObject toJson() const;

    /**
     * @brief Creates an IrregularCsd from a JSON object.
     * @param jsonObject JSON object containing IrregularCsd data.
     * @param result Operation result indicating success or failure.
     * @return Constructed IrregularCsd object, under the form of a QSharedPointer.
     * If the operation failed, value is nullptr.
     */
    static QSharedPointer<IrregularCsd> fromJson(const QJsonObject& jsonObject,
        Util::ResultOfOperation &result);

    /**
     * @brief Creates a duplicate of the IrregularCsd. ID and/or name can be changed
     * if requested.
     * @param keepSameId If true, the ID is kept identical. If false, a new ID is created.
     * @param keepSameName If true, the name is kept identical. If false, the new name
     * is prefixed with "Copy of".
     * @return Duplicated IrregularCsd object, wrapped around a QSharedPointer.
     */
    QSharedPointer<IrregularCsd> duplicate(bool keepSameId, bool keepSameName) const;

    /**
     * @brief Copy a provided QHash<QUuid,QSharedPointer<IrregularCsd>>. For each pair,
     * creates a new QSharedPointer for value part, with the exact same content that the input one.
     * Key stays the same.
     * @param map
     */
    static QHash<QUuid,QSharedPointer<IrregularCsd>> deepCopyHashmap(
        const QHash<QUuid,QSharedPointer<IrregularCsd>> &in);



    // *** getters/setters ***

    /**
     * @brief Gets the amount set.
     * @return Map of dates to AmountInfo objects.
     */
    QMap<QDate, AmountInfo> getAmountSet() const;

    /**
     * @brief Sets the amount set.
     * @param newAmountSet Map of dates to AmountInfo objects.
     */
    void setAmountSet(const QMap<QDate, AmountInfo> &newAmountSet);


private:

    /// @brief Map of dates to AmountInfo objects (key: valid QDate, value: amount and note).
    QMap<QDate,AmountInfo> amountSet;

    /**
     * @struct validateKeysResult
     * @brief Structure to hold validation results for keys and values.
     */
    struct validateKeysResult{
        /// @brief Indicates if validation succeeded.
        bool valid;
        /// @brief Reason for validation failure (UI-friendly).
        QString reasonUI;
        /// @brief Reason for validation failure (for logging).
        QString reasonLog;
    };

    /**
     * @brief Validates keys and values in the amount set.
     * @param set Map of dates to AmountInfo objects to validate.
     * @return Validation result.
     */
    static validateKeysResult validateKeysAndValues(const QMap<QDate,AmountInfo> set);
};

#endif // IRREGULARCSD_H
