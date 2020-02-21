// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/overviewpage.h>
#include <qt/forms/ui_overviewpage.h>

#include <qt/ringunits.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/transactionfilterproxy.h>
#include <qt/transactiontablemodel.h>
#include <qt/walletmodel.h>
#include <qt/hivetablemodel.h>  // Ring-fork: Hive
#include <qt/hivedialog.h>      // Ring-fork: Hive: For formatLargeNoLocale()
#include <miner.h>              // Ring-fork: The Village: For DEFAULT_GENERATE

#include <QAbstractItemDelegate>
#include <QPainter>

#define DECORATION_SIZE 54
#define NUM_ITEMS 5

Q_DECLARE_METATYPE(interfaces::WalletBalances)

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    explicit TxViewDelegate(const PlatformStyle *_platformStyle, QObject *parent=nullptr):
        QAbstractItemDelegate(parent), unit(RingUnits::RNG),
        platformStyle(_platformStyle)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(TransactionTableModel::RawDecorationRole));
        QRect mainRect = option.rect;
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon = platformStyle->TextColorIcon(icon);    // Ring-fork: Skinning: Force single colour here
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(value.canConvert<QBrush>())
        {
            QBrush brush = qvariant_cast<QBrush>(value);
            foreground = brush.color();
        }

        painter->setPen(foreground);
        QRect boundingRect;
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address, &boundingRect);

        if (index.data(TransactionTableModel::WatchonlyRole).toBool())
        {
            QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
            QRect watchonlyRect(boundingRect.right() + 5, mainRect.top()+ypad+halfheight, 16, halfheight);
            iconWatchonly.paint(painter, watchonlyRect);
        }

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
        }
        painter->setPen(foreground);
        QString amountText = RingUnits::formatWithUnit(unit, amount, true, RingUnits::separatorAlways);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;
    const PlatformStyle *platformStyle;

};
#include <qt/overviewpage.moc>

OverviewPage::OverviewPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    clientModel(nullptr),
    walletModel(nullptr),
    showBuilding1(false),   // Ring-fork: The village
    showBuilding2(false),
    showBuilding3(false),
    dayNightTimer(NULL),
    scene(new QGraphicsScene(this)),
    txdelegate(new TxViewDelegate(platformStyle, this))
{
    ui->setupUi(this);

    m_balances.balance = -1;

    // use a SingleColorIcon for the "out of sync warning" icon
    QIcon icon = platformStyle->SingleColorIcon(":/icons/warning");
    icon.addPixmap(icon.pixmap(QSize(64,64), QIcon::Normal), QIcon::Disabled); // also set the disabled icon because we are using a disabled QPushButton to work around missing HiDPI support of QLabel (https://bugreports.qt.io/browse/QTBUG-42503)
    ui->labelTransactionsStatus->setIcon(icon);
    ui->labelWalletStatus->setIcon(icon);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, &QListView::clicked, this, &OverviewPage::handleTransactionClicked);

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
    connect(ui->labelWalletStatus, &QPushButton::clicked, this, &OverviewPage::handleOutOfSyncWarningClicks);
    connect(ui->labelTransactionsStatus, &QPushButton::clicked, this, &OverviewPage::handleOutOfSyncWarningClicks);

    // Ring-fork: Hive
    cost = rewardsPaid = profit = 0;
    
    // Ring-fork: The Village
    ui->graphicsView->setScene(scene);
    drawVillage();
    setDayNightTimer();

    // Fire a resize
    QTimer::singleShot(500, [this]{
        ui->graphicsView->resize(ui->graphicsView->width(), ui->graphicsView->height());
        ui->graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    });
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        Q_EMIT transactionClicked(filter->mapToSource(index));
}

