// Copyright (c) 2018-2019 The Ring Developers
// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <miner.h>

#include <qt/miningpage.h>
#include <qt/forms/ui_miningpage.h>

#include <qt/ringunits.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/transactionfilterproxy.h>
#include <qt/transactiontablemodel.h>
#include <qt/walletmodel.h>

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QTimer>

#define DECORATION_SIZE 54
#define NUM_ITEMS 5

Q_DECLARE_METATYPE(interfaces::WalletBalances)

#include <qt/miningpage.moc>

MiningPage::MiningPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MiningPage),
    clientModel(nullptr)
{
    ui->setupUi(this);

    displayUpdateTimer = new QTimer();
    connect(displayUpdateTimer, SIGNAL(timeout()), this, SLOT(updateHashRateDisplay()));

    ui->threadCountSpinner->setMaximum(GetNumCores());
    updateDisplayedOptions();
}

MiningPage::~MiningPage()
{
    delete ui;
}

void MiningPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model) {
        connect(model, &ClientModel::generateChanged, this, &MiningPage::updateDisplayedOptions);
        connect(model, &ClientModel::numConnectionsChanged, this, &MiningPage::numConnectionsChanged);
        connect(model, &ClientModel::blockFound, this, &MiningPage::increaseBlocksFound);

        ui->networkWarningLabel->setVisible(model->getNumConnections()==0);
    }
}

void MiningPage::increaseBlocksFound()
{
    static unsigned int blocksFound = 0;
    ui->blocksFoundLabel->setText(QString::number(++blocksFound));
}

void MiningPage::numConnectionsChanged(int connections)
{
    ui->networkWarningLabel->setVisible(connections==0);
}

void MiningPage::on_useAllCoresCheckbox_stateChanged() {
    if (ui->useAllCoresCheckbox->isChecked()) {
        ui->threadCountSpinner->setEnabled(false);
        ui->threadCountSpinner->setValue(ui->threadCountSpinner->maximum());
    } else
        ui->threadCountSpinner->setEnabled(true);
}

void MiningPage::on_toggleMiningButton_clicked() {
    bool generate = gArgs.GetBoolArg("-gen", DEFAULT_GENERATE);
    generate = !generate;

    int genproclimit = ui->useAllCoresCheckbox->isChecked() ? -1 : ui->threadCountSpinner->value();

    gArgs.ForceSetArg("-gen", generate ? "1" : "0");
    gArgs.ForceSetArg("-genproclimit", itostr(genproclimit));
    
    MineCoins(generate, genproclimit, Params());
}

void MiningPage::updateHashRateDisplay() {
    ui->hashRateDisplayLabel->setText(QString::number(dHashesPerSec/1000.0));
}

void MiningPage::updateDisplayedOptions() {
    int genproclimit = gArgs.GetArg("-genproclimit", DEFAULT_GENERATE_THREADS);
    bool mining = gArgs.GetBoolArg("-gen", DEFAULT_GENERATE);

    if (genproclimit == -1) {
        ui->threadCountSpinner->setValue(ui->threadCountSpinner->maximum());
        ui->useAllCoresCheckbox->setChecked(true);
    } else {
        ui->threadCountSpinner->setValue(genproclimit);
        ui->useAllCoresCheckbox->setChecked(false);
    }

    if (mining) {
        updateHashRateDisplay();
        if (!displayUpdateTimer->isActive())
            displayUpdateTimer->start(5000);
        ui->toggleMiningButton->setText("Stop mining");
        ui->threadCountSpinner->setEnabled(false);
        ui->useAllCoresCheckbox->setEnabled(false);
        ui->toggleMiningButton->setStyleSheet("background-color: rgba(204, 22, 22, 1);");
    } else {
        displayUpdateTimer->stop();
        ui->toggleMiningButton->setText("Start mining");
        ui->useAllCoresCheckbox->setEnabled(true);
        ui->threadCountSpinner->setEnabled(genproclimit != -1);        
        ui->toggleMiningButton->setStyleSheet("background-color: rgba(46, 204, 113, 1);");
        ui->hashRateDisplayLabel->setText("N/A");        
    }
}
