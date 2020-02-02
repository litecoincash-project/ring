// Copyright (c) 2019 The Ring Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Ring-fork: Hive

#ifndef RING_QT_POPDIALOG_H
#define RING_QT_POPDIALOG_H

#include <qt/guiutil.h>
#include <uint256.h>

class PlatformStyle;
class ClientModel;
class WalletModel;

class CAvailableGame;
class Game0Board;

class QPixmap;

namespace Ui {
    class PopDialog;
}

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
QT_END_NAMESPACE

class PopDialog : public QDialog
{
    Q_OBJECT

public:
    enum ColumnWidths {
        TYPE_COLUMN_WIDTH = 100,
        HASH_COLUMN_WIDTH = 100,
        BLOCKSLEFT_COLUMN_WIDTH = 100,
        TIME_COLUMN_WIDTH = 120,
        POP_COL_MIN_WIDTH = 100
    };

    explicit PopDialog(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~PopDialog();

    void setClientModel(ClientModel *_clientModel);
    void setModel(WalletModel *model);

public Q_SLOTS:
    void setEncryptionStatus(int status);
    void updateGamesAvailable();

Q_SIGNALS:

private:
    Ui::PopDialog *ui;
    GUIUtil::TableViewLastColumnResizingFixer *columnResizingFixer;
    ClientModel *clientModel;
    WalletModel *model;
    const PlatformStyle *platformStyle;

	Game0Board* game0board;

private Q_SLOTS:
	void submitSolution(CAvailableGame *g, uint8_t gameType, std::vector<unsigned char> solution);
    void on_releaseSwarmButton_clicked();
    void on_playSelectedButton_clicked();
};

#endif // RING_QT_POPDIALOG_H
