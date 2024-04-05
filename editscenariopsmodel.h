#ifndef EDITSCENARIOPSMODEL_H
#define EDITSCENARIOPSMODEL_H

#include <QAbstractTableModel>
#include <QUuid>
#include "periodicsimplefestreamdef.h"

// Table Model for "Periodic Simple" Incomes or expenses
// Displayed columns are :
// #1 : active
// #2 : amount
// #3 : name
// #4 : period + multiplier
// #5 : validity period

class EditScenarioPsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit EditScenarioPsModel(bool isIncome,QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // specific methods
    void addReplaceStreamDef(PeriodicSimpleFeStreamDef sDef);
    void removeStreamDef(QList<QUuid> idList);
    int getPositionForId(QUuid id);
    QUuid getStreamDefIdForIndex(int index);
    PeriodicSimpleFeStreamDef getStreamDefForId(QUuid id, bool &found);

    // Getters/setters
    QMap<QUuid,PeriodicSimpleFeStreamDef> getPsStreams() const;
    void setPsStreams(const QMap<QUuid,PeriodicSimpleFeStreamDef> &newPsStreams);
    quint8 getCurrencyNoOfDecimal() const;
    void setCurrencyNoOfDecimal(quint8 newCurrencyNoOfDecimal);

private :
    QMap<QUuid,PeriodicSimpleFeStreamDef> psStreams;    // the data for the model. Key is the unique id
    QList<QUuid> sortedIdList;  // list of ordered Stream Def IDs according to the current sort criteria (buy default = ascending name)
    quint8 currencyNoOfDecimal;
    bool isIncome;
    void rebuildSortedIdList();
};

#endif // EDITSCENARIOPSMODEL_H
