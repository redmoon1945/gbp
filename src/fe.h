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

#ifndef FE_H
#define FE_H

#include <QtGlobal>
#include <QWeakPointer>
#include <QDate>
#include <QUuid>
#include "csd.h"
#include "currencyhelper.h"


/**
 * @struct Fe
 * @brief Represent one Financial Event (FE), which is a dated amount.
 * @details A Financial Event is the fundamental unit in GBP representing money flow at a
 * specific date. FEs are generated from Cash Stream Definitions (CSDs) and collected into
 * streams for analysis and visualization. Each FE contains an amount (positive for income,
 * negative for expenses) and a reference to the CSD that generated it.
 *
 * Size: 16 bytes (optimized for performance in large collections).
 *
 * @see Csd Base class that generates Financial Events
 * @see PeriodicCsd Generates FEs on regular schedule
 * @see IrregularCsd Generates FEs from irregular occurrences
 * @see FeStream Container for collections of FEs
 * @see Growth Pattern used to adjust FE amounts over time
 */
struct Fe{
    /**
     * @brief Total and final amount for a specific day.  Negative number if expense.
     * In currency unit.
     */
    double amount;
    /**
     * @brief Reference to the Csd having generated this Fe.
     * DO NOT FREE DIRECTLY THIS POINTER.
     * @details Before we used the Csd ID. This had the inconvenent of long repetitive search
     * in QMap. Replacing it with a direct pointer to the CSD speed up things enormously. As for
     * making sure the CSD still exists and we dont get a pointer to garbage, this is guaranteed
     * by the fact that if a CSD is removed, all data will be rebuilt, which implies
     * recreating all Fe objects.
     */
    QWeakPointer<Csd> csdPtr;

    /**
     * @brief operator ==
     * @details For the amount , a "loose" comparison is performed. 2 double are declared equal if
     * the difference is less than the smallest unit of all the currency available (3 decimals + 1
     * spare for rounding). For the reference to Csd, the 2 references are deemped equal if they
     * point to the same Csd (i.e. same ID).
     * @param o The object to compare to.
     * @return True if both Fe are equal, false otherwise.
     */
    bool operator==(const Fe& o) const;

    /**
     * @brief Inverse of  ==
     * @param o The object to compare to.
     * @return False if both Fe are equal, true otherwise.
     */
    bool operator!=(const Fe& o) const;

    /**
     * @brief operator =
     * @param o The object to assign to.
     * @return This object.
     */
    Fe &operator=(const Fe &o) ;

    // For display purpose, in the list box of the Daily Info of the Main Window.
    QString toString(const QString &streamDefName, const CurrencyInfo& currInfo,
        const QLocale& locale) const;
};


/**
 * @struct FeMinMaxInfo
 * @brief Structure to hold min/max absolute y values for financial event stream visualization.
 * @details Used to track the range of financial event amounts for chart scaling and display
 * purposes. Values are always non-negative (absolute values).
 */
struct FeMinMaxInfo{
    quint64 yMin;  ///< Minimum absolute value in the financial event stream
    quint64 yMax;  ///< Maximum absolute value in the financial event stream
};



#endif // FE_H
