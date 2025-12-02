#include "festream.h"
#include "csd.h"
#include "currencyhelper.h"



FeStream::FeStream(quint32 noOfDays, QWeakPointer<Csd> csdPtr)
{
    // Must have at least 1 element
    if (noOfDays == 0) {
        throw std::invalid_argument("noOfDays is 0");
    }
    // Must not exceed the max size
    if (noOfDays > MAX_DAYS) {
        throw std::invalid_argument("noOfDays is over the maximum allowed");
    }
    // check that the Csd still exist (required at creation time)
    // auto testPtr = csdPtr.toStrongRef();
    // if(testPtr==nullptr){
    //     throw std::invalid_argument("refered Csd does not exist");
    // }

    this->csdPtr = csdPtr; // Simple weak pointer duplication
    this->noOfDays = noOfDays;

    // Allocate the array at full size.
    // resize() changes the list’s size(), not just its capacity.
    amountSet.resize(noOfDays);
    // Init the array
    amountSet.fill(-1); // all elements marked unused.
    noOfElementsUsed = 0;
}


FeStream::FeStream(QDate tomorrow, quint32 noOfDays, QWeakPointer<Csd> csdPtr,
    QMap<QDate, double> data) : FeStream(noOfDays, csdPtr)
{
    // Tomorrow must be a valid date
    if (tomorrow.isValid()==false) {
        throw std::invalid_argument("Tomorrow date is invalid");
    }

    // convert map data to index entries in amountSet
    qint64 tomorrowJulianDays = tomorrow.toJulianDay();
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        const QDate& date = it.key();
        double value = it.value();
        qint64 index = date.toJulianDay() - tomorrowJulianDays;
        if (index < 0) {
            throw std::invalid_argument("date cannot occur tomorrow");
        }
        if (index >= noOfDays) {
            throw std::invalid_argument("date is above the maximum expected");
        }
        amountSet[index] = value;
        noOfElementsUsed++;
    }
}


bool FeStream::operator==(const FeStream &o) const
{
    if ( this->amountSet!=o.amountSet ) {
        return false;
    }
    if ( this->noOfDays!=o.noOfDays ) {
        return false;
    }
    if ( this->csdPtr!=o.csdPtr ) {
        return false;
    }
    if ( this->noOfElementsUsed!=o.noOfElementsUsed ) {
        return false;
    }
    return true;
}


bool FeStream::operator!=(const FeStream &o) const
{
    return !(*this==o);
}


FeStream &FeStream::operator=(const FeStream &o)
{
    if (this != &o){                // to protect against self-assignment
        this->amountSet = o.amountSet;
        this->noOfDays = o.noOfDays;
        this->noOfElementsUsed = o.noOfElementsUsed;
        this->csdPtr = o.csdPtr; // weak reference can be copied
    }
    return *this;
}


void FeStream::reset()
{
    amountSet.fill(-1);
    noOfElementsUsed = 0;
}


void FeStream::add(quint32 day, quint64 amount)
{
    // Check out of range day
    if ( day > (noOfDays - 1) ) {
        throw std::invalid_argument("day is out of range");
    }
    // check out max value
    if ( amount > static_cast<quint64>(CurrencyHelper::maxValueAllowedForAmount()) ) {
        throw std::invalid_argument("amount is too big");
    }
    // Set
    if (amountSet[day] == -1) {
        noOfElementsUsed++;
    }
    amountSet[day] = amount;
}


void FeStream::remove(quint32 day)
{
    // Check out of range day
    if ( day > (noOfDays - 1) ) {
        throw std::invalid_argument("day is out of range");
    }
    // Set
    if (amountSet[day] != -1) {
        noOfElementsUsed--;
        amountSet[day] = -1;
    }
}


QString FeStream::toString()
{
    QStringList sl;
    sl.append(QString("MaxNoOfDays=%1 Size=%2").arg(noOfDays).arg(noOfElementsUsed));
    for(int i=0; i<amountSet.size(); i++){
        if (amountSet[i] != -1){
            sl.append(QString("v[%1]=%2").arg(i).arg(amountSet[i]));
        }
    }
    return sl.join(" , ");
}


QList<qint64> FeStream::getAmountSet() const
{
    return amountSet;
}


QWeakPointer<Csd> FeStream::getCsdPtr() const
{
    return csdPtr;
}

quint32 FeStream::getNoOfElementsUsed() const
{
    return noOfElementsUsed;
}

void FeStream::setCsdPtr(QWeakPointer<Csd> newCsdPtr)
{
    csdPtr = newCsdPtr;
}

quint32 FeStream::getNoOfDays() const
{
    return noOfDays;
}
