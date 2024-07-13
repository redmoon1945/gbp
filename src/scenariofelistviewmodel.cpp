#include "scenariofelistviewmodel.h"
#include <QMap>

ScenarioFeListViewModel::ScenarioFeListViewModel(QLocale aLocale, QObject *parent )
    : QAbstractListModel(parent)
{
    this->theLocale = aLocale;


}


ScenarioFeListViewModel::~ScenarioFeListViewModel()
{

}


int ScenarioFeListViewModel::rowCount(const QModelIndex &parent) const
{
    return displayStringList.size();
}


QVariant ScenarioFeListViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role==Qt::DisplayRole){
        int row = index.row();
        return(displayStringList.at(row));
    }

    return QVariant();
}


// Everytime scenario changes, this must be called (including first time set)
// Filters are untouched.
void ScenarioFeListViewModel::newScenario(CurrencyInfo cInfo, QMap<QUuid,PeriodicFeStreamDef> newIncomesDefPeriodic, QMap<QUuid,IrregularFeStreamDef> newIncomesDefIrregular,
                    QMap<QUuid,PeriodicFeStreamDef> newExpensesDefPeriodic, QMap<QUuid,IrregularFeStreamDef> newExpensesDefIrregular)
{
    currInfo = cInfo;
    incomesDefPeriodic = newIncomesDefPeriodic;
    incomesDefIrregular = newIncomesDefIrregular;
    expensesDefPeriodic = newExpensesDefPeriodic;
    expensesDefIrregular = newExpensesDefIrregular;

    rebuildLists();
}


void ScenarioFeListViewModel::addModifyPeriodicItem(PeriodicFeStreamDef p)
{
    if(p.getIsIncome()){
        incomesDefPeriodic.insert(p.getId(),p);
    } else{
        expensesDefPeriodic.insert(p.getId(),p);
    }
    rebuildLists();
}


void ScenarioFeListViewModel::addModifyIrregularItem(IrregularFeStreamDef p)
{
    if(p.getIsIncome()){
        incomesDefIrregular.insert(p.getId(),p);
    } else{
        expensesDefIrregular.insert(p.getId(),p);
    }
    rebuildLists();
}


void ScenarioFeListViewModel::removeItems(QList<QUuid> toRemove)
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


QUuid ScenarioFeListViewModel::duplicateItem(QUuid id, bool &found)
{
    found = false;

    // find the item and duplicate
    if (incomesDefPeriodic.contains(id)) {
        found = true;
        PeriodicFeStreamDef p = incomesDefPeriodic.value(id);
        PeriodicFeStreamDef p2 = p.duplicate();
        incomesDefPeriodic.insert(p2.getId(),p2);
        return p2.getId();
    } else if (incomesDefIrregular.contains(id)) {
        found = true;
        IrregularFeStreamDef p = incomesDefIrregular.value(id);
        IrregularFeStreamDef p2 = p.duplicate();
        incomesDefIrregular.insert(p2.getId(),p2);
        return p2.getId();
    } else if (expensesDefPeriodic.contains(id)) {
        found = true;
        PeriodicFeStreamDef p = expensesDefPeriodic.value(id);
        PeriodicFeStreamDef p2 = p.duplicate();
        expensesDefPeriodic.insert(p2.getId(),p2);
        return p2.getId();
    } else if (expensesDefIrregular.contains(id)) {
        found = true;
        IrregularFeStreamDef p = expensesDefIrregular.value(id);
        IrregularFeStreamDef p2 = p.duplicate();
        expensesDefIrregular.insert(p2.getId(),p2);
        return p2.getId();
    }

    return QUuid(); // dummy value as element has not been found

}


void ScenarioFeListViewModel::activateItems(QList<QUuid> toActivate)
{
    foreach(QUuid id, toActivate){
        if (incomesDefPeriodic.contains(id)) {
            PeriodicFeStreamDef p = incomesDefPeriodic.value(id);
            p.setActive(true);
            incomesDefPeriodic.insert(p.getId(),p);
        }
        if (incomesDefIrregular.contains(id)) {
            IrregularFeStreamDef p = incomesDefIrregular.value(id);
            p.setActive(true);
            incomesDefIrregular.insert(p.getId(),p);
        }
        if (expensesDefPeriodic.contains(id)) {
            PeriodicFeStreamDef p = expensesDefPeriodic.value(id);
            p.setActive(true);
            expensesDefPeriodic.insert(p.getId(),p);
        }
        if (expensesDefIrregular.contains(id)) {
            IrregularFeStreamDef p = expensesDefIrregular.value(id);
            p.setActive(true);
            expensesDefIrregular.insert(p.getId(),p);
        }
    }

    rebuildLists();
}


void ScenarioFeListViewModel::desactivateItems(QList<QUuid> toDesactivate)
{
    foreach(QUuid id, toDesactivate){
        if (incomesDefPeriodic.contains(id)) {
            PeriodicFeStreamDef p = incomesDefPeriodic.value(id);
            p.setActive(false);
            incomesDefPeriodic.insert(p.getId(),p);
        }
        if (incomesDefIrregular.contains(id)) {
            IrregularFeStreamDef p = incomesDefIrregular.value(id);
            p.setActive(false);
            incomesDefIrregular.insert(p.getId(),p);
        }
        if (expensesDefPeriodic.contains(id)) {
            PeriodicFeStreamDef p = expensesDefPeriodic.value(id);
            p.setActive(false);
            expensesDefPeriodic.insert(p.getId(),p);
        }
        if (expensesDefIrregular.contains(id)) {
            IrregularFeStreamDef p = expensesDefIrregular.value(id);
            p.setActive(false);
            expensesDefIrregular.insert(p.getId(),p);
        }
    }

    rebuildLists();
}


