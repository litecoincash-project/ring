// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Ring-fork: Pop

#include <qt/pop/availablegamestablemodel.h>

#include <qt/ringunits.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/uicolours.h>

#include <clientversion.h>
#include <streams.h>
#include <chainparams.h>

#include <validation.h> // For block subs

AvailableGamesTableModel::AvailableGamesTableModel(const PlatformStyle *_platformStyle, WalletModel *parent) : platformStyle(_platformStyle), QAbstractTableModel(parent), walletModel(parent)
{
    // Set column headings
    columns << tr("Game type") << tr("Reward") << tr("Blocks left") << tr("Source hash");

    sortOrder = Qt::DescendingOrder;
    sortColumn = 0;
}

AvailableGamesTableModel::~AvailableGamesTableModel() {
    // Empty destructor
}

void AvailableGamesTableModel::updateGames(std::vector<CAvailableGame>& vGames) {
    if (walletModel) {
        // Clear existing
        beginResetModel();
        list.clear();
        endResetModel();

        // Load entries from wallet
        walletModel->getAvailableGames(vGames);
        beginInsertRows(QModelIndex(), 0, 0);
        for (CAvailableGame& game : vGames)
            list.prepend(game);
        endInsertRows();

        // Maintain correct sorting
        sort(sortColumn, sortOrder);
    }
}

int AvailableGamesTableModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);

    return list.length();
}

int AvailableGamesTableModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);

    return columns.length();
}

QVariant AvailableGamesTableModel::data(const QModelIndex &index, int role) const {
    if(!index.isValid() || index.row() >= list.length())
        return QVariant();

    const CAvailableGame *rec = &list[index.row()];
    if(role == Qt::DisplayRole || role == Qt::EditRole) {        
        switch(index.column()) {
            case GameType:
                return rec->isPrivate ? "Private" : "Public";
            case Reward:
                return RingUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), rec->isPrivate ? GetBlockSubsidyPopPrivate(Params().GetConsensus()) : GetBlockSubsidyPopPublic(Params().GetConsensus())) + " " + RingUnits::shortName(this->walletModel->getOptionsModel()->getDisplayUnit());
            case BlocksLeft:
                return QString::number(rec->blocksRemaining) + " (" + secondsToString(rec->blocksRemaining * Params().GetConsensus().nExpectedBlockSpacing) + ")";
            case Hash:
                return QString::fromStdString(rec->gameSourceHash.ToString().substr(0,8) + "...");
        }
    } else if (role == Qt::TextAlignmentRole) {
        if (index.column() == BlocksLeft)
            return (int)(Qt::AlignRight|Qt::AlignVCenter);
        else
            return (int)(Qt::AlignCenter|Qt::AlignVCenter);
    } else if (role == Qt::ForegroundRole) {
        if (index.column() == BlocksLeft && rec->blocksRemaining <= 10)
            return QColor(255, 0, 0);
        else if (rec->isPrivate)
            return QColor(SKIN_BG_BUTTON);
        else
            return QColor(SKIN_TEXT);
    } else if (role == Qt::DecorationRole) {
        if (index.column() == BlocksLeft && rec->blocksRemaining <= 10)
            return platformStyle->SingleColorIcon(":/icons/warning");
    }
    return QVariant();
}

bool AvailableGamesTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    return true;
}

QVariant AvailableGamesTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if(orientation == Qt::Horizontal)
        if(role == Qt::DisplayRole && section < columns.size())
            return columns[section];

    return QVariant();
}

void AvailableGamesTableModel::sort(int column, Qt::SortOrder order) {
    sortColumn = column;
    sortOrder = order;
    qSort(list.begin(), list.end(), CAvailableGameLessThan(column, order));
    Q_EMIT dataChanged(index(0, 0, QModelIndex()), index(list.size() - 1, NUMBER_OF_COLUMNS - 1, QModelIndex()));
}

bool CAvailableGameLessThan::operator()(CAvailableGame &left, CAvailableGame &right) const {
    CAvailableGame *pLeft = &left;
    CAvailableGame *pRight = &right;
    if (order == Qt::DescendingOrder)
        std::swap(pLeft, pRight);

    switch(column) {
        case AvailableGamesTableModel::Hash:
            return pLeft->gameSourceHash < pRight->gameSourceHash;
        case AvailableGamesTableModel::BlocksLeft:
            return pLeft->blocksRemaining < pRight->blocksRemaining;        

        default:
            return pLeft->isPrivate < pRight->isPrivate;            
    }
}

QString AvailableGamesTableModel::secondsToString(qint64 seconds) {
    const qint64 DAY = 86400;
    qint64 days = seconds / DAY;
    QTime t = QTime(0,0).addSecs(seconds % DAY);
    qint64 hours = t.hour();
    qint64 mins = t.minute();

    if (days == 0 && hours == 0 && mins == 0)
        return QString("< 1 min!");

    QString s;
    if (days == 1)
        s += "1 day ";
    else if (days > 1)
        s += QString("%1 days ").arg(days);
    
    if (hours != 0 || days > 0) {
        if (hours == 1)
            s += mins > 0 ? "1 hr" : "1 hour";
        else
            s += mins > 0 ? QString("%1 hrs").arg(hours) : QString("%1 hours").arg(hours);
    }

    if (mins == 1)
        s += " 1 min";
    else if (mins > 0)
        s += QString(" %1 mins").arg(mins);

    if (s[0] == ' ')
        s = s.right(s.size() - 1);  // Dobby has no sock
    return s;
}
