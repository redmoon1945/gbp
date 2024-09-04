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

#ifndef PERIODICFESTREAMDEF_H
#define PERIODICFESTREAMDEF_H

#include <QCoreApplication>
#include "festreamdef.h"
#include "util.h"

// Definition of a Fe stream based on a single amount generated at a specific interval.
// The amount may changed due to optional "growth" pattern (defined in this Stream Definition) or optional inflation (defined outside).
// It is one of the other.
class PeriodicFeStreamDef : public FeStreamDef
{
    Q_DECLARE_TR_FUNCTIONS(PeriodicFeStreamDef)


public:

    enum PeriodType {DAILY=0, WEEKLY=1,MONTHLY=2,END_OF_MONTHLY=3,YEARLY=4};
    enum GrowthStrategy {NONE=0, INFLATION=1, CUSTOM=2};
    static const int PERIOD_MULTIPLIER_MAX;
    static const int PERIOD_MULTIPLIER_MIN;
    static const int GROWTH_APP_PERIOD_MAX;
    static const int GROWTH_APP_PERIOD_MIN;
    static const double MAX_INFLATION_ADJUSTMENT_FACTOR;


    // constructors and destructor
    PeriodicFeStreamDef();    // required for qmap.value default
    PeriodicFeStreamDef(const PeriodicFeStreamDef& o);
    PeriodicFeStreamDef(PeriodType periodicType, quint16 periodMultiplier, qint64 amount, const Growth &growth, const GrowthStrategy &growthStrategy,
                        quint16 growthApplicationPeriod, const QUuid &id, const QString &name, const QString &desc, bool active,
                        bool isIncome, const QColor& decorationColor, const DateRange &validityRange, double inflationAdjustmentFactor);
    virtual ~PeriodicFeStreamDef();

    // operators
    PeriodicFeStreamDef& operator=(const PeriodicFeStreamDef& o);
    bool operator==(const PeriodicFeStreamDef& o) const;

    // methods
    QList<Fe> generateEventStream(DateRange fromto, const Growth &inflation, double pvDiscountRate, QDate pvPresent, uint &saturationCount) const;
    QString toStringForDisplay(CurrencyInfo currInfo, QLocale locale) const;
    QJsonObject toJson() const;
    static PeriodicFeStreamDef fromJson(const QJsonObject& jsonObject, Util::OperationResult &result);
    PeriodicFeStreamDef duplicate() const ; // copy of this object with a different ID

    // Getters and setters
    PeriodType getPeriod() const;
    quint16 getPeriodMultiplier() const;
    qint64 getAmount() const;
    Growth getGrowth() const;
    GrowthStrategy getGrowthStrategy() const;
    quint16 getGrowthApplicationPeriod() const;
    DateRange getValidityRange() const;
    double getInflationAdjustmentFactor() const;

private:

    // Variables
    PeriodType period;
    quint16 periodMultiplier;   // multiplier of the period ( e.g. 2 means 1 Fe per 2 weeks). Value must be in [PERIOD_MULTIPLIER_MIN,PERIOD_MULTIPLIER_MAX]
    qint64 amount;              // in smallest currency unit ! Always a non negative number even if this is an expense
    Growth growth;              // Custom growth for an instance of PeriodicFeStreamDef, used when growStrategy is CUSTOM
    GrowthStrategy growthStrategy;
    quint16 growthApplicationPeriod;    // apply growth every "growthApplicationPeriod" occurence of amount. Value must be in [GROWTH_APP_PERIOD_MIN,GROWTH_APP_PERIOD_MAX ]
    DateRange validityRange;
    double inflationAdjustmentFactor;   // if not 1, change the value of scenario inflation applied as a growth to this element
                                        // (each growth value is multiplied by this factor). Cannot be negative. Max = MAX_INFLATION_ADJUSTMENT_FACTOR

    // methods
    QDate getNextEventDate(QDate date) const;
    Util::PeriodType convertPeriodTypeToUtil(PeriodicFeStreamDef::PeriodType periodType) const;

};

#endif // PERIODICFESTREAMDEF_H
