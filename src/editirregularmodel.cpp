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

#include "editirregularmodel.h"
#include "gbpcontroller.h"
#include "qbrush.h"
#include <QFont>
#include <QCoreApplication>


EditIrregularModel::EditIrregularModel(QLocale aLocale, QObject *parent)
    : QAbstractTableModel(parent)
{
    this->theLocale = aLocale;

    // *** WARNING ***
    // CurrencyInfo must be set before using the model (it changes with the scenario loaded)
    // we initialize it with dummy value so nothing crash
    this->currInfo = {.name="US Dollar", .symbol="$", .isoCode="USD", .noOfDecimal=2, };
}


QVariant EditIrregularModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return tr("Date");
        case 1:
            return tr("Amount");
        case 2:
            return tr("Notes");
        }
    }
    return QVariant();
}


int EditIrregularModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return items.size();
}


int EditIrregularModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 3;
}


QVariant EditIrregularModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int row = index.row();
    int col = index.column();

    if (role==Qt::DisplayRole){
        QList<QDate> listKeys = items.keys();
        if ( row <= (listKeys.size()-1) ){
            QDate key = listKeys.at(row);
            if (col==0){    // *** date ***
                return theLocale.toString(key);
                //return theLocale.toString(key,"yyyy-MMM-dd (ddd)");
            } else if (col==1){ //*** Amount ***
                IrregularFeStreamDef::AmountInfo ai = items.value(key);
                int result;
                QString vStr = CurrencyHelper::quint64ToDoubleString(ai.amount,currInfo,theLocale,false,result);
                if(result!=0){
                    // should never happen...
                    return "";
                }
                return vStr ;
            } else if (col==2) {   //*** notes ***
                IrregularFeStreamDef::AmountInfo ai = items.value(key);
                return ai.notes;
            }
        }
    } else if (role==Qt::TextAlignmentRole){
        if (col==1){
            return int(Qt::AlignRight | Qt::AlignVCenter);
        } else {
            return int(Qt::AlignHCenter| Qt::AlignVCenter);
        }
    } else if (role==Qt::FontRole){

        if (col==1) {
            return monoTableFont;
        } else {
            return defaultTableFont;
        }

    } else if (role == Qt::ForegroundRole){
        QList<QDate> listKeys = items.keys();
        QDate key = listKeys.at(row) ;
        if (key < GbpController::getInstance().getTomorrow()) {
            // this is past
            return QBrush(QColor(128,128,128));
        }

    }

    return QVariant();
}


// get position in the list of items for the provided date
// Return -1 if not found.
int EditIrregularModel::getPositionForDate(QDate aDate)
{
    QList<QDate> keys = items.keys();
    return keys.indexOf(aDate);
}


// GETTERS / SETTERS ***

QMap<QDate, IrregularFeStreamDef::AmountInfo> EditIrregularModel::getItems() const
{
    return items;
}

void EditIrregularModel::setItems(const QMap<QDate, IrregularFeStreamDef::AmountInfo> &newItems)
{

    // we assume model has completely changed (easier that way)
    emit beginResetModel();
    items = newItems;
    emit endResetModel();
}

QLocale EditIrregularModel::getTheLocale() const
{
    return theLocale;
}

void EditIrregularModel::setTheLocale(const QLocale &newTheLocale)
{
    theLocale = newTheLocale;
}

CurrencyInfo EditIrregularModel::getCurrInfo() const
{
    return currInfo;
}

void EditIrregularModel::setCurrInfo(const CurrencyInfo &newCurrInfo)
{
    currInfo = newCurrInfo;
}

QFont EditIrregularModel::getDefaultTableFont() const
{
    return defaultTableFont;
}

void EditIrregularModel::setDefaultTableFont(const QFont &newDefaultTableFont)
{
    defaultTableFont = newDefaultTableFont;
}

QFont EditIrregularModel::getMonoTableFont() const
{
    return monoTableFont;
}

void EditIrregularModel::setMonoTableFont(const QFont &newMonoTableFont)
{
    monoTableFont = newMonoTableFont;
}
