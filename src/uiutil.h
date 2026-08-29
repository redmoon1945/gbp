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

#ifndef UIUTIL_H
#define UIUTIL_H

#include <QWidget>
#include <QTableWidget>
#include <QTableView>
#include <QDateTimeEdit>

/**
 * @brief UI-only utility functions that depend on Qt widget/layout headers.
 * Kept separate from Util so that non-UI translation units do not pull in
 * heavyweight widget dependencies.
 */
class UiUtil
{
public:
    UiUtil() = delete;

    /**
     * @brief Scale all fixed-size spacers in a widget's layout tree using font metrics.
     * @details Horizontal Fixed spacers are scaled by mA/20 (where mA = advance width of "M").
     * Vertical Fixed spacers are scaled by mH/30 (where mH = font line height).
     * Uses QWidget::findChildren<QLayout*>() to visit every descendant layout so that
     * spacers inside QGroupBox, QTabWidget pages, etc. are all reached automatically.
     * New spacers added in .ui files are picked up without any code change.
     * @param widget The root widget whose descendant layouts are scanned.
     */
    static void scaleFixedSpacers(QWidget* widget);

    /**
     * @brief Resize each column of a @c QTableWidget to fit both cell content and header label,
     *        and ensure the header row is tall enough to render all glyphs without clipping.
     *
     * @details
     * <b>Column width algorithm</b><br>
     * For each column, the final width is:
     * @code
     *   max( max(cellWidth, headerWidth), minWidth )
     * @endcode
     * where:
     * - @c cellWidth  = @c resizeColumnToContents() + one @c '8'-width padding.
     *   @c resizeColumnToContents() uses each cell's actual font (e.g. a monospace font set
     *   on individual items), so it accounts correctly for wider-than-average glyphs.
     * - @c headerWidth = advance("header text", appFont) + 2 × @c PM_HeaderMargin
     *   + one @c '8'-width padding.
     *   The header text is measured with @c QApplication::font() because that is the font
     *   the header view actually renders with on Windows (confirmed by observation).
     *   @c QStyle::PM_HeaderMargin is the exact style metric Qt uses internally when computing
     *   @c CT_HeaderSection sizes, so it matches the real left/right padding applied by the
     *   platform style.
     * - @c minWidth  = four @c '8'-widths, so columns with no data remain visible.
     *
     * <b>Why not @c QHeaderView::sectionSizeHint()?</b><br>
     * @c sectionSizeHint() is unreliable on Windows: it queries an internal font that is
     * smaller than @c QApplication::font(), causing it to underestimate the rendered header
     * width. The direct font-metrics approach above avoids this pitfall.
     *
     * <b>Why not @c header->resizeSections(ResizeToContents)?</b><br>
     * @c resizeSections(ResizeToContents) also uses Qt's internal (smaller) font for headers
     * on Windows, producing the same underestimate. When the cell-content width exceeds the
     * underestimated header width, the column is sized to the cell content — which may not
     * leave room for the full header text.
     *
     * <b>Why @c QApplication::font() for @c minWidth and @c extraPadding?</b><br>
     * On Windows, @c QAbstractItemView receives a class-specific platform font that is smaller
     * than the application font. Using @c QApplication::font() for pixel measurements ensures
     * consistent sizing regardless of what the widget font reports.
     *
     * <b>Header row height</b><br>
     * @c setMinimumSectionSize() is called with @c fmTable.height() + 2 × @c PM_HeaderMargin
     * to ensure descenders (e.g. 'g', 'p') are not clipped. Without this, the header height
     * is derived from the smaller platform widget font and may be insufficient for the
     * application font.
     *
     * @param tableWidget The table whose columns and header are to be resized. Must not be null.
     */
    static void resizeTableColumns(QTableWidget* tableWidget);

    /**
     * @brief Resize selected columns of a @c QTableView (a model/view table, as opposed to
     *        the item-based @c QTableWidget) to fit both cell content and header label.
     *
     * @details Uses the same algorithm and rationale as @c resizeTableColumns() - see its
     * documentation for the full explanation of why each measurement is taken the way it is
     * (in particular, the Windows-specific pitfalls around header/section fonts). The only
     * differences are:
     * - The header label text comes from the model's @c headerData(), since a @c QTableView
     *   has no @c QTableWidgetItem of its own to ask.
     * - Only the columns listed in @p columns are touched. Unlike @c resizeTableColumns(),
     *   which resizes every column, a @c QTableView's other columns may already have their
     *   own content-aware sizing (e.g. a currency amount column sized from its longest
     *   possible formatted value) or may be meant to stretch and fill remaining space -
     *   forcing every column to shrink-to-content would undo that.
     *
     * Call this again whenever the view's model content changes (e.g. rows added/removed/
     * reloaded), since - like @c resizeTableColumns() - it only reflects whatever is in the
     * view at the moment it runs; it is not automatically kept up to date afterwards.
     *
     * @param tableView The view whose columns are to be resized. Must not be null, and must
     * have a model set.
     * @param columns Indices of the columns to resize; any other column is left untouched.
     */
    static void resizeTableViewColumns(QTableView* tableView, const QList<int>& columns);

    /**
     * @brief Widen a @c QDateEdit/@c QDateTimeEdit a bit beyond its natural size, without
     * risking it becoming too narrow to show its own content.
     *
     * @details Sets the widget's minimum width to:
     * @code
     *   dateEdit->sizeHint().width() + extraPaddingChars × averageCharWidth
     * @endcode
     * @c sizeHint() is Qt's own size calculation for this widget - it already accounts for
     * the widget's @c displayFormat(), current locale, and current font, so (unlike a plain
     * @c QTableView column, which has no idea what content is coming) there is nothing to
     * guess here: the base width is always correct for whatever this widget will actually
     * display. @p extraPaddingChars is added purely for a roomier look on top of that correct
     * base, so even a generous or imprecise padding value can only ever make the widget a
     * little more or less spacious - never too narrow to render its own date.
     *
     * @param dateEdit The widget to widen. Must not be null.
     * @param extraPaddingChars Extra width to add on top of @c sizeHint(), expressed as a
     * multiple of the widget's average character width. Default is 4.
     */
    static void widenDateEdit(QDateTimeEdit* dateEdit, int extraPaddingChars = 4);

    /**
     * @brief Returns the system monospace font sized to match @c QApplication::font().
     * @details Uses @c QFontDatabase::FixedFont on all platforms, then overrides the size with
     * the application font size to ensure consistent rendering across widget classes.
     * @param context Caller description logged in DEBUG mode to identify the call site.
     * @return A @c QFont suitable for rendering monospaced content.
     */
    static QFont screenMonoFont(const QString& context);
};

#endif // UIUTIL_H
