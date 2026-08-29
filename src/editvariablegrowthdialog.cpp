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

#include "editvariablegrowthdialog.h"
#include "gbplogger.h"
#include "loadvariablegrowthtextfiledialog.h"
#include <QTimer>
#include "ui_editvariablegrowthdialog.h"
#include <QString>
#include <QMessageBox>
#include <QItemSelectionModel>
#include <QCoreApplication>
#include "editvariablegrowthmodel.h"
#include "gbpcontroller.h"
#include "uiutil.h"
#include "gbpqmessage.h"


EditVariableGrowthDialog::EditVariableGrowthDialog(QString newGrowthName, QLocale locale,
    QWidget *parent) : QDialog(parent), ui(new Ui::EditVariableGrowthDialog)
{
    this->locale = locale;
    ui->setupUi(this);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);

    growthName = newGrowthName;

    // set the model (no internal data for now)
    tableModel = new EditVariableGrowthModel(growthName, this->locale);
    ui->growthTableView->setModel(tableModel);
    ui->growthTableView->setFont(QApplication::font());

    // Columns previously all had an equal, stretched width regardless of content, which could
    // leave the Transition date column too narrow for some locale's date format (or the
    // annual/monthly columns too narrow for the growth name they display). Re-fit each column
    // to its actual content every time the table's data changes instead - setGrowthData()
    // always goes through beginResetModel()/endResetModel(), so this single connection
    // covers every place the table gets (re)populated.
    connect(tableModel, &QAbstractItemModel::modelReset, this, [this]() {
        UiUtil::resizeTableViewColumns(ui->growthTableView, {0, 1, 2});
    });
    UiUtil::resizeTableViewColumns(ui->growthTableView, {0, 1, 2});

    QFont appFont = QApplication::font();
    ui->growthTableView->horizontalHeader()->setFont(appFont);
    ui->growthTableView->horizontalHeader()->setMinimumHeight(
        QFontMetrics(appFont).height() + 10);

    // Makes note characters smaller and italic
    QFont noteFont = appFont;
    Util::changeFontSize(noteFont, Util::FontResizeIntensity::AVERAGE, true,
        "EditVariableGrowthDialog - note");
    ui->noteLabel->setFont(noteFont);

    // Make action buttons smaller
    QFont actionFont = appFont;
    Util::changeFontSize(actionFont, Util::FontResizeIntensity::WEAK, true,
        "EditVariableGrowthDialog - action buttons");
    ui->SelectAllPushButton->setFont(actionFont);
    ui->unselectAllPushButton->setFont(actionFont);
    ui->addPushButton->setFont(actionFont);
    ui->deletePushButton->setFont(actionFont);
    ui->editPushButton->setFont(actionFont);

    // the edit add element dialog
    ege = new EditGrowthElementDialog(newGrowthName,locale,this);        // auto-destroyed by Qt
    ege->setModal(true);

    // the import dialog
    importDlg = new LoadVariableGrowthTextFileDialog(newGrowthName, locale, this);  // auto-destroyed by Qt
    importDlg->setModal(true);

    // connections with edition of growth element
    QObject::connect(this, &EditVariableGrowthDialog::signalEditElementPrepareContent, ege,
        &EditGrowthElementDialog::slotPrepareContent);
    QObject::connect(ege, &EditGrowthElementDialog::signalEditElementResult, this,
        &EditVariableGrowthDialog::slotEditElementResult);
    QObject::connect(ege, &EditGrowthElementDialog::signalEditElementCompleted, this,
        &EditVariableGrowthDialog::slotEditElementCompleted);

    // connections with import dialog
    QObject::connect(this, &EditVariableGrowthDialog::signalImportPrepareContent,
        importDlg, &LoadVariableGrowthTextFileDialog::slotPrepareContent);
    QObject::connect(importDlg, &LoadVariableGrowthTextFileDialog::signalImportResult,
        this, &EditVariableGrowthDialog::slotImportResult);
    QObject::connect(importDlg, &LoadVariableGrowthTextFileDialog::signalImportCompleted,
        this, &EditVariableGrowthDialog::slotImportCompleted);
}


EditVariableGrowthDialog::~EditVariableGrowthDialog()
{
    delete ui;
    delete tableModel; // dont forget, because we have not set "parent" !
}


// Prepare for a new editing session.
// To be called before displaying the EditComplexGrowthDialog window, to setup content
void EditVariableGrowthDialog::slotPrepareContent(const Growth newGrowth)
{
    QString tmp = Util::wordCapitalize(true,QString(tr("Edit variable %1"))
        .arg(tableModel->getGrowthName()));
    this->setWindowTitle(tmp);

    ui->noteLabel->setText(QString(tr("%1 : Value is 0 before the oldest transition date is "
        "defined. It is always applied on a monthly basis, even if defined on an annual basis "
        "(for convenience purpose). Value stays the same until a new transition date + value is "
        "defined.")).arg(tableModel->getGrowthName()));
    tableModel->setGrowthName(tableModel->getGrowthName());
    // update model (the view will be automatically updated)
    tableModel->setGrowthData(newGrowth);

    ui->growthTableView->setFocus();
}


