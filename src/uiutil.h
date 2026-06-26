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
     * @brief Returns the system monospace font sized to match @c QApplication::font().
     * @details Uses @c QFontDatabase::FixedFont on all platforms, then overrides the size with
     * the application font size to ensure consistent rendering across widget classes.
     * @param context Caller description logged in DEBUG mode to identify the call site.
     * @return A @c QFont suitable for rendering monospaced content.
     */
    static QFont screenMonoFont(const QString& context);
};

#endif // UIUTIL_H
