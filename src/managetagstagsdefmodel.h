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

#ifndef MANAGETAGSTAGSDEFMODEL_H
#define MANAGETAGSTAGSDEFMODEL_H

#include <QAbstractTableModel>
#include <QLocale>
#include <QFont>
#include <qcoreapplication.h>
#include "tags.h"

class ManageTagsTagsDefModel : public QAbstractTableModel
{

    Q_DECLARE_TR_FUNCTIONS(ManageTagsTagsDefModel)

public:
    ManageTagsTagsDefModel(QLocale aLocale, QFont nameFont, QFont descFont,
        QObject *parent = nullptr);
    ~ManageTagsTagsDefModel();

    // model's methods to implement as subclass of QAbstractListModel
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Methods
    void setModelData(Tags newTags);
    QUuid getId(uint row, bool &found);
    int getRow(QUuid id, bool &found);

private:

    // compact structure holding all the info required to quickly fill the table
    struct ItemInfo
    {
        QUuid id;
        QString name;
        QString desc;
    };

    // *** Variables ***
    QLocale theLocale;
    QFont nameFont;
    QFont descFont;
    // conveniance data structure to display data fast in the table. All the same items for
    // similar index. Already sorted (by name, local-aware)
    QList<ItemInfo> itemsList;

    // *** Methods ***
    void rebuildInternalData(Tags tags);
};

#endif // MANAGETAGSTAGSDEFMODEL_H
