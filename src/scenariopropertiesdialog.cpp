#include "scenariopropertiesdialog.h"
#include "ui_scenariopropertiesdialog.h"
#include "gbpcontroller.h"
#include <qfileinfo.h>
#


ScenarioPropertiesDialog::ScenarioPropertiesDialog(QLocale theLocale, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ScenarioPropertiesDialog)
{
    ui->setupUi(this);
    locale = theLocale;
    QFontMetrics fm(ui->pathPlainTextEdit->font());
    ui->pathPlainTextEdit->setFixedHeight(fm.height()*5); // 5 lines min
}


ScenarioPropertiesDialog::~ScenarioPropertiesDialog()
{
    delete ui;
}


void ScenarioPropertiesDialog::slotPrepareContent()
{
    // clear all
    ui->formatVersionLabel->clear();
    ui->namePlainTextEdit->clear();
    ui->filenamePlainTextEdit->clear();
    ui->pathPlainTextEdit->clear();
    ui->fileCreationDateLabel->clear();
    ui->fileModifDateLabel->clear();
    ui->noPeriodicIncomesLabel->clear();
    ui->noIrregularIncomesLabel->clear();
    ui->noPeriodicExpensesLabel->clear();
    ui->noIrregularExpensesLabel->clear();
    ui->currencyLabel->clear();
    ui->inflationLabel->clear();

    // make sure there is something to show
    if(GbpController::getInstance().isScenarioLoaded()!=false){
        //scenario can still be new and not saved yet on disk

        QSharedPointer<Scenario> theScenario = GbpController::getInstance().getScenario();
        ui->formatVersionLabel->setText(theScenario->getVersion());
        ui->namePlainTextEdit->setPlainText(theScenario->getName());

        // File info
        QString fullName = GbpController::getInstance().getFullFileName();
        if(fullName==""){
            ui->filenamePlainTextEdit->setPlainText(tr("Not set yet"));
            ui->pathPlainTextEdit->setPlainText(tr("Not set yet"));
        } else {
            QFileInfo fileInfo(fullName);
            QString baseFileName = fileInfo.baseName();
            QString filePath = fileInfo.absolutePath();
            ui->filenamePlainTextEdit->setPlainText(baseFileName);
            ui->pathPlainTextEdit->setPlainText(filePath);
            QDateTime dt = fileInfo.birthTime();
            if (dt.isValid()){
                QString s = locale.toString(dt);
                ui->fileCreationDateLabel->setText(s);
            } else {
                ui->fileCreationDateLabel->setText(tr("Info not available"));
            }
            dt = fileInfo.lastModified();
            if (dt.isValid()){
                QString s = locale.toString(dt);
                ui->fileModifDateLabel->setText(s);
            } else {
                ui->fileModifDateLabel->setText(tr("Info not available"));
            }
        }

        ui->noPeriodicIncomesLabel->setText(QString::number(theScenario->getNoOfPeriodicIncomes()));
        ui->noIrregularIncomesLabel->setText(QString::number(theScenario->getNoOfIrregularIncomes()));
        ui->noPeriodicExpensesLabel->setText(QString::number(theScenario->getNoOfPeriodicExpenses()));
        ui->noIrregularExpensesLabel->setText(QString::number(theScenario->getNoOfIrregularExpenses()));

        bool found;
        CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromCountryCode(locale, theScenario->getCountryCode(), found);
        if(found){
            ui->currencyLabel->setText(QString("%1 (%2)").arg(currInfo.name,currInfo.isoCode));
        } else {
            ui->currencyLabel->setText(tr("Unknown"));
        }

        if (theScenario->getInflation().getType()==Growth::CONSTANT){
            qint64 intInf = theScenario->getInflation().getAnnualConstantGrowth();
            double inf = Growth::fromDecimalToDouble(intInf);
            ui->inflationLabel->setText(tr("Constant annual inflation of %1 percent").arg(inf));
        } else{
            ui->inflationLabel->setText(tr("Variable inflation"));
        }

    }
}


void ScenarioPropertiesDialog::on_closePushButton_clicked()
{
    hide();
    emit signalScenarioPropertiesCompleted();
}


void ScenarioPropertiesDialog::on_ScenarioPropertiesDialog_rejected()
{
    on_closePushButton_clicked();
}

