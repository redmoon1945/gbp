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

#ifndef EDITIRREGULARMODEL_H
#define EDITIRREGULARMODEL_H

#include <QAbstractTableModel>
#include <QLocale>
#include <QMap>
#include <QFont>
#include <QUuid>
#include "irregularfestreamdef.h"
#include "currencyhelper.h"
#include "visualizeoccurrencesdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class EditIrregularModel; }
QT_END_NAMESPACE


class EditIrregularModel: public QAbstractTableModel
{
    Q_OBJECT


public:
    explicit EditIrregularModel(QLocale aLocale, QObject *parent = nullptr);

    // Methods to override
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // methods
    int getPositionForDate(QDate date);

    // Getters / Setters
    QMap<QDate, IrregularFeStreamDef::AmountInfo> getItems() const;
    void setItems(const QMap<QDate, IrregularFeStreamDef::AmountInfo> &newItems);
    QLocale getTheLocale() const;
    void setTheLocale(const QLocale &newTheLocale);
    CurrencyInfo getCurrInfo() const;
    void setCurrInfo(const CurrencyInfo &newCurrInfo);
    QFont getDefaultTableFont() const;
    void setDefaultTableFont(const QFont &newDefaultTableFont);
    QFont getMonoTableFont() const;
    void setMonoTableFont(const QFont &newMonoTableFont);

private:
    // Model Data - key order corresponds to display order
    QMap<QDate, IrregularFeStreamDef::AmountInfo> items;

    // misc variables
    QLocale theLocale;
    CurrencyInfo currInfo;  // must be set by setter before model is used
    QFont defaultTableFont;
    QFont monoTableFont;
};

#endif // EDITIRREGULARMODEL_H
