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

#ifndef PERIODICCSD_H
#define PERIODICCSD_H

#include <QCoreApplication>
#include <QHash>
#include "csd.h"
#include "festream.h"
#include "growth.h"
#include "util.h"
#include "daterange.h"
#include "fe.h"

/**
 * @class PeriodicCsd
 * @brief Definition of a Periodic Csd.
 * @details It is essentially a amount of money that is repeated over a specific time interval.
 * The amount may actually change due to optional "growth" pattern (defined in this Csd) or optional
 * inflation (defined in the scenario it belongs to).
 *
 * There is a "Start" date and "End Date" to describe the range of time inside which all FE can be
 * generated. Start date indicates the first date where there will be a FE generated, except if the
 * period type is END_OF_MONTH, in which casethe FE will occurs at the end of the month of
 * Start date. The "effective" End date can be specified in 2 ways :
 *   -> "Static" value : useScenarioForEndDate = false. The end date is fixed and specified by
 *      endDate value. It can never be smaller than Start date.
 *   -> "Dynamic" value : useScenarioForEndDate = true. In that case, the "effective" End date is
 *      calculated from external data, that is the date of "today" and the scenario's
 *      feGenerationDuration value. Because it is dynamic, it means we cannot validate at creation
 *      time that "effective" End date >= Start date. If it occurs that
 *      "effective" End date < Start date, then there will be no FE generated.
 */
class PeriodicCsd : public Csd
{
    Q_DECLARE_TR_FUNCTIONS(PeriodicCsd)


public:

    /// @brief Enumeration for period types.
    enum class PeriodType {DAILY, WEEKLY,MONTHLY,END_OF_MONTHLY,YEARLY};

    /// @brief Enumeration for growth strategies.
    enum class GrowthStrategy {NONE, INFLATION, CUSTOM};

    /// @brief Maximum period multiplier
    static const int PERIOD_MULTIPLIER_MAX;

    /// @brief Minimum period multiplier.
    static const int PERIOD_MULTIPLIER_MIN;

    /// @brief Maximum growth application period.
    static const int GROWTH_APP_PERIOD_MAX;

    /// @brief Minimum growth application period.
    static const int GROWTH_APP_PERIOD_MIN;

    /// @brief Maximum inflation adjustment factor.
    static const double MAX_INFLATION_ADJUSTMENT_FACTOR;

    /**
     * @brief Minimum start date value.
     * @details This is to prevent a crazy date in the past, which also generates a lot of data
     * in the occurrence dates list (e.g. year 1000...). This is a design choice.
     */
    static const QDate MIN_START_DATE;

    // *** constructors and destructor ***

    /**
     * @brief Default constructor. The Csd created is valid, but values are dummy.
     */
    PeriodicCsd();

    /**
     * @brief Copy constructor.
     * @param o The PeriodicCsd object to copy.
     */
    PeriodicCsd(const PeriodicCsd& o);

    /**
     * @brief Constructor with full initialization.
     * @param periodicType Type of period (e.g., DAILY, WEEKLY).
     * @param periodMultiplier Multiplier for the period.
     * @param amount Amount to be repeated, in smallest currency unit.
     * @param growth Growth pattern for the amount.
     * @param growthStrategy Strategy for growth (NONE, INFLATION, CUSTOM).
     * @param growthApplicationPeriod Frequency of growth application.
     * @param id Unique identifier to be applied to this PeriodicCsd.
     * @param name Name of the PeriodicCsd.
     * @param desc Description of the PeriodicCsd.
     * @param active Whether the PeriodicCsd is active.
     * @param isIncome Whether the PeriodicCsd represents income.
     * @param decorationColor Color for UI decoration.
     * @param startDate Start date for the stream. Must NOT be < MIN_START_DATE.
     * @param endDate End date for the stream.
     * @param useScenarioForEndDate Whether to use scenario's end date.
     * @param inflationAdjustmentFactor Factor to adjust scenario inflation.
     */
    PeriodicCsd(PeriodType periodicType, quint16 periodMultiplier, quint64 amount,
        const Growth &growth, const GrowthStrategy &growthStrategy, quint16 growthApplicationPeriod,
        const QUuid &id, const QString &name, const QString &desc, bool active, bool isIncome,
        const QColor& decorationColor, const QDate &startDate, const QDate &endDate,
        bool useScenarioForEndDate, double inflationAdjustmentFactor);
    virtual ~PeriodicCsd();

