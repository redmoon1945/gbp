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

#ifndef DATERANGE_H
#define DATERANGE_H
#include <QDate>
#include <QJsonObject>
#include <QCoreApplication>
#include "util.h"

/**
 * @class DateRange
 * @brief Represents a date interval with a specific type: BOUNDED, INFINITE, or EMPTY.
 * @details An object of this class is immutable. A DateRange can represent:
 * - BOUNDED: A finite range with inclusive start and end dates, where start <= end.
 * - INFINITE: A range from negative infinity to positive infinity.
 * - EMPTY: A range containing no dates.
 * The range is limited to a maximum of 1000 years for BOUNDED ranges to ensure practical usage
 * in the context of the application. Dates before year 1000 AD are not allowed for BOUNDED ranges
 * due to a bug in QDate::toString(ISO_DATE).
 * The class is designed to be immutable after construction.
 */
class DateRange
{
    Q_DECLARE_TR_FUNCTIONS(DateRange)

public:

    /// @brief Strongly typed enum defining the type of date range.
    enum class Type { EMPTY,BOUNDED,INFINITE};

    /// @brief Maximum number of years a BOUNDED DateRange can span.
    static const long MAX_YEARS = 1000;

    /// @brief Minimum year allowed for BOUNDED DateRange.
    static const long MIN_YEAR = 1000;

    // *** Constructors and destructor ***

    /**
     * @brief Constructs an EMPTY DateRange.
     * @details The start and end dates are set to invalid QDate objects (QDate()) and
     * should not be used.
     */
    DateRange();

    /**
     * @brief Constructs a BOUNDED DateRange with specified start and end dates.
     * @param from The start date (inclusive, year >= 1000 CE).
     * @param to The end date (inclusive, year >= 1000 CE).
     * @throws std::invalid_argument if dates are invalid, start > end, range exceeds MAX_YEARS,
     * or year < MIN_YEAR.
     */
    DateRange( const QDate from, const QDate to );

    /**
     * @brief Copy constructor.
     * @param o The DateRange to copy.
     */
    DateRange(const DateRange & o);

    /**
     * @brief Constructs a DateRange of the specified type.
     * @details For INFINITE or EMPTY types, start and end dates are set to invalid QDate objects
     * (QDate()) and should not be used. For BOUNDED ranges, use the constructor with start and
     * end dates.
     * @param r The type of the DateRange (EMPTY, BOUNDED, or INFINITE).
     * @throws std::invalid_argument If type is BOUNDED (use the from/to constructor instead).
     */
    DateRange(Type r);

    virtual ~DateRange();

    // *** operators ***

    /**
     * @brief Assignment operator.
     * @param o The DateRange object to assign from.
     * @return Reference to this DateRange object.
     */
    DateRange& operator=(const DateRange&o) ;

    /**
     * @brief Equality operator.
     * @param o The DateRange to compare with.
     * @return True if the DateRanges are equal, false otherwise.
     *
     * Two DateRanges are equal if:
     * - Both are INFINITE.
     * - Both are EMPTY (regardless of start/end dates)
     * - Both are BOUNDED with identical start and end dates.
     */
    bool operator==(const DateRange & o) const;

    /**
     * @brief Inequality operator.
     * @param o The DateRange to compare with.
     * @return True if the DateRanges are not equal, false otherwise.
     */
    bool operator!=(const DateRange & o) const;


    // *** methods ***

    /**
     * @brief Calculates the number of years spanned by the DateRange.
     * @return The number of years spanned (inclusive).
     * @throws std::out_of_range If the DateRange is INFINITE.
     */
    int GetNoOfYearsSpanned() const;

    /**
     * @brief Checks if this DateRange intersects with another.
     * @param o The other DateRange.
     * @return True if the ranges intersect, false otherwise.
     * @details Intersection rules:
     * - INFINITE intersects with INFINITE or BOUNDED.
     * - BOUNDED intersects with INFINITE or another BOUNDED range if their dates overlap.
     * - EMPTY does not intersect with any range.
     */
    bool intersectWith(const DateRange o) const;

