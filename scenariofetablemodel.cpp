/*
 *  Copyright (C) 2024 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
 *  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
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

#include "scenariofetablemodel.h"


ScenarioFeTableModel::ScenarioFeTableModel(QLocale aLocale, QFont activeTableFont, QFont inactiveTableFont, QFont amountActiveTableFont,
                                           QFont amountInactiveTableFont, QFont infoActiveTableFont, QFont infoInactiveTableFont, bool allowDecorationColor,
                                           QObject *parent)
    : QAbstractTableModel{parent}
{
    this->theLocale = aLocale;
    this->activeTableFont = activeTableFont;
    this->inactiveTableFont = inactiveTableFont;
    this->amountActiveTableFont = amountActiveTableFont;
    this->amountInactiveTableFont = amountInactiveTableFont;
    this->infoActiveFont = infoActiveTableFont;
    this->infoInactiveFont = infoInactiveTableFont;
    this->allowDecorationColor = allowDecorationColor;

    // *** WARNING ***
    // CurencyInfo must be set before using the model (it changes with the scenario loaded)
    // we initialize it with dummy value so nothing crash
    this->currInfo = {.name="US Dollar", .symbol="$", .isoCode="USD", .noOfDecimal=2, };

    // filters default : all data is displayed except Expenses
    displayPeriodicItems = true;
    displayIrregularItems = true;
    displayActiveItems = true;
    displayInactiveItems = true;
    displayIncomes = true;
    displayExpenses = false;
}

ScenarioFeTableModel::~ScenarioFeTableModel()
{
}

QVariant ScenarioFeTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return tr("Type");
        case 1:
            return tr("Name");
        case 2:
            return tr("Amount");
        case 3:
            return tr("Info");
        }
    }
    return QVariant();
}


int ScenarioFeTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return itemsList.size();
}


int ScenarioFeTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 4;
}


QVariant ScenarioFeTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int row = index.row();
    int col = index.column();

    ItemInfo info = itemsList.at(row);
    if (role==Qt::DisplayRole){
        if ( row <= (itemsList.size()-1) ){
            if (col==0){            // *** type ***
                return info.type;
            } else if (col==1){     // *** name ***
                return info.name;
            } else if (col==2){     // *** Amount ***
                return info.amount;
            } else if (col==3){     // *** info ***
                return info.info;
            }
        }
    } else if (role==Qt::TextAlignmentRole){
        if (col==2){
            return int(Qt::AlignRight | Qt::AlignVCenter);
        } else if (col==3){
            return int(Qt::AlignLeft | Qt::AlignVCenter);
        } else {
            return int(Qt::AlignHCenter| Qt::AlignVCenter);
        }
    } else if (role == Qt::ForegroundRole){
        if ( (allowDecorationColor==true) && (col==1) && (info.decorationColor.isValid()) ) {
            return QVariant(info.decorationColor);
        } else {
            return QVariant();
        }
    } else if (role==Qt::FontRole){
        if (info.isActive){
            if (col==2) {
                return amountActiveTableFont;
            } else if (col==3){
                return infoActiveFont;
            } else {
                return activeTableFont;
            }
        } else {
            if (col==2) {
                return amountInactiveTableFont;
            } else if (col==3) {
                return infoInactiveFont;
            } else {
                return inactiveTableFont;
            }
        }

    }

    return QVariant();
}



// Everytime scenario changes, this must be called (including first time set)
// Filters are untouched.
void ScenarioFeTableModel::newScenario(CurrencyInfo cInfo, QMap<QUuid, PeriodicFeStreamDef> newIncomesDefPeriodic, QMap<QUuid, IrregularFeStreamDef> newIncomesDefIrregular, QMap<QUuid, PeriodicFeStreamDef> newExpensesDefPeriodic, QMap<QUuid, IrregularFeStreamDef> newExpensesDefIrregular)
{
    currInfo = cInfo;
    incomesDefPeriodic = newIncomesDefPeriodic;
    incomesDefIrregular = newIncomesDefIrregular;
    expensesDefPeriodic = newExpensesDefPeriodic;
    expensesDefIrregular = newExpensesDefIrregular;

    rebuildLists();
}

void ScenarioFeTableModel::addModifyPeriodicItem(PeriodicFeStreamDef p)
{
    if(p.getIsIncome()){
        incomesDefPeriodic.insert(p.getId(),p);
    } else{
        expensesDefPeriodic.insert(p.getId(),p);
    }
    rebuildLists();
}


void ScenarioFeTableModel::addModifyIrregularItem(IrregularFeStreamDef p)
{
    if(p.getIsIncome()){
        incomesDefIrregular.insert(p.getId(),p);
    } else{
        expensesDefIrregular.insert(p.getId(),p);
    }
    rebuildLists();
}


void ScenarioFeTableModel::removeItems(QList<QUuid> toRemove)
{
    foreach(QUuid id, toRemove){
        if (incomesDefPeriodic.contains(id)) {
            incomesDefPeriodic.remove(id);
        }
        if (incomesDefIrregular.contains(id)) {
            incomesDefIrregular.remove(id);
        }
        if (expensesDefPeriodic.contains(id)) {
            expensesDefPeriodic.remove(id);
        }
        if (expensesDefIrregular.contains(id)) {
            expensesDefIrregular.remove(id);
        }
    }

    rebuildLists();
}


QUuid ScenarioFeTableModel::duplicateItem(QUuid id, bool &found)
{
    found = false;
    QUuid newId;

    // find the item and duplicate
    if (incomesDefPeriodic.contains(id)) {
        found = true;
        PeriodicFeStreamDef p = incomesDefPeriodic.value(id);
        PeriodicFeStreamDef p2 = p.duplicate();
        incomesDefPeriodic.insert(p2.getId(),p2);
        newId = p2.getId();
    } else if (incomesDefIrregular.contains(id)) {
        found = true;
        IrregularFeStreamDef p = incomesDefIrregular.value(id);
        IrregularFeStreamDef p2 = p.duplicate();
        incomesDefIrregular.insert(p2.getId(),p2);
        newId = p2.getId();
    } else if (expensesDefPeriodic.contains(id)) {
        found = true;
        PeriodicFeStreamDef p = expensesDefPeriodic.value(id);
        PeriodicFeStreamDef p2 = p.duplicate();
        expensesDefPeriodic.insert(p2.getId(),p2);
        newId = p2.getId();
    } else if (expensesDefIrregular.contains(id)) {
        found = true;
        IrregularFeStreamDef p = expensesDefIrregular.value(id);
        IrregularFeStreamDef p2 = p.duplicate();
        expensesDefIrregular.insert(p2.getId(),p2);
        newId = p2.getId();
    } else{
        // not found !
        return id;
    }

    rebuildLists();
    return newId;

}


void ScenarioFeTableModel::changeActiveStatusItems(QList<QUuid> idList, bool enable)
{
    // look everywhere
    foreach(QUuid id, idList){
        if (incomesDefPeriodic.contains(id)) {
            PeriodicFeStreamDef p = incomesDefPeriodic.value(id);
            p.setActive(enable);
            incomesDefPeriodic.insert(p.getId(),p);
        }
        if (incomesDefIrregular.contains(id)) {
            IrregularFeStreamDef p = incomesDefIrregular.value(id);
            p.setActive(enable);
            incomesDefIrregular.insert(p.getId(),p);
        }
        if (expensesDefPeriodic.contains(id)) {
            PeriodicFeStreamDef p = expensesDefPeriodic.value(id);
            p.setActive(enable);
            expensesDefPeriodic.insert(p.getId(),p);
        }
        if (expensesDefIrregular.contains(id)) {
            IrregularFeStreamDef p = expensesDefIrregular.value(id);
            p.setActive(enable);
            expensesDefIrregular.insert(p.getId(),p);
        }
    }

    rebuildLists();
}



int ScenarioFeTableModel::getRow(QUuid id, bool &found)
{
    found = false;
    int row=0;
    foreach(ItemInfo item, itemsList){
        if (item.id==id){
            found = true;
            return row;
        }
        row++;
    }
    return -1;
}


QUuid ScenarioFeTableModel::getId(int row, bool &found)
{
    found = false;
    if ( (row<0) || (row>itemsList.size()-1) ){
        return QUuid::createUuid(); // dummy
    }
    found = true;
    return itemsList.at(row).id;
}


FeStreamDef::FeStreamType ScenarioFeTableModel::getTypeOfFeStreamDef(QUuid id, bool &found)
{
    found = false;
    // find the item and duplicate
    if (incomesDefPeriodic.contains(id)) {
        found = true;
        PeriodicFeStreamDef p = incomesDefPeriodic.value(id);
        return p.getStreamType();
    } else if (incomesDefIrregular.contains(id)) {
        found = true;
        IrregularFeStreamDef p = incomesDefIrregular.value(id);
        return p.getStreamType();
    } else if (expensesDefPeriodic.contains(id)) {
        found = true;
        PeriodicFeStreamDef p = expensesDefPeriodic.value(id);
        return p.getStreamType();
    } else if (expensesDefIrregular.contains(id)) {
        found = true;
        IrregularFeStreamDef p = expensesDefIrregular.value(id);
        return p.getStreamType();
    }
    return FeStreamDef::FeStreamType::PERIODIC; // dummy since not found
}


PeriodicFeStreamDef ScenarioFeTableModel::getPeriodicFeStreamDef(QUuid id, bool &found)
{
    found = false;
    // find the item and duplicate
    if (incomesDefPeriodic.contains(id)) {
        found = true;
        PeriodicFeStreamDef p = incomesDefPeriodic.value(id);
        return p;
    } else if (expensesDefPeriodic.contains(id)) {
        found = true;
        PeriodicFeStreamDef p = expensesDefPeriodic.value(id);
        return p;
    }
    return PeriodicFeStreamDef(); // dummy since not found
}


IrregularFeStreamDef ScenarioFeTableModel::getIrregularFeStreamDef(QUuid id, bool &found)
{
    found = false;
    // find the item and duplicate
    if (incomesDefIrregular.contains(id)) {
        found = true;
        IrregularFeStreamDef p = incomesDefIrregular.value(id);
        return p;
    } else if (expensesDefIrregular.contains(id)) {
        found = true;
        IrregularFeStreamDef p = expensesDefIrregular.value(id);
        return p;
    }
    return IrregularFeStreamDef(); // dummy since not found
}


void ScenarioFeTableModel::filtersChanged(bool showIncome, bool showPeriodic, bool showIrregular, bool showActive, bool showInactive)
{
    // updates filters
    if (showIncome==true){
        displayIncomes = true;
        displayExpenses = false;
    } else {
        displayIncomes = false;
        displayExpenses = true;
    }
    displayPeriodicItems = showPeriodic;
    displayIrregularItems = showIrregular;
    displayActiveItems = showActive;
    displayInactiveItems = showInactive;
    // rebuild item list and the table view
    rebuildLists();
}


int ScenarioFeTableModel::getNoItems()
{
    return itemsList.size();
}


// *** PRIVATE ***

// From raw data, recreate the compact list of data used to fill the table.
// Sorting must be done at this stage (locale-aware sorting)
void ScenarioFeTableModel::rebuildLists()
{
    emit beginResetModel();

    // we need this in order to sort in a locale-aware way
    class MyString : public QString {
    public:
        MyString(const QString& text) : QString(text) {
        }
        bool operator<(const MyString& other) const {
            if ( QString::localeAwareCompare(*this,other) < 0 ){
                return true;
            } else {
                return false;
            }
        }
    };

    // intermediate data structure, used just to sort items by name.
    // Name must be made unique, because there could be duplicates, even if this is not recommended.
    // For that, we append the ID at the very end, filling with Space. It means that duplicate will be sorted by QUuid.
    QMap<MyString,ItemInfo> proxyList;

    // clear the items list
    itemsList.clear();

    if (displayIncomes){
        if (displayPeriodicItems) {
            foreach(PeriodicFeStreamDef item, incomesDefPeriodic.values() ){
                bool active = item.getActive();
                if ( (active==true && displayActiveItems==true) || (active==false && displayInactiveItems==true) ) {
                    ItemInfo info;
                    info.id = item.getId();
                    info.name = item.getName();
                    info.type = tr("Periodic");
                    info.isActive = item.getActive();
                    info.info = item.toStringForDisplay(currInfo,theLocale);
                    info.decorationColor = item.getDecorationColor();
                    int ok;
                    info.amount = CurrencyHelper::quint64ToDoubleString(item.getAmount(), currInfo, theLocale, false, ok);;
                    if ( ok != 0 ){
                        info.amount = QString("Error");  // amount or noOfCurrencyDecimals is too big, should not happen
                    }
                    MyString sortKey = MyString(item.getName().leftJustified(FeStreamDef::NAME_MAX_LEN,' ', true).append(info.id.toString()));
                    proxyList.insert(sortKey, info);
                }
            }
        }
        if (displayIrregularItems) {
            foreach(IrregularFeStreamDef item, incomesDefIrregular.values() ){
                bool active = item.getActive();
                if ( (active==true && displayActiveItems==true) || (active==false && displayInactiveItems==true) ) {
                    ItemInfo info;
                    info.id = item.getId();
                    info.name = item.getName();
                    info.type = tr("Irregular");
                    info.isActive = item.getActive();
                    info.info = item.toStringForDisplay(currInfo,theLocale);
                    info.amount = "";
                    info.decorationColor = item.getDecorationColor();
                    MyString sortKey = MyString(item.getName().leftJustified(FeStreamDef::NAME_MAX_LEN,' ',true).append(info.id.toString()));
                    proxyList.insert(sortKey, info);
                }
            }
        }
    }

    if (displayExpenses) {
        if (displayPeriodicItems) {
            foreach(PeriodicFeStreamDef item, expensesDefPeriodic.values() ){
                bool active = item.getActive();
                if ( (active==true && displayActiveItems==true) || (active==false && displayInactiveItems==true) ) {
                    ItemInfo info;
                    info.id = item.getId();
                    info.name = item.getName();
                    info.type = tr("Periodic");
                    info.isActive = item.getActive();
                    info.info = item.toStringForDisplay(currInfo,theLocale);
                    int ok;
                    info.amount = CurrencyHelper::quint64ToDoubleString(item.getAmount(), currInfo, theLocale, false, ok);
                    info.decorationColor = item.getDecorationColor();
                    if ( ok != 0 ){
                        info.amount = QString("Error");  // amount or noOfCurrencyDecimals is too big, should not happen
                    }
                    MyString sortKey = MyString(item.getName().leftJustified(FeStreamDef::NAME_MAX_LEN,' ',true).append(info.id.toString()));
                    proxyList.insert(sortKey.toLower(), info);
                }
            }
        }
        if (displayIrregularItems) {
            foreach(IrregularFeStreamDef item, expensesDefIrregular.values() ){
                bool active = item.getActive();
                if ( (active==true && displayActiveItems==true) || (active==false && displayInactiveItems==true) ) {
                    ItemInfo info;
                    info.id = item.getId();
                    info.name = item.getName();
                    info.type = tr("Irregular");
                    info.isActive = item.getActive();
                    info.info = item.toStringForDisplay(currInfo,theLocale);
                    info.amount = "";
                    info.decorationColor = item.getDecorationColor();
                    MyString sortKey = MyString(item.getName().leftJustified(FeStreamDef::NAME_MAX_LEN,' ',true).append(info.id.toString()));
                    proxyList.insert(sortKey.toLower(), info);
                }
            }
        }
    }

    // rebuild the itemsList
    foreach(QString name, proxyList.keys()){
        ItemInfo info = proxyList.value(name);
        itemsList.append(info);
    }

    emit endResetModel();

}


// *** Getters / Setters ***

CurrencyInfo ScenarioFeTableModel::getCurrInfo() const
{
    return currInfo;
}

void ScenarioFeTableModel::setCurrInfo(const CurrencyInfo &newCurrInfo)
{
    currInfo = newCurrInfo;
}


QMap<QUuid, PeriodicFeStreamDef> ScenarioFeTableModel::getIncomesDefPeriodic() const
{
    return incomesDefPeriodic;
}

QMap<QUuid, IrregularFeStreamDef> ScenarioFeTableModel::getIncomesDefIrregular() const
{
    return incomesDefIrregular;
}

QMap<QUuid, PeriodicFeStreamDef> ScenarioFeTableModel::getExpensesDefPeriodic() const
{
    return expensesDefPeriodic;
}

QMap<QUuid, IrregularFeStreamDef> ScenarioFeTableModel::getExpensesDefIrregular() const
{
    return expensesDefIrregular;
}

bool ScenarioFeTableModel::getAllowDecorationColor() const
{
    return allowDecorationColor;
}

void ScenarioFeTableModel::setAllowDecorationColor(bool newAllowDecorationColor)
{
    // we want to refresh the display right afterward
    emit beginResetModel();
    allowDecorationColor = newAllowDecorationColor;
    emit endResetModel();
}




