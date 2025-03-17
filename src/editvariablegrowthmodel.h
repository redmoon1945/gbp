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

#ifndef EDITVARIABLEGROWTHMODEL_H
#define EDITVARIABLEGROWTHMODEL_H

#include <QAbstractTableModel>
#include <QLocale>
#include "growth.h"


QT_BEGIN_NAMESPACE
namespace Ui { class EditVariableGrowthModel; }
QT_END_NAMESPACE

class EditVariableGrowthModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit EditVariableGrowthModel(QString newGrowthName, QLocale aLocale, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // methods
    int getPositionForDate(QDate aDate);

    // Getters/setters
    Growth getGrowthData() const;
    void setGrowthData(const Growth &newGrowthData);
    QString getGrowthName() const;
    void setGrowthName(const QString &newGrowthName);
    QLocale getTheLocale() const;
    void setTheLocale(const QLocale &newTheLocale);

private:
    Growth growthData; //  variable Growth (monthly growth, expresssed on an annual basis)
    QString growthName;
    QLocale theLocale; // for formatting percentage and date


};

#endif // EDITVARIABLEGROWTHMODEL_H
