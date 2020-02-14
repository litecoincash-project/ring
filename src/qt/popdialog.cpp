// Copyright (c) 2019 The Ring Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Ring-fork: Pop

#include <wallet/wallet.h>
#include <wallet/fees.h>

#include <qt/popdialog.h>
#include <qt/forms/ui_popdialog.h>
#include <qt/clientmodel.h>
#include <qt/addressbookpage.h>
#include <qt/addresstablemodel.h>
#include <qt/ringunits.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/receiverequestdialog.h>
#include <qt/walletmodel.h>
#include <qt/pop/availablegamestablemodel.h>
#include <qt/pop/game0/game0board.h>
#include <qt/uicolours.h>
#include <validation.h>
#include <chainparams.h>
#include <crypto/pop/popgame.h>

#include <QAction>
#include <QCursor>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextDocument>
#include <QTextEdit>
#include <QCheckBox>
#include <QDialog>

PopDialog::PopDialog(const PlatformStyle *_platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PopDialog),
    model(0),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);

	// Create board. In the future, there might be a tabset each with a board / playspace.
	game0board = new Game0Board(ui->gameFrame);
    //connect(game0board, static_cast<void (Game0Board::*)(const CAvailableGameType*, uint8_t, std::vector<unsigned char>)>(&Game0Board::solutionReady), this, &PopDialog::submitSolution);
    connect(game0board, &Game0Board::solutionReady, this, &PopDialog::submitSolution);
}

void PopDialog::setClientModel(ClientModel *_clientModel) {
    this->clientModel = _clientModel;

    if (_clientModel)
        connect(_clientModel, SIGNAL(numBlocksChanged(int,QDateTime,double,bool)), this, SLOT(updateGamesAvailable()));
}

void PopDialog::setModel(WalletModel *_model) {
    this->model = _model;

    if(_model && _model->getOptionsModel()) {
        _model->getAvailableGamesTableModel()->sort(AvailableGamesTableModel::BlocksLeft, Qt::AscendingOrder);

        //connect(_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &PopDialog::updateDisplayUnit);
        //updateDisplayUnit();
        
        if (_model->getEncryptionStatus() != WalletModel::Locked)
            ui->releaseSwarmButton->hide();
        connect(_model, SIGNAL(encryptionStatusChanged(int)), this, SLOT(setEncryptionStatus(int)));

        QTableView* tableView = ui->gameTable;
        tableView->verticalHeader()->hide();
        tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        tableView->setModel(_model->getAvailableGamesTableModel());
        tableView->setAlternatingRowColors(true);
        tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        tableView->setColumnWidth(AvailableGamesTableModel::GameType, TYPE_COLUMN_WIDTH);
        tableView->setColumnWidth(AvailableGamesTableModel::Reward, REWARD_COLUMN_WIDTH);
        tableView->setColumnWidth(AvailableGamesTableModel::BlocksLeft, BLOCKSLEFT_COLUMN_WIDTH);
        tableView->setColumnWidth(AvailableGamesTableModel::Hash, HASH_COLUMN_WIDTH);        

        // Last 2 columns are set by the columnResizingFixer, when the table geometry is ready.
        columnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(tableView, HASH_COLUMN_WIDTH, POP_COL_MIN_WIDTH, this);

        // Populate initial data
        updateGamesAvailable();
    }
}

PopDialog::~PopDialog() {
    delete ui;
}

void PopDialog::updateGamesAvailable() {
    if(IsInitialBlockDownload() || chainActive.Height() == 0)
        return;

    if(model && model->getAvailableGamesTableModel()) {
        std::vector<CAvailableGame> games;        
        model->getAvailableGamesTableModel()->updateGames(games);

        CAvailableGame *currentGame = game0board->getCurrentGame();
        if (!currentGame)
            return;

        int targetScore = model->getCurrentScoreTarget();

        for (CAvailableGame& game : games) {
            if (game.gameSourceHash == currentGame->gameSourceHash) {
                game0board->updateCurrentGameInfo(targetScore, game.blocksRemaining);
                return;
            }
        }
        game0board->updateCurrentGameInfo(targetScore, -1);
    }
}

void PopDialog::setEncryptionStatus(int status) {
    switch(status) {
        case WalletModel::Unencrypted:
        case WalletModel::Unlocked:
            ui->releaseSwarmButton->hide();
            break;
        case WalletModel::Locked:
            ui->releaseSwarmButton->show();
            break;
    }
}

void PopDialog::on_releaseSwarmButton_clicked() {
    if(model)
        model->requestUnlock(true);
}

void PopDialog::on_playSelectedButton_clicked() {
    if (!model)
        return;

    if (!ui->gameTable->selectionModel()->hasSelection())
        return;

    QModelIndexList selection = ui->gameTable->selectionModel()->selectedRows();
    QModelIndex index = selection.at(0);
    const AvailableGamesTableModel *submodel = model->getAvailableGamesTableModel();
    const CAvailableGame *game = &(submodel->entry(index.row()));
    
    int targetScore = model->getCurrentScoreTarget();
    if (game)
        game0board->newGame(targetScore, game);
}

void PopDialog::submitSolution(CAvailableGame *g, uint8_t gameType, std::vector<unsigned char> solution) {
    if (!model)
        return;

    std::string strError;
    if (!model->submitSolution(g, gameType, solution, strError))
        QMessageBox::critical(this, tr("Error"), QString::fromStdString("Error: " + strError));
}