    // *** operators ***

    /**
     * @brief Assignment operator.
     * @param o The PeriodicCsd object to assign.
     * @return Reference to this object.
     */
    PeriodicCsd& operator=(const PeriodicCsd& o);

    /**
     * @brief Equality operator.
     * @param o The PeriodicCsd object to compare.
     * @return True if equal, false otherwise.
     */
    bool operator==(const PeriodicCsd& o) const;

    /**
     * @brief Inequality operator.
     * @param o The PeriodicCsd object to compare.
     * @return True if not equal, false otherwise.
     */
    bool operator!=(const PeriodicCsd& o) const;


    // methods

    /**
     * @brief Generate a list of financial events from the properties of that Periodic Csd.
     * @details It is important to know that the growth will be applied from the "start" date
     * and not from "tomorrow" date. However, the first occurrence date kept will never be smaller
     * than tomorrow.
     * @param feStream FeStream to put the result in. It is reset before use. It is up
     * to the caller of this method to set the QWeakPointer to the proper Csd.
     * @param tomorrow The date of tomorrow as defined in gbp. No event will be kept if
     * generated before this date.
     * @param fromto Interval of time outside which no events will be kept.
     * Must be of type BOUNDED.
     * @param maxDateScenarioFeGeneration Max date when an event can be generated (taken
     * from scenario max duration, and added to "tomorrow"). There is no limit on this date.
     * @param inflation Scenario's inflation to apply to the events. Internally corrected with the
     * Inflation Adjustment Factor.
     * @param pvDiscountRate ANNUAL discount rate in percentage to apply to transform the amounts to
     * Present Value. Value of 0 means do not transform future values into present value.
     * @param pvPresent Define what is the "present" as far as PV conversation is concerned.
     * Note that this date can be afer the first occurrence of events.
     * @param saturationCount Number of times the FE amount was over the maximum allowed.
     * @param minMaxInfo Computed min/max for values (absolute value, never negative). Invalid if
     * 0 element returned.
     */
    void generateEventStream(FeStream& feStream, QDate tomorrow, DateRange fromto,
        QDate maxDateScenarioFeGeneration, const Growth &inflation, double pvDiscountRate,
        QDate pvPresent, uint &saturationCount, FeMinMaxInfo& minMaxInfo) const;

    /**
     * @brief Evaluates if two PeriodicCsd objects generate the same financial event list.
     * @details If true is returned, this is always exact. If false
     * is returned, sometimes it may be actually true.
     * @param o The PeriodicCsd object to compare.
     * @param diff QString describing what has been identified as a cause for non identical.
     * FeStream.
     * @return True if the financial event lists are identical, false otherwise.
     */
    bool evaluateIfSameFeList(const PeriodicCsd& o, QString& diff) const;

    /**
     * @brief Converts the PeriodicCsd to a display string.
     * @param currInfo Currency information for formatting.
     * @param locale Locale for formatting.
     * @return Formatted string for display.
     */
    QString toStringForDisplay(CurrencyInfo currInfo, QLocale locale) const;

    /**
     * @brief Converts the PeriodicCsd to a JSON object.
     * @return JSON representation of the PeriodicCsd.
     */
    QJsonObject toJson() const;

    /**
     * @brief Creates a PeriodicCsd from a JSON object.
     * @param jsonObject JSON object containing PeriodicCsd data.
     * @param result Operation result indicating success or failure.
     * @return Constructed PeriodicCsd object, under the form of a QSharedPointer. If the operation
     * failed, value is nullptr.
     */
    static QSharedPointer<PeriodicCsd> fromJson(const QJsonObject& jsonObject,
        Util::ResultOfOperation &result);

