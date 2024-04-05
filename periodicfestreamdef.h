/*
 *  Copyright (C) 2024 Claude Dumas <claudedumas63@protonmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PERIODICFESTREAMDEF_H
#define PERIODICFESTREAMDEF_H

#include <QCoreApplication>
#include "festreamdef.h"
#include "util.h"

// Definition of a Fe stream based on a single amount generated at a specific interval. At the beginning of each year on Jan 1,
// the amount may changed due to optional "growth" pattern (defined in this Stream Definition) or optional inflation (defined outside).
// It is one of the other.
class PeriodicFeStreamDef : public FeStreamDef
{
    Q_DECLARE_TR_FUNCTIONS(PeriodicFeStreamDef)


public:

    enum PeriodType {DAILY=0, WEEKLY=1,MONTHLY=2,END_OF_MONTHLY=3,YEARLY=4};
    enum GrowthStrategy {NONE=0, INFLATION=1, CUSTOM=2};
    static const int PERIOD_MULTIPLIER_MAX = 365*100;   // 100 years of daily occurences
    static const int PERIOD_MULTIPLIER_MIN = 1;
    static const int GROWTH_APP_PERIOD_MAX = 100*12;    // once every 100 years
    static const int GROWTH_APP_PERIOD_MIN = 1;

    // constructors and destructor
    PeriodicFeStreamDef();    // required for qmap.value default
    PeriodicFeStreamDef(const PeriodicFeStreamDef& o);
    PeriodicFeStreamDef(PeriodType periodicType, quint16 periodMultiplier, qint64 amount, const Growth &growth, GrowthStrategy &growthStrategy,
                        quint16 &growthApplicationPeriod, const QUuid &id, const QString &name, const QString &desc, bool active,
                        bool isIncome, const DateRange &validityRange);
    virtual ~PeriodicFeStreamDef();

    // operators
    PeriodicFeStreamDef& operator=(const PeriodicFeStreamDef& o);
    bool operator==(const PeriodicFeStreamDef& o) const;

    // methods
    QList<Fe> generateEventStream(DateRange fromto, const Growth &inflation, uint &saturationCount) const;
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

private:

    // Variables
    PeriodType period;
    quint16 periodMultiplier;   // multiplier of the period ( e.g. 2 means 1 Fe per 2 weeks). Value must be in [PERIOD_MULTIPLIER_MIN,PERIOD_MULTIPLIER_MAX]
    qint64 amount;              // in smallest currency unit ! Always a non negative number even if this is an expense
    Growth growth;              // Custom growth for an instance of PeriodicFeStreamDef, used when growStrategy is CUSTOM
    GrowthStrategy growthStrategy;
    quint16 growthApplicationPeriod;    // apply growth every "growthApplicationPeriod" occurence of amount. Value must be in [GROWTH_APP_PERIOD_MIN,GROWTH_APP_PERIOD_MAX ]
    DateRange validityRange;

    // methods
    QDate getNextEventDate(QDate date) const;
    Util::PeriodType convertPeriodTypeToUtil(PeriodicFeStreamDef::PeriodType periodType) const;

};

#endif // PERIODICFESTREAMDEF_H
