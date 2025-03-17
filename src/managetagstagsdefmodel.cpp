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


#include "managetagstagsdefmodel.h"

ManageTagsTagsDefModel::ManageTagsTagsDefModel(QLocale aLocale, QFont nameFont, QFont descFont,
    QObject *parent) : QAbstractTableModel{parent}
{
    this->theLocale = aLocale;
    this->nameFont = nameFont;
    this->descFont = descFont;
}


ManageTagsTagsDefModel::~ManageTagsTagsDefModel()
{
}


QVariant ManageTagsTagsDefModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case 0:
                return tr("Name");
            case 1:
                return tr("Description");
        }
    }
    return QVariant();
}


int ManageTagsTagsDefModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return itemsList.size();
}


int ManageTagsTagsDefModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 2;
}


QVariant ManageTagsTagsDefModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int row = index.row();
    int col = index.column();

    ItemInfo info = itemsList.at(row);
    if (role==Qt::DisplayRole){
        if ( row <= (itemsList.size()-1) ){
            if (col==0){            // *** name ***
                return info.name;
            } else if (col==1){     // *** desc ***
                return info.desc;
            }
        }
    } else if (role==Qt::TextAlignmentRole){
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    } else if (role==Qt::FontRole){
        if (col==0) {
            return nameFont;
        } else {
            return descFont;
        }
    }

    return QVariant();
}


void ManageTagsTagsDefModel::setModelData(Tags newTags)
{
    rebuildInternalData(newTags);
}


QUuid ManageTagsTagsDefModel::getId(uint row, bool &found)
{
    found = false;
    if( row < itemsList.size() ){
        found = true;
        return itemsList[row].id;
    } else {
        return QUuid();
    }
}


int ManageTagsTagsDefModel::getRow(QUuid id, bool &found)
{
    found = false;
    int row=0;
    foreach(ItemInfo item, itemsList){
        if (item.id==id){
            found = true;
            return row;
        }
        row++;
    }
    return -1;
}


void ManageTagsTagsDefModel::rebuildInternalData(Tags tags)
{
    emit beginResetModel();

    // we need this in order to sort in a locale-aware way
    class MyString : public QString {
    public:
        MyString(const QString& text) : QString(text) {
        }
        bool operator<(const MyString& other) const {
            if ( QString::localeAwareCompare(*this,other) < 0 ){
                return true;
            } else {
                return false;
            }
        }
    };

    // intermediate data structure, used just to sort items by name. Name must be unique,
    QMap<MyString,ItemInfo> proxyList;

    // clear the items list
    itemsList.clear();

    QSet<Tag> tagSet = tags.getTags();
    foreach(Tag tag, tagSet ){
        ItemInfo info;
        info.id = tag.getId();
        info.name = tag.getName();
        info.desc = tag.getDescription().replace("\n"," ");
        MyString sortKey = MyString(info.name);
        proxyList.insert(sortKey, info);
    }

    // rebuild the itemsList
    foreach(QString name, proxyList.keys()){
        ItemInfo info = proxyList.value(name);
        itemsList.append(info);
    }

    emit endResetModel();

}
