#ifndef CSVEXPORTER_H
#define CSVEXPORTER_H

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QList>
#include <QDate>
#include <QLocale>
#include "currencyhelper.h"

/**
 * @brief Specifies the ISO-8601 granularity of a date column in CSV export
 *
 * All values produce culture-neutral ISO-8601 output unless the @c locale flag
 * is set on the column — in which case @c YearMonthDay uses @c QLocale::ShortFormat
 * for human-readable output. @c Year and @c YearMonth always produce ISO output
 * regardless of the locale flag (Qt provides no partial locale date formats).
 */
enum class CsvDateFormat {
    Year,         ///< ISO-8601, year only: "2025"
    YearMonth,    ///< ISO-8601, year and month: "2025-03"
    YearMonthDay  ///< ISO-8601 "2025-03-15", or locale short "28/03/2026" when locale flag is set
};

/**
 * @struct CsvColumnType
 * @brief Defines the data type and formatting options for a CSV column
 *
 * This struct combines the column type with type-specific parameters using
 * a compact representation. Use the static factory methods to create instances
 * rather than constructing directly.
 *
 * @note Field meanings:
 *       - @c param — Date: @c CsvDateFormat cast to int; Double: decimal places (0 = full);
 *                    others: unused
 *       - @c locale — Int, Double: if @c true, use locale-aware formatting
 *                     (e.g. thousands separators, locale decimal point); others: unused
 *
 * @note For any column type, passing a default-constructed @c QVariant{} as the cell
 *       value produces an empty cell in the output, regardless of the column type.
 *       This is the standard way to represent an absent or unknown value.
 *
 * @par Factory method quick-reference:
 * | Factory call                                       | Output example    |
 * |----------------------------------------------------|-------------------|
 * | @c CsvColumnType::string()                         | @c Hello          |
 * | @c CsvColumnType::date(CsvDateFormat::Year)          | @c 2025           |
 * | @c CsvColumnType::date(CsvDateFormat::YearMonth)      | @c 2025-03        |
 * | @c CsvColumnType::date(CsvDateFormat::YearMonthDay)   | @c 2025-03-15     |
 * | @c CsvColumnType::date(CsvDateFormat::YearMonthDay, true) | @c 28/03/2026 |
 * | @c CsvColumnType::integer()                        | @c 42             |
 * | @c CsvColumnType::integer(true)                    | @c 1,042          |
 * | @c CsvColumnType::number(2)                        | @c 3.14           |
 * | @c CsvColumnType::number(2, true)                  | @c 3,14           |
 * | @c CsvColumnType::numberFull()                     | @c 3.141592653589793 |
 * | @c CsvColumnType::currency()                       | @c 1234.56        |
 *
 * @sa CsvExporter, CsvDateFormat
 */
struct CsvColumnType {
    /**
     * @brief Enumeration of supported column data types
     */
    enum Type {
        String,   ///< Plain text, output as-is
        Date,     ///< QDate, formatted according to CsvDateFormat
        Int,      ///< Integer value, no decimal places
        Double,   ///< Floating-point value with configurable decimal precision
        Currency  ///< Monetary value, formatted via CurrencyHelper
    };

    Type type;    ///< The column data type
    int  param;   ///< Date: CsvDateFormat cast to int; Double: precision (0 = full); others: unused
    bool locale;  ///< Date/YearMonthDay, Int, Double, Currency: use locale-aware formatting
                  ///<   when @c true; others: unused

    /**
     * @brief Creates a String column type
     * @return CsvColumnType configured for plain text data
     */
    static CsvColumnType string() { return {String, 0, false}; }

    /**
     * @brief Creates a Date column type with the specified format
     * @param format The ISO-8601 granularity (Year, YearMonth, or YearMonthDay)
     * @param locale If @c true and @p format is @c YearMonthDay, formats using
     *               the locale's short date format (e.g. "28/03/2026").
     *               Ignored for @c Year and @c YearMonth (always ISO). Default: @c false.
     * @return CsvColumnType configured for date data
     */
    static CsvColumnType date(CsvDateFormat format, bool locale = false) {
        return {Date, static_cast<int>(format), locale};
    }

    /**
     * @brief Creates an Int column type
     * @param locale If @c true, format with locale-aware separators (e.g. "1,042");
     *               if @c false, plain digits (e.g. "1042"). Default: @c false.
     * @return CsvColumnType configured for integer data
     */
    static CsvColumnType integer(bool locale = false) { return {Int, 0, locale}; }

