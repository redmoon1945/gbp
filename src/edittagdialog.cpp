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


#include "edittagdialog.h"
#include <QTimer>
#include "gbpcontroller.h"
#include "gbplogger.h"
#include "ui_edittagdialog.h"
#include "uiutil.h"
#include <QMessageBox>
#include <QListWidgetItem>
#include "gbpqmessage.h"

quint64 EditTagDialog::MAX_NO_SUGGESTIONS = 1000;

EditTagDialog::EditTagDialog(const QLocale &aLocale, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EditTagDialog), locale(aLocale)
{
    ui->setupUi(this);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);

    ui->nameLineEdit->setMaxLength(Tag::MAX_NAME_LEN);

    QFont appFont = QApplication::font();

    // use smaller font for description list
    QFont descFont = appFont;
    Util::changeFontSize(descFont, Util::FontResizeIntensity::AVERAGE, true,
        "EditTagDialog - description");
    ui->descPlainTextEdit->setFont(descFont);

    // make action buttons smaller
    QFont actionFont = appFont;
    Util::changeFontSize(actionFont, Util::FontResizeIntensity::WEAK, true,
        "EditTagDialog - action buttons");
    ui->bothRadioButton->setFont(actionFont);
    ui->incomesRadioButton->setFont(actionFont);
    ui->expensesRadioButton->setFont(actionFont);

    ui->suggestionsListWidget->setFont(appFont);

    // force description widget to be small (cant do it in Qt Designer...)
    QFontMetrics fm(ui->descPlainTextEdit->font());
    ui->descPlainTextEdit->setFixedHeight(fm.height()*3); // 3 lines

    // Plain Text Edition Dialog
    editDescriptionDialog = new PlainTextEditionDialog(this);     // auto-destroyed by Qt because it is a child
    editDescriptionDialog->setModal(true);

    // load all suggestions, taking into account locale
    loadSuggestions();

    // Fill list
    fillList();

    // connect emitters & receivers for Dialogs : Description Edition
    QObject::connect(this, &EditTagDialog::signalPlainTextDialogPrepareContent, editDescriptionDialog, &PlainTextEditionDialog::slotPrepareContent);
    QObject::connect(editDescriptionDialog, &PlainTextEditionDialog::signalPlainTextEditionResult, this, &EditTagDialog::slotPlainTextEditionResult);
    QObject::connect(editDescriptionDialog, &PlainTextEditionDialog::signalPlainTextEditionCompleted, this, &EditTagDialog::slotPlainTextEditionCompleted);
}


EditTagDialog::~EditTagDialog()
{
    delete ui;
}


// tagToEdit must be in currentTags
void EditTagDialog::slotPrepareContent(const Tags &currentTags, const Tag &tagToEdit,
    bool isForNewTag)
{
    existingTags = currentTags;
    newTag = isForNewTag;

    if (newTag==true){
        ui->applyPushButton->setText(tr("Create"));
        ui->cancelPushButton->setText(tr("Close"));
        ui->nameLineEdit->setText("");
        ui->descPlainTextEdit->setPlainText("");
        this->setWindowTitle(tr("Create new tag"));
        tagBeingEdited = Tag(QUuid::createUuid(),""); // name will be filled in later
    } else {
        ui->applyPushButton->setText(tr("Apply"));
        ui->cancelPushButton->setText(tr("Cancel"));
        this->setWindowTitle(tr("Edit tag"));
        bool found;
        Tag tag = currentTags.getTag(tagToEdit.getId(), found);
        if (found==false) {
            // we have been passed an inexisting tag ID, this is invalid
            throw std::domain_error(QString("%1: Tag passed is invalid")
                .arg(Q_FUNC_INFO).toStdString());
        }
        ui->nameLineEdit->setText(tagToEdit.getName());
        ui->descPlainTextEdit->setPlainText(tagToEdit.getDescription());
        tagBeingEdited = tagToEdit;
    }
    ui->suggestionsListWidget->clearSelection();

    ui->nameLineEdit->setFocus();
}


void EditTagDialog::slotPlainTextEditionResult(QString result)
{
    ui->descPlainTextEdit->setPlainText(result.left(Tag::MAX_DESC_LEN));
}


void EditTagDialog::slotPlainTextEditionCompleted()
{

}


void EditTagDialog::on_EditTagDialog_rejected()
{
    on_cancelPushButton_clicked();
}