void OverviewPage::handleOutOfSyncWarningClicks()
{
    Q_EMIT outOfSyncWarningClicked();
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setBalance(const interfaces::WalletBalances& balances)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    m_balances = balances;
    if (walletModel->privateKeysDisabled()) {
        ui->labelBalance->setText(RingUnits::formatWithUnit(unit, balances.watch_only_balance, false, RingUnits::separatorAlways));
        ui->labelUnconfirmed->setText(RingUnits::formatWithUnit(unit, balances.unconfirmed_watch_only_balance, false, RingUnits::separatorAlways));
        ui->labelImmature->setText(RingUnits::formatWithUnit(unit, balances.immature_watch_only_balance, false, RingUnits::separatorAlways));
        ui->labelTotal->setText(RingUnits::formatWithUnit(unit, balances.watch_only_balance + balances.unconfirmed_watch_only_balance + balances.immature_watch_only_balance, false, RingUnits::separatorAlways));
    } else {
        ui->labelBalance->setText(RingUnits::formatWithUnit(unit, balances.balance, false, RingUnits::separatorAlways));
        ui->labelUnconfirmed->setText(RingUnits::formatWithUnit(unit, balances.unconfirmed_balance, false, RingUnits::separatorAlways));
        ui->labelImmature->setText(RingUnits::formatWithUnit(unit, balances.immature_balance, false, RingUnits::separatorAlways));
        ui->labelTotal->setText(RingUnits::formatWithUnit(unit, balances.balance + balances.unconfirmed_balance + balances.immature_balance, false, RingUnits::separatorAlways));
        ui->labelWatchAvailable->setText(RingUnits::formatWithUnit(unit, balances.watch_only_balance, false, RingUnits::separatorAlways));
        ui->labelWatchPending->setText(RingUnits::formatWithUnit(unit, balances.unconfirmed_watch_only_balance, false, RingUnits::separatorAlways));
        ui->labelWatchImmature->setText(RingUnits::formatWithUnit(unit, balances.immature_watch_only_balance, false, RingUnits::separatorAlways));
        ui->labelWatchTotal->setText(RingUnits::formatWithUnit(unit, balances.watch_only_balance + balances.unconfirmed_watch_only_balance + balances.immature_watch_only_balance, false, RingUnits::separatorAlways));
    }
    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = balances.immature_balance != 0;
    bool showWatchOnlyImmature = balances.immature_watch_only_balance != 0;

    // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelImmature->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelWatchImmature->setVisible(!walletModel->privateKeysDisabled() && showWatchOnlyImmature); // show watch-only immature balance
}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{
    ui->labelSpendable->setVisible(showWatchOnly);      // show spendable label (only when watch-only is active)
    ui->labelWatchonly->setVisible(showWatchOnly);      // show watch-only label
    ui->lineWatchBalance->setVisible(showWatchOnly);    // show watch-only balance separator line
    ui->labelWatchAvailable->setVisible(showWatchOnly); // show watch-only available balance
    ui->labelWatchPending->setVisible(showWatchOnly);   // show watch-only pending balance
    ui->labelWatchTotal->setVisible(showWatchOnly);     // show watch-only total balance

    if (!showWatchOnly)
        ui->labelWatchImmature->hide();
}

void OverviewPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
        // Show warning if this is a prerelease version
        connect(model, &ClientModel::alertsChanged, this, &OverviewPage::updateAlerts);
        updateAlerts(model->getStatusBarWarnings());

        // Ring-fork: The Village
        connect(model, &ClientModel::generateChanged, this, &OverviewPage::checkVillageGenerate);
    }
}

void OverviewPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter.reset(new TransactionFilterProxy());
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter.get());
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        interfaces::Wallet& wallet = model->wallet();
        interfaces::WalletBalances balances = wallet.getBalances();
        setBalance(balances);
        connect(model, &WalletModel::balanceChanged, this, &OverviewPage::setBalance);

        connect(model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &OverviewPage::updateDisplayUnit);

        updateWatchOnlyLabels(wallet.haveWatchOnly() && !model->privateKeysDisabled());
        connect(model, &WalletModel::notifyWatchonlyChanged, [this](bool showWatchOnly) {
            updateWatchOnlyLabels(showWatchOnly && !walletModel->privateKeysDisabled());
        });

        // Ring-fork: Hive: Connect summary updater
        connect(model, SIGNAL(newHiveSummaryAvailable()), this, SLOT(updateHiveSummary()));

        // Ring-fork: The Village: Set village name
        setVillageName();
    }

    // update the display unit, to not use the default ("RNG")
    updateDisplayUnit();
}

