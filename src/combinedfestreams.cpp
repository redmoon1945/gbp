/*
 *  Copyright (C) 2024-2026 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
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

#include "combinedfestreams.h"
#include "currencyhelper.h"


CombinedFeStreams::CombinedFeStreams(quint32 noOfDays)
{
    if(noOfDays==0){
        throw std::invalid_argument(QString("%1: maxNoOfDays is zero")
            .arg(Q_FUNC_INFO).toStdString());
    }
    // Must not exceed the max size
    if (noOfDays > FeStream::MAX_DAYS) {
        throw std::invalid_argument(QString("%1: maxNoOfDays is over the maximum allowed")
            .arg(Q_FUNC_INFO).toStdString());
    }
    this->noOfDays = noOfDays;

    // Allocate the array at full size. For 100 years, this is roughly 36600 elements.
    // resize() changes the list’s size(), not just its capacity.
    combinedStreams.resize(noOfDays);

    // Init the array by marking all entries as "unused"
    for (uint i = 0; i < noOfDays; ++i) {
        combinedStreams[i] = {.used=false, .totalIncomes=0, .totalExpenses=0,
            .incomesList={}, .expensesList={}};
    }
    noOfElementsUsed = 0;
    noOfFe = 0;
    csdContributors = {};
}


CombinedFeStreams::CombinedFeStreams(const CombinedFeStreams &o)
{
    this->combinedStreams = o.combinedStreams;
}


CombinedFeStreams &CombinedFeStreams::operator=(const CombinedFeStreams &o)
{
    if (this != &o){// to protect against self-assignment
        this->combinedStreams = o.combinedStreams;
        this->noOfDays = o.noOfDays;
        this->noOfFe = o.noOfFe;
        this->noOfElementsUsed = o.noOfElementsUsed;
        this->csdContributors = o.csdContributors;
    }
    return *this;
}


bool CombinedFeStreams::operator==(const CombinedFeStreams &o) const
{
    if ( !(this->combinedStreams == o.combinedStreams) ||
        (this->noOfDays != o.noOfDays) || (this->noOfElementsUsed != o.noOfElementsUsed) ||
        (this->noOfFe != o.noOfFe) || (this->csdContributors != o.csdContributors) ) {
        return false;
    } else {
        return true;
    }
}


CombinedFeStreams::~CombinedFeStreams()
{
}


void CombinedFeStreams::addStream(const FeStream &theStream, const CurrencyInfo &currInfo)
{
    // convert weak reference of theStream to strong reference.
    // The Csd must exist. Read only operation.
    QSharedPointer<const Csd> csdStrongRefPtr = theStream.getCsdPtr().toStrongRef();
    if (csdStrongRefPtr==nullptr){
        return; // Csd does not exist anymore
    }

    // Check that theStream has the same no of elements than combinedStreams
    if (theStream.getNoOfDays() != noOfDays) {
        throw std::invalid_argument(QString("%1: New FeStream has not the right no of elements")
            .arg(Q_FUNC_INFO).toStdString());
    }

    // Make sure this CSD has not already contributed, otherwise add it to the contributors.
    if( csdContributors.contains(csdStrongRefPtr->getId())){
        return;
    } else {
        csdContributors.insert(csdStrongRefPtr->getId());
    }

    // merge
    uint feSize = theStream.size();
    for (uint i = 0; i < feSize; ++i) {
        if (theStream.contains(i) == true) {
            qint64 amountInt = theStream.get(i);

            // This is one more financial event in comb
            noOfFe++;
            // convert the amount from decimal to double (currency unit)
            int convResult;
            double amount = CurrencyHelper::amountQint64ToDouble(amountInt, currInfo.noOfDecimal,
                convResult) ;
            if (convResult != 0){
                continue; // should never happen
            }
            if (combinedStreams[i].used == false) {
                // this is a new entry for that day

                // Update no of element used
                noOfElementsUsed++;
                // Mark the entry has used
                combinedStreams[i].used = true;
                // Set it
                if (csdStrongRefPtr->getIsIncome()) {
                    // new income
                    combinedStreams[i].totalIncomes = amount;
                    combinedStreams[i].totalExpenses = 0;
                    // there wont be duplication
                    combinedStreams[i].incomesList.append(
                        {.amount=amount, .csdPtr=theStream.getCsdPtr()});
                } else {
                    // new expense
                    combinedStreams[i].totalIncomes = 0;
                    combinedStreams[i].totalExpenses = -amount;
                    combinedStreams[i].expensesList.append(
                        {.amount=(-amount), .csdPtr=theStream.getCsdPtr()});
                }
            } else {
                // this is a existing entry for that day.
                if (csdStrongRefPtr->getIsIncome()) {
                    // additional income
                    combinedStreams[i].totalIncomes = CurrencyHelper::add(
                        combinedStreams[i].totalIncomes, amount, currInfo.noOfDecimal);
                    combinedStreams[i].incomesList.append(
                        {.amount=amount, .csdPtr=theStream.getCsdPtr()});
                } else {
                    // additional expense
                    combinedStreams[i].totalExpenses = CurrencyHelper::add(
                        combinedStreams[i].totalExpenses, -amount, currInfo.noOfDecimal);
                    combinedStreams[i].expensesList.append(
                        {.amount=(-amount), .csdPtr=theStream.getCsdPtr()});
                }
            }
        }
    }

}


// for the amount , a "loose" comparison is performed. 2 double are declared equal if
// the difference is less than the smallest unit of all the currency available
// (3 decimals + 1 spare for rounding)
bool CombinedFeStreams::DailyInfo::operator==(const DailyInfo& o) const{
    if( used != o.used ){
        return false;
    }
    if( fabs(totalIncomes - o.totalIncomes) >= 0.0001 ) {
        return false;
    }
    if( fabs(totalExpenses - o.totalExpenses) >= 0.0001 ){
        return false;
    }
    if(incomesList != o.incomesList){
        return false;
    }
    if(expensesList != o.expensesList){
        return false;
    }
    return true;
}


CombinedFeStreams::DailyInfo &CombinedFeStreams::DailyInfo::operator=(const DailyInfo &o)
{
    used = o.used;
    totalIncomes = o.totalIncomes;
    totalExpenses = o.totalExpenses;
    incomesList = o.incomesList;
    expensesList = o.expensesList;
    return *this;
}


QString CombinedFeStreams::DailyInfo::toString() const
{
    if (used == false) {
        return "Unused";
    }

    // income list
    QStringList incomeListSl;
    incomeListSl.append(QString("IL:"));
    if (incomesList.size()==0) {
        incomeListSl.append(QString("Empty"));
    } else {
        for (int var = 0; var < incomesList.size(); ++var) {
            auto csdStrongRefPtr = incomesList[var].csdPtr.toStrongRef();
            if (csdStrongRefPtr==nullptr){
                continue; // Csd does not exist anymore
            }
            incomeListSl.append(QString("(%1,%2)")
                .arg(csdStrongRefPtr->getId().toString(QUuid::StringFormat::WithoutBraces))
                .arg(incomesList[var].amount));
        }
    }
    QString incomeListString = incomeListSl.join(" ");

    // expense list
    QStringList expenseListSl;
    expenseListSl.append(QString("EL:"));
    if (expensesList.size()==0) {
        expenseListSl.append(QString("Empty"));
    } else {
        for (int var = 0; var < expensesList.size(); ++var) {
            auto csdStrongRefPtr = expensesList[var].csdPtr.toStrongRef();
            if (csdStrongRefPtr==nullptr){
                continue; // Csd does not exist anymore
            }
            expenseListSl.append(QString("(%1,%2)")
                .arg(csdStrongRefPtr->getId().toString(QUuid::StringFormat::WithoutBraces))
                .arg(expensesList[var].amount));
        }
    }
    QString expenseListString = expenseListSl.join(" ");

    // all together
    return QString("TI=%2 TE=%3 %4 %5").arg(totalIncomes).arg(totalExpenses)
        .arg(incomeListString).arg(expenseListString) ;
}


// getters

QList<CombinedFeStreams::DailyInfo> CombinedFeStreams::getCombinedStreams() const
{
    return combinedStreams;
}


quint32 CombinedFeStreams::getNoOfDays() const
{
    return noOfDays;
}


quint32 CombinedFeStreams::getNoOfElementsUsed() const
{
    return noOfElementsUsed;
}

QSet<QUuid> CombinedFeStreams::getCsdContributors() const
{
    return csdContributors;
}

quint32 CombinedFeStreams::getNoOfFe() const
{
    return noOfFe;
}


