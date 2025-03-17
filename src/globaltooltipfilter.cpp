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


#include "globaltooltipfilter.h"
#include <qcoreevent.h>

GlobalTooltipFilter::GlobalTooltipFilter(QObject *parent)
    : QObject{parent}
{}

bool GlobalTooltipFilter::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        // Ignore tooltip events
        return true;
    }
    return QObject::eventFilter(object, event);
}
