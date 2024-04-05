#ifndef SCENARIOFELISTVIEWMODEL_H
#define SCENARIOFELISTVIEWMODEL_H

#include <QAbstractListModel>
#include <QMap>
#include <QUuid>
#include <QLocale>
#include "irregularfestreamdef.h"
#include "periodicfestreamdef.h"
#include "currencyhelper.h"



QT_BEGIN_NAMESPACE
namespace Ui { class ScenarioFeListViewModel; }
QT_END_NAMESPACE


class ScenarioFeListViewModel : public QAbstractListModel
{

    Q_OBJECT

public:
    explicit ScenarioFeListViewModel(QLocale aLocale, QObject *parent = nullptr);
    ~ScenarioFeListViewModel();

    // model's methods to implement as subclass of QAbstractListModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override ;

    // helper methods
    void newScenario(CurrencyInfo cInfo, QMap<QUuid,PeriodicFeStreamDef> newIncomesDefPeriodic, QMap<QUuid,IrregularFeStreamDef> newIncomesDefIrregular,
                    QMap<QUuid,PeriodicFeStreamDef> newExpensesDefPeriodic, QMap<QUuid,IrregularFeStreamDef> newExpensesDefIrregular);
    void addModifyPeriodicItem(PeriodicFeStreamDef p);
    void addModifyIrregularItem(IrregularFeStreamDef p);
    void removeItems(QList<QUuid> toRemove);
    QUuid duplicateItem(QUuid id, bool &found);
    void activateItems( QList<QUuid> toActivate);
    void desactivateItems( QList<QUuid> toDesactivate);
    int getRow(QUuid id, bool &found);

    // Getters / Setters
    QLocale getTheLocale() const;
    void setTheLocale(const QLocale &newTheLocale);
    CurrencyInfo getCurrInfo() const;
    void setCurrInfo(const CurrencyInfo &newCurrInfo);
    QMap<QUuid, PeriodicFeStreamDef> getIncomesDefPeriodic() const;
    QMap<QUuid, IrregularFeStreamDef> getIncomesDefIrregular() const;
    QMap<QUuid, PeriodicFeStreamDef> getExpensesDefPeriodic() const;
    bool getDisplayPeriodicItems() const;
    void setDisplayPeriodicItems(bool newDisplayPeriodicItems);
    bool getDisplayIrregularItems() const;
    void setDisplayIrregularItems(bool newDisplayIrregularItems);
    bool getDisplayActiveItems() const;
    void setDisplayActiveItems(bool newDisplayActiveItems);
    bool getDisplayInactiveItems() const;
    void setDisplayInactiveItems(bool newDisplayInactiveItems);
    bool getDisplayIncomes() const;
    void setDisplayIncomes(bool newDisplayIncomes);
    bool getDisplayExpenses() const;
    void setDisplayExpenses(bool newDisplayExpenses);

private:

    struct ItemInfo
    {
        QString displayString;
        bool isIncome;
        bool isPeriodic;
        bool isActive;
        QUuid id;
    };
    // some misc variables
    QLocale theLocale;
    CurrencyInfo currInfo;

    // data themselves
    QMap<QUuid,PeriodicFeStreamDef> incomesDefPeriodic;         // key is Stream Def ID
    QMap<QUuid,IrregularFeStreamDef> incomesDefIrregular;       // key is Stream Def ID
    QMap<QUuid,PeriodicFeStreamDef> expensesDefPeriodic;        // key is Stream Def ID
    QMap<QUuid,IrregularFeStreamDef> expensesDefIrregular;      // key is Stream Def ID

    // conveniance structures to display data fast in the ListView. All the same items for similar index.
    QList<QString> displayStringList;
    QList<QUuid> uuidList;
    QList<bool> isActiveList;

    // filters
    bool displayPeriodicItems;
    bool displayIrregularItems;
    bool displayActiveItems;
    bool displayInactiveItems;
    bool displayIncomes;
    bool displayExpenses;

    // methods
    void rebuildLists();

};

#endif // SCENARIOFELISTVIEWMODEL_H
