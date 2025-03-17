/*
 *  Copyright (C) 2024-2025 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
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

#ifndef EDITTAGDIALOG_H
#define EDITTAGDIALOG_H

#include <QDialog>
#include <QLocale>
#include <QString>
#include <QList>
#include <qlistwidget.h>
#include "plaintexteditiondialog.h"
#include "tags.h"

namespace Ui {
class EditTagDialog;
}


// do not use this structure outside this module
namespace edittag {
    struct Suggestion{
        QUuid id;
        QString name;
        QString description;
        bool operator==(const Suggestion &o) const;
    };

    // Hash function for cases where Suggestion used as a key for QSet or QHash.
    size_t qHash(const edittag::Suggestion &t, size_t seed = 0);
}

class EditTagDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditTagDialog(QLocale aLocale, QWidget *parent = nullptr);
    ~EditTagDialog();

signals:
    // For client of ManageTagsDialog : send result and edition completion notification
    void signalEditTagResult(Tag newtag);
    void signalEditTagCompleted();
    // edition of description : prepare Dialog before edition
    void signalPlainTextDialogPrepareContent(QString title, QString content, bool readOnly);

public slots:
    // From client of EditTagDialog : Prepare edition. Call this before show()
    void slotPrepareContent(Tags currentTags, Tag tagIdToEdit, bool isForNewTag);
    // PlainTextEdition child Dialog : receive result and edition completion notification
    void slotPlainTextEditionResult(QString result);
    void slotPlainTextEditionCompleted();

private slots:
    void on_EditTagDialog_rejected();
    void on_applyPushButton_clicked();
    void on_cancelPushButton_clicked();
    void on_fullViewPushButton_clicked();
    void on_bothRadioButton_toggled(bool checked);
    void on_incomesRadioButton_toggled(bool checked);
    void on_expensesRadioButton_toggled(bool checked);
    void on_suggestionsListWidget_itemSelectionChanged();

private:
    Ui::EditTagDialog *ui;

    // Children dialogs
    PlainTextEditionDialog* editDescriptionDialog;

    // Variables
    bool newTag; // Are we editing or creating a tag
    Tags existingTags; // existing tags
    Tag tagBeingEdited; // Tag being edited
    QHash<QUuid, edittag::Suggestion> incomeSuggestions;  // key = tag name, value = description
    QHash<QUuid, edittag::Suggestion> expenseSuggestions; // key = tag name, value = description
    QLocale locale;
    static quint64 MAX_NO_SUGGESTIONS;

    // Methods
    void fillList();
    void loadSuggestions();
    QList<QString> parseSuggestionLine(QString line);
    QString buildDisplayLine(QString name, QString description);
};

#endif // EDITTAGDIALOG_H
