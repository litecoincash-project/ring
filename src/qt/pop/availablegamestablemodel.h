// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Ring-fork: Pop

#ifndef RING_QT_AVAILABLEGAMESTABLEMODEL_H
#define RING_QT_AVAILABLEGAMESTABLEMODEL_H

#include <qt/walletmodel.h>
#include <wallet/wallet.h>

#include <QAbstractTableModel>
#include <QStringList>
#include <QDateTime>

class CWallet;

class CAvailableGameLessThan
{
public:
    CAvailableGameLessThan(int nColumn, Qt::SortOrder fOrder):
        column(nColumn), order(fOrder) {}
    bool operator()(CAvailableGame &left, CAvailableGame &right) const;
    
private:
    int column;
    Qt::SortOrder order;
};

class AvailableGamesTableModel: public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit AvailableGamesTableModel(const PlatformStyle *_platformStyle, WalletModel *parent);
    ~AvailableGamesTableModel();

    enum ColumnIndex {
        GameType = 0,
        Hash = 1,
        BlocksLeft = 2,
        EstimatedTime = 3,
        NUMBER_OF_COLUMNS
    };

    void updateGames(std::vector<CAvailableGame>& vGames);
    const CAvailableGame &entry(int row) const { return list[row]; }
    static QString secondsToString(qint64 seconds);

    // Stuff overridden from QAbstractTableModel
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

public Q_SLOTS:
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

private:
    void addGame(const CAvailableGame &game);

    const PlatformStyle *platformStyle;
    WalletModel *walletModel;
    QStringList columns;
    QList<CAvailableGame> list;
    int sortColumn;
    Qt::SortOrder sortOrder;
};

#endif // RING_QT_AVAILABLEGAMESTABLEMODEL_H
