// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Ring-fork: Hive

#include <qt/hivetablemodel.h>

#include <qt/ringunits.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/hivedialog.h>  // For formatLargeNoLocale()
#include <qt/uicolours.h>

#include <clientversion.h>
#include <streams.h>
#include <chainparams.h>

HiveTableModel::HiveTableModel(const PlatformStyle *_platformStyle, WalletModel *parent) : platformStyle(_platformStyle), QAbstractTableModel(parent), walletModel(parent)
{
    // Set column headings
    columns << tr("Created") << tr("Dwarf count") << tr("Dwarf status") << tr("Estimated time until status change") << tr("Dwarf cost") << tr("Rewards earned");

    sortOrder = Qt::DescendingOrder;
    sortColumn = 0;

    rewardsPaid = cost = profit = 0;
    immature = mature = dead = blocksFound = 0; 
}

HiveTableModel::~HiveTableModel() {
    // Empty destructor
}

void HiveTableModel::updateDCTs(bool includeDeadDwarves) {
    if (walletModel) {
        // Clear existing
        beginResetModel();
        list.clear();
        endResetModel();

        // Load entries from wallet
        std::vector<CDwarfCreationTransactionInfo> vDwarfCreationTransactions;
        walletModel->getDCTs(vDwarfCreationTransactions, includeDeadDwarves);
        beginInsertRows(QModelIndex(), 0, 0);
        immature = 0, mature = 0, dead = 0, blocksFound = 0;
        cost = rewardsPaid = profit = 0;
        for (const CDwarfCreationTransactionInfo& dct : vDwarfCreationTransactions) {
            if (dct.dwarfStatus == "mature")
                mature += dct.dwarfCount;
            else if (dct.dwarfStatus == "immature")
                immature += dct.dwarfCount;
            else if (dct.dwarfStatus == "expired")
                dead += dct.dwarfCount;

            blocksFound += dct.blocksFound;
            cost += dct.dwarfFeePaid;
            rewardsPaid += dct.rewardsPaid;
            profit += dct.profit;

            list.prepend(dct);
        }
        endInsertRows();

        // Maintain correct sorting
        sort(sortColumn, sortOrder);

        // Fire signal
        QMetaObject::invokeMethod(walletModel, "newHiveSummaryAvailable", Qt::QueuedConnection);
    }
}

void HiveTableModel::getSummaryValues(int &_immature, int &_mature, int &_dead, int &_blocksFound, CAmount &_cost, CAmount &_rewardsPaid, CAmount &_profit) {
    _immature = immature;
    _mature = mature;
    _blocksFound = blocksFound;
    _cost = cost;
    _rewardsPaid = rewardsPaid;
    _dead = dead;
    _profit = profit;
}

int HiveTableModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);

    return list.length();
}

int HiveTableModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);

    return columns.length();
}

