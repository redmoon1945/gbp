/*
 *  Copyright (C) 2024 Claude Dumas <claudedumas63@protonmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "exportchartdialog.h"
#include "ui_exportchartdialog.h"
#include "gbpcontroller.h"
#include <QFile>
#include <QMessageBox>
#include <QFileDialog>
#include <QCoreApplication>


ExportChartDialog::ExportChartDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ExportChartDialog)
{
    ui->setupUi(this);

    lastDirUsed = GbpController::getInstance().getLastDir();
}


ExportChartDialog::~ExportChartDialog()
{
    delete ui;
}


void ExportChartDialog::slotPrepareContent(QWidget *chartWidget)
{
    this->chartWidget = chartWidget;
    ui->filenameLineEdit->setText("");
}


void ExportChartDialog::on_cancelPushButton_clicked()
{
    emit signalExportChartCompleted();
    hide();
}


void ExportChartDialog::on_exportPushButton_clicked()
{
    int quality = ui->qualitySpinBox->value();
    QString filename = ui->filenameLineEdit->text().trimmed();

    if (filename.size()==0 ){
        QMessageBox::critical(nullptr,tr("File Error"), tr("No file name selected"));
        return;
    }

    // check if parent directory exists (double validation)
    QFileInfo fi = QFileInfo(QFile(filename));
    if(!QDir(fi.path()).exists()){
        QMessageBox::critical(nullptr,tr("File Error"), tr("The specified directory does not exist"));
        return;
    }

    bool successful;
    if (ui->pngRadioButton->isChecked()){
        successful = chartWidget->grab().save(filename,"PNG", quality) ;
    } else{
        successful = chartWidget->grab().save(filename,"JPG", quality) ;
    }
    if(successful == false){
        QMessageBox::critical(nullptr,tr("Export Failed"), tr("The creation of the image file did not succeed"));
        return;
    }

    emit signalExportChartResult(successful);
    emit signalExportChartCompleted();
    hide();
}


void ExportChartDialog::on_browsePushButton_clicked()
{
    QString defaultExtensionPng = ".png";
    QString defaultExtensionUsedPng = ".png";
    QString defaultExtensionJpg = ".jpg";
    QString defaultExtensionUsedJpg = ".jpg";
    QString filter = tr("Image Files (*.png *.PNG *.jpg *.JPG)");
    QString fileName;
    if (ui->pngRadioButton->isChecked()) {
        fileName = QFileDialog::getSaveFileName(this, tr("Save Image"), lastDirUsed, filter, &defaultExtensionUsedPng);
    } else {
        fileName = QFileDialog::getSaveFileName(this, tr("Save Image"), lastDirUsed, filter, &defaultExtensionUsedJpg);
    }
    if (fileName != ""){
        // fix the filename to add the proper suffix
        QFileInfo fi(fileName);
        if(fi.suffix()==""){    // user has not specified an extension
            if (ui->pngRadioButton->isChecked()) {
                fileName.append(defaultExtensionPng);
            } else{
                fileName.append(defaultExtensionJpg);
            }
        }
        // save last dir used
        lastDirUsed = fi.path();
        // set filename
        ui->filenameLineEdit->setText(fileName);
    }
}


void ExportChartDialog::on_ExportChartDialog_rejected()
{
    on_cancelPushButton_clicked();
}



void ExportChartDialog::on_jpgRadioButton_clicked()
{
    imageTypeChanged();
}


void ExportChartDialog::on_pngRadioButton_clicked()
{
    imageTypeChanged();
}


void ExportChartDialog::imageTypeChanged()
{
    QString fName = ui->filenameLineEdit->text();
    if( fName.size()==0 ){
        return;
    }

    // check to see if file extension should be changed
    QString extension = QFileInfo(fName).suffix();
    if (ui->pngRadioButton->isChecked()==true) {
        // we want PNG
        if ( (extension=="jpg") || (extension=="JPG") ){
            QString newFName = fName.replace(".jpg",".png",Qt::CaseSensitivity::CaseInsensitive);
            ui->filenameLineEdit->setText(newFName);
        }
    } else if (ui->jpgRadioButton->isChecked()==true) {
        // we want JPG
        if ( (extension=="png") || (extension=="PNG") ){
            QString newFName = fName.replace(".png",".jpg",Qt::CaseSensitivity::CaseInsensitive);
            ui->filenameLineEdit->setText(newFName);
        }
    }
}