    /**
     * @brief Creates a duplicate of the PeriodicCsd. ID and/or name can be changed
     * if requested.
     * @param keepSameId If true, the ID is kept identical. If false, a new ID is created.
     * @param keepSameName If true, the name is kept identical. If false, the new name
     * @return Duplicated PeriodicCsd object, wrapped in a QSharedPointer.
     */
    QSharedPointer<PeriodicCsd> duplicate(bool keepSameId, bool keepSameName) const ;

    /**
     * @brief Copy a provided QHash<QUuid,QSharedPointer<PeriodicCsd>>. For each pair,
     * creates a new QSharedPointer for value part, with the exact same content that the input one.
     * Key stays the same.
     * @param map
     */
    static QHash<QUuid,QSharedPointer<PeriodicCsd>> deepCopyHashmap(
        const QHash<QUuid,QSharedPointer<PeriodicCsd>> &in);

    /**
     * @brief Gets the effective end date, considering the end date set for the Csd and
     * the max date set in the scenario.
     * @details It make sure that, in all circonstances, the effective end date is never greater
     * than the maximum set in the scenario (max duration).
     * @param maxDateScenario Maximum date defined in the scenario.
     * @return Effective end date.
     */
    QDate getRealEndDate(const QDate maxDateScenario) const;


    // *** Getters ***

    /**
     * @brief Gets the period type.
     * @return Period type.
     */
    PeriodType getPeriod() const;

    /**
     * @brief Gets the period multiplier.
     * @return Period multiplier.
     */
    quint16 getPeriodMultiplier() const;

    /**
     * @brief Gets the amount (decimal form).
     * @return Amount in smallest currency unit.
     */
    quint64 getAmount() const;

    /**
     * @brief Sets the amount.
     * @param newAmount Amount in smallest currency unit.
     */
    void setAmount(quint64 newAmount);

    /**
     * @brief Gets the growth pattern.
     * @return Growth pattern.
     */
    Growth getGrowth() const;

    /**
     * @brief Gets the growth strategy.
     * @return Growth strategy.
     */
    GrowthStrategy getGrowthStrategy() const;

    /**
     * @brief Gets the growth application period.
     * @return Growth application period.
     */
    quint16 getGrowthApplicationPeriod() const;

    /**
     * @brief Gets the inflation adjustment factor.
     * @return Inflation adjustment factor.
     */
    double getInflationAdjustmentFactor() const;

    /**
     * @brief Gets the start date.
     * @return Start date.
     */
    QDate getStartDate() const;

    /**
     * @brief Gets the end date.
     * @return End date.
     */
    QDate getEndDate() const;

    /**
     * @brief Checks if scenario end date is used.
     * @return True if scenario end date is used, false otherwise.
     */
    bool getUseScenarioForEndDate() const;

    /**
     * @brief Gets the next event date after the given date, i.e. date + 1 * (period multiplier)
     * time units.
     * @details The result is computed from the given date only, with no knowledge of the
     * original start date. Depending on the period type :
     *   - DAILY, WEEKLY : date + (multiplier) days or weeks. No side effect.
     *   - MONTHLY, YEARLY : uses QDate::addMonths() / QDate::addYears(), which clamp to the last
     *     day of the target month when the day does not exist (e.g. Jan 31 + 1 month = Feb 28,
     *     Feb 29 + 1 year = Feb 28).
     *   - END_OF_MONTHLY : if date is already an end-of-month, returns the last day of the month
     *     (multiplier) months later. Otherwise, returns the last day of the month
     *     (multiplier - 1) months later (i.e. the end of the current month when multiplier = 1).
     *
     * **WARNING - do not chain calls to generate an occurrence series.** For MONTHLY and
     * YEARLY, calling this method on its own previous result causes a "day-of-month drift" :
     * once a day has been clamped in a short month, the original day is lost for good. For
     * example, a monthly series starting on Jan 31 would become Feb 28, Mar 28, Apr 28, ...
     * instead of Feb 28, Mar 31, Apr 30, ... and a yearly series starting on Feb 29 2024 would
     * stay on Feb 28 even in later leap years. To generate a series, use getNthEventDate(),
     * which always computes from the first occurrence (anchor).
     *
     * **WARNING - do not use it to get the first occurrence.** For END_OF_MONTHLY with a
     * mid-month start date and a multiplier > 1, getNextEventDate(startDate) skips
     * (multiplier - 1) ends-of-month. Use getFirstEventDate() instead.
     * @param date Reference date. Must be valid.
     * @return Next event date.
     * @throws std::invalid_argument If date is invalid or the period type is unknown.
     * @see getNthEventDate()
     */
    QDate getNextEventDate(QDate date) const;

