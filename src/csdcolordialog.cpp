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


#include "csdcolordialog.h"
#include "ui_csdcolordialog.h"
#include "gbplogger.h"
#include "uiutil.h"
#include <QBuffer>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QTimer>

namespace {

// The button shows "<its text> (<square>)". A button cannot draw an image inside its text, so the
// text is moved to a label laid out in the button. The clicks go through it to the button.
void showColorSquare(QPushButton* button, const QColor& color)
{
    const int side = button->fontMetrics().height() * 3 / 4;
    QPixmap square(side, side);
    square.fill(color);
    QPainter painter(&square);
    painter.setPen(button->palette().mid().color());
    painter.drawRect(0, 0, side-1, side-1);
    painter.end();

    QByteArray png;
    QBuffer buffer(&png);
    square.save(&buffer, "PNG");

    auto* label = new QLabel(QString("%1 (<img align=\"middle\" src=\"data:image/png;base64,%2\">)")
        .arg(button->text().toHtmlEscaped(), QString::fromLatin1(png.toBase64())));
    label->setTextFormat(Qt::RichText);
    label->setAlignment(Qt::AlignCenter);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto* layout = new QHBoxLayout(button);
    const int margin = button->fontMetrics().horizontalAdvance("m");
    layout->setContentsMargins(margin, 0, margin, 0);
    layout->addWidget(label);
    // A button sizes itself on its own text only
    button->setMinimumWidth(label->sizeHint().width() + 2*margin);
    button->setAccessibleName(button->text());
    button->setText(QString());
}

}

CsdColorDialog::CsdColorDialog(const std::optional<QColor>& colorToCopy, bool canSetSystemDefault,
    const std::optional<QColor>& clipboardColor, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CsdColorDialog)
{
    ui->setupUi(this);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);

    ui->copyPushButton->setEnabled(colorToCopy.has_value());
    if (colorToCopy.has_value()) {
        showColorSquare(ui->copyPushButton, *colorToCopy);
    }
    ui->systemDefaultPushButton->setEnabled(canSetSystemDefault);
    ui->pastePushButton->setEnabled(clipboardColor.has_value());
    if (clipboardColor.has_value()) {
        showColorSquare(ui->pastePushButton, *clipboardColor);
    }
}


CsdColorDialog::~CsdColorDialog()
{
    delete ui;
}


void CsdColorDialog::on_copyPushButton_clicked()
{
    done(COPY);
}


void CsdColorDialog::on_pastePushButton_clicked()
{
    done(PASTE);
}


void CsdColorDialog::on_editPushButton_clicked()
{
    done(EDIT);
}


void CsdColorDialog::on_systemDefaultPushButton_clicked()
{
    done(SYSTEM_DEFAULT);
}


void CsdColorDialog::on_cancelPushButton_clicked()
{
    reject();
}


void CsdColorDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("CsdColorDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
    });
}
