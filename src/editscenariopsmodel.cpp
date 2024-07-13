#include "editscenariopsmodel.h"
#include "currencyhelper.h"
#include "util.h"
#include <QFont>
#include <QColor>
#include <QList>


EditScenarioPsModel::EditScenarioPsModel(bool isIncome,QObject *parent): QAbstractTableModel(parent)
{
    this->isIncome = isIncome;
}


QVariant EditScenarioPsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return QString("Active");
        case 1:
            return QString("Amount");
        case 2:
            return QString("Name");
        case 3:
            return QString("Period");
        case 4:
            return QString("Validity");
        }
    }
    return QVariant();
}


int EditScenarioPsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return psStreams.size();
}


int EditScenarioPsModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 5;
}


QVariant EditScenarioPsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role==Qt::DisplayRole){
        int row = index.row();
        int col = index.column();
        if ( row <= (sortedIdList.size()-1) ){
            QUuid key = sortedIdList.at(row);
            PeriodicSimpleFeStreamDef streamDef = psStreams.value(key); // must be always found
            if(col==0){ // active
                if (streamDef.getActive()) {
                    return "Yes";
                } else {
                    return "No";
                }

            } else if (col==1){ // amount
                return CurrencyHelper::quint64ToDoubleString(streamDef.getAmount(),currencyNoOfDecimal,QLocale::system());
            } else if (col==2){ // name (elide to 50 chars)
                return Util::elideText(streamDef.getName(),50,true);
            } else if (col==3){ // period + multiplier
                return streamDef.getStringForPeriod();
            } else if (col==4){ // validity period
                return streamDef.getValidityRange().toString();
            }
        }

    } else if (role==Qt::TextAlignmentRole){
        if (index.column()==1) {                                // amount
            return int(Qt::AlignRight | Qt::AlignVCenter);
        } else if (index.column()==2) {
            return int(Qt::AlignLeft | Qt::AlignVCenter);       // name
        } else {
             return int(Qt::AlignHCenter | Qt::AlignVCenter);   // else
        }
    } else if(role == Qt::FontRole) {
        QFont font;
        if(index.column() == 1){
            font.setStyleHint(QFont::Monospace);
            return font;
        }
    } else  if (role == Qt::ForegroundRole) {
        // Check if the index corresponds to the desired column
        if (index.column() == 1) {  //amount
            // Create a QColor object with the desired color
            if(isIncome){
                QColor color(0,192,0);
                return QVariant(color);
            } else{
                QColor color(192,0,0);
                return QVariant(color);
            }
        }
    }

    return QVariant();
}


// add a new /replace an existing PeriodicSimpleFeStreamDef in the list of known PeriodicSimpleFeStreamDef
void EditScenarioPsModel::addReplaceStreamDef(PeriodicSimpleFeStreamDef sDef)
{
    emit beginResetModel();
    psStreams.insert(sDef.getId(), sDef); // replaced if existing, added if new
    rebuildSortedIdList();
    emit endResetModel();
}


void EditScenarioPsModel::removeStreamDef(QList<QUuid> idList)
{
    emit beginResetModel();
    foreach (QUuid id, idList) {
        psStreams.remove(id); // delete entry if it exists
    }
    rebuildSortedIdList();
    emit endResetModel();
}


// Return in sortedIdList the index of the Stream Def identified buy "id"
// -1 if not found
int EditScenarioPsModel::getPositionForId(QUuid id)
{
    return sortedIdList.indexOf(id);
}


// get the StreamDef ID for an index corresponding to position inside the sortedIdList
QUuid EditScenarioPsModel::getStreamDefIdForIndex(int index)
{
    if ( (index<0) || (index>(sortedIdList.length()-1)) ) {
        return QUuid(); // null UUID
    }
    return sortedIdList.at(index);
}


// Get the StreamDef associated to this ID
PeriodicSimpleFeStreamDef EditScenarioPsModel::getStreamDefForId(QUuid id, bool& found)
{
    found = false;
    if (!psStreams.contains(id)) {
        return PeriodicSimpleFeStreamDef(); // dummy
    }
    found = true;
    return psStreams.value(id);
}


// Rebuild sortedIdList, according to the current sorting criteria (by default : ascending name)
void EditScenarioPsModel::rebuildSortedIdList()
{
    QList<PeriodicSimpleFeStreamDef> myList = psStreams.values();
    std::sort(myList.begin(), myList.end(), [](const PeriodicSimpleFeStreamDef& b1, const PeriodicSimpleFeStreamDef& b2) {
        return b1.getName() < b2.getName();
    });
    sortedIdList.clear();
    foreach (PeriodicSimpleFeStreamDef sd,myList ) {
        sortedIdList.append(sd.getId());
    }
}





// Getters / Setters
QMap<QUuid, PeriodicSimpleFeStreamDef> EditScenarioPsModel::getPsStreams() const
{
    return psStreams;
}

// change the data
void EditScenarioPsModel::setPsStreams(const QMap<QUuid,PeriodicSimpleFeStreamDef> &newPsStreams)
{
    // rebuild the sorting list of Stream Df based on latest sorting criteria (for now : ascending name, later : may criteria possible)
    emit beginResetModel();
    psStreams = newPsStreams;
    rebuildSortedIdList();
    emit endResetModel();
}

quint8 EditScenarioPsModel::getCurrencyNoOfDecimal() const
{
    return currencyNoOfDecimal;
}


// this will affect the view
void EditScenarioPsModel::setCurrencyNoOfDecimal(quint8 newCurrencyNoOfDecimal)
{
    emit beginResetModel();
    currencyNoOfDecimal = newCurrencyNoOfDecimal;
    emit endResetModel();
}

