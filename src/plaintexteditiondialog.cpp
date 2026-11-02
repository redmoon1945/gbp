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

#include "plaintexteditiondialog.h"
#include "gbplogger.h"
#include "uiutil.h"
#include <QApplication>
#include <QTimer>
#include "ui_plaintexteditiondialog.h"

bool PlainTextEditionDialog::useMonoFont = false;

PlainTextEditionDialog::PlainTextEditionDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PlainTextEditionDialog)
{
    ui->setupUi(this);
    readOnly = false;

    // Warning sign displayed next to the buttons when there are changes not applied yet
    // (same as in EditScenarioDialog). Shown/hidden by contentHasChanged().
    ui->modifiedWarningLabel->setText("\u26A0");
    ui->modifiedWarningLabel->setStyleSheet("color: #FF9800;");
    ui->modifiedWarningLabel->setVisible(false);

    QObject::connect(ui->plainTextEdit, &QPlainTextEdit::textChanged,
        this, &PlainTextEditionDialog::updateContentChanged);
}


void PlainTextEditionDialog::applyFont()
{
    if (useMonoFont) {
        ui->plainTextEdit->setFont(UiUtil::screenMonoFont("PlainTextEditionDialog"));
    } else {
        ui->plainTextEdit->setFont(QApplication::font());
    }
}


void PlainTextEditionDialog::on_MonoFontCheckBox_toggled(bool checked)
{
    useMonoFont = checked;
    applyFont();
}

PlainTextEditionDialog::~PlainTextEditionDialog()
{
    delete ui;
}


void PlainTextEditionDialog::slotPrepareContent(QString title, QString content, bool isItReadOnly)
{
    readOnly = isItReadOnly;
    fillingForm = true;
    ui->plainTextEdit->setPlainText(content);
    fillingForm = false;
    // Keep the text as displayed, to detect changes not applied yet
    originalText = ui->plainTextEdit->toPlainText();

    // [*] : replaced by "*" when there are changes not applied yet (see setWindowModified() in
    // contentHasChanged())
    this->setWindowTitle(title + "[*]");
    ui->plainTextEdit->setReadOnly(readOnly);
    contentHasChanged(false);   // only "Close" at first

    // Sync with the session-wide choice (another instance may have changed it)
    {
        const QSignalBlocker blocker(ui->MonoFontCheckBox);
        ui->MonoFontCheckBox->setChecked(useMonoFont);
    }
    applyFont();

    ui->plainTextEdit->setFocus();

}


void PlainTextEditionDialog::on_applyChangesPushButton_clicked()
{
    QString content = ui->plainTextEdit->toPlainText();
    emit signalPlainTextEditionResult(content);
    on_closePushButton_clicked();
}


// Changes not applied : discard them and close
void PlainTextEditionDialog::on_cancelPushButton_clicked()
{
    on_closePushButton_clicked();
}


void PlainTextEditionDialog::on_closePushButton_clicked()
{
    this->hide();
    fillingForm = true;
    ui->plainTextEdit->setPlainText(""); // dont hold the text, no use for that now
    fillingForm = false;
    originalText.clear();
    emit signalPlainTextEditionCompleted();
}


void PlainTextEditionDialog::on_PlainTextEditionDialog_rejected()
{
    on_cancelPushButton_clicked();
}


void PlainTextEditionDialog::contentHasChanged(bool v)
{
    // override the value, don't care if it is the same as the cost is negligible
    contentChanged.set(v);

    // Visual indicators : warning sign next to the buttons, "*" in the window title
    ui->modifiedWarningLabel->setVisible(hasContentChanged());
    setWindowModified(hasContentChanged());

    // Update buttons to reflect the new situation
    if (hasContentChanged()==true) {
        ui->closePushButton->setVisible(false);
        ui->cancelPushButton->setVisible(true);
        ui->applyChangesPushButton->setVisible(true);
    } else {
        ui->closePushButton->setVisible(true);
        ui->cancelPushButton->setVisible(false);
        ui->applyChangesPushButton->setVisible(false);
    }
}


bool PlainTextEditionDialog::hasContentChanged() const
{
    return contentChanged.get();
}


void PlainTextEditionDialog::updateContentChanged()
{
    if (fillingForm || readOnly) {
        return;     // text set by the dialog itself, or cannot be changed : nothing to check
    }
    contentHasChanged( ui->plainTextEdit->toPlainText() != originalText );
}


void PlainTextEditionDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("PlainTextEditionDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
    });
}

