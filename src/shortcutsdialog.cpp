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


#include "shortcutsdialog.h"
#include "ui_shortcutsdialog.h"
#include "gbpcontroller.h"
#include "gbplogger.h"
#include "gbpqmessage.h"
#include "uiutil.h"
#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeySequence>
#include <QLabel>
#include <QScreen>
#include <QScrollBar>
#include <QStyle>
#include <QTableWidgetItem>
#include <QTimer>

ShortcutsDialog::ShortcutsDialog(const QList<QAction*>& menuActions, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ShortcutsDialog)
    , menuActions(menuActions)
    , keysColumnWidth(0)
    , textColumnWidth(0)
    , sectionWidth(0)
{
    ui->setupUi(this);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);

    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    // palette() : the colors follow the current theme
    ui->tableWidget->setStyleSheet(
        "QLabel#keyCap { background-color: palette(button); color: palette(button-text);"
        " border: 1px solid palette(mid); border-radius: 4px; padding: 1px 6px; }");
}


ShortcutsDialog::~ShortcutsDialog()
{
    delete ui;
}


void ShortcutsDialog::on_closePushButton_clicked()
{
    hide();
}


void ShortcutsDialog::on_exportHtmlPushButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Select an HTML file"),
        GbpController::getInstance().getLastDirExport(), tr("HTML files (*.html)"), nullptr);
    if (fileName.isEmpty()) {
        return;
    }

    // fix the filename to add the proper suffix
    QFileInfo fi(fileName);
    if (fi.suffix().isEmpty()) {
        fileName.append(".html");
    }
    GbpController::getInstance().setLastDirExport(fi.absolutePath());

    // Everything is in this one file. No page size : the browser uses the paper of the user. The
    // colors are fixed, because the file does not depend on the current theme.
    const QString html = QString(
        "<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"utf-8\">\n<title>%1</title>\n<style>\n"
        "body { font-family: sans-serif; color: #000; background: #fff; }\n"
        "table { border-collapse: collapse; width: 100%; }\n"
        "td { padding: 4px 8px; vertical-align: top; }\n"
        "td.keys { white-space: nowrap; width: 1%; }\n"
        "tr { break-inside: avoid; }\n"
        "tr.section { break-after: avoid; }\n"
        "tr.section td { font-weight: bold; background: #ddd; "
        "print-color-adjust: exact; -webkit-print-color-adjust: exact; }\n"
        "kbd { font: inherit; border: 1px solid #888; border-radius: 4px; padding: 0 6px; }\n"
        "</style>\n</head>\n<body>\n<h2>%1</h2>\n<table>\n%2</table>\n</body>\n</html>\n")
        .arg(windowTitle().toHtmlEscaped(), htmlRows);

    QFile file(fileName);
    const QByteArray content = html.toUtf8();
    if (!file.open(QIODevice::WriteOnly) || file.write(content) != content.size()) {
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            tr("Export failed. The creation of the HTML file did not succeed"), {tr("OK")}, 0, 0);
    }
}


void ShortcutsDialog::showEvent(QShowEvent* event)
{
    // Built at each show : the section colors come from the palette, and the theme may have changed
    buildTable();
    adjustSizeToTable();
    QDialog::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("ShortcutsDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
    });
}


void ShortcutsDialog::buildTable()
{
    const QString ctrl = tr("Ctrl");
    const QString shift = tr("Shift");
    const QString wheel = tr("Mouse wheel");

    keysColumnWidth = 0;
    textColumnWidth = 0;
    sectionWidth = 0;
    htmlRows.clear();
    ui->tableWidget->setRowCount(0);

    addSection(tr("Main window"));
    for (const QAction* action : std::as_const(menuActions)) {
        addRow(action->shortcut().toString(QKeySequence::NativeText).split('+'),
            action->toolTip());
    }

    addSection(tr("Charts : main window, Visualize occurrences, Compare CSD"));
    addRow({wheel}, tr("Zoom, horizontally and vertically"));
    addRow({shift, wheel}, tr("Zoom horizontally only"));
    addRow({ctrl, wheel}, tr("Zoom vertically only"));
    addRow({tr("Click and drag")}, tr("Move the chart"));
    addRow({tr("Click a point")}, tr("Show its date and its value"));

    addSection(tr("Edit scenario"));
    addRow({ctrl, shift, "P"}, tr("Create a new Periodic Cash Stream Definition"));
    addRow({ctrl, shift, "I"}, tr("Create a new Irregular Cash Stream Definition"));
    addRow({ctrl, shift, "C"}, tr("Copy the color of the selected Cash Stream Definition"));
    addRow({ctrl, shift, "V"},
        tr("Paste the copied color to the selected Cash Stream Definitions"));

    addSection(tr("Lists : Edit scenario, Irregular CSD, Variable growth, Manage Tags"));
    addRow({tr("Double-click a line")}, tr("Edit this line"));

    addSection(tr("Analysis : pie chart of the Relative weight tab"));
    addRow({tr("Click a slice")}, tr("Select only this item"));
    addRow({ctrl, tr("Click a slice")}, tr("Add this item to the selection, or remove it"));
    addRow({shift, tr("Click a slice")}, tr("Select all the items up to this one"));

}


