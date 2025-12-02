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

#ifndef CSD_H
#define CSD_H
#include <QDate>
#include <QUuid>
#include <QJsonObject>
#include <QCoreApplication>
#include "util.h"

/**
 * @class Csd
 * @brief Abstract base class for Cash Stream Definitions (CSDs), which is a declarative definition
 * of an income or expense stream.
 * @details Every Csd has a unique identifier (UUID), but not necessarily unique name.
 */
class Csd
{
    Q_DECLARE_TR_FUNCTIONS(Csd)

public:
    /// @brief Maximum length for the name field.
    static constexpr int NAME_MAX_LEN = 100;
    /// @brief Maximum length for the description field.
    static constexpr int DESC_MAX_LEN = 4000;

    /**
     * @brief Enum defining the types of Csds.
     * @details PERIODIC streams have a repeating amount over a user-defined period with optional
     * early growth factor and inflation. IRREGULAR streams consist of date-amount pairs
     * without repetition or growth factors.
     */
    enum class CsdType { PERIODIC, IRREGULAR };

    /**
     * @brief Default constructor for QMap compatibility.
     * @details Initializes with a random UUID, empty name and description, PERIODIC type,
     * inactive status, income set to true, and an invalid decoration color.
     */
    Csd();

    /**
     * @brief Copy constructor.
     * @param o The Csd object to copy from.
     * @details Copies all attributes, truncating name and description if they exceed maximum
     * lengths.
     */
    Csd(const Csd& o);

    /**
     * @brief Constructor with full initialization.
     * @param id Unique identifier (UUID) for the Csd.
     * @param name Name of the Csd (truncated to NAME_MAX_LEN).
     * @param desc Description of the Csd (truncated to DESC_MAX_LEN).
     * @param streamType Type of Csd (PERIODIC or IRREGULAR).
     * @param active Whether the Csd generates financial events.
     * @param isIncome Whether the Csd represents income (true) or expense (false).
     * @param decorationColor Name's color, for display purposes (invalid if not used).
     */
    Csd(const QUuid &id, const QString &name, const QString &desc, CsdType type,
        bool active, bool isIncome, const QColor &decorationColor);

    /**
     * @brief Virtual destructor.
     * @details Ensures proper cleanup in derived classes.
     */
    virtual ~Csd();

    /**
     * @brief Assignment operator.
     * @param o The Csd object to assign from.
     * @return Reference to this Csd object.
     * @details Copies all attributes, including the UUID, and truncates name and description
     * if needed.
     */
    Csd& operator=(const Csd& o);

    /**
     * @brief Equality operator.
     * @param o The Csd object to compare with.
     * @return True if all attributes are equal, false otherwise.
     */
    bool operator==(const Csd& o) const;

    /**
     * @brief Inequality operator.
     * @param o The Csd object to compare with.
     * @return True if any attributes differ, false otherwise.
     */
    bool operator!=(const Csd& o) const;

    /**
     * @brief Serializes the object to a JSON object.
     * @param jsonObject The QJsonObject to store the serialized data.
     * @details Includes all attributes, with decorationColor omitted if invalid.
     */
    void toJson(QJsonObject &jsonObject) const;

    /**
     * @brief Evaluates if two Csds generate identical financial event lists.
     * @param o The Csd object to compare with.
     * @param diff QString describing what has been identified as a cause for non identical
     * @return True if 100% sure the Csds produce the same financial events,
     *  false if they MAY produce non identical financial event lists.
     */
    bool evaluateIfSameFeList(const Csd& o, QString& diff) const;

    /**
     * @brief Deserializes a JSON object into Csd components.
     * @param jsonObject The QJsonObject containing the serialized data.
     * @param expectedStreamType The expected CsdType (PERIODIC or IRREGULAR).
     * @param id Output parameter for the UUID.
     * @param name Output parameter for the name.
     * @param desc Output parameter for the description.
     * @param active Output parameter for the activity status.
     * @param isIncome Output parameter for the income status.
     * @param decorationColor Output parameter for the decoration color.
     * @param result Output parameter for the operation result (success or error details).
     * @details Validates JSON data and populates output parameters if successful.
     */
    static void fromJson(const QJsonObject &jsonObject, CsdType expectedStreamType,
        QUuid &id, QString &name, QString &desc, bool &active, bool &isIncome,
        QColor &decorationColor, Util::ResultOfOperation &result);