    /**
     * @brief Creates a Double column type with specified decimal precision
     * @param precision Number of decimal places (must be >= 1). Use @c integer() instead
     *                  of @c number(0) — they produce identical output and @c integer()
     *                  better expresses the intent.
     * @param locale If @c true, format with locale decimal point and separators.
     *               If @c false, use culture-neutral dot separator. Default: @c false.
     * @return CsvColumnType configured for floating-point data
     */
    static CsvColumnType number(int precision, bool locale = false) {
        return {Double, precision, locale};
    }

    /**
     * @brief Creates a Double column type with maximum precision
     *
     * Uses Qt's default full-precision formatting, producing the shortest representation
     * that round-trips back to the same double value.
     *
     * @param locale If @c true, format with locale decimal point and separators.
     *               If @c false, use culture-neutral dot separator. Default: @c false.
     * @return CsvColumnType configured for full-precision floating-point data
     */
    static CsvColumnType numberFull(bool locale = false) { return {Double, 0, locale}; }

    /**
     * @brief Creates a Currency column type
     * @param locale If @c true, format with locale-specific currency symbols via
     *               CurrencyHelper. If @c false, plain numeric format. Default: @c false.
     * @return CsvColumnType configured for monetary data
     */
    static CsvColumnType currency(bool locale = false) { return {Currency, 0, locale}; }
};

/**
 * @struct CsvColumnDescriptor
 * @brief Describes a single column in the CSV export
 *
 * Pairs a column header with its data type configuration.
 */
struct CsvColumnDescriptor {
    QString header;      ///< Column header text (first row of CSV)
    CsvColumnType type;  ///< Data type and formatting options
};

/**
 * @struct CsvExportResult
 * @brief Contains the result and status information of a CSV export operation
 *
 * Provides detailed feedback about the export outcome, including success/failure
 * status, the output file path, number of rows exported, and any error messages.
 */
struct CsvExportResult {
    /**
     * @brief Possible outcomes of a CSV export operation
     */
    enum class Status {
        Success,       ///< Export completed successfully
        Canceled,      ///< User canceled the file selection dialog
        FileOpenError, ///< Failed to open the target file for writing
        WriteError,    ///< Write failed mid-export (e.g., disk full)
        DataError      ///< Data validation error (e.g., column count mismatch)
    };

    Status status;           ///< Outcome of the export operation
    QString fileName;        ///< Base file name with extension (e.g. "report.csv");
                             ///<   empty if file was not created
    QString directory;       ///< Absolute path to the containing directory (empty if not created)
    QString filePath;        ///< Full absolute path: directory + file name (empty if not created)
    int rowsExported;        ///< Number of data rows written (excludes header row)
    QString errorMessage;    ///< Human-readable error description (empty on success)
};

/**
 * @class CsvExporter
 * @brief Utility class for exporting tabular data to CSV files
 *
 * Provides a generic, reusable mechanism for exporting data matrices to CSV format.
 * Supports multiple column types (String, Date, Int, Double, Currency) with
 * configurable formatting options.
 *
 * Features:
 * - Automatic file dialog for destination selection
 * - Configurable column separator (default: tab for LibreOffice Calc compatibility)
 * - Currency formatting via CurrencyHelper with optional localization
 * - ISO-8601 date formatting with configurable granularity
 * - Optional (absent) cell values via @c QVariant{} — produces an empty cell for any type
 * - Detailed export results for error handling and user feedback
 *
 * @note All dialogs are application-modal (block all application windows).
 *
 * @par How to Use:
 * Follow these three steps to export data to CSV:
 *
 * **Step 1 — Declare columns.**
 * Build a @c QList<CsvColumnDescriptor> that defines the header text and data type
 * for each column. Column order here determines column order in the output file.
 * @code
 * QList<CsvColumnDescriptor> columns;
 * columns.append({tr("Period"),  CsvColumnType::date(CsvDateFormat::YearMonth)});
 * columns.append({tr("Label"),   CsvColumnType::string()});
 * columns.append({tr("Count"),   CsvColumnType::integer()});
 * columns.append({tr("Amount"),  CsvColumnType::currency()});
 * columns.append({tr("Growth"),  CsvColumnType::number(2)});
 * @endcode
 *
 * **Step 2 — Populate rows.**
 * Build a @c QList<QList<QVariant>> where each inner list is one data row.
 * Every row must contain exactly as many values as there are columns, in the same order.
 * Use a default-constructed @c QVariant{} for any absent/unknown value — it produces an
 * empty cell regardless of the column type.
 * @code
 * QList<QList<QVariant>> data;
 * data.append({QDate(2025, 3, 1), tr("Groceries"), 42,   1234.56, 5.3      });
 * data.append({QDate(2025, 4, 1), tr("Transport"), 38,    980.00, QVariant{}}); // no growth
 * @endcode
 *
 * **Step 3 — Call exportToCsv() and handle the result.**
 * The method shows a Save-As dialog, writes the file, and returns a @c CsvExportResult.
 * Always check the status — @c Canceled is a normal, non-error outcome when the user
 * closes the dialog without choosing a file.
 * @code
 * CsvExportResult result = CsvExporter::exportToCsv(
 *     "Monthly Report", columns, data, locale, currInfo,
 *     '\t'     // separator: tab (default, good for LibreOffice Calc)
 * );
 *
 * switch (result.status) {
 *     case CsvExportResult::Status::Success:
 *         QMessageBox::information(this, tr("Export"),
 *             tr("%1 rows exported to %2")
 *                 .arg(result.rowsExported)
 *                 .arg(result.fileName));   // e.g. "report.csv"
 *         // result.directory → "/home/user/exports"
 *         // result.filePath  → "/home/user/exports/report.csv"
 *         break;
 *     case CsvExportResult::Status::Canceled:
 *         break; // user closed the dialog — nothing to do
 *     case CsvExportResult::Status::FileOpenError:
 *     case CsvExportResult::Status::DataError:
 *         QMessageBox::warning(this, tr("Export failed"), result.errorMessage);
 *         break;
 * }
 * @endcode
 *
 * @sa CsvColumnType, CsvColumnDescriptor, CsvExportResult, CsvDateFormat
 */
