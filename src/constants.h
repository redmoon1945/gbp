/*
 *  Copyright (C) 2024-2026 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
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

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "qtypes.h"
#include <QDate>

namespace Constants {

/**
 * @brief When setting the maximum duration for Financial Event generation by a scenario, this value
 * is the smallest one allowed, in years. It starts from "tomorrow" as defined by GBP.
 */
inline constexpr quint8 MIN_DURATION_FE_GENERATION = 1;

/**
 * @brief When setting the maximum duration for Financial Event generation by a scenario, this value
 * is the greatest one allowed, in years. It starts from "tomorrow" as defined by GBP.
 */
inline constexpr quint8 MAX_DURATION_FE_GENERATION = 100;

/**
 * @brief When setting the maximum duration for Financial Event generation by a scenario, this value
 * is the default one suggested to the user, in years. It starts from "tomorrow" as defined by GBP.
 */
inline constexpr quint8 DEFAULT_DURATION_FE_GENERATION = 25;

/**
 * @brief Name of the application.
 */
inline const QString APP_NAME = QString("graphical-budget-planner");

/**
 * @brief Version of the application.
 */
inline const QString APP_VERSION  = QString("1.8.1");


}


#endif // CONSTANTS_H