    /**
     * @brief Gets the unique identifier of the Csd.
     * @return The UUID of the Csd.
     */
    QUuid getId() const;

    /**
     * @brief Sets the unique identifier of the Csd.
     * @param newId The new UUID to set.
     */
    void setId(const QUuid &newId);

    /**
     * @brief Gets the name of the Csd.
     * @return The name of the Csd.
     */
    QString getName() const;

    /**
     * @brief Sets the name of the Csd.
     * @param newName The new name to set.
     * @note Truncates the name to NAME_MAX_LEN if necessary.
     */
    void setName(const QString &newName);

    /**
     * @brief Gets the description of the Csd.
     * @return The description of the Csd.
     */
    QString getDesc() const;

    /**
     * @brief Sets the description of the Csd.
     * @param newDesc The new description to set.
     * @note Truncates the description to DESC_MAX_LEN if necessary.
     */
    void setDesc(const QString &newDesc);

    /**
     * @brief Gets the Csd type.
     * @return The Csd type (PERIODIC or IRREGULAR).
     */
    CsdType getType() const;

    /**
     * @brief Sets the Csd type.
     * @param newStreamType The new stream type to set.
     */
    void setType(CsdType newType);

    /**
     * @brief Gets the activity status of the Csd.
     * @return True if the stream is active, false otherwise.
     */
    bool getActive() const;

    /**
     * @brief Sets the activity status of the Csd.
     * @param newActive The new activity status to set.
     */
    void setActive(bool newActive);

    /**
     * @brief Gets the income status of the Csd.
     * @return True if the Csd is income, false if it is an expense.
     */
    bool getIsIncome() const;

    /**
     * @brief Sets the income status of the Csd.
     * @param newIsIncome The new income status to set.
     */
    void setIsIncome(bool newIsIncome);

    /**
     * @brief Gets the decoration color of the Csd.
     * @return The decoration color (invalid if not used).
     */
    QColor getDecorationColor() const;

    /**
     * @brief Sets the decoration color of the Csd.
     * @param newDecorationColor The new decoration color to set.
     */
    void setDecorationColor(const QColor &newDecorationColor);

protected:
    /// @brief Unique identifier (UUID) of the Csd, set by the parent class.
    QUuid id;
    /// @brief Name of the Csd, limited to NAME_MAX_LEN characters.
    QString name;
    /// @brief Description of the Csd, limited to DESC_MAX_LEN characters.
    QString desc;
    /// @brief Type of the Csd (PERIODIC or IRREGULAR).
    CsdType type;
    /// @brief Activity status; if false, no financial events are generated.
    bool active;
    /// @brief Income status; true for income, false for expenses.
    bool isIncome;
    /**
     * @brief An optional field representing the color associated to the Csd name.
     * @details If set to "invalid color" (=QColor()), then it means there is no specific color
     * associated to this Csd. In that case, system color will be used.
     */
    QColor decorationColor;

private:

    /**
     * @brief Convert an enum Csd Type value into an int, for JSon streaming purpose.
     * @details For compatibility reason with old versions of GBP, we must always
     * have for the int representation : PERIODIC=0, IRREGULAR=1.
     * @param value The int to convert.
     * @param result Csd Type resulting from the conversion.
     * @return True if the conversion was successful, false otherwise.
     */
    static bool convertTypeFromIntToEnum(int value, Csd::CsdType& result);

    /**
     * @brief Convert an int to an enum Csd Type value, for JSon streaming purpose.
     * @details For compatibility reason with old versions of GBP, we must always
     * have for the int representation : PERIODIC=0, IRREGULAR=1.     * @param type The StreamType to convert into an int.
     * @return The conversion result.
     */
    static int convertTypeFromEnumToInt(Csd::CsdType type);
};


#endif // CSD_H
