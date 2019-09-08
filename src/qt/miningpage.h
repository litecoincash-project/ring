// Copyright (c) 2018-2019 The Ring Developers
// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RING_QT_MININGPAGE_H
#define RING_QT_MININGPAGE_H

#include <interfaces/wallet.h>

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsItem>

#include <memory>

class ClientModel;
class PlatformStyle;
class WalletModel;

extern double dHashesPerSec;

namespace Ui {
    class MiningPage;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

class MiningPage : public QWidget
{
    Q_OBJECT

public:
    explicit MiningPage(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~MiningPage();

    void setClientModel(ClientModel *clientModel);

private:
    Ui::MiningPage *ui;
    ClientModel *clientModel;
    QTimer *displayUpdateTimer;

    QGraphicsScene *scene;
    QPixmap minotaurs[8];
    void drawMinotaurs(int coloured);
    void makeMinotaurs();

private Q_SLOTS:
    void on_toggleMiningButton_clicked();
    void on_useAllCoresCheckbox_stateChanged();
    void updateHashRateDisplay();
    void updateDisplayedOptions();
    void numConnectionsChanged(int connections);
    void increaseBlocksFound();
};

#endif // RING_QT_MININGPAGE_H