    /**
     * @brief Gets the date of the n-th occurrence following an anchor (first occurrence) date.
     * @details The date is always computed directly from the anchor, i.e. anchor + n * (period
     * multiplier) time units, and never by stepping from the previous occurrence. This prevents
     * the "day-of-month drift" that happens when stepping occurrence by occurrence : for
     * example, a monthly event starting on Jan 31 would otherwise become Feb 28, Mar 28,
     * Apr 28, ... instead of Feb 28, Mar 31, Apr 30, ... When the anchor's day does not exist in
     * the target month (MONTHLY) or year (YEARLY, Feb 29), the last day of that month is used.
     * For END_OF_MONTHLY, the result is always the last day of the target month.
     * For DAILY and WEEKLY, the result is identical to repeated calls to getNextEventDate().
     * @param anchor Date of the first occurrence (n = 0). Must be valid. For END_OF_MONTHLY,
     * it must be an end-of-month date.
     * @param n Index of the occurrence to compute. n = 0 returns the anchor.
     * @return The date of the n-th occurrence.
     * @throws std::invalid_argument If anchor is invalid, the period type is unknown, or anchor
     * is not an end-of-month date for END_OF_MONTHLY.
     * @throws std::out_of_range If, for MONTHLY, END_OF_MONTHLY or YEARLY, n * periodMultiplier
     * does not fit in an int (required by QDate::addMonths() and QDate::addYears()).
     * @see getNextEventDate()
     */
    QDate getNthEventDate(QDate anchor, quint32 n) const;

    /**
     * @brief Gets the date of the first occurrence (first financial event) of this Csd.
     * @details This is startDate, except for END_OF_MONTHLY when startDate is not an
     * end-of-month : the first occurrence is then the last day of startDate's month, WHATEVER
     * the period multiplier is. The period multiplier only defines the interval BETWEEN
     * occurrences. For example, with END_OF_MONTHLY every 3 months and startDate = 2025-01-15,
     * the first occurrence is 2025-01-31 (then 2025-04-30, 2025-07-31, ...).
     * Note : getNextEventDate(startDate) does NOT return the first occurrence. It returns the
     * occurrence that follows an occurrence on startDate, i.e. the SECOND occurrence when
     * startDate is itself the first one (all period types, except END_OF_MONTHLY with a
     * mid-month startDate). For END_OF_MONTHLY with a mid-month startDate, it counts
     * (multiplier) ends-of-month from startDate : e.g. every 3 months from 2025-01-15 gives
     * 2025-03-31, while the first occurrence is 2025-01-31. Always use this method to get the
     * first occurrence.
     * The end date is not taken into account : the returned date may be after the effective
     * end date, in which case there is no occurrence at all.
     * @return The date of the first occurrence.
     * @see getNthEventDate() to get the following occurrences from this date.
     */
    QDate getFirstEventDate() const;

    /**
     * @brief Convert an PeriodType class enum value to an int. This is used when serializing
     * to a JSON object or as a QVarian for e.g. Listbox.
     * @details To maintain compatibility with GBP 1.6.3 and earlier, the following values must be
     * returned :
     *   PeriodType::DAILY = 0
     *   PeriodType::WEEKLY = 1
     *   PeriodType::MONTHLY = 2
     *   PeriodType::END_OF_MONTHLY = 3
     *   PeriodType::YEARLY = 4
     * @throws std::invalid_argument if periodType is unknown.
     * @param pType PeriodType class enum value to convert.
     * @return The equivalent int.
     */
    static int periodTypeToInt(PeriodType pType);

