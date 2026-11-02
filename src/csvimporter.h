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

#ifndef CSVIMPORTER_H
#define CSVIMPORTER_H

#include <QCoreApplication>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>

/**
 * @brief Describes one column of a CSV file to be imported.
 *
 * Optional columns must be the last ones. An empty optional cell is accepted for any type.
 */
struct CsvImportColumn {
    /**
     * @brief Type of the values of a column, which is what makes a cell valid or not.
     */
    enum class Type {
        Date,    ///< ISO 8601 date (YYYY-MM-DD). Given as a QDate.
        Number,  ///< Finite number, with "." as the decimal separator. Given as a double.
        Text     ///< Any text, possibly quoted the RFC 4180 way. Given as a QString.
    };

    QString name;           ///< Name used in the error messages (already translated)
    Type type;              ///< Type of the values
    bool optional = false;  ///< If true, the column can be absent from a line
};

/**
 * @brief One line of data of an imported CSV file.
 */
struct CsvImportRow {
    int lineNo;             ///< Line number in the file (the first line is 1)
    QStringList rawCells;   ///< Cells as found in the file, one for each declared column
    QList<QVariant> cells;  ///< Converted cells. An absent optional cell is a QVariant{}
};

/**
 * @brief Result of the import of a CSV file.
 */
struct CsvImportResult {
    bool success = false;       ///< False if the file cannot be read or has an invalid line
    QString userErrorMessage;   ///< Error for the user (rich text), empty on success
    QString logErrorMessage;    ///< Error for the log, empty on success
    bool headerSkipped = false; ///< True if the first line has been considered a header
    QString headerLine;         ///< The first line of the file if headerSkipped is true
    QList<CsvImportRow> rows;   ///< The lines of data, in the order of the file
};

/**
 * @brief Reads TAB-separated CSV files, whatever their content is about.
 *
 * The client declares its columns and gets back converted values. Checks about the meaning of
 * the values (e.g. maximum of an amount) are left to the client.
 *
 * The file must be Unicode-encoded. Each line holds one item, and empty lines are not allowed.
 * Spaces around dates and numbers are ignored, while Text cells are kept as they are.
 *
 * @par Header
 * The first line is a header if it has a valid number of fields and at least one of its cells
 * is not valid for its column. The content of a header is not restricted in any way. As a
 * consequence, a first line of data having an invalid cell cannot be told from a header : this
 * is why headerMessage() is to be shown to the user. Text columns accept anything, so a client
 * must declare at least one Date or Number column for a header to be detected.
 */
class CsvImporter
{
    Q_DECLARE_TR_FUNCTIONS(CsvImporter)

public:
    /// Maximum no of characters of the first line shown by successMessage()
    static const int HEADER_PREVIEW_MAX_LEN = 50;

    /**
     * @brief Reads a CSV file.
     * @param fileName  Name of the file.
     * @param columns   Definition of the columns. Must not be empty.
     * @return The lines of data, or the description of the first error found. No partial data
     * is returned in case of error.
     */
    static CsvImportResult importFile(const QString &fileName,
        const QList<CsvImportColumn> &columns);

    /**
     * @brief Builds the message for a successful import (rich text).
     * @details If the first line has been skipped as a header, the message also shows that line,
     * shortened to HEADER_PREVIEW_MAX_LEN characters, so the user can tell if it was a mistake.
     * @param noOfEntries  No of entries imported.
     * @param result       Result of the import.
     * @return The message.
     */
    static QString successMessage(int noOfEntries, const CsvImportResult &result);

private:
    /**
     * @brief Tells if a cell is valid for its column.
     * @param cell    The cell, as found in the file.
     * @param column  The column of the cell.
     * @return True if the cell is valid.
     */
    static bool isCellValid(const QString &cell, const CsvImportColumn &column);

    /**
     * @brief Converts a valid cell.
     * @param cell    The cell, as found in the file. Must be valid.
     * @param column  The column of the cell.
     * @return The value (see CsvImportColumn::Type), or QVariant{} for an empty optional cell.
     */
    static QVariant convertCell(const QString &cell, const CsvImportColumn &column);

    /**
     * @brief Removes the RFC 4180 quoting of a Text cell, as done by the CSV export.
     * @param cell  The cell, e.g. "say ""hi""" (with its outer quotes).
     * @return The cell without quoting, e.g. say "hi". A cell that is not between double quotes
     * is returned unchanged.
     */
    static QString unquote(const QString &cell);
};

#endif // CSVIMPORTER_H
