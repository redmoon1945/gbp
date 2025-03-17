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

#ifndef FESTREAMDEF_H
#define FESTREAMDEF_H
#include <QDate>
#include <QUuid>
#include <QJsonObject>
#include <QCoreApplication>
#include "util.h"



// Abstract base class for all Cash Stream Definitions (CSDs). It was called previously
// "Financial Event Stream Definition", hence the name of the class.
// Each one has a unique id (UUID, independant of the name).
class FeStreamDef
{
    Q_DECLARE_TR_FUNCTIONS(FeStreamDef)

public:
    static int NAME_MAX_LEN;    // max length of the name
    static int DESC_MAX_LEN;    // max length of the description


    // PERIODIC : unique amount that repeats over a user-defined period, with optional yearly growth factor and yearly inflation
    // IRREGULAR : set of couples (date,amount), not repeated over any period, no growth factor, does not take account inflation
    enum FeStreamType {PERIODIC, IRREGULAR};

    // constructors and destructor
    FeStreamDef();  // required for default value in some QMap operations
    FeStreamDef(const FeStreamDef& o);
    FeStreamDef(const QUuid &id, const QString &name, const QString &desc, FeStreamType streamType, bool active, bool isIncome, const QColor& decorationColor);
    virtual ~FeStreamDef();

    // operators
    FeStreamDef& operator=(const FeStreamDef& o);
    bool operator==(const FeStreamDef& o) const;
    bool operator!=(const FeStreamDef& o) const;

    // methods
    void toJson(QJsonObject &jsonObject) const;
    bool evaluateIfSameFeList(const FeStreamDef& o) const;
    static void fromJson(const QJsonObject &jsonObject, FeStreamType expectedStreamType, QUuid &id, QString &name, QString &desc,
                         bool &active, bool &isIncome, QColor& decorationColor, Util::OperationResult &result); // we cant build a base class, so we return instead the components

    // Getters/setters
    QUuid getId() const;
    void setId(const QUuid &newId);
    QString getName() const;
    void setName(const QString &newName);
    QString getDesc() const;
    void setDesc(const QString &newDesc);
    FeStreamType getStreamType() const;
    void setStreamType(FeStreamType newStreamType);
    bool getActive() const;
    void setActive(bool newActive);
    bool getIsIncome() const;
    void setIsIncome(bool newIsIncome);
    QColor getDecorationColor() const;
    void setDecorationColor(const QColor &newDecorationColor);


protected:
    QUuid id;                   // this is the unique key of this FeStreamDef, but created by the parent class
    QString name;
    QString desc;
    FeStreamType streamType;    // type of CSD
    bool active;                // do not generate any Financial Events if set to False
    bool isIncome;
    QColor decorationColor; // Color of the name, for display purpose. Help to classify and
                            // identify CSD. If invalid, then use system's default

};

#endif // FESTREAMDEF_H
