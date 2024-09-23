/*
 *  Copyright (C) 2024 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
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

#include "welcomedialog.h"
#include "ui_welcomedialog.h"
#include "gbpcontroller.h"
#include <QDir>
#include <QDesktopServices>


WelcomeDialog::WelcomeDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::WelcomeDialog)
{
    ui->setupUi(this);

    // fill the content
    fillContent();

}


WelcomeDialog::~WelcomeDialog()
{
    delete ui;
}


void WelcomeDialog::on_pushButton_clicked()
{
    close();
}


void WelcomeDialog::fillContent()
{

    // build resource name and check if it exists (it should)
//    QFile welcomeFile(QString(":/Doc/resources/Graphical Budget Planner - Welcome-en.html"));
    QFile welcomeFile(QString("/home/pascal/tmp/welcome.md"));
    if(welcomeFile.exists()==false){
        GbpController::getInstance().log(GbpController::LogLevel::Minimal, GbpController::Error, QString("Viewing Welcome : %1 does not exist in the resource file").arg(welcomeFile.fileName()));
        return;
    }

    // fill the textEdit with the file content
    if (!welcomeFile.open(QFile::ReadOnly | QFile::Text)) {
        // handle error
        return;
    }
    QTextStream in(&welcomeFile);
    QString fileContent;
    while (!in.atEnd()) {
        fileContent += in.readLine() + "\n";
    }
//    ui->textEdit->setHtml(fileContent);
ui->textEdit->setMarkdown(fileContent);

}

