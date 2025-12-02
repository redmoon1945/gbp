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

#include "gbpqmessage.h"
#include <qmessagebox.h>

GbpQMessage::GbpQMessage() {}


int GbpQMessage::messageBoxQuestion(QWidget *parent, GbpQMessage::Type msgType, QString title,
    QString message, QStringList buttonsText, uint defaultButtonIndex, uint escapeButtonIndex)
{
    // check integrity of parameters
    if (buttonsText.size() < 1) {
        throw std::invalid_argument("Custom Message Box : buttonsText "
            "must contain at least one item");
    }
    if (buttonsText.size() > 5) {
        throw std::invalid_argument("Custom Message Box : buttonsText "
            "exceeds the max no of buttons supported (5)");
    }
    if (buttonsText.size() <= defaultButtonIndex) {
        throw std::invalid_argument("Custom Message Box : invalid defaultButtonIndex");
    }
    if (buttonsText.size() <= escapeButtonIndex) {
        throw std::invalid_argument("Custom Message Box : invalid escapeButtonIndex");
    }
    // Display the messagebox
    QMessageBox msgBox(parent);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setTextFormat(Qt::RichText);

    QList<QPushButton *> buttons; // the custom buttons we are going to create, in order
    for (int var = 0; var < buttonsText.size(); ++var) {
        QPushButton* b = msgBox.addButton(buttonsText.at(var), QMessageBox::ActionRole);
        buttons.append(b);
    }
    msgBox.setDefaultButton(buttons.at(defaultButtonIndex));
    msgBox.setEscapeButton((QAbstractButton *)(buttons.at(escapeButtonIndex)));

    // choose the icon to display
    switch (msgType) {
        case GbpQMessage::Type::INFORMATION:
            msgBox.setIcon(QMessageBox::Information);
            break;
        case GbpQMessage::Type::WARNING:
            msgBox.setIcon(QMessageBox::Warning);
            break;
        case GbpQMessage::Type::ERROR:
            msgBox.setIcon(QMessageBox::Critical);
            break;
        case GbpQMessage::Type::QUESTION:
            msgBox.setIcon(QMessageBox::Question);
            break;
        default:
            msgBox.setIcon(QMessageBox::Information);
            break;
    }



    msgBox.exec();

    // process the answer
    QAbstractButton *clickedButton = msgBox.clickedButton(); // bad QT design...should be PushButton
    if (clickedButton == nullptr){
        return -1;  // user escape the dialog
    }
    for (int var = 0; var < buttons.size(); ++var) {
        if (clickedButton == (QAbstractButton *)(buttons.at(var)) ) {
            return var;
        }
    }
    // should never happen
    return -1;
}