int ScenarioFeListViewModel::getRow(QUuid id, bool &found)
{
    found = false;
    if (uuidList.contains(id)==false ){
        return -1;
    }
    found = true;
    return uuidList.indexOf(id);
}




// rebuild data structures and update the view
void ScenarioFeListViewModel::rebuildLists()
{

    emit beginResetModel();

    // intermediate data structure, used just to sort items by name
    QMap<QString,ItemInfo> proxyList;

    // clear the lists
    displayStringList.clear();
    uuidList.clear();
    isActiveList.clear();

    if (displayIncomes){
        if (displayPeriodicItems) {
            foreach(PeriodicFeStreamDef item, incomesDefPeriodic.values() ){
                bool active = item.getActive();
                if ( (active==true && displayActiveItems==true) || (active==false && displayInactiveItems==true) ) {
                    ItemInfo info;
                    info.id = item.getId();
                    info.isActive = item.getActive();
                    info.isIncome = true;
                    info.isPeriodic = true;
                    info.displayString = item.toStringForDisplay(currInfo,theLocale);
                    proxyList.insert(item.getName(), info);
                }
            }
        }
        if (displayIrregularItems) {
            foreach(IrregularFeStreamDef item, incomesDefIrregular.values() ){
                bool active = item.getActive();
                if ( (active==true && displayActiveItems==true) || (active==false && displayInactiveItems==true) ) {
                    ItemInfo info;
                    info.id = item.getId();
                    info.isActive = item.getActive();
                    info.isIncome = true;
                    info.isPeriodic = false;
                    info.displayString = item.toStringForDisplay(currInfo,theLocale);
                    proxyList.insert(item.getName(), info);
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
                    info.isActive = item.getActive();
                    info.isIncome = false;
                    info.isPeriodic = true;
                    info.displayString = item.toStringForDisplay(currInfo,theLocale);
                    proxyList.insert(item.getName(), info);
                }
            }
        }
        if (displayIrregularItems) {
            foreach(IrregularFeStreamDef item, expensesDefIrregular.values() ){
                bool active = item.getActive();
                if ( (active==true && displayActiveItems==true) || (active==false && displayInactiveItems==true) ) {
                    ItemInfo info;
                    info.id = item.getId();
                    info.isActive = item.getActive();
                    info.isIncome = false;
                    info.isPeriodic = false;
                    info.displayString = item.toStringForDisplay(currInfo,theLocale);
                    proxyList.insert(item.getName(), info);
                }
            }
        }
    }

    // rebuild the lists
    foreach(QString name, proxyList.keys()){
        ItemInfo info = proxyList.value(name);
        displayStringList.append(info.displayString);
        uuidList.append(info.id);
        isActiveList.append(info.isActive);
    }

    emit endResetModel();

}



// *** Getters / Setters ***

QLocale ScenarioFeListViewModel::getTheLocale() const
{
    return theLocale;
}

void ScenarioFeListViewModel::setTheLocale(const QLocale &newTheLocale)
{
    theLocale = newTheLocale;
}

CurrencyInfo ScenarioFeListViewModel::getCurrInfo() const
{
    return currInfo;
}

void ScenarioFeListViewModel::setCurrInfo(const CurrencyInfo &newCurrInfo)
{
    currInfo = newCurrInfo;
}

QMap<QUuid, PeriodicFeStreamDef> ScenarioFeListViewModel::getIncomesDefPeriodic() const
{
    return incomesDefPeriodic;
}

QMap<QUuid, IrregularFeStreamDef> ScenarioFeListViewModel::getIncomesDefIrregular() const
{
    return incomesDefIrregular;
}

QMap<QUuid, PeriodicFeStreamDef> ScenarioFeListViewModel::getExpensesDefPeriodic() const
{
    return expensesDefPeriodic;
}

bool ScenarioFeListViewModel::getDisplayPeriodicItems() const
{
    return displayPeriodicItems;
}

void ScenarioFeListViewModel::setDisplayPeriodicItems(bool newDisplayPeriodicItems)
{
    displayPeriodicItems = newDisplayPeriodicItems;
}

bool ScenarioFeListViewModel::getDisplayIrregularItems() const
{
    return displayIrregularItems;
}

void ScenarioFeListViewModel::setDisplayIrregularItems(bool newDisplayIrregularItems)
{
    displayIrregularItems = newDisplayIrregularItems;
}

bool ScenarioFeListViewModel::getDisplayActiveItems() const
{
    return displayActiveItems;
}

void ScenarioFeListViewModel::setDisplayActiveItems(bool newDisplayActiveItems)
{
    displayActiveItems = newDisplayActiveItems;
}

bool ScenarioFeListViewModel::getDisplayInactiveItems() const
{
    return displayInactiveItems;
}

void ScenarioFeListViewModel::setDisplayInactiveItems(bool newDisplayInactiveItems)
{
    displayInactiveItems = newDisplayInactiveItems;
}

bool ScenarioFeListViewModel::getDisplayIncomes() const
{
    return displayIncomes;
}

void ScenarioFeListViewModel::setDisplayIncomes(bool newDisplayIncomes)
{
    displayIncomes = newDisplayIncomes;
}

bool ScenarioFeListViewModel::getDisplayExpenses() const
{
    return displayExpenses;
}

void ScenarioFeListViewModel::setDisplayExpenses(bool newDisplayExpenses)
{
    displayExpenses = newDisplayExpenses;
}


