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

#ifndef GBPQMESSAGE_H
#define GBPQMESSAGE_H

#include <QString>
#include <QLocale>
#include <QCoreApplication>


/**
 * @brief Version of "Question" QMessageBox with localized buttons texts.
 *
 */
class GbpQMessage
{
public:
    GbpQMessage();

    enum class Type{INFORMATION, WARNING, QUESTION, ERROR};
    /**
     * @brief Custom QMesssage question. Support 1,2,3,4 or 5 buttons.
     * @param parent The parent QWidget.
     * @param msgType Type of message (used to select the icon displayed).
     * @param title Title of the QMessageBox.
     * @param message Message inside the QMessageBox.
     * @param buttonsText Text for each button, beginning from the left.
     * @param defaultButtonIndex Index of the default button.
     * @param escapeButtonIndex Index of the escape button.
     * @return Return index of the button selected (0 being the first) or -1 if cancel.
     */
    int static messageBoxQuestion(QWidget *parent, GbpQMessage::Type msgType, QString title,
        QString message, QStringList buttonsText, uint defaultButtonIndex, uint escapeButtonIndex);

};

#endif // GBPQMESSAGE_H