void EditTagDialog::on_applyPushButton_clicked()
{
    // we remove leading and trailing spaces
    QString cleanedName = ui->nameLineEdit->text().trimmed();

    // no of time a tag has a name = content of the name edit box
    quint16 duplicateNameNo = existingTags.containsTagName(cleanedName);

    if (newTag==true){

        // be sure the name does not already exist
        if (duplicateNameNo > 0) {
            GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
                tr("An existing tag already uses this name"), {tr("OK")}, 0, 0);
            return;
        }
        // create the tag (adjust the name)
        tagBeingEdited.setName(cleanedName);
        tagBeingEdited.setDescription(ui->descPlainTextEdit->toPlainText());
        // Send it to owner as the result
        signalEditTagResult(tagBeingEdited);
        // add to the list of existing tag
        existingTags.insert(tagBeingEdited);
        // prepare for new tag
        tagBeingEdited = Tag(QUuid::createUuid(),"");
        ui->nameLineEdit->clear();
        ui->descPlainTextEdit->clear();
        ui->suggestionsListWidget->clearSelection();
        ui->nameLineEdit->setFocus();

    } else {

        if (cleanedName!=tagBeingEdited.getName()) {
            // name has changed, make sure the new value does not already exist
            if (duplicateNameNo > 0) {
                GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
                    tr("An existing tag already uses this name"), {tr("OK")}, 0, 0);
                return;
            }
        }

        tagBeingEdited.setName(cleanedName);
        tagBeingEdited.setDescription(ui->descPlainTextEdit->toPlainText());
        signalEditTagResult(tagBeingEdited);
        signalEditTagCompleted();
        hide();
    }
}


void EditTagDialog::on_cancelPushButton_clicked()
{
    signalEditTagCompleted();
    hide();
}


void EditTagDialog::on_fullViewPushButton_clicked()
{
    emit signalPlainTextDialogPrepareContent(tr("Edit description"),
        ui->descPlainTextEdit->toPlainText(), false);
    editDescriptionDialog->show();
}


void EditTagDialog::fillList()
{
    ui->suggestionsListWidget->clear();
    QList<QString> tokens;
    QListWidgetItem *item;

    if ( (ui->incomesRadioButton->isChecked()==true) || (ui->bothRadioButton->isChecked()==true) ) {
        foreach (QUuid id, incomeSuggestions.keys()) {
            edittag::Suggestion sug = incomeSuggestions.value(id);
            QString displayLine = buildDisplayLine(sug.name, sug.description);
            item = new QListWidgetItem(displayLine);
            item->setData(Qt::UserRole, QVariant::fromValue(sug.id));  // we remember the id
            ui->suggestionsListWidget->addItem(item);
        }
    }
    if ( (ui->expensesRadioButton->isChecked()==true)||(ui->bothRadioButton->isChecked()==true) ) {
        foreach (QUuid id, expenseSuggestions.keys()) {
            edittag::Suggestion sug = expenseSuggestions.value(id);
            QString displayLine = buildDisplayLine(sug.name, sug.description);
            item = new QListWidgetItem(displayLine);
            item->setData(Qt::UserRole, QVariant::fromValue(sug.id));  // we remember the id
            ui->suggestionsListWidget->addItem(item);
        }
    }
}


void EditTagDialog::loadSuggestions()
{
    // First, determine the language to choose
    QString langCode = locale.languageToCode(locale.language()) ;

    // build resource name for incomes and expenses
    QFile incomesFile(QString(":/Doc/resources/tag_suggestions_incomes-%1.txt").
        arg((langCode=="fr")?("fr"):("en")));
    QFile expensesFile(QString(":/Doc/resources/tag_suggestions_expenses-%1.txt").
        arg((langCode=="fr")?("fr"):("en")));
    if(incomesFile.exists()==false){
        LOG_ERROR( QString("Load tag suggestions : %1 does not exist in the resource file")
            .arg(incomesFile.fileName()));
        return;
    }
    if(expensesFile.exists()==false){
        LOG_ERROR( QString("Load tag suggestions : %1 does not exist in the resource file")
            .arg(expensesFile.fileName()));
        return;
    }

    // *** INCOMES ***
    incomeSuggestions.clear();
    // open the file
    if (incomesFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        try {
            // read and parse lines
            QTextStream stream(&incomesFile);
            stream.setAutoDetectUnicode(true);
            while ( (!stream.atEnd()) && (incomeSuggestions.size()<=MAX_NO_SUGGESTIONS)){
                QString line = stream.readLine(Tag::MAX_NAME_LEN+Tag::MAX_DESC_LEN+1).trimmed();
                if ( (!line.isNull()) && (line.size()!=0) ){
                    QList<QString> tokens = parseSuggestionLine(line);
                    if (tokens.size()==2) {
                        QUuid id = QUuid::createUuid();
                        edittag::Suggestion sug = {.id=id, .name=tokens[0], .description=tokens[1]};
                        incomeSuggestions.insert(id,sug);
                    }
                }
            }
        } catch (const std::exception& e) {
            QString logErrorMessage = QString("An unexpected error has occurred.\n\nDetails : %1")
                .arg(e.what());
            LOG_ERROR( QString("Import failed : %1").arg(logErrorMessage) );
            return;
        }
    } else {
        QString logErrorMessage = QString("Cannot open file %1 in read-only mode")
            .arg(REDACT(incomesFile.fileName()));
        LOG_ERROR( QString("Import failed for Tag Suggestions (incomes): %1")
            .arg(logErrorMessage));
        return ;
    }

    // *** EXPENSES ***
    expenseSuggestions.clear();
    // open the file
    if (expensesFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        try {
            // read lines
            QTextStream stream(&expensesFile);
            stream.setAutoDetectUnicode(true);
            while ( (!stream.atEnd()) && (incomeSuggestions.size()<=MAX_NO_SUGGESTIONS)){
                QString line = stream.readLine(Tag::MAX_NAME_LEN+Tag::MAX_DESC_LEN+1).trimmed();
                if ( (!line.isNull()) && (line.size()!=0) ){
                    QList<QString> tokens = parseSuggestionLine(line);
                    if (tokens.size()==2) {
                        QUuid id = QUuid::createUuid();
                        edittag::Suggestion sug = {.id=id, .name=tokens[0], .description=tokens[1]};
                        expenseSuggestions.insert(id,sug); // name, description
                    }
                }
            }
        } catch (const std::exception& e) {
            QString logErrorMessage = QString("An unexpected error has occurred.\n\nDetails : %1")
            .arg(e.what());
            LOG_ERROR( QString("Import failed : %1").arg(logErrorMessage) );
            return;
        }
    } else {
        QString logErrorMessage = QString("Cannot open file %1 in read-only mode")
        .arg(expensesFile.fileName());
        LOG_ERROR( QString("Import failed for Tag Suggestions (expenses): %1")
            .arg(logErrorMessage) );
        return ;
    }
}


