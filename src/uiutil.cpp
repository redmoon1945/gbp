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

#include "uiutil.h"
#include "gbplogger.h"
#include <QApplication>
#include <QFontDatabase>
#include <QFontInfo>
#include <QFontMetrics>
#include <QHeaderView>
#include <QLayout>
#include <QSpacerItem>
#include <QStyle>


void UiUtil::scaleFixedSpacers(QWidget* widget)
{
    if (!widget) return;
    QFontMetrics fm(widget->font());
    int mA = fm.horizontalAdvance("M");
    int mH = fm.height();
    for (QLayout* layout : widget->findChildren<QLayout*>()) {
        for (int i = 0; i < layout->count(); ++i) {
            QLayoutItem* item = layout->itemAt(i);
            if (!item) continue;
            QSpacerItem* sp = item->spacerItem();
            if (!sp) continue;
            QSizePolicy pol = sp->sizePolicy();
            int w = sp->sizeHint().width();
            int h = sp->sizeHint().height();
            bool changed = false;
            if (pol.horizontalPolicy() == QSizePolicy::Fixed && w > 0) {
                w = qRound(w * mA / 20.0);
                changed = true;
            }
            if (pol.verticalPolicy() == QSizePolicy::Fixed && h > 0) {
                h = qRound(h * mH / 30.0);
                changed = true;
            }
            if (changed) {
                sp->changeSize(w, h, pol.horizontalPolicy(), pol.verticalPolicy());
            }
        }
    }
    if (widget->layout()) {
        widget->layout()->invalidate();
    }
}


void UiUtil::resizeTableColumns(QTableWidget* tableWidget)
{
    QFontMetrics fmTable(QApplication::font());
    int minWidth = fmTable.horizontalAdvance(QString(4, '8'));
    int extraPadding = fmTable.horizontalAdvance(QLatin1Char('8'));
    QHeaderView *header = tableWidget->horizontalHeader();
    int headerMargin = tableWidget->style()->pixelMetric(QStyle::PM_HeaderMargin, nullptr, header);
    header->setMinimumSectionSize(fmTable.height() + 2 * headerMargin);

    for (int col = 0; col < tableWidget->columnCount(); ++col) {
        tableWidget->resizeColumnToContents(col);
        int contentWidth = tableWidget->columnWidth(col) + extraPadding;
        QString headerText = tableWidget->horizontalHeaderItem(col)
            ? tableWidget->horizontalHeaderItem(col)->text() : QString();
        int headerWidth = fmTable.horizontalAdvance(headerText) + 2 * headerMargin + extraPadding;
        tableWidget->setColumnWidth(col, qMax(qMax(contentWidth, headerWidth), minWidth));
    }
}


QFont UiUtil::screenMonoFont(const QString& context)
{
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    const QFont appFont = QApplication::font();
    if (appFont.pointSizeF() > 0){
        mono.setPointSizeF(appFont.pointSizeF());
    }
    else if (appFont.pixelSize() > 0){
        mono.setPixelSize(appFont.pixelSize());
    }

    const QFontInfo fi(mono);
    const QFontInfo fiApp(appFont);
    auto boolStr = [](bool b) { return b ? QStringLiteral("yes") : QStringLiteral("no"); };
    LOG_DEBUG_INFO(QString("screenMonoFont [%1] requested=%2 resolved=%3"
        " pointSizeF=%4 pixelSize=%5 fixedPitch=%6 bold=%7 italic=%8"
        " | appFont pointSizeF=%9 pixelSize=%10 bold=%11 italic=%12")
        .arg(context)
        .arg(mono.family())
        .arg(fi.family())
        .arg(fi.pointSizeF())
        .arg(fi.pixelSize())
        .arg(boolStr(fi.fixedPitch()))
        .arg(boolStr(fi.bold()))
        .arg(boolStr(fi.italic()))
        .arg(fiApp.pointSizeF())
        .arg(fiApp.pixelSize())
        .arg(boolStr(fiApp.bold()))
        .arg(boolStr(fiApp.italic())));

    return mono;
}