    /**
     * @brief Computes the intersection of this DateRange with another.
     * @param o The other DateRange.
     * @return A new DateRange representing the intersection.
     * @details Returns an EMPTY DateRange if there is no intersection.
     */
    DateRange intersection(const DateRange o) const;

    /**
     * @brief Checks if a date is included in this DateRange. If "o" is invalid, "false"
     * is returned.
     * @details For INFINITE ranges, all valid dates are included. For EMPTY ranges, no dates are
     * included.
     * @param o The date to check.
     * @return True if the date is within the range, false otherwise.
     */
    bool includeDate(const QDate o) const;

    /**
     * @brief Gets a list of all dates in the DateRange.
     * @return A QList of QDate objects representing all dates in the range.
     * @throws std::out_of_range If the DateRange is INFINITE.
     * @details For EMPTY ranges, returns an empty list. For BOUNDED ranges, returns all dates
     * from start to end (inclusive).
     */
    QList<QDate> getDateList() const;

    /**
     * @brief Converts the DateRange to a string representation.
     * @return A QString describing the DateRange.
     * @details Returns "Empty" for EMPTY ranges, "Infinite" for INFINITE ranges, or
     * "[YYYY-MM-DD,YYYY-MM-DD]" for BOUNDED ranges.
     */
    QString toString() const;

    /**
     * @brief Serializes the DateRange to a JSON object.
     * @return A QJsonObject containing the start, end, and type.
     * @details For INFINITE and EMPTY ranges, start and end fields contain
     * "1000-01-01" and "1000-12-31" respectively.
     */
    QJsonObject toJson() const;

    /**
     * @brief Deserializes a DateRange from a JSON object.
     * @param jsonObject The JSON object to parse.
     * @param result The operation result indicating success or failure.
     * @return A DateRange object constructed from the JSON data.
     * @details Expects valid ISO dates for start and end; for BOUNDED, ensures start <= end.
     */
    static DateRange fromJson(const QJsonObject& o, Util::ResultOfOperation &result);

    // *** getters ***

    /**
     * @brief Gets the start date of the DateRange.
     * @return The start date (only meaningful for BOUNDED ranges).
     */
    QDate getStart() const;

    /**
     * @brief Gets the end date of the DateRange.
     * @return The end date (only meaningful for BOUNDED ranges).
     */
    QDate getEnd() const;

    /**
     * @brief Gets the type of the DateRange.
     * @return The type (EMPTY, BOUNDED, or INFINITE).
     */
    Type getType() const;
    /**
     * @brief Return no of days in the interval, including start and end date. 0 if
     * this object id of type is NOT "BOUND.". This field is NOT serialized in JSon object.
     */
    uint getNoOfDays() const;

    /**
     * @brief Return the index of a date in an hypothetical array with index 0 corresponding
     * to "start" date and having "noOfDays" elements (one per day). Type of this object must be
     * BOUND, otherwise 0 is returned.
     * @return The index if type is BOUND, 0 otherwise.
     */
    uint getDayIndex(const QDate& date) const;

private:

    /**
     * @brief Start date (inclusive, only meaningful for BOUNDED ranges, but always valid).
     */
    QDate start;
    /**
     * @brief End date (inclusive, only meaningful for BOUNDED ranges, but always valid).
     */
    QDate end;
    /**
     * @brief Type of the DateRange (EMPTY, BOUNDED, or INFINITE).
     */
    Type type;
    /**
     * @brief No of days in the interval, including start and end date. If type is not
     * BOUND, value is 0. This field is NOT streamed to JSon serialization object.
     */
    uint noOfDays;

    // *** methods ***

    /**
     * @brief Convert Enum "Type" to int value, for JSon streaming.
     * @details For compatibility with previous GBP version (1.6.3 and below), int value must be :
     * EMPTY = 0
     * BOUNDED = 1
     * INFINITE = 2
     * @return The converted int value.
     */
    static int convertTypeFromEnumToInt(DateRange::Type t);

    /**
     * @brief Convert an int value to enum Type.
     * @param value the value to convert.
     * @param success The result of conversion : true if successful, false otherwise.
     * @return The convert enum Type value.
     */
    static DateRange::Type convertTypeFromIntToEnum(int value, bool& success);

};

#endif // DATERANGE_H