// Convert a suggestion line from suggestion file into tag and description components.
// line is assumed already trimmed. If format error is encountered, empty QList is returned
QList<QString> EditTagDialog::parseSuggestionLine(QString line)
{
    QList<QString> result;
    if(line.size()==0){
        return result;
    }
    QStringList tokens;
    tokens = line.split('\t'); // can have 1 or 2 components, since desc is optional
    if (tokens.length() == 1){
        result.append(tokens[0]);   // tag name
         result.append("");         // empty description
    } else if (tokens.length() == 2){
        result.append(tokens[0]);   // tag name
        result.append(tokens[1]);   // tag description
    } else {
        return result;
    }
    return result;
}


// from tag name and description, build the line that will be displayed in the list
QString EditTagDialog::buildDisplayLine(QString name, QString description)
{
    QString line;
    line = QString("%1").arg(name);
    return line;
}


void EditTagDialog::on_bothRadioButton_toggled(bool checked)
{
    fillList();
}


void EditTagDialog::on_incomesRadioButton_toggled(bool checked)
{
    fillList();
}


void EditTagDialog::on_expensesRadioButton_toggled(bool checked)
{
    fillList();
}


bool edittag::Suggestion::operator==(const edittag::Suggestion &o) const
{
    if ( (this->id!=o.id) || (this->name!=o.name) || (this->description!=o.description) ) {
        return false;
    } else {
        return true;
    }
}


// This is a global function. Create a Hash value for Suggestion, required to use
// Qt doc says : "there must also be a GLOBAL qHash()
// function that returns a hash value for an argument of the key's type."
size_t edittag::qHash(const edittag::Suggestion &t, size_t seed ) {
    return qHash(t.id,seed); // ID is a unique identifier
}


void EditTagDialog::on_suggestionsListWidget_itemSelectionChanged()
{
    // get the current item selected
    QList<QListWidgetItem *> selection = ui->suggestionsListWidget->selectedItems();
    if ( selection.count()!=1 ) {
        return;
    }
    QUuid id = selection[0]->data(Qt::UserRole).value<QUuid>();

    // search in the 2 list for this ID, must be there
    if( incomeSuggestions.contains(id)==true){
        edittag::Suggestion sug = incomeSuggestions.value(id);
        ui->nameLineEdit->setText(sug.name);
        ui->descPlainTextEdit->setPlainText(sug.description);
    } else if (expenseSuggestions.contains(id)==true){
        edittag::Suggestion sug = expenseSuggestions.value(id);
        ui->nameLineEdit->setText(sug.name);
        ui->descPlainTextEdit->setPlainText(sug.description);
    } else {
        // should never happen
        return;
    }
}


void EditTagDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    // QTimer::singleShot(0) defers execution until after all show-related events have been
    // processed, including the platform style's focusInEvent which calls selectAll() on the
    // focused QLineEdit. Calling deselect() directly in slotPrepareContent() has no effect
    // because that slot runs before the dialog is shown.
    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("EditTagDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
        ui->nameLineEdit->deselect();
    });
}