QVariant HiveTableModel::data(const QModelIndex &index, int role) const {
    if(!index.isValid() || index.row() >= list.length())
        return QVariant();

    const CDwarfCreationTransactionInfo *rec = &list[index.row()];
    if(role == Qt::DisplayRole || role == Qt::EditRole) {        
        switch(index.column()) {
            case Created:
                return (rec->time == 0) ? "Not in chain yet" : GUIUtil::dateTimeStr(rec->time);
            case Count:
                return HiveDialog::formatLargeNoLocale(rec->dwarfCount);
            case Status:
                {
                    QString status = QString::fromStdString(rec->dwarfStatus);
                    status[0] = status[0].toUpper();
                    return status;
                }
            case EstimatedTime:
                {
                    QString status = "";
                    if (rec->dwarfStatus == "immature") {
                        int blocksTillMature = rec->blocksLeft - Params().GetConsensus().dwarfLifespanBlocks;
                        status = "Matures in " + QString::number(blocksTillMature) + " blocks (" + secondsToString(blocksTillMature * Params().GetConsensus().nPowTargetSpacing / 2) + ")";
                    } else if (rec->dwarfStatus == "mature")
                        status = "Expires in " + QString::number(rec->blocksLeft) + " blocks (" + secondsToString(rec->blocksLeft * Params().GetConsensus().nPowTargetSpacing / 2) + ")";
                    return status;
                }
            case Cost:
                return RingUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), rec->dwarfFeePaid) + " " + RingUnits::shortName(this->walletModel->getOptionsModel()->getDisplayUnit());
            case Rewards:
                if (rec->blocksFound == 0)
                    return "No blocks mined";
                return RingUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), rec->rewardsPaid)
                    + " " + RingUnits::shortName(this->walletModel->getOptionsModel()->getDisplayUnit()) 
                    + " (" + QString::number(rec->blocksFound) + " blocks mined)";
        }
    } else if (role == Qt::TextAlignmentRole) {
        /*if (index.column() == Rewards && rec->blocksFound == 0)
            return (int)(Qt::AlignCenter|Qt::AlignVCenter);
        else*/ if (index.column() == Cost || index.column() == Rewards || index.column() == Count)
            return (int)(Qt::AlignRight|Qt::AlignVCenter);
        else
            return (int)(Qt::AlignCenter|Qt::AlignVCenter);
    } else if (role == Qt::ForegroundRole) {
        const CDwarfCreationTransactionInfo *rec = &list[index.row()];

        if (index.column() == Rewards) {
            if (rec->blocksFound == 0)
                return QColor(200, 0, 0);
            if (rec->profit < 0)
                return QColor(170, 70, 0);
            return QColor(27, 170, 45);
        }
        
        if (index.column() == Status) {
            if (rec->dwarfStatus == "expired")
                return QColor(200, 0, 0);
            if (rec->dwarfStatus == "immature")
                return QColor(170, 70, 0);
            return QColor(27, 170, 45);
        }

        return SKIN_TEXT;
    } else if (role == Qt::DecorationRole) {
        const CDwarfCreationTransactionInfo *rec = &list[index.row()];
        if (index.column() == Status) {
            QString iconStr = ":/icons/agentstatus_dead";    // Dead
            if (rec->dwarfStatus == "mature")
                iconStr = ":/icons/agentstatus_mature";
            else if (rec->dwarfStatus == "immature")
                iconStr = ":/icons/agentstatus_immature";                
            return platformStyle->SingleColorIcon(iconStr);
        }
    }
    return QVariant();
}

bool HiveTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    return true;
}

QVariant HiveTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if(orientation == Qt::Horizontal)
        if(role == Qt::DisplayRole && section < columns.size())
            return columns[section];

    return QVariant();
}

void HiveTableModel::sort(int column, Qt::SortOrder order) {
    sortColumn = column;
    sortOrder = order;
    qSort(list.begin(), list.end(), CDwarfCreationTransactionInfoLessThan(column, order));
    Q_EMIT dataChanged(index(0, 0, QModelIndex()), index(list.size() - 1, NUMBER_OF_COLUMNS - 1, QModelIndex()));
}

bool CDwarfCreationTransactionInfoLessThan::operator()(CDwarfCreationTransactionInfo &left, CDwarfCreationTransactionInfo &right) const {
    CDwarfCreationTransactionInfo *pLeft = &left;
    CDwarfCreationTransactionInfo *pRight = &right;
    if (order == Qt::DescendingOrder)
        std::swap(pLeft, pRight);

    switch(column) {
        case HiveTableModel::Count:
            return pLeft->dwarfCount < pRight->dwarfCount;
        case HiveTableModel::Status:
        case HiveTableModel::EstimatedTime:
            return pLeft->blocksLeft < pRight->blocksLeft;
        case HiveTableModel::Cost:
            return pLeft->dwarfFeePaid < pRight->dwarfFeePaid;
        case HiveTableModel::Rewards:
            return pLeft->rewardsPaid < pRight->rewardsPaid;
        case HiveTableModel::Created:
        default:
            return pLeft->time < pRight->time;
    }
}

QString HiveTableModel::secondsToString(qint64 seconds) {
    const qint64 DAY = 86400;
    qint64 days = seconds / DAY;
    QTime t = QTime(0,0).addSecs(seconds % DAY);
    return QString("%1 days %2 hrs %3 mins").arg(days).arg(t.hour()).arg(t.minute());
}
