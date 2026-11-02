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

#include "csvimporter.h"
#include "gbplogger.h"
#include <QDate>
#include <QFile>
#include <QTextStream>
#include <cmath>


CsvImportResult CsvImporter::importFile(const QString &fileName,
    const QList<CsvImportColumn> &columns)
{
    CsvImportResult result;

    QFile file(fileName);   // file is closed automatically by Qt
    if (!file.exists()) {
        result.userErrorMessage = tr("File %1 does not exist").arg(fileName);
        result.logErrorMessage = QString("File %1 does not exist").arg(REDACT(fileName));
        return result;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.userErrorMessage = tr("Cannot open file %1 in read-only mode").arg(fileName);
        result.logErrorMessage = QString("Cannot open file %1 in read-only mode")
            .arg(REDACT(fileName));
        return result;
    }

    // Valid no of fields for a line
    int maxFields = columns.size();
    int minFields = 0;
    for (const CsvImportColumn &column : columns) {
        if (!column.optional) {
            minFields++;
        }
    }

    QList<CsvImportRow> rows;
    QTextStream stream(&file);
    stream.setAutoDetectUnicode(true);
    int lineNo = 0;
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        lineNo++;

        // Check the no of fields. Spaces are not removed from the line, since they can be part
        // of a Text cell
        QStringList tokens = line.split('\t');
        if ( (tokens.size() < minFields) || (tokens.size() > maxFields) ) {
            QString expected = (minFields==maxFields) ? (QString::number(maxFields))
                : (tr("%1 to %2").arg(minFields).arg(maxFields));
            result.userErrorMessage = tr("Bad format for line no %1 (expected %2 TAB-separated "
                "fields, got %3).").arg(lineNo).arg(expected).arg(tokens.size());
            result.logErrorMessage = QString("Bad format for line no %1 (expected %2 to %3 "
                "fields, got %4).").arg(lineNo).arg(minFields).arg(maxFields).arg(tokens.size());
            return result;
        }

        // First line : a cell not valid for its column means this is a header
        if (lineNo == 1) {
            bool allValid = true;
            for (int i = 0; i < tokens.size(); i++) {
                allValid = allValid && isCellValid(tokens[i], columns[i]);
            }
            if (!allValid) {
                result.headerSkipped = true;
                result.headerLine = line;
                continue;
            }
        }

        // Any other line : all the cells must be valid
        CsvImportRow row;
        row.lineNo = lineNo;
        for (int i = 0; i < columns.size(); i++) {
            QString cell = (i < tokens.size()) ? (tokens[i]) : (QString());
            if ( !isCellValid(cell, columns[i]) ) {
                if (columns[i].type == CsvImportColumn::Type::Date) {
                    result.userErrorMessage = tr("Date \"%1\" is invalid at line no %2.")
                        .arg(cell.toHtmlEscaped()).arg(lineNo);
                    result.logErrorMessage = QString("Date \"%1\" is invalid at line no %2.")
                        .arg(cell).arg(lineNo);
                } else {
                    result.userErrorMessage = tr("%1 \"%2\" is not a valid number at line no %3.")
                        .arg(columns[i].name, cell.toHtmlEscaped()).arg(lineNo);
                    result.logErrorMessage = QString("Value \"%1\" of column %2 is not a valid "
                        "number at line no %3.").arg(cell).arg(i+1).arg(lineNo);
                }
                result.headerSkipped = false;
                result.headerLine.clear();
                return result;
            }
            row.rawCells.append(cell);
            row.cells.append(convertCell(cell, columns[i]));
        }
        rows.append(row);
    }

    result.rows = rows;
    result.success = true;
    return result;
}


QString CsvImporter::successMessage(int noOfEntries, const CsvImportResult &result)
{
    QString message = tr("Import succeeded, %1 entries have been imported.").arg(noOfEntries);
    if (result.headerSkipped) {
        QString preview = result.headerLine;
        preview.replace('\t', ", ");
        if (preview.size() > HEADER_PREVIEW_MAX_LEN) {
            preview = preview.left(HEADER_PREVIEW_MAX_LEN);
            if (preview.back().isHighSurrogate()) {
                preview.chop(1);    // do not cut a character in two
            }
            preview.append("...");
        }
        message += "<br><br>" + tr("First line ignored as a header : %1")
            .arg(preview.toHtmlEscaped());
    }
    return message;
}


bool CsvImporter::isCellValid(const QString &cell, const CsvImportColumn &column)
{
    QString trimmed = cell.trimmed();
    if ( (column.type != CsvImportColumn::Type::Text) && column.optional && trimmed.isEmpty() ) {
        return true;
    }
    switch (column.type) {
        case CsvImportColumn::Type::Date:
            return QDate::fromString(trimmed, Qt::ISODate).isValid();
        case CsvImportColumn::Type::Number: {
            // NaN and infinite values are accepted by toDouble(), but are not valid numbers
            bool ok;
            double d = trimmed.toDouble(&ok);
            return ok && std::isfinite(d);
        }
        case CsvImportColumn::Type::Text:
            return true;
    }
    return false;
}


QVariant CsvImporter::convertCell(const QString &cell, const CsvImportColumn &column)
{
    QString trimmed = cell.trimmed();
    if ( (column.type != CsvImportColumn::Type::Text) && trimmed.isEmpty() ) {
        return QVariant{};
    }
    switch (column.type) {
        case CsvImportColumn::Type::Date:
            return QDate::fromString(trimmed, Qt::ISODate);
        case CsvImportColumn::Type::Number:
            return trimmed.toDouble();
        case CsvImportColumn::Type::Text:
            return unquote(cell);
    }
    return QVariant{};
}


QString CsvImporter::unquote(const QString &cell)
{
    // RFC 4180 quoting, as done by the CSV export : "say ""hi""" stands for say "hi"
    if (cell.size() < 2 || !cell.startsWith('"') || !cell.endsWith('"')) {
        return cell;
    }
    QString unquoted = cell.mid(1, cell.size() - 2);
    unquoted.replace("\"\"", "\"");
    return unquoted;
}