// Ring-fork: Hive: Update the hive summary
void OverviewPage::updateHiveSummary() {
    if (walletModel && walletModel->getHiveTableModel()) {
        int immature, mature, dead, blocksFound;
        walletModel->getHiveTableModel()->getSummaryValues(immature, mature, dead, blocksFound, cost, rewardsPaid, profit);

        ui->rewardsPaidLabel->setText(
            RingUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), rewardsPaid)
            + " "
            + RingUnits::shortName(walletModel->getOptionsModel()->getDisplayUnit())
        );
        ui->costLabel->setText(
            RingUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), cost)
            + " "
            + RingUnits::shortName(walletModel->getOptionsModel()->getDisplayUnit())
        );
        ui->profitLabel->setText(
            RingUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), profit)
            + " "
            + RingUnits::shortName(walletModel->getOptionsModel()->getDisplayUnit())
        );
        ui->matureLabel->setText(HiveDialog::formatLargeNoLocale(mature));
        ui->immatureLabel->setText(HiveDialog::formatLargeNoLocale(immature));
        ui->blocksFoundLabel->setText(HiveDialog::formatLargeNoLocale(blocksFound));
        
        if (dead > 0) {
            ui->deadLabel->setText(HiveDialog::formatLargeNoLocale(dead));
            ui->deadLabel->setVisible(true);
            ui->deadPreLabel->setVisible(true);
            ui->deadPostLabel->setVisible(true);
        } else {
            ui->deadLabel->setVisible(false);
            ui->deadPreLabel->setVisible(false);
            ui->deadPostLabel->setVisible(false);
        }

        // Ring-fork: The Village: Show building 2 when bees are mature
        if (mature > 0 && !showBuilding2) {
            showBuilding2 = true;
            drawVillage();
        } else if (mature == 0 && showBuilding2) {
            showBuilding2 = false;
            drawVillage();
        }      
    }
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if (m_balances.balance != -1) {
            setBalance(m_balances);
        }

        // Update txdelegate->unit with the current unit
        txdelegate->unit = walletModel->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->update();

        // Ring-fork: Hive: Update CAmounts in hive summary
        ui->rewardsPaidLabel->setText(
            RingUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), rewardsPaid)
            + " "
            + RingUnits::shortName(walletModel->getOptionsModel()->getDisplayUnit())
        );
        ui->costLabel->setText(
            RingUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), cost)
            + " "
            + RingUnits::shortName(walletModel->getOptionsModel()->getDisplayUnit())
        );
        ui->profitLabel->setText(
            RingUnits::format(walletModel->getOptionsModel()->getDisplayUnit(), profit)
            + " "
            + RingUnits::shortName(walletModel->getOptionsModel()->getDisplayUnit())
        );        
    }
}

void OverviewPage::updateAlerts(const QString &warnings)
{
    this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    this->ui->labelAlerts->setText(warnings);
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}

// Ring-fork: Hive: Handle dwarf button click
void OverviewPage::on_hiveButton_clicked() {
    Q_EMIT hiveButtonClicked();
}

