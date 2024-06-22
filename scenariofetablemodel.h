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

#ifndef SCENARIOFETABLEMODEL_H
#define SCENARIOFETABLEMODEL_H

#include <QAbstractTableModel>
#include <QMap>
#include <QUuid>
#include <QLocale>
#include <QCoreApplication>

#include <QFont>
#include "irregularfestreamdef.h"
#include "periodicfestreamdef.h"
#include "currencyhelper.h"

class ScenarioFeTableModel : public QAbstractTableModel
{

    Q_DECLARE_TR_FUNCTIONS(ScenarioFeTableModel)

public:
    explicit ScenarioFeTableModel(QLocale aLocale, QFont activeTableFont, QFont inactiveTableFont, QFont amountActiveTableFont,
                                  QFont amountInactiveTableFont, QFont infoActiveTableFont, QFont infoInactiveTableFont, bool allowDecorationColor,
                                  QObject *parent = nullptr);
    ~ScenarioFeTableModel();

    // model's methods to implement as subclass of QAbstractListModel
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;


    // methods
    void newScenario(CurrencyInfo cInfo, QMap<QUuid,PeriodicFeStreamDef> newIncomesDefPeriodic, QMap<QUuid,IrregularFeStreamDef> newIncomesDefIrregular,
                     QMap<QUuid,PeriodicFeStreamDef> newExpensesDefPeriodic, QMap<QUuid,IrregularFeStreamDef> newExpensesDefIrregular);
    void addModifyPeriodicItem(PeriodicFeStreamDef p);
    void addModifyIrregularItem(IrregularFeStreamDef p);
    void removeItems(QList<QUuid> toRemove);
    QUuid duplicateItem(QUuid id, bool &found);
    void changeActiveStatusItems( QList<QUuid> idList, bool enable);
    int getRow(QUuid id, bool &found);
    QUuid getId(int row, bool &found);
    FeStreamDef::FeStreamType getTypeOfFeStreamDef(QUuid id, bool &found);
    PeriodicFeStreamDef getPeriodicFeStreamDef(QUuid id, bool &found);
    IrregularFeStreamDef getIrregularFeStreamDef(QUuid id, bool &found);
    void filtersChanged(bool showIncome, bool showPeriodic, bool showIrregular, bool showActive, bool showInactive);
    int getNoItems();

    // Getters / Setters
    CurrencyInfo getCurrInfo() const;
    void setCurrInfo(const CurrencyInfo &newCurrInfo);
    QMap<QUuid, PeriodicFeStreamDef> getIncomesDefPeriodic() const;
    QMap<QUuid, IrregularFeStreamDef> getIncomesDefIrregular() const;
    QMap<QUuid, PeriodicFeStreamDef> getExpensesDefPeriodic() const;
    QMap<QUuid, IrregularFeStreamDef> getExpensesDefIrregular() const;
    bool getAllowDecorationColor() const;
    void setAllowDecorationColor(bool newAllowDecorationColor);

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
        QColor decorationColor;
    };
    // some misc variables
    QLocale theLocale;
    CurrencyInfo currInfo;
    QFont activeTableFont;          // Active non amount
    QFont inactiveTableFont;        // Inactive non amount
    QFont amountActiveTableFont;    // Active Amount
    QFont amountInactiveTableFont;  // Inactive amount
    QFont infoActiveFont;           // Info active
    QFont infoInactiveFont;         // Info inactive
    bool allowDecorationColor;

    // real data (raw)
    QMap<QUuid,PeriodicFeStreamDef> incomesDefPeriodic;         // key is Stream Def ID
    QMap<QUuid,IrregularFeStreamDef> incomesDefIrregular;       // key is Stream Def ID
    QMap<QUuid,PeriodicFeStreamDef> expensesDefPeriodic;        // key is Stream Def ID
    QMap<QUuid,IrregularFeStreamDef> expensesDefIrregular;      // key is Stream Def ID

    // conveniance structures to display data fast in the table. All the same items for similar index.
    // Already sorted (by name, decapitalized)
    QList<ItemInfo> itemsList;

    // filters
    bool displayPeriodicItems;
    bool displayIrregularItems;
    bool displayActiveItems;
    bool displayInactiveItems;
    bool displayIncomes;
    bool displayExpenses;

    // methods
    void rebuildLists();
};

#endif // SCENARIOFETABLEMODEL_H