// This can be for an edition of existing element or the definition of a new element
void EditVariableGrowthDialog::slotEditElementResult(bool isEdition, QDate oldDate,
    QDate newDate, double growthInPercentage)
{
    // get the current data from the model
    Growth ag = tableModel->getGrowthData();   // necessarily a Variable type

    // convert growth in percentge to growth in hundredth of percentage
    qint64 gInt = Growth::fromDoubleToDecimal(growthInPercentage); // necessarily always valid

    // update the data
    QMap<QDate,qint64> factors = ag.getAnnualVariableGrowth();
    if ( isEdition && (oldDate!=newDate) ){
        // delete the old entry since date has changed
        factors.remove(oldDate);
    }
    factors.insert(newDate,gInt); // if existing date, value is replaced
    Growth newAg = Growth::fromVariableDataAnnualBasisDecimal(factors);

    // update the model (the table view will be updated automatically)
    tableModel->setGrowthData(newAg);

    // select the edited item
    int rowToSelect = tableModel->getPositionForDate(newDate);
    if (rowToSelect != -1){ // should never happen
        ui->growthTableView->selectRow(rowToSelect);
    }
}


void EditVariableGrowthDialog::slotEditElementCompleted()
{
    ui->growthTableView->setFocus();
}


void EditVariableGrowthDialog::slotImportResult(QMap<QDate,qint64> factors)
{
    Growth newGrowth = Growth::fromVariableDataAnnualBasisDecimal(factors);
    tableModel->setGrowthData(newGrowth);
}


void EditVariableGrowthDialog::slotImportCompleted()
{
}


void EditVariableGrowthDialog::on_importPushButton_clicked()
{
    emit signalImportPrepareContent();
    importDlg->show();
}


void EditVariableGrowthDialog::on_applyPushButton_clicked()
{
    Growth g = tableModel->getGrowthData();
    emit signalEditVariableGrowthResult(g);
    hide();
    emit signalEditVariableGrowthCompleted();
}


void EditVariableGrowthDialog::on_cancelPushButton_clicked()
{
    hide();
    emit signalEditVariableGrowthCompleted();
}


void EditVariableGrowthDialog::on_addPushButton_clicked()
{
    QList<QDate> existingDates = tableModel->getGrowthData().getAnnualVariableGrowth().keys();
    emit signalEditElementPrepareContent(false,existingDates,GbpController::getInstance()
        .getTomorrow(),0); // last 2 values are dummy
    ege->show();
}


void EditVariableGrowthDialog::on_editPushButton_clicked()
{
    QMap<QDate,qint64> factors = tableModel->getGrowthData().getAnnualVariableGrowth();
    QList<QDate> existingDates = factors.keys();
    QList<int> selectedRows = getSelectedRows();
    if (selectedRows.size()!=1){
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            tr("Select exactly one row"), {tr("OK")}, 0, 0);
        ui->growthTableView->setFocus(); // fix strange behavior
        return;
    }
    QDate aDate = existingDates.at(selectedRows.at(0));
    qint64 growthInt = (factors.value(aDate));
    double growthDouble = Growth::fromDecimalToDouble(growthInt);   // necessarily always valid
    emit signalEditElementPrepareContent(true,existingDates,aDate,growthDouble);
    ege->show();
}


void EditVariableGrowthDialog::on_deletePushButton_clicked()
{
    QMap<QDate,qint64> factors = tableModel->getGrowthData().getAnnualVariableGrowth();
    QList<QDate> existingDates = factors.keys();
    QList<int> selectedRows = getSelectedRows();
    if (selectedRows.size()==0){
        GbpQMessage::messageBoxQuestion(this, GbpQMessage::Type::ERROR, tr("Error"),
            tr("Select at least one row"), {tr("OK")}, 0, 0);
        ui->growthTableView->setFocus(); // fix strange behavior
        return;
    }
    // remove selected rows from factors
    foreach(int row,selectedRows ){
        factors.remove(existingDates.at(row));
    }
    // update model data and redisplay
    Growth newAg = Growth::fromVariableDataAnnualBasisDecimal(factors);
    tableModel->setGrowthData(newAg); // view will update itself
}



QList<int> EditVariableGrowthDialog::getSelectedRows()
{
    QItemSelectionModel* selectionModel = ui->growthTableView->selectionModel();
    QModelIndexList selectedIndexes = selectionModel->selectedRows();
    QList<int> selectedRows;
    foreach (const QModelIndex& index, selectedIndexes) {
        selectedRows.append(index.row());
    }
    return selectedRows;
}


void EditVariableGrowthDialog::on_growthTableView_doubleClicked(const QModelIndex &index)
{
    on_editPushButton_clicked();
}


// dialog has been closed ("x" button) manually by user
void EditVariableGrowthDialog::on_EditVariableGrowthDialog_rejected()
{
    on_cancelPushButton_clicked();
}


void EditVariableGrowthDialog::on_SelectAllPushButton_clicked()
{
    ui->growthTableView->selectAll();
    ui->growthTableView->setFocus(); // fix strange behavior or bug
}


void EditVariableGrowthDialog::on_unselectAllPushButton_clicked()
{
    ui->growthTableView->clearSelection();
}


void EditVariableGrowthDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("EditVariableGrowthDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
    });
}

