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

#ifndef COMBINEDFESTREAMS_H
#define COMBINEDFESTREAMS_H
#include <QDate>
#include <QCoreApplication>
#include <QMap>
#include "fe.h"
#include "currencyhelper.h"
#include "festream.h"


/**
 * @class CombinedFeStreams
 * @brief Aggregator class intended to merge several FeStream produced by a set of Csd.
 * @details All FeStream must have the same no of days (i.e. size). The cumulative amount for a
 * given day is a double and is NOT capped. It means that if several CSD with amount near saturation
 * are added, then the cumulative amount for that day may loose some precision.
 */
class CombinedFeStreams
{
    // this is to be able to use the "tr" translation function
    Q_DECLARE_TR_FUNCTIONS(CombinedFeStreams)

public:


    /**
     * @struct DailyInfo
     * @brief Pack all daily occurrence information tied to a specific date. In currency unit.
     * @details If totalIncomes is negative, it means this struct is invalid. This is used to :
     * to set a default value in a "value" method Map call, so that if the key (date) is not found,
     * we know. This prevent doing first a "contains" then a "value" call, which cut the processing
     * time in half
     */
    struct DailyInfo{
        /**
         * @brief True if this entry is used, False otherwise.
         */
        bool used;
        /**
         * @brief Sum of all incomes occurring during that day. An income is always >= 0.
         * If = -1, it means this entry is invalid.
         */
        double totalIncomes;
        /**
         * @brief Sum of all expenses occurring during that day. An expense is always <= 0.
         */
        double totalExpenses;
        /**
         * @brief List of all incomes occurring during that day. Only one contribution per Csd.
         */
        QList<Fe> incomesList;
        /**
         * @brief List of all expenses occurring during that day. Only one contribution per Csd.
         */
        QList<Fe> expensesList;

        bool operator==(const DailyInfo& o) const;
        DailyInfo& operator=(const DailyInfo& o);

        /**
         * @brief Provide a String representation of this object. For test purpose only.
         * @return QString representation.
         */
        QString toString() const;
        };

    // *** Constructors and destructor ***

    /**
     * @brief Default constructor cannot be used.
     */
    CombinedFeStreams() = delete;

    /**
     * @brief CombinedFeStreams
     * @param noOfDays No of days of Financial events. Must be <= FeStream::MAX_DAYS.
     * @throw std::invalid_argument if noOfDays = 0.
     * @throw std::invalid_argument if noOfDays > MAX_DAYS.
     */
    CombinedFeStreams(quint32 noOfDays);

    /**
     * @brief Copy Constructor.
     * @param o The CombinedFeStreams object to copy.
     */
    CombinedFeStreams(const CombinedFeStreams& o);

    /**
     * @brief Virtual destructor.
     */
    virtual ~CombinedFeStreams();

    // *** operators ***

    /**
     * @brief operator ==
     * @param o The CombinedFeStreams object to compare against.
     * @return
     */
    bool operator==(const CombinedFeStreams &o) const;

    /**
     * @brief operator =
     * @param o The CombinedFeStreams object to assign from.
     * @return
     */
    CombinedFeStreams& operator=(const CombinedFeStreams &o);

    // *** Methods ***

    /**
     * @brief Add a stream of Financial Events from a particular CSD, merging data with the current
     * combinedStreams. theStream must have the same number of days than internal "combinedStreams".
     * @details The Csd referenced in theStream must still exist, otherwise no action is performed.
     * If this object already contains contribution from this CSD, the method returns without
     * performing any changes.
     * @param theStream The FeStream to add.
     * @param currInfo Currency information.
     * @throw std::invalid_argument noOfDays in the FeStream is different from the one in this
     * combinedStreams object.
     */
    void addStream(const FeStream theStream, CurrencyInfo currInfo);

    // getters
    QList<CombinedFeStreams::DailyInfo> getCombinedStreams() const;
    quint32 getNoOfDays() const;
    quint32 getNoOfElementsUsed() const;
    QSet<QUuid> getCsdContributors() const;
    quint32 getNoOfFe() const;

private:

    /**
     * @brief Hold the merged list of FeStream. The size must be the same as all the FeStream
     * that will be added. Index 0 is for "tomorrow", index 1 for the day after, etc.
     * @details This array is big. 100 years means roughly 36600 days. If every day has 20
     * events, it grows roughly to 15 MB.
     */
    QList<DailyInfo> combinedStreams;

    /**
     * @brief No of days (a.k.a. "entries") any FE stream have.  First day is always "tomorrow".
     */
    quint32 noOfDays;

    /**
     * @brief No of elements in use in the "combinedStreams" array. Corresponds to the number
     * of eventful days.
     */
    quint32 noOfElementsUsed;

    /**
     * @brief Total no of Financial Events in the "combinedStreams" array.
     */
    quint32 noOfFe;

    /**
     * @brief List of CSD having contributed to this CombinedFeStreams via addStream().
     * @details QSet is used to guarantee uniqueness. A Csd can contribute just once.
     */
    QSet<QUuid> csdContributors;

};

#endif // COMBINEDFESTREAMS_H