// Ring-fork: The village
void OverviewPage::drawVillage() {    
    scene->clear();
    
    int hour = QDateTime::currentDateTime().time().hour();
    bool isNight = (hour >= 20 || hour < 9);
    QString prefix = isNight ? "night_" : "day_";

    if (showBuilding1 && showBuilding2 && showBuilding3) {  // Everything's on!
        scene->addPixmap(QPixmap::fromImage(QImage(":/icons/" + prefix + "village_on")));
    } else {
        scene->addPixmap(QPixmap::fromImage(QImage(":/icons/" + prefix + "village_off")));

        if (showBuilding1) {    // Pow mining is on
            QGraphicsPixmapItem *item = scene->addPixmap(QPixmap::fromImage(QImage(":/icons/" + prefix + "b1_on")));
            if (isNight)
                item->setOffset(331, 10);
            else
                item->setOffset(306, 135);
        }
        if (showBuilding2) {    // Hive dwarves are mature
            QGraphicsPixmapItem *item = scene->addPixmap(QPixmap::fromImage(QImage(":/icons/" + prefix + "b2_on")));
            if (isNight) 
                item->setOffset(516, 377);
            else
                item->setOffset(525, 332);
        }
        if (showBuilding3) {    // A pop block has been found this session
            QGraphicsPixmapItem *item = scene->addPixmap(QPixmap::fromImage(QImage(":/icons/" + prefix + "b3_on")));
            if (isNight) 
                item->setOffset(741, 231);
            else
                item->setOffset(739, 204);
        }
    }

    ui->graphicsView->resize(ui->graphicsView->width(), ui->graphicsView->height());
    ui->graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

// Ring-fork: The Village: Show building 1 when pow mining
void OverviewPage::checkVillageGenerate() {
    if (gArgs.GetBoolArg("-gen", DEFAULT_GENERATE) && !showBuilding1) {
        showBuilding1 = true;
        drawVillage();
    } else if (!gArgs.GetBoolArg("-gen", DEFAULT_GENERATE) && showBuilding1) {
        showBuilding1 = false;
        drawVillage();
    }
}

// Ring-fork: The Village: Enable pop building
void OverviewPage::enableVillagePop() {
    if (!showBuilding3) {
        showBuilding3 = true;
        drawVillage();
    }
}

// Ring-fork: The Village: Handle resize
void OverviewPage::resizeEvent(QResizeEvent* event) {
    ui->graphicsView->resize(ui->graphicsView->width(), ui->graphicsView->height());
    ui->graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

// Ring-fork: The Village: Set the village name
void OverviewPage::setVillageName() {
    CKeyID seed = walletModel->getHDChainSeed();
    if (seed.IsNull())
        return;

    QString digraphs[] = {
         "la",	"ve",	"ta",	"re",	"or",	"za",	"us",	"ac",
         "te",	"ce",	"at",	"a",	"e",	"o",	"le",	"fa",
         "he",	"na",	"ar",	"to",	"oi",	"ne",	"no",	"ba",
         "bo",	"ha",	"ve",	"va",	"ax",	"is",	"or",	"in",
         "mo",	"on",	"cra",	"ud",	"sa",	"tu",	"ju",	"pi",
         "mi",	"gu",	"it",	"ob",	"os",	"ut",	"ne",	"as",
         "en",	"ky",	"tha",	"um",	"ka",	"qt",	"zi",	"ou",
         "ga",	"dro",	"dre",	"pha",	"phi",	"sha",	"she",	"fo",
         "cre",	"tri",	"ro",	"sta",	"stu",	"de",	"gi",	"pe",
         "the",	"thi",	"thy",	"lo",	"ol",	"clu",	"cla",	"le",
         "di",	"so",	"ti",	"es",	"ed",	"bi",	"po",	"ni",
         "ex",	"ad",	"un",	"pho",	"ci",	"ge",	"se",	"co",
         "ja",	"vu",	"ta",	"re",	"or",	"za",	"us",	"ac",
         "te",	"ce",	"at",	"a",	"e",	"o",	"le",	"fa",
         "he",	"na",	"ar",	"to",	"oi",	"ne",	"no",	"ba",
         "bo",	"ha",	"vo",	"va",	"ax",	"is",	"or",	"in"
    };

    QString name;
    unsigned int digraphCount = (seed.ByteAt(0) & 3) + 2;
    for (unsigned int i = 0; i < digraphCount; i++)
        name += digraphs[seed.ByteAt(i+1) & 127];
        
    name[0] = name[0].toUpper();

    ui->villageNameLabel->setText("(\"" + name + "\")");
}

// Ring-fork: The Village: Set the day/night cycle timer
void OverviewPage::setDayNightTimer() {
    QDateTime t = QDateTime::currentDateTime();
    int hour = t.time().hour();

    if (hour < 8)
        t.setTime(QTime::fromString("08:00", "hh:mm"));
    else if (hour < 20)
        t.setTime(QTime::fromString("20:00", "hh:mm"));
    else {
        t = t.addDays(1);
        t.setTime(QTime::fromString("08:00", "hh:mm"));
    }

    qint64 delay = QDateTime::currentDateTime().msecsTo(t);
    delay += 10 * 1000; // Add a small delay
    if (dayNightTimer) {
        dayNightTimer->stop();
        delete dayNightTimer;
    }
    dayNightTimer = new QTimer();
    connect(dayNightTimer, SIGNAL(timeout()), this, SLOT(tickDayNightCycle()));
    dayNightTimer->start(delay);
}

// Ring-fork: The Village: Redraw village (day/night just changed) and reset the timer
void OverviewPage::tickDayNightCycle() {
    drawVillage();
    setDayNightTimer();
}
