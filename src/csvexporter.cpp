#include "csvexporter.h"
#include "gbpcontroller.h"
#include "gbplogger.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QObject>

CsvExportResult CsvExporter::exportToCsv(
    const QString& operationName,
    const QList<CsvColumnDescriptor>& columns,
    const QList<QList<QVariant>>& data,
    const QLocale& locale,
    const CurrencyInfo& currInfo,
    QChar separator)
{
    CsvExportResult result;
    result.rowsExported = 0;

    LOG_INFO(QString("Initiating CSV export '%1' with %2 columns and %3 rows")
        .arg(operationName)
        .arg(columns.size())
        .arg(data.size()));

    // Validate column count
    if (columns.isEmpty()) {
        result.status = CsvExportResult::Status::DataError;
        result.errorMessage = QObject::tr("No columns defined");
        LOG_ERROR("CSV export failed: No columns defined");
        return result;
    }

    // Ask user for file
    QString defaultExtensionUsed = "CSV files (*.csv *.CSV)";
    QString filter = QObject::tr("CSV files (*.csv *.CSV);;Text files (*.txt *.TXT);;"
                                 "All files (*)");
    QString fileName = QFileDialog::getSaveFileName(nullptr, QObject::tr("Select a file"),
        GbpController::getInstance().getLastDirExport(), filter, &defaultExtensionUsed);

    if (fileName.isEmpty()) {
        result.status = CsvExportResult::Status::Canceled;
        result.errorMessage = QObject::tr("Export canceled by user");
        LOG_INFO("CSV export canceled by user");
        return result;
    }

    // Fix filename to add proper suffix
    if (QFileInfo(fileName).suffix().isEmpty()) {
        fileName.append(".csv");
    }
    QFileInfo fi(fileName);
    GbpController::getInstance().setLastDirExport(fi.absolutePath());
    result.fileName  = fi.fileName();
    result.directory = fi.absolutePath();
    result.filePath  = fi.absoluteFilePath();

    // Open file for writing
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        result.status = CsvExportResult::Status::FileOpenError;
        result.errorMessage = QObject::tr("Cannot open file for writing: %1").arg(fileName);
        LOG_ERROR(QString("CSV export failed: Cannot open file %1").arg(REDACT(fileName)));
        return result;
    }

    // Helper: write a line and check immediately for write error
    auto writeLine = [&](const QString& line) -> bool {
        file.write(line.toUtf8());
        return file.error() == QFileDevice::NoError;
    };

    auto handleWriteError = [&]() {
        result.status = CsvExportResult::Status::WriteError;
        result.errorMessage = QObject::tr("Write failed: %1").arg(file.errorString());
        LOG_ERROR(QString("CSV export failed: write error on %1: %2")
            .arg(REDACT(fileName)).arg(file.errorString()));
        file.close();
    };

    // Write header row
    QStringList headerCells;
    for (const auto& col : columns) {
        headerCells.append(col.header);
    }
    if (!writeLine(headerCells.join(separator) + "\n")) {
        handleWriteError();
        return result;
    }

    // Write data rows
    int columnCount = columns.size();
    for (int rowIndex = 0; rowIndex < data.size(); ++rowIndex) {
        const QList<QVariant>& row = data.at(rowIndex);

        if (row.size() != columnCount) {
            file.close();
            result.status = CsvExportResult::Status::DataError;
            result.errorMessage = QObject::tr("Row %1 has %2 columns, expected %3")
                .arg(rowIndex + 1).arg(row.size()).arg(columnCount);
            LOG_ERROR(QString("CSV export failed: Row %1 has %2 columns, expected %3")
                .arg(rowIndex + 1).arg(row.size()).arg(columnCount));
            return result;
        }

        QStringList rowCells;
        for (int colIndex = 0; colIndex < columnCount; ++colIndex) {
            QString cellValue = formatCell(row.at(colIndex), columns.at(colIndex).type,
                locale, currInfo);
            rowCells.append(cellValue);
        }
        if (!writeLine(rowCells.join(separator) + "\n")) {
            handleWriteError();
            return result;
        }
        result.rowsExported++;
    }

    file.close();

    result.status = CsvExportResult::Status::Success;
    result.errorMessage.clear();
    LOG_INFO(QString("CSV export succeeded: %1 rows written to %2")
        .arg(result.rowsExported).arg(REDACT(fileName)));

    return result;
}


QString CsvExporter::formatCell(
    const QVariant& value,
    const CsvColumnType& colType,
    const QLocale& locale,
    const CurrencyInfo& currInfo)
{
    if (!value.isValid() || value.isNull())
        return QString{};

    switch (colType.type) {
        case CsvColumnType::String:
            return value.toString();

        case CsvColumnType::Date: {
            QDate date = value.toDate();
            if (!date.isValid()) {
                return QString();
            }

            CsvDateFormat granularity = static_cast<CsvDateFormat>(colType.param);
            switch (granularity) {
                case CsvDateFormat::Year:
                    return date.toString("yyyy");
                case CsvDateFormat::YearMonth:
                    return date.toString("yyyy-MM");
                case CsvDateFormat::YearMonthDay:
                    return colType.locale
                        ? locale.toString(date, QLocale::ShortFormat)
                        : date.toString("yyyy-MM-dd");
            }
            return date.toString("yyyy-MM-dd");
        }

        case CsvColumnType::Int:
            if (colType.locale)
                return locale.toString(value.toInt());
            return QString::number(value.toInt());

        case CsvColumnType::Double: {
            double val = value.toDouble();
            if (colType.param == 0) {
                return colType.locale ? locale.toString(val) : QString::number(val);
            }
            return colType.locale
                ? locale.toString(val, 'f', colType.param)
                : QString::number(val, 'f', colType.param);
        }

        case CsvColumnType::Currency: {
            double amount = value.toDouble();
            if (colType.locale) {
                return CurrencyHelper::formatAmount(amount, currInfo, locale, false);
            } else {
                return QString::number(amount, 'f', currInfo.noOfDecimal);
            }
        }
    }

    return value.toString();
}
