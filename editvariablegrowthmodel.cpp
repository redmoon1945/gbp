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

#include "editvariablegrowthmodel.h"
#include <QCoreApplication>


EditVariableGrowthModel::EditVariableGrowthModel(QString newGrowthName, QLocale aLocale, QObject *parent)
    : QAbstractTableModel(parent)
{
    this->growthName = newGrowthName;
    this->theLocale = aLocale;
}


QVariant EditVariableGrowthModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return QString(tr("Transition Date"));
        case 1:
            return QString(tr("%1 (annual basis)")).arg(growthName);
        case 2:
            return QString(tr("%1 (monthly basis)")).arg(growthName);
        }
    }
    return QVariant();
}


int EditVariableGrowthModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return growthData.getAnnualVariableGrowth().size();
}


int EditVariableGrowthModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 3;
}


QVariant EditVariableGrowthModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role==Qt::DisplayRole){
        int row = index.row();
        int col = index.column();
        QMap<QDate,qint64> factors = growthData.getAnnualVariableGrowth();
        QList<QDate> listKeys = growthData.getAnnualVariableGrowth().keys();
        if ( row <= (listKeys.size()-1) ){
            QDate key = listKeys.at(row);
            if (col==0){
                return theLocale.toString(key);
            } else if (col==1){
                double d = Growth::fromDecimalToDouble(factors.value(key));
                int noDec = Growth::NO_OF_DECIMALS;
                QString vStr = theLocale.toString(d,'f',noDec);
                return vStr+ QString(" %") ;
            } else if (col==2){
                double d = Growth::fromDecimalToDouble(factors.value(key));
                double monthlyBasis = Util::annualToMonthlyGrowth(d);
                QString vStr = theLocale.toString(monthlyBasis,'f');    // display as much decimal as possible
                return vStr+ QString(" %") ;
            }
        }
    } else if (role==Qt::TextAlignmentRole){
        return int(Qt::AlignHCenter | Qt::AlignVCenter);
    }

    return QVariant();
}


// get position in the list of (date,growth) items for the provided date
// Return -1 if not found.
int EditVariableGrowthModel::getPositionForDate(QDate aDate)
{
    QMap<QDate,qint64> map = growthData.getAnnualVariableGrowth();
    QList<QDate> keys = map.keys();
    return keys.indexOf(aDate);
}


// Getters / setters

Growth EditVariableGrowthModel::getGrowthData() const
{
    return growthData;
}


void EditVariableGrowthModel::setGrowthData(const Growth &newGrowthData)
{
    // Must be variable type
    if (newGrowthData.getType()!=Growth::VARIABLE){
        throw std::invalid_argument("Growth must be of type Complex");
    }
    // we assume model has completely changed (easier that way)
    emit beginResetModel();
    growthData = newGrowthData;
    emit endResetModel();
}


QString EditVariableGrowthModel::getGrowthName() const
{
    return growthName;
}


void EditVariableGrowthModel::setGrowthName(const QString &newGrowthName)
{
    growthName = newGrowthName;
}

QLocale EditVariableGrowthModel::getTheLocale() const
{
    return theLocale;
}

void EditVariableGrowthModel::setTheLocale(const QLocale &newTheLocale)
{
    theLocale = newTheLocale;
}