class CsvExporter
{
public:
    /**
     * @brief Exports a data matrix to a CSV file
     *
     * Displays a file dialog for the user to select a destination, then writes
     * the provided data as a CSV file. Handles file extension validation,
     * directory persistence, and log reporting. Errors are not reported to user.
     *
     * @note File dialogs are application-modal (block all application windows).
     * @note The @c ".csv" extension is automatically appended if not already present.
     * @note The last export directory is persisted via GbpController.
     * @note For any column type, a default-constructed @c QVariant{} cell value
     *       produces an empty cell in the output (absent / unknown value convention).
     *
     * @param[in] operationName Name of the export operation, used for logging
     * @param[in] columns Column descriptors defining headers and data types.
     *                    Must not be empty.
     * @param[in] data Data matrix where each inner QList represents one row.
     *                 Every row must contain exactly as many elements as @p columns.
     *                 Expected QVariant types per column type:
     *                 - String:   QString
     *                 - Date:     QDate
     *                 - Int:      int (or any type implicitly convertible via QVariant)
     *                 - Double:   double (or any type implicitly convertible via QVariant)
     *                 - Currency: double (or any type implicitly convertible via QVariant)
     *                 - Any type: @c QVariant{} (invalid) → empty cell
     * @param[in] locale Locale used for number and currency formatting
     * @param[in] currInfo Currency information for formatting Currency columns
     * @param[in] separator Column separator character (default: tab @c '\\t')
     *
     * @return CsvExportResult containing:
     *         - @c status:        Success, Canceled, FileOpenError, WriteError, or DataError
     *         - @c fileName:      Base file name with extension (empty if not created)
     *         - @c directory:     Absolute path to the containing directory (empty if not created)
     *         - @c filePath:      Full absolute path: directory + file name (empty if not created)
     *         - @c rowsExported:  Number of data rows written (excludes header row)
     *         - @c errorMessage:  Human-readable error description (empty on success)
     */
    static CsvExportResult exportToCsv(
        const QString& operationName,
        const QList<CsvColumnDescriptor>& columns,
        const QList<QList<QVariant>>& data,
        const QLocale& locale,
        const CurrencyInfo& currInfo,
        QChar separator = '\t'
    );

private:
    /**
     * @brief Formats a single cell value according to its column type
     *
     * If @p value is invalid or null (@c QVariant{}), returns an empty string
     * regardless of @p colType, producing an empty cell in the CSV output.
     *
     * @param[in] value     The cell value as QVariant; @c QVariant{} means absent
     * @param[in] colType   Column type and formatting parameters
     * @param[in] locale    Locale used for number and currency formatting
     * @param[in] currInfo  Currency information for Currency columns
     *
     * @return Formatted string for the cell, or an empty string if @p value is absent
     */
    static QString formatCell(
        const QVariant& value,
        const CsvColumnType& colType,
        const QLocale& locale,
        const CurrencyInfo& currInfo
    );
};

#endif // CSVEXPORTER_H
