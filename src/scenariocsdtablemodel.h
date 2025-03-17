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

#ifndef SCENARIOCSDTABLEMODEL_H
#define SCENARIOCSDTABLEMODEL_H

#include <QAbstractTableModel>
#include <QMap>
#include <QUuid>
#include <QLocale>
#include <QCoreApplication>
#include <QFont>
#include "irregularfestreamdef.h"
#include "periodicfestreamdef.h"
#include "currencyhelper.h"
#include "tagcsdrelationships.h"
#include "tags.h"
#include "filtertags.h"


// In EditScenario Dialog, Table model to display all the Csds (Cash Stream Definitions)
// of the scenario. 3 things will lead to the content of the table to be updated :
// 1) change in the set of Csds
// 2) change in the filters (including filter tags)
// 3) change in the tags and/or relationships
// The table keeps no internal copy of the data. Instead, it has its own simplified list of QString
// (built from the data) used only to fill the content of the table.
class ScenarioCsdTableModel : public QAbstractTableModel
{

    Q_DECLARE_TR_FUNCTIONS(ScenarioFeTableModel)

public:

    explicit ScenarioCsdTableModel(QLocale aLocale, QFont activeTableFont, QFont inactiveTableFont,
        QFont amountActiveTableFont, QFont amountInactiveTableFont, QFont infoActiveTableFont,
        QFont infoInactiveTableFont, bool allowColoredCsdNames, QObject *parent = nullptr);
    ~ScenarioCsdTableModel();

    // model's methods to implement as subclass of QAbstractListModel
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;


    // *** methods ***

    // Send all the data required to rebuild internal data and completey refresh the table's content
    void refresh(CurrencyInfo curInfo, QMap<QUuid,PeriodicFeStreamDef> incomesDefPeriodic,
        QMap<QUuid,IrregularFeStreamDef> incomesDefIrregular, QMap<QUuid,PeriodicFeStreamDef>
        expensesDefPeriodic, QMap<QUuid,IrregularFeStreamDef> expensesDefIrregular, Tags tags,
        TagCsdRelationships tagsRelationships, bool showIncomes, bool showExpenses,
        bool showPeriodic, bool showIrregular, bool showActive, bool showInactive,
        FilterTags filterTags);

    // Misc querries
    int getRow(QUuid id, bool &found);
    QUuid getId(int row, bool &found);
    int getNoItems();

    // Getters / Setters
    bool getAllowColoredCsdNames() const;
    void setAllowColoredCsdNames(bool newAllowColorizationOfCsdNames);

private:

    // compact structure holding all the info required to quickly fill the table
    struct ItemInfo
    {
        QString name;
        QString type;
        QString amount;
        QString info;
        bool isActive;
        QUuid id;
        QColor csdNameColor;
    };

    // some misc variables
    QLocale theLocale;
    QFont activeTableFont;          // Active non amount
    QFont inactiveTableFont;        // Inactive non amount
    QFont amountActiveTableFont;    // Active Amount
    QFont amountInactiveTableFont;  // Inactive amount
    QFont infoActiveFont;           // Info active
    QFont infoInactiveFont;         // Info inactive
    bool allowColoredCsdNames;

    // conveniance structures to display data fast in the table. All the same items for similar
    // index. Already sorted (by name, decapitalized)
    QList<ItemInfo> itemsList;

    // methods
    bool isItemPassTagFilter(QUuid csdId, TagCsdRelationships tagCsdRelationships,
        FilterTags filterTags);
};

#endif // SCENARIOCSDTABLEMODEL_H