    /**
     * @brief Convert an int to a PeriodType class enum value. This is used when serializing
     * from a JSON object or as a QVarian for e.g. Listbox.
     * @param value The int to convert.
     * @param convertedType Result of the conversion. Valid only if conversion succeeded.
     * @return True if the conversion succeeded, false otherwise (int value is invalid).
     */
    static bool intToPeriodType(int value, PeriodType& convertedType);

    static QString getPeriodName(PeriodType period, bool capitalizeFirstLetter, bool plural) ;


private:

    // *** Variables ***

    /// @brief Period type. The complete periodicity is given by period and periodMultiplier.
    PeriodType period;

    /**
     * @brief Multiplier of the period, used to define the complete periodicity.
     * @details Value must be in [PERIOD_MULTIPLIER_MIN,PERIOD_MULTIPLIER_MAX].
     */
    quint16 periodMultiplier;

    /**
     * @brief Amount to be repeated, expressed in the smallest currency unit. Always a non negative
     * number even if this is an expense.
     */
    quint64 amount;

    /// @brief Grow Strategy.
    GrowthStrategy growthStrategy;

    /// @brief Custom growth for an instance of PeriodicCsd, used when growStrategy is CUSTOM.
    Growth growth;

    /**
     * @brief Apply growth every "growthApplicationPeriod" occurrence of amount. Value must be in
     * [GROWTH_APP_PERIOD_MIN,GROWTH_APP_PERIOD_MAX ]. Not used when growStrategy is None.
     */
    quint16 growthApplicationPeriod;

    /**
     * @brief Defines when the event stream is allowed to start.
     * @details This is the first date when a financial even can occur for that Csd. In the special
     * case where type = END-OF-MONTH, the actual date is the next end-of-month if startDate
     * is not already an end-of-month. Cannot be smaller than MIN_START_DATE.
     */
    QDate startDate;

    /**
     * @brief Defines the date for which the last financial events is allowed to occur. Valid only
     * if useScenarioforEndDate == false.
     * @details There is no limit on the date passed in the constructor, but :
     *   -> it must be >= start.
     *   -> it will be capped to maxScenarioDate when FeStream is created
     *      with generateEventStream().
     *  Cannot be smaller than MIN_START_DATE.addDays(1).
     */
    QDate endDate;

    /**
     * @brief Determine if the real "effective" End date is static or dynamic. In the first case,
     * useScenarioForEndDate == False and the value of endDate is used as the final End
     * date. In the second case, useScenarioForEndDate == True and End date is determined
     * by external value ("today" and scenario's feGenerationDuration).
     */
    bool useScenarioForEndDate;

    /**
     * @brief Factor to adjust scenario inflation.
     * @details If not 1, change the value of scenario inflation applied as a growth to this
     * element (each growth value is multiplied by this factor). Cannot be negative.
     * Max = MAX_INFLATION_ADJUSTMENT_FACTOR. Used only if growthStrategy = INFLATION.
     */
    double inflationAdjustmentFactor;

    // *** methods ***

     /**
     * @brief Convert an GrowthStrategy class enum value to an int.
     * @details To maintain compatibility with GBP 1.6.3 and earlier, the following values must be
     * returned :
     *   GrowthStrategy::NONE = 0
     *   GrowthStrategy::INFLATION = 1
     *   GrowthStrategy::CUSTOM = 2
     * @throws std::invalid_argument if periodType is unknown.
     * @param gs GrowthStrategy class enum value to convert.
     * @return The equivalent int.
     */
    static int growthStrategyToInt(GrowthStrategy gs);

    /**
     * @brief Convert an int to a GrowthStrategy class enum value.
     * @param value The int to convert.
     * @param convertedValue Result of the conversion. Valid only if conversion succeeded.
     * @return True if the conversion succeeded, false otherwise (int value is invalid).
     */
    static bool intToGrowthStrategy(int value, GrowthStrategy& convertedValue);



};



#endif // PERIODICCSD_H
