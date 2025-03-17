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


#ifndef MANAGETAGSCSDINFO_H
#define MANAGETAGSCSDINFO_H

#include <QUuid>
#include <QString>
#include <qcolor.h>

// Structure shared by ManageTags modules and intended to carry useful information
// for a CSD (Cash Stream Definition)

namespace managetags {

struct CsdItem {
        QUuid id;
        QString name; // could be empty...
        bool isIncome;
        QColor color; // name color when globally enabled. Color is invalid if not used.
        bool operator==(const CsdItem &o) const;
    };

    // Hash function for cases where FsdItem is used as a key for QSet or QHash.
    size_t qHash(const managetags::CsdItem &t, size_t seed = 0);

}



#endif // MANAGETAGSCSDINFO_H
