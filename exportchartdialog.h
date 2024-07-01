/*
 *  Copyright (C) 2024 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
 *  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
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

#ifndef EXPORTCHARTDIALOG_H
#define EXPORTCHARTDIALOG_H

#include <QDialog>

namespace Ui {
class ExportChartDialog;
}

class ExportChartDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportChartDialog(QWidget *parent = nullptr);
    ~ExportChartDialog();

signals:
    // For client of ExportChartDialog : sending edition completion notification
    void signalExportChartResult(bool successful);
    void signalExportChartCompleted();

public slots:
    // From client of ExportChartDialog, to be called just before showing
    void slotPrepareContent(QWidget* chartWidget);



private slots:
    void on_ExportChartDialog_rejected();
    void on_cancelPushButton_clicked();
    void on_exportPushButton_clicked();
    void on_browsePushButton_clicked();
    void on_jpgRadioButton_clicked();
    void on_pngRadioButton_clicked();

private:
    Ui::ExportChartDialog *ui;

    // variables
    QWidget* chartWidget;
    QString lastDirUsed;

    // methods
    void imageTypeChanged();

};

#endif // EXPORTCHARTDIALOG_H
