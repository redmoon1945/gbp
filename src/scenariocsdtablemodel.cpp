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

#include "scenariocsdtablemodel.h"
#include <qbrush.h>


ScenarioCsdTableModel::ScenarioCsdTableModel(QLocale aLocale, QFont activeTableFont,
    QFont inactiveTableFont, QFont amountActiveTableFont, QFont amountInactiveTableFont,
    QFont infoActiveTableFont, QFont infoInactiveTableFont, bool allowColoredCsdNames,
    QObject *parent) : QAbstractTableModel{parent}
{
    this->theLocale = aLocale;
    this->activeTableFont = activeTableFont;
    this->inactiveTableFont = inactiveTableFont;
    this->amountActiveTableFont = amountActiveTableFont;
    this->amountInactiveTableFont = amountInactiveTableFont;
    this->infoActiveFont = infoActiveTableFont;
    this->infoInactiveFont = infoInactiveTableFont;
    this->allowColoredCsdNames = allowColoredCsdNames;
}


ScenarioCsdTableModel::~ScenarioCsdTableModel()
{
}


QVariant ScenarioCsdTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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


int ScenarioCsdTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return itemsList.size();
}


int ScenarioCsdTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 4;
}


QVariant ScenarioCsdTableModel::data(const QModelIndex &index, int role) const
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
        if (info.isActive) {
            if ( (allowColoredCsdNames==true) && (col==1) && (info.csdNameColor.isValid()) ) {
                return QVariant(info.csdNameColor);
            } else {
                return QVariant();
            }
        } else {
            return QVariant(QColor(128,128,128));
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


// Do a complete refresh of the table content, using the data passed
void ScenarioCsdTableModel::refresh(CurrencyInfo currInfo, QMap<QUuid, PeriodicFeStreamDef>
    incomesDefPeriodic, QMap<QUuid, IrregularFeStreamDef> incomesDefIrregular,
    QMap<QUuid, PeriodicFeStreamDef> expensesDefPeriodic, QMap<QUuid, IrregularFeStreamDef>
    expensesDefIrregular, Tags tags, TagCsdRelationships tagsRelationships, bool showIncomes,
    bool showExpenses, bool showPeriodic, bool showIrregular, bool showActive, bool showInactive,
    FilterTags filterTags)
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

    // intermediate data structure, used just to sort items by name. Name must be made unique,
    // because there could be duplicates, even if this is not recommended. For that, we append the
    // ID at the very end, filling with Space. It means that duplicate will be sorted by QUuid.
    QMap<MyString,ItemInfo> proxyList;

    // clear the items list
    itemsList.clear();

    if (showIncomes){
        if (showPeriodic) {
            foreach(PeriodicFeStreamDef item, incomesDefPeriodic.values() ){
                bool active = item.getActive();
                if ( (active==true && showActive==true) ||
                    (active==false && showInactive==true) ) {

                    // Does it pass the tag filter test ?
                    if ( false == isItemPassTagFilter(item.getId(), tagsRelationships, filterTags)){
                        continue; // item does not pass the tag filter test, it cannot be displayed
                    }

                    ItemInfo info;
                    info.id = item.getId();
                    info.name = item.getName();
                    info.type = tr("Periodic");
                    info.isActive = item.getActive();
                    info.info = item.toStringForDisplay(currInfo,theLocale);
                    info.csdNameColor = item.getDecorationColor();
                    int ok;
                    info.amount = CurrencyHelper::quint64ToDoubleString(item.getAmount(), currInfo,
                        theLocale, false, ok);;
                    if ( ok != 0 ){
                        // amount or noOfCurrencyDecimals is too big, should not happen
                        info.amount = QString("Error");
                    }
                    MyString sortKey = MyString(item.getName().leftJustified(
                        FeStreamDef::NAME_MAX_LEN,' ', true).append(info.id.toString()));
                    proxyList.insert(sortKey, info);
                }
            }
        }
        if (showIrregular) {
            foreach(IrregularFeStreamDef item, incomesDefIrregular.values() ){
                bool active = item.getActive();
                if ( (active==true && showActive==true) ||
                    (active==false && showInactive==true) ) {

                    // Does it pass the tag filter test ?
                    if ( false == isItemPassTagFilter(item.getId(), tagsRelationships, filterTags)){
                        continue; // item does not pass the tag filter test, it cannot be displayed
                    }

                    ItemInfo info;
                    info.id = item.getId();
                    info.name = item.getName();
                    info.type = tr("Irregular");
                    info.isActive = item.getActive();
                    info.info = item.toStringForDisplay(currInfo,theLocale);
                    info.amount = "";
                    info.csdNameColor = item.getDecorationColor();
                    MyString sortKey = MyString(item.getName().leftJustified(
                        FeStreamDef::NAME_MAX_LEN,' ',true).append(info.id.toString()));
                    proxyList.insert(sortKey, info);
                }
            }
        }
    }

    if (showExpenses) {
        if (showPeriodic) {
            foreach(PeriodicFeStreamDef item, expensesDefPeriodic.values() ){
                bool active = item.getActive();
                if ( (active==true && showActive==true) ||
                    (active==false && showInactive==true) ) {

                    // Does it pass the tag filter test ?
                    if ( false == isItemPassTagFilter(item.getId(), tagsRelationships, filterTags)){
                        continue; // item does not pass the tag filter test, it cannot be displayed
                    }

                    ItemInfo info;
                    info.id = item.getId();
                    info.name = item.getName();
                    info.type = tr("Periodic");
                    info.isActive = item.getActive();
                    info.info = item.toStringForDisplay(currInfo,theLocale);
                    int ok;
                    info.amount = CurrencyHelper::quint64ToDoubleString(item.getAmount(), currInfo,
                        theLocale, false, ok);
                    info.csdNameColor = item.getDecorationColor();
                    if ( ok != 0 ){
                        // amount or noOfCurrencyDecimals is too big, should not happen
                        info.amount = QString("Error");
                    }
                    MyString sortKey = MyString(item.getName().leftJustified(
                        FeStreamDef::NAME_MAX_LEN,' ',true).append(info.id.toString()));
                    proxyList.insert(sortKey.toLower(), info);
                }
            }
        }
        if (showIrregular) {
            foreach(IrregularFeStreamDef item, expensesDefIrregular.values() ){
                bool active = item.getActive();
                if ( (active==true && showActive==true) ||
                    (active==false && showInactive==true) ) {

                    // Does it pass the tag filter test ?
                    if ( false == isItemPassTagFilter(item.getId(), tagsRelationships, filterTags)){
                        continue; // item does not pass the tag filter test, it cannot be displayed
                    }

                    ItemInfo info;
                    info.id = item.getId();
                    info.name = item.getName();
                    info.type = tr("Irregular");
                    info.isActive = item.getActive();
                    info.info = item.toStringForDisplay(currInfo,theLocale);
                    info.amount = "";
                    info.csdNameColor = item.getDecorationColor();
                    MyString sortKey = MyString(item.getName().leftJustified(
                        FeStreamDef::NAME_MAX_LEN,' ',true).append(info.id.toString()));
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

    // refresh the table
    emit endResetModel();
}


int ScenarioCsdTableModel::getRow(QUuid id, bool &found)
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


QUuid ScenarioCsdTableModel::getId(int row, bool &found)
{
    found = false;
    if ( (row<0) || (row>itemsList.size()-1) ){
        return QUuid::createUuid(); // dummy
    }
    found = true;
    return itemsList.at(row).id;
}


int ScenarioCsdTableModel::getNoItems()
{
    return itemsList.size();
}


// *** PRIVATE ***

// Check if this Csd passes the tags filter test. If returns false, the Csd cannot be displayed.
// If returnes true, the Csd is allowed to be displayed if it passes the other filter tests.
bool ScenarioCsdTableModel::isItemPassTagFilter(QUuid csdId, TagCsdRelationships
    tagCsdRelationships, FilterTags filterTags)
{
    if( filterTags.getEnableFilterByTags()==false){
        return true;
    }

    // get linked tags for this csd
    QSet<QUuid> csdTagIdSet = tagCsdRelationships.getRelationshipsForCsd(csdId);

    // Test
    if (filterTags.getMode()==FilterTags::Mode::ALL)  {
        foreach (QUuid filtertagId, filterTags.getFilterTagIdSet()) {
            if (csdTagIdSet.contains(filtertagId)==false) {
                return false;
            }
        }
        return true;
    } else if (filterTags.getMode()==FilterTags::Mode::ANY){
        foreach (QUuid filtertagId, filterTags.getFilterTagIdSet()) {
            if (csdTagIdSet.contains(filtertagId)==true) {
                return true;
            }
        }
        return false;
    } else { // NONE
        foreach (QUuid filtertagId, filterTags.getFilterTagIdSet()) {
            if (csdTagIdSet.contains(filtertagId)==true) {
                return false;
            }
        }
        return true;
    }
}


// *** Getters / Setters ***

bool ScenarioCsdTableModel::getAllowColoredCsdNames() const
{
    return allowColoredCsdNames;
}

void ScenarioCsdTableModel::setAllowColoredCsdNames(bool newAllowColoredCsdNames)
{
    // we want to refresh the display right afterward
    emit beginResetModel();
    allowColoredCsdNames = newAllowColoredCsdNames;
    emit endResetModel();
}




