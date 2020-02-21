// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RING_QT_OVERVIEWPAGE_H
#define RING_QT_OVERVIEWPAGE_H

#include <interfaces/wallet.h>

#include <QGraphicsScene>   // Ring-fork: The village
#include <QGraphicsView>    // Ring-fork: The village
#include <QGraphicsItem>    // Ring-fork: The village
#include <QWidget>
#include <memory>

class ClientModel;
class TransactionFilterProxy;
class TxViewDelegate;
class PlatformStyle;
class WalletModel;

namespace Ui {
    class OverviewPage;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Overview ("home") page widget */
class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~OverviewPage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    void showOutOfSyncWarning(bool fShow);
    void enableVillagePop();                    // Ring-fork: The Village

protected:
    void resizeEvent(QResizeEvent* event);      // Ring-fork: The Village

public Q_SLOTS:
    void setBalance(const interfaces::WalletBalances& balances);

Q_SIGNALS:
    void transactionClicked(const QModelIndex &index);
    void hiveButtonClicked();                   // Ring-fork: Hive
    void outOfSyncWarningClicked();

private:
    Ui::OverviewPage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    interfaces::WalletBalances m_balances;
    CAmount cost, rewardsPaid, profit;          // Ring-fork: Hive
    QTimer *dayNightTimer;                      // Ring-fork: The Village
    
    TxViewDelegate *txdelegate;
    std::unique_ptr<TransactionFilterProxy> filter;

    // Ring-fork: The village
    QGraphicsScene *scene;
    void setVillageName();
    void drawVillage();
    void setDayNightTimer();
    bool showBuilding1, showBuilding2, showBuilding3;

private Q_SLOTS:
    void updateDisplayUnit();
    void handleTransactionClicked(const QModelIndex &index);
    void updateAlerts(const QString &warnings);
    void updateWatchOnlyLabels(bool showWatchOnly);
    void handleOutOfSyncWarningClicks();

    void checkVillageGenerate();                // Ring-fork: The Village
    void tickDayNightCycle();                   // Ring-fork: The Village
    void on_hiveButton_clicked();               // Ring-fork: Hive: Hive button handler
    void updateHiveSummary();                   // Ring-fork: Hive: Update hive summary
};

#endif // RING_QT_OVERVIEWPAGE_H
