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

#ifndef IRREGULARFESTREAMDEF_H
#define IRREGULARFESTREAMDEF_H

#include "festreamdef.h"
#include "currencyhelper.h"
#include <quuid.h>
#include <QCoreApplication>



// Definition of a Fe stream based on a set of amounts defined over infinite year range.
// Each amount occurs once and is not repeated. Amount can be negative.
// There is no growth concept applicable, nor inflation adjustment : amounts cannot
// be changed this way, since there are presumed to take this already into account. However,
// amounts must always be considered as future values and conversion to present value is
// available if requested.
class IrregularFeStreamDef : public FeStreamDef
{

    // this is to be able to use the "tr" translation function
    Q_DECLARE_TR_FUNCTIONS(IrregularFeStreamDef)

public:

    // for Irregular Fe Stream Def
    struct AmountInfo{
        qint64 amount;      // CANNOT be negative (see argument "bool isIncome" in contructor)
        QString notes;      // length limited

        static int NOTES_MAX_LEN;
        bool operator==(const AmountInfo &o) const;
        QJsonObject toJson() const;
        static AmountInfo fromJson(const QJsonObject& jsonObject, Util::OperationResult &result);
    };

    // constructors and destructor
    IrregularFeStreamDef();     // used only for some QMap operations
    IrregularFeStreamDef(const IrregularFeStreamDef& o);
    IrregularFeStreamDef(const QMap<QDate, AmountInfo> &amountSet);
    IrregularFeStreamDef(QMap<QDate,AmountInfo> amountSet, const QUuid &id, const QString &name, const QString &desc, bool active, bool isIncome, const QColor& decorationColor);
    virtual ~IrregularFeStreamDef();

    // operators
    IrregularFeStreamDef& operator=(const IrregularFeStreamDef &o);
    bool operator==(const IrregularFeStreamDef &o) const;

    // methods
    QList<Fe> generateEventStream(DateRange fromto,  double pvAnnualDiscountRate, QDate pvPresent, uint &saturationCount) const;
    QString toStringForDisplay(CurrencyInfo currInfo, QLocale locale) const;
    QJsonObject toJson() const;
    static IrregularFeStreamDef fromJson(const QJsonObject& jsonObject, Util::OperationResult &result);
    IrregularFeStreamDef duplicate() const;

    // getters and setters
    QMap<QDate, AmountInfo> getAmountSet() const;

private:
    QMap<QDate,AmountInfo> amountSet;   // key: valid QDate, (29 feb is allowed)  value : amount info = amount and associated note

    struct validateKeysResult{
        bool valid;
        QString reasonUI;
        QString reasonLog;
    };
    static validateKeysResult validateKeysAndValues(const QMap<QDate,AmountInfo> set);
};

#endif // IRREGULARFESTREAMDEF_H