void ShortcutsDialog::addSection(const QString& title)
{
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);
    ui->tableWidget->setSpan(row, 0, 1, 2);

    auto* item = new QTableWidgetItem(title);
    QFont font = ui->tableWidget->font();
    font.setBold(true);
    item->setFont(font);
    item->setFlags(Qt::ItemIsEnabled);
    item->setBackground(palette().highlight());
    item->setForeground(palette().highlightedText());
    ui->tableWidget->setItem(row, 0, item);

    sectionWidth = qMax(sectionWidth, QFontMetrics(font).horizontalAdvance(title));

    htmlRows += QString("<tr class=\"section\"><td colspan=\"2\">%1</td></tr>\n")
        .arg(title.toHtmlEscaped());
}


void ShortcutsDialog::addRow(const QStringList& keys, const QString& description)
{
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);

    // Left cell : one "key cap" per key, separated by a "+"
    auto* keysWidget = new QWidget();
    auto* layout = new QHBoxLayout(keysWidget);
    const int margin = fontMetrics().height() / 4;
    layout->setContentsMargins(2*margin, margin, 2*margin, margin);
    layout->setSpacing(margin);
    for (int i = 0; i < keys.size(); ++i) {
        if (i != 0) {
            layout->addWidget(new QLabel("+"));
        }
        auto* keyLabel = new QLabel(keys.at(i));
        keyLabel->setObjectName("keyCap");
        layout->addWidget(keyLabel);
    }
    layout->addStretch();
    ui->tableWidget->setCellWidget(row, 0, keysWidget);

    // Right cell
    auto* item = new QTableWidgetItem(description);
    item->setFlags(Qt::ItemIsEnabled);
    ui->tableWidget->setItem(row, 1, item);

    keysColumnWidth = qMax(keysColumnWidth, keysWidget->sizeHint().width());
    textColumnWidth = qMax(textColumnWidth, fontMetrics().horizontalAdvance(description));

    QStringList htmlKeys;
    for (const QString& key : keys) {
        htmlKeys.append(QString("<kbd>%1</kbd>").arg(key.toHtmlEscaped()));
    }
    htmlRows += QString("<tr><td class=\"keys\">%1</td><td>%2</td></tr>\n")
        .arg(htmlKeys.join(" + "), description.toHtmlEscaped());
}


// Show the whole table without scrollbar, unless the screen is too small for that. If the table
// is too wide for the screen (very big font), the descriptions are wrapped.
void ShortcutsDialog::adjustSizeToTable()
{
    QTableWidget* table = ui->tableWidget;
    const int padding = 2 * fontMetrics().horizontalAdvance("mm");
    const int frame = 2 * table->frameWidth();

    int maxWidth = QWIDGETSIZE_MAX;
    int maxHeight = QWIDGETSIZE_MAX;
    const QScreen* theScreen = screen();
    if (theScreen != nullptr) {
        maxWidth = theScreen->availableGeometry().width() * 9 / 10;
        maxHeight = qMin(700, theScreen->availableGeometry().height() * 3 / 4);
    }

    // Width and height of the table for the given width reserved for a vertical scrollbar
    int width = 0;
    int height = 0;
    auto layOut = [&](int scrollBarWidth) {
        const int minTextWidth = 10 * fontMetrics().horizontalAdvance("m");
        int textWidth = qMax(textColumnWidth + padding, sectionWidth + padding - keysColumnWidth);
        textWidth = qMin(textWidth, maxWidth - frame - keysColumnWidth - scrollBarWidth);
        textWidth = qMax(textWidth, minTextWidth);
        table->setColumnWidth(0, keysColumnWidth);
        table->setColumnWidth(1, textWidth);
        table->resizeRowsToContents();

        height = frame;
        for (int row = 0; row < table->rowCount(); ++row) {
            height += table->rowHeight(row);
        }
        width = frame + keysColumnWidth + textWidth + scrollBarWidth;
    };

    layOut(0);
    if (height > maxHeight) {
        layOut(qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent));
        height = maxHeight;
    }
    table->setMinimumSize(width, height);
    adjustSize();
}
