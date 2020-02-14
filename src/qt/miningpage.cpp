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
#include <qt/walletmodel.h>
#include <qt/pop/availablegamestablemodel.h>

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QTimer>
#include <QGraphicsPixmapItem>

#include <qt/miningpage.moc>

#include <rpc/mining.h>

MiningPage::MiningPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MiningPage),
    clientModel(nullptr),
    blocksFound(0),
    previousBlocksFound(0)
{
    ui->setupUi(this);

    displayUpdateTimer = new QTimer();
    connect(displayUpdateTimer, SIGNAL(timeout()), this, SLOT(updateHashRateDisplay()));

    ui->threadCountSpinner->setMaximum(GetNumCores());
    
    makeMinotaurs();
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

void MiningPage::setModel(WalletModel *_model) {
    this->model = _model;

    if(_model) {
        previousBlocksFound = _model->getMinedBlockCount();
        ui->previousBlocksFoundLabel->setText(QString::number(previousBlocksFound));
    }
}

void MiningPage::increaseBlocksFound()
{
    blocksFound++;
    previousBlocksFound++;
    ui->blocksFoundLabel->setText(QString::number(blocksFound));
    ui->previousBlocksFoundLabel->setText(QString::number(previousBlocksFound));
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
    ui->hashRateDisplayLabel->setText(QString::number(std::floor(dHashesPerSec/1000)));

    double timeToSolve = (this->clientModel) ? this->clientModel->getTimeToSolve() : 0;

    if (timeToSolve > 0)
        ui->timeToSolveDisplayLabel->setText(timeToSolve > 172800 ? "> 2 days" : AvailableGamesTableModel::secondsToString(std::floor(timeToSolve)));
    else
        ui->timeToSolveDisplayLabel->setText("N/A");
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
        drawMinotaurs(genproclimit);
        updateHashRateDisplay();
        if (!displayUpdateTimer->isActive())
            displayUpdateTimer->start(5000);
        ui->toggleMiningButton->setText("Stop mining");
        ui->miningStatusLabel->setText("Miners running");
        ui->threadCountSpinner->setEnabled(false);
        ui->useAllCoresCheckbox->setEnabled(false);
        ui->toggleMiningButton->setStyleSheet("background-color: rgba(204, 22, 22, 1);");        
    } else {
        displayUpdateTimer->stop();
        drawMinotaurs(0);
        ui->toggleMiningButton->setText("Start mining");
        ui->miningStatusLabel->setText("Miners stopped");
        ui->useAllCoresCheckbox->setEnabled(true);
        ui->threadCountSpinner->setEnabled(genproclimit != -1);        
        ui->toggleMiningButton->setStyleSheet("background-color: rgba(46, 204, 113, 1);");
        ui->hashRateDisplayLabel->setText("N/A");
        ui->timeToSolveDisplayLabel->setText("N/A");
    }
}

void MiningPage::makeMinotaurs() {
    QImage img(":/icons/minotaur");
    img = img.convertToFormat(QImage::Format_ARGB32);
    QColor cols[8] = {Qt::black, Qt::red, QColor("#FFA500"), Qt::yellow, Qt::green, Qt::blue, QColor("#4b0082"), QColor("#EE82EE")};
    for (int i = 0; i < 8; i++) {
        QColor c = cols[i];
        for (int x = img.width(); x--;) {
            for (int y = img.height(); y--;) {
                const QRgb rgb = img.pixel(x, y);
                img.setPixel(x, y, qRgba(c.red(), c.green(), c.blue(), i == 0 ? qAlpha(rgb) / 2 : qAlpha(rgb)));
            }
        }
        minotaurs[i] = QPixmap::fromImage(img);
    }
}

void MiningPage::drawMinotaurs(int coloured) {
    if (scene) {
        scene->clear();
        delete scene;
    }
    scene = new QGraphicsScene(this);

    ui->graphicsView->setAlignment(Qt::AlignRight | Qt::AlignBottom);
    ui->graphicsView->setScene(scene);

    if (coloured == 0) {
        QGraphicsPixmapItem *item = scene->addPixmap(minotaurs[0]);
        item->setOffset(0, 0);
        return;
    }

    int total = GetNumCores();
    if (coloured == -1)
        coloured = total;
    if (coloured > 16)
        coloured = 16;

    for (int i = 0; i < coloured; i++) {
        int which = (i % 7) + 1;
        QGraphicsPixmapItem *item = scene->addPixmap(minotaurs[which]);
        item->setOffset(60 * i, 0);
    }
}