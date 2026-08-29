#include "scenariopropertiesdialog.h"
#include "gbplogger.h"
#include <QTimer>
#include "ui_scenariopropertiesdialog.h"
#include "gbpcontroller.h"
#include "util.h"
#include "uiutil.h"
#include <qfileinfo.h>
#include <QDir>


ScenarioPropertiesDialog::ScenarioPropertiesDialog(QLocale theLocale, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ScenarioPropertiesDialog)
{
    ui->setupUi(this);

    /// Override fixed-pixel spacers from .ui with font-metric sizes (H: 20px=1×mA, V: 30px=1×mH).
    UiUtil::scaleFixedSpacers(this);

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
    ui->feGenerationDurationLabel->clear();
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
            QString fileName = fileInfo.fileName();
            QString filePath = QDir::toNativeSeparators(fileInfo.absolutePath());
            ui->filenamePlainTextEdit->setPlainText(fileName);
            ui->pathPlainTextEdit->setPlainText(filePath);
            QDateTime dt = fileInfo.birthTime();
            if (dt.isValid()){
                QString s = locale.toString(dt, "yyyy-MMM-dd HH:mm:ss t");
                ui->fileCreationDateLabel->setText(s);
            } else {
                ui->fileCreationDateLabel->setText(tr("Info not available"));
            }
            dt = fileInfo.lastModified();
            if (dt.isValid()){
                QString s = locale.toString(dt, "yyyy-MMM-dd HH:mm:ss t");
                ui->fileModifDateLabel->setText(s);
            } else {
                ui->fileModifDateLabel->setText(tr("Info not available"));
            }
        }

        ui->noPeriodicIncomesLabel->setText(locale.toString(
            theScenario->getNoOfPeriodicIncomes(false)));
        ui->noIrregularIncomesLabel->setText(locale.toString(
            theScenario->getNoOfIrregularIncomes(false)));
        ui->noPeriodicExpensesLabel->setText(locale.toString(
            theScenario->getNoOfPeriodicExpenses(false)));
        ui->noIrregularExpensesLabel->setText(locale.toString(
            theScenario->getNoOfIrregularExpenses(false)));

        bool found;
        CurrencyInfo currInfo = CurrencyHelper::getCurrencyInfoFromIsoCode(
            theScenario->getCurrencyIsoCode(), locale.language(), found);
        if(found){
            ui->currencyLabel->setText(QString("%1 (%2)").arg(currInfo.name,currInfo.isoCode));
        } else {
            ui->currencyLabel->setText(tr("Unknown"));
        }

        // label_15 (next to inflationLabel in the .ui) is a static "percent, annually" caption
        // that doesn't translate naturally alongside a full sentence and can't adapt to the
        // "Variable inflation" case at all - inflationLabel is a self-contained, fully
        // translatable sentence on its own, so label_15 is always hidden.
        ui->label_15->setVisible(false);
        if (theScenario->getInflation().getType()==Growth::Type::CONSTANT){
            qint64 intInf = theScenario->getInflation().getAnnualConstantGrowth();
            double inf = Growth::fromDecimalToDouble(intInf);
            ui->inflationLabel->setText(tr("Constant annual inflation of %1 percent").arg(
                Util::formatDouble(inf, locale, Util::DoubleFormatMode::Standard, {})));
        } else{
            ui->inflationLabel->setText(tr("Variable inflation"));
        }

        ui->feGenerationDurationLabel->setText(
            locale.toString(theScenario->getFeGenerationDuration()));

    }

    ui->closePushButton->setFocus();
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


void ScenarioPropertiesDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    QTimer::singleShot(0, this, [this]() {
        LOG_DEBUG_INFO(QString("ScenarioPropertiesDialog initial size : %1 x %2")
            .arg(width()).arg(height()));
    });
}

