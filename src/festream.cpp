#include "festream.h"
#include "csd.h"
#include "currencyhelper.h"



FeStream::FeStream(quint32 noOfDays, QWeakPointer<Csd> csdPtr, const QDate& firstDate)
{
    // Must have at least 1 element
    if (noOfDays == 0) {
        throw std::invalid_argument(QString("%1: noOfDays is 0").arg(Q_FUNC_INFO).toStdString());
    }
    // Must not exceed the max size
    if (noOfDays > MAX_DAYS) {
        throw std::invalid_argument(QString("%1: noOfDays is over the maximum allowed")
            .arg(Q_FUNC_INFO).toStdString());
    }
    // check that firstDate is valid
    if (!firstDate.isValid()) {
        throw std::invalid_argument(QString("%1: firstDate is invalid")
            .arg(Q_FUNC_INFO).toStdString());
    }

    // check that the Csd still exist (required at creation time)
    // auto testPtr = csdPtr.toStrongRef();
    // if(testPtr==nullptr){
    //     throw std::invalid_argument("refered Csd does not exist");
    // }

    this->csdPtr = csdPtr; // Simple weak pointer duplication. May be invalid at this point.
    this->noOfDays = noOfDays;
    this->firstDate = firstDate;

    // Allocate the array at full size.
    // resize() changes the list's size(), not just its capacity.
    amountSet.resize(noOfDays);

    // Init the array
    amountSet.fill(-1); // all elements marked unused.
    noOfElementsUsed = 0;
}


FeStream::FeStream(QDate tomorrow, quint32 noOfDays, QWeakPointer<Csd> csdPtr,
    const QMap<QDate, double> &data) : FeStream(noOfDays, csdPtr, tomorrow)
{
    // Tomorrow must be a valid date (already checked in delegating constructor)

    // convert map data to index entries in amountSet
    qint64 tomorrowJulianDays = tomorrow.toJulianDay();
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        const QDate& date = it.key();
        double value = it.value();
        qint64 index = date.toJulianDay() - tomorrowJulianDays;
        if (index < 0) {
            throw std::invalid_argument(QString("%1: date cannot occur tomorrow")
                .arg(Q_FUNC_INFO).toStdString());
        }
        if (index >= noOfDays) {
            throw std::invalid_argument(QString("%1: date is above the maximum expected")
                .arg(Q_FUNC_INFO).toStdString());
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
    if ( this->firstDate!=o.firstDate ) {
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
        this->firstDate = o.firstDate;
    }
    return *this;
}


void FeStream::reset()
{
    amountSet.fill(-1);
    noOfElementsUsed = 0;
}


qint64 FeStream::get(quint32 day) const
{
    if (day >= noOfDays) {
        throw std::out_of_range(QString("%1: day is out of range").arg(Q_FUNC_INFO).toStdString());
    }
    return amountSet[day];
}


qint64 FeStream::get(const QDate& date) const
{
    return amountSet[dateToIndex(date)];
}


void FeStream::set(quint32 day, quint64 amount)
{
    // Check out of range day
    if (day >= noOfDays) {
        throw std::out_of_range(QString("%1: day is out of range").arg(Q_FUNC_INFO).toStdString());
    }
    // check out max value
    if ( amount > static_cast<quint64>(CurrencyHelper::maxValueAllowedForAmount()) ) {
        throw std::invalid_argument(QString("%1: amount is too big")
            .arg(Q_FUNC_INFO).toStdString());
    }
    // Set
    if (amountSet[day] == -1) {
        noOfElementsUsed++;
    }
    amountSet[day] = amount;
}


void FeStream::set(const QDate& date, quint64 amount)
{
    set(dateToIndex(date), amount);
}


void FeStream::remove(quint32 day)
{
    // Check out of range day
    if (day >= noOfDays) {
        throw std::out_of_range(QString("%1: day is out of range").arg(Q_FUNC_INFO).toStdString());
    }
    // Set
    if (amountSet[day] != -1) {
        noOfElementsUsed--;
        amountSet[day] = -1;
    }
}


void FeStream::remove(const QDate& date)
{
    remove(dateToIndex(date));
}


bool FeStream::contains(quint32 day) const
{
    if (day >= noOfDays) {
        throw std::out_of_range(QString("%1: day is out of range").arg(Q_FUNC_INFO).toStdString());
    }
    return amountSet[day] != -1;
}


bool FeStream::contains(const QDate& date) const
{
    return amountSet[dateToIndex(date)] != -1;
}


bool FeStream::isEmpty() const
{
    return noOfElementsUsed == 0;
}


quint32 FeStream::size() const
{
    return noOfDays;
}


quint32 FeStream::count() const
{
    return noOfElementsUsed;
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


quint32 FeStream::dateToIndex(const QDate& date) const
{
    if (!date.isValid()) {
        throw std::invalid_argument(QString("%1: date is invalid").arg(Q_FUNC_INFO).toStdString());
    }

    qint64 daysDiff = firstDate.daysTo(date);

    if (daysDiff < 0) {
        throw std::invalid_argument(QString("%1: date is before firstDate")
            .arg(Q_FUNC_INFO).toStdString());
    }

    if (daysDiff >= static_cast<qint64>(noOfDays)) {
        throw std::invalid_argument(QString("%1: date is beyond the range of this stream")
            .arg(Q_FUNC_INFO).toStdString());
    }

    return static_cast<quint32>(daysDiff);
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

QDate FeStream::getFirstDate() const
{
    return firstDate;
}

QDate FeStream::getLastDate() const
{
    return firstDate.addDays(noOfDays - 1);
}


