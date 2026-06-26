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

#ifndef GBPQMESSAGE_H
#define GBPQMESSAGE_H

#include <QString>
#include <QLocale>
#include <QCoreApplication>


/**
 * @brief Utility class for displaying customisable application-modal message boxes.
 *
 * Wraps QMessageBox to provide a uniform look across all message prompts: the
 * application font is enforced, button labels are fully caller-controlled, and
 * up to five buttons are supported. The return value is the zero-based index of
 * the button the user clicked, or -1 if the dialog was dismissed without a choice.
 */
class GbpQMessage
{
public:

    /** @brief Constructs a GbpQMessage instance (stateless; all functionality is static). */
    GbpQMessage();

    /**
     * @brief Severity / intent of the message, used to select the dialog icon.
     */
    enum class Type {
        INFORMATION, ///< Informational message (blue i icon).
        WARNING,     ///< Non-critical warning (yellow triangle icon).
        QUESTION,    ///< Decision prompt (question-mark icon).
        ERROR        ///< Critical error (red X icon).
    };

    /**
     * @brief Display a modal message box and return the index of the clicked button.
     *
     * @param parent            Parent widget; the dialog is centred over it.
     * @param msgType           Icon shown in the dialog (see Type).
     * @param title             Text displayed in the window title bar.
     * @param message           Body text of the message (rich text / HTML supported).
     * @param buttonsText       Labels for the buttons, left to right (1–5 entries).
     * @param defaultButtonIndex Zero-based index of the button activated by Enter.
     * @param escapeButtonIndex  Zero-based index of the button activated by Escape.
     * @return Zero-based index of the clicked button, or -1 if the dialog was closed
     *         without a button click.
     * @throws std::invalid_argument if @p buttonsText is empty, has more than 5 entries,
     *         or if @p defaultButtonIndex / @p escapeButtonIndex are out of range.
     */
    int static messageBoxQuestion(QWidget *parent, GbpQMessage::Type msgType,
        const QString &title, const QString &message, const QStringList &buttonsText,
        uint defaultButtonIndex, uint escapeButtonIndex);

};

#endif // GBPQMESSAGE_H
