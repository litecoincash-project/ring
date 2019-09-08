// Copyright (c) 2018-2019 The Ring Developers
// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/walletview.h>

#include <qt/addressbookpage.h>
#include <qt/askpassphrasedialog.h>
#include <qt/ringgui.h>
#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/overviewpage.h>
#include <qt/miningpage.h>      // Ring-fork: Mining page
#include <qt/hivedialog.h>      // Ring-fork: Hive: Hive page
#include <qt/platformstyle.h>
#include <qt/receivecoinsdialog.h>
#include <qt/sendcoinsdialog.h>
#include <qt/signverifymessagedialog.h>
#include <qt/transactiontablemodel.h>
#include <qt/transactionview.h>
#include <qt/walletmodel.h>
#include <boost/thread.hpp>     // Ring-fork: Key import helper
#include <wallet/rpcwallet.h>   // Ring-fork: Key import helper
#include <wallet/wallet.h>      // Ring-fork: Key import helper
#include <validation.h>         // Ring-fork: Key import helper
#include <key_io.h>             // Ring-fork: Key import helper
#include <miner.h>              // Ring-fork: Key import helper: For DEFAULT_GENERATE
#include <qt/uicolours.h>       // Ring-fork: Skinning

#include <interfaces/node.h>
#include <ui_interface.h>

#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QProgressDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QInputDialog>         // Ring-fork: Key import helper

WalletView::WalletView(const PlatformStyle *_platformStyle, QWidget *parent):
    QStackedWidget(parent),
    clientModel(nullptr),
    walletModel(nullptr),
    platformStyle(_platformStyle)
{
    // Create tabs
    overviewPage = new OverviewPage(platformStyle);

    miningPage = new MiningPage(platformStyle); // Ring-fork: Mining page
    hivePage = new HiveDialog(platformStyle);   // Ring-fork: Hive page

    transactionsPage = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout();
    QHBoxLayout *hbox_buttons = new QHBoxLayout();
    transactionView = new TransactionView(platformStyle, this);
    vbox->addWidget(transactionView);
    QPushButton *exportButton = new QPushButton(tr("&Export"), this);
    exportButton->setToolTip(tr("Export the data in the current tab to a file"));
    if (platformStyle->getImagesOnButtons()) {
        exportButton->setIcon(platformStyle->SingleColorIcon(":/icons/export"));
    }
    hbox_buttons->addStretch();
    hbox_buttons->addWidget(exportButton);
    vbox->addLayout(hbox_buttons);
    transactionsPage->setLayout(vbox);

    receiveCoinsPage = new ReceiveCoinsDialog(platformStyle);
    sendCoinsPage = new SendCoinsDialog(platformStyle);

    usedSendingAddressesPage = new AddressBookPage(platformStyle, AddressBookPage::ForEditing, AddressBookPage::SendingTab, this);
    usedReceivingAddressesPage = new AddressBookPage(platformStyle, AddressBookPage::ForEditing, AddressBookPage::ReceivingTab, this);

    addWidget(overviewPage);
    addWidget(transactionsPage);
    addWidget(receiveCoinsPage);
    addWidget(sendCoinsPage);
    addWidget(miningPage);          // Ring-fork: Mining page
    addWidget(hivePage);            // Ring-fork: Hive page

    // Clicking on a transaction on the overview pre-selects the transaction on the transaction history page
    connect(overviewPage, &OverviewPage::transactionClicked, transactionView, static_cast<void (TransactionView::*)(const QModelIndex&)>(&TransactionView::focusTransaction));

    connect(overviewPage, &OverviewPage::outOfSyncWarningClicked, this, &WalletView::requestedSyncWarningInfo);

    // Highlight transaction after send
    connect(sendCoinsPage, &SendCoinsDialog::coinsSent, transactionView, static_cast<void (TransactionView::*)(const uint256&)>(&TransactionView::focusTransaction));

    // Clicking on "Export" allows to export the transaction list
    connect(exportButton, &QPushButton::clicked, transactionView, &TransactionView::exportClicked);

    // Pass through messages from sendCoinsPage
    connect(sendCoinsPage, &SendCoinsDialog::message, this, &WalletView::message);
    // Pass through messages from transactionView
    connect(transactionView, &TransactionView::message, this, &WalletView::message);

    // Ring-fork: Skinning the tabs
    overviewPage->setStyleSheet(
        "QLabel, QListView {color: " + SKIN_TEXT + "};"
    );
    miningPage->setStyleSheet("QLabel, QCheckBox {color: " + SKIN_TEXT + ";} #miningForm {background-color: " + SKIN_BG_PANEL + ";}");
    receiveCoinsPage->setStyleSheet(
        "QLabel, #useBech32 {color: " + SKIN_TEXT + ";}"
        "#frame2 {background-color: " + SKIN_BG_PANEL + ";}"
        "#recentRequestsView {color: " + SKIN_TEXT + "; background-color: " + SKIN_BG_PANEL + "; alternate-background-color: " + SKIN_BG_ROW_ALT + ";}"
    );
    transactionsPage->setStyleSheet("QTableView {color: " + SKIN_TEXT + "; background-color: " + SKIN_BG_PANEL + "; alternate-background-color: " + SKIN_BG_ROW_ALT + ";}");
    sendCoinsPage->setStyleSheet(
        "#frameFee, #frameCoinControl, #scrollArea, #scrollAreaWidgetContents {background-color: " + SKIN_BG_PANEL + ";}"
        "#frameFee QLabel, #frameFee QCheckBox, #frameFee QRadioButton {color: " + SKIN_TEXT + ";}"
        "#frameCoinControl QLabel, #frameCoinControl QCheckBox, #frameCoinControl QRadioButton {color: " + SKIN_TEXT + ";}"
        "#scrollAreaWidgetContents QLabel, #scrollAreaWidgetContents QCheckBox, #scrollAreaWidgetContents QRadioButton {color: " + SKIN_TEXT + ";}"
        "#label, #labelBalance {color: " + SKIN_TEXT + ";}"
    );
    hivePage->setStyleSheet(
        "#createDwarvesTitleLabel, #showAdvancedStatsCheckbox, #globalNetworkHiveTitleLabel, #yourHiveStatsLabel, #includeDeadDwarvesCheckbox {color: " + SKIN_TEXT + ";}"
        "#createDwarvesForm QLabel, #createDwarvesForm QRadioButton, #createDwarvesForm QCheckBox {color: " + SKIN_TEXT + ";}"
        "#globalHiveFrame QLabel, #globalHiveFrame QRadioButton, #globalHiveFrame QCheckBox {color: " + SKIN_TEXT + ";}"
        "#walletHiveStatsFrame QLabel, #walletHiveStatsFrame QRadioButton, #walletHiveStatsFrame QCheckBox {color: " + SKIN_TEXT + ";}"
        "#createDwarvesForm, #globalHiveSummaryError, #globalHiveSummary {background-color: " + SKIN_BG_PANEL + ";}"
        "#currentHiveView {color: " + SKIN_TEXT + "; background-color: " + SKIN_BG_PANEL + "; alternate-background-color: " + SKIN_BG_ROW_ALT + ";}"
        "#globalHiveSummaryErrorLabel {color: " + SKIN_TEXT + ";}"
    );
    
}

WalletView::~WalletView()
{
}

void WalletView::setRingGUI(RingGUI *gui)
{
    if (gui)
    {
        // Clicking on a transaction on the overview page simply sends you to transaction history page
        connect(overviewPage, &OverviewPage::transactionClicked, gui, &RingGUI::gotoHistoryPage);

        // Ring-fork: Hive: Go to hive page if button on overview clicked
        connect(overviewPage, SIGNAL(hiveButtonClicked()), gui, SLOT(gotoHivePage()));

        // Navigate to transaction history page after send
        connect(sendCoinsPage, &SendCoinsDialog::coinsSent, gui, &RingGUI::gotoHistoryPage);

        // Receive and report messages
        connect(this, &WalletView::message, [gui](const QString &title, const QString &message, unsigned int style) {
            gui->message(title, message, style);
        });

        // Pass through encryption status changed signals
        connect(this, &WalletView::encryptionStatusChanged, gui, &RingGUI::updateWalletStatus);

        // Pass through transaction notifications
        connect(this, &WalletView::incomingTransaction, gui, &RingGUI::incomingTransaction);

        // Connect HD enabled state signal
        connect(this, &WalletView::hdEnabledStatusChanged, gui, &RingGUI::updateWalletStatus);

        // Ring-fork: Hive: Connect hive status update signal
        connect(hivePage, &HiveDialog::hiveStatusIconChanged, gui, &RingGUI::updateHiveStatusIcon);
    }
}

void WalletView::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;

    overviewPage->setClientModel(_clientModel);
    sendCoinsPage->setClientModel(_clientModel);
    miningPage->setClientModel(_clientModel);   // Ring-fork: Mining page
    hivePage->setClientModel(_clientModel);     // Ring-fork: Hive page
}

void WalletView::setWalletModel(WalletModel *_walletModel)
{
    this->walletModel = _walletModel;

    // Put transaction list in tabs
    transactionView->setModel(_walletModel);
    overviewPage->setWalletModel(_walletModel);
    receiveCoinsPage->setModel(_walletModel);
    sendCoinsPage->setModel(_walletModel);
    usedReceivingAddressesPage->setModel(_walletModel ? _walletModel->getAddressTableModel() : nullptr);
    usedSendingAddressesPage->setModel(_walletModel ? _walletModel->getAddressTableModel() : nullptr);
    hivePage->setModel(_walletModel);         // Ring-fork: Hive page
    
    if (_walletModel)
    {
        // Receive and pass through messages from wallet model
        connect(_walletModel, &WalletModel::message, this, &WalletView::message);

        // Handle changes in encryption status
        connect(_walletModel, &WalletModel::encryptionStatusChanged, this, &WalletView::encryptionStatusChanged);
        updateEncryptionStatus();

        // update HD status
        Q_EMIT hdEnabledStatusChanged();

        // Balloon pop-up for new transaction
        connect(_walletModel->getTransactionTableModel(), &TransactionTableModel::rowsInserted, this, &WalletView::processNewTransaction);

        // Ask for passphrase if needed
        connect(_walletModel, &WalletModel::requireUnlock, this, &WalletView::unlockWallet);

		// Ring-fork: Hive: Ask for passphrase if needed hive only
        connect(_walletModel, &WalletModel::requireUnlockHive, this, &WalletView::unlockWalletHive);

        // Show progress dialog
        connect(_walletModel, &WalletModel::showProgress, this, &WalletView::showProgress);
    }
}

void WalletView::processNewTransaction(const QModelIndex& parent, int start, int /*end*/)
{
    // Prevent balloon-spam when initial block download is in progress
    if (!walletModel || !clientModel || clientModel->node().isInitialBlockDownload())
        return;

    TransactionTableModel *ttm = walletModel->getTransactionTableModel();
    if (!ttm || ttm->processingQueuedTransactions())
        return;

    QString date = ttm->index(start, TransactionTableModel::Date, parent).data().toString();
    qint64 amount = ttm->index(start, TransactionTableModel::Amount, parent).data(Qt::EditRole).toULongLong();
    QString type = ttm->index(start, TransactionTableModel::Type, parent).data().toString();
    QModelIndex index = ttm->index(start, 0, parent);
    QString address = ttm->data(index, TransactionTableModel::AddressRole).toString();
    QString label = ttm->data(index, TransactionTableModel::LabelRole).toString();

    Q_EMIT incomingTransaction(date, walletModel->getOptionsModel()->getDisplayUnit(), amount, type, address, label, walletModel->getWalletName());
}

void WalletView::gotoOverviewPage()
{
    setCurrentWidget(overviewPage);
}

// Ring-fork: Mining page
void WalletView::gotoMiningPage()
{
    setCurrentWidget(miningPage);
}

// Ring-fork: Hive page
void WalletView::gotoHivePage()
{
    hivePage->updateData();
    setCurrentWidget(hivePage);
}

void WalletView::gotoHistoryPage()
{
    setCurrentWidget(transactionsPage);
}

void WalletView::gotoReceiveCoinsPage()
{
    setCurrentWidget(receiveCoinsPage);
}

void WalletView::gotoSendCoinsPage(QString addr)
{
    setCurrentWidget(sendCoinsPage);

    if (!addr.isEmpty())
        sendCoinsPage->setAddress(addr);
}

void WalletView::gotoSignMessageTab(QString addr)
{
    // calls show() in showTab_SM()
    SignVerifyMessageDialog *signVerifyMessageDialog = new SignVerifyMessageDialog(platformStyle, this);
    signVerifyMessageDialog->setAttribute(Qt::WA_DeleteOnClose);
    signVerifyMessageDialog->setModel(walletModel);
    signVerifyMessageDialog->showTab_SM(true);

    if (!addr.isEmpty())
        signVerifyMessageDialog->setAddress_SM(addr);
}

void WalletView::gotoVerifyMessageTab(QString addr)
{
    // calls show() in showTab_VM()
    SignVerifyMessageDialog *signVerifyMessageDialog = new SignVerifyMessageDialog(platformStyle, this);
    signVerifyMessageDialog->setAttribute(Qt::WA_DeleteOnClose);
    signVerifyMessageDialog->setModel(walletModel);
    signVerifyMessageDialog->showTab_VM(true);

    if (!addr.isEmpty())
        signVerifyMessageDialog->setAddress_VM(addr);
}

bool WalletView::handlePaymentRequest(const SendCoinsRecipient& recipient)
{
    return sendCoinsPage->handlePaymentRequest(recipient);
}

void WalletView::showOutOfSyncWarning(bool fShow)
{
    overviewPage->showOutOfSyncWarning(fShow);
}

void WalletView::updateEncryptionStatus()
{
    Q_EMIT encryptionStatusChanged(walletModel->getEncryptionStatus());     // Ring-fork: Hive: Support locked wallets
}

void WalletView::encryptWallet(bool status)
{
    if(!walletModel)
        return;
    AskPassphraseDialog dlg(status ? AskPassphraseDialog::Encrypt : AskPassphraseDialog::Decrypt, this);
    dlg.setModel(walletModel);
    dlg.exec();

    updateEncryptionStatus();
}

void WalletView::backupWallet()
{
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Backup Wallet"), QString(),
        tr("Wallet Data (*.dat)"), nullptr);

    if (filename.isEmpty())
        return;

    if (!walletModel->wallet().backupWallet(filename.toLocal8Bit().data())) {
        Q_EMIT message(tr("Backup Failed"), tr("There was an error trying to save the wallet data to %1.").arg(filename),
            CClientUIInterface::MSG_ERROR);
        }
    else {
        Q_EMIT message(tr("Backup Successful"), tr("The wallet data was successfully saved to %1.").arg(filename),
            CClientUIInterface::MSG_INFORMATION);
    }
}

void WalletView::changePassphrase()
{
    AskPassphraseDialog dlg(AskPassphraseDialog::ChangePass, this);
    dlg.setModel(walletModel);
    dlg.exec();
}

void WalletView::unlockWallet()
{
    if(!walletModel)
        return;
    // Unlock wallet when requested by wallet model
    if (walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        AskPassphraseDialog dlg(AskPassphraseDialog::Unlock, this);
        dlg.setModel(walletModel);
        dlg.exec();
    }
}

// Ring-fork: Hive: Unlock wallet just for hive
void WalletView::unlockWalletHive()
{
    if(!walletModel)
        return;
    // Unlock wallet when requested by wallet model
    if (walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        AskPassphraseDialog dlg(AskPassphraseDialog::UnlockHiveMining, this);
        dlg.setModel(walletModel);
        dlg.exec();
    }
}

void WalletView::usedSendingAddresses()
{
    if(!walletModel)
        return;

    GUIUtil::bringToFront(usedSendingAddressesPage);
}

void WalletView::usedReceivingAddresses()
{
    if(!walletModel)
        return;

    GUIUtil::bringToFront(usedReceivingAddressesPage);
}

void WalletView::showProgress(const QString &title, int nProgress)
{
    if (nProgress == 0) {
        progressDialog = new QProgressDialog(title, tr("Cancel"), 0, 100);
        GUIUtil::PolishProgressDialog(progressDialog);
        progressDialog->setWindowModality(Qt::ApplicationModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setAutoClose(false);
        progressDialog->setValue(0);
    } else if (nProgress == 100) {
        if (progressDialog) {
            progressDialog->close();
            progressDialog->deleteLater();
            progressDialog = nullptr;
        }
    } else if (progressDialog) {
        if (progressDialog->wasCanceled()) {
            getWalletModel()->wallet().abortRescan();
        } else {
            progressDialog->setValue(nProgress);
        }
    }
}

void WalletView::requestedSyncWarningInfo()
{
    Q_EMIT outOfSyncWarningClicked();
}

// Ring-fork: Key import helper
void WalletView::doRescan(CWallet* pwallet, int64_t startTime)
{
    WalletRescanReserver reserver(pwallet);
    if (!reserver.reserve()) {
        QMessageBox::critical(0, tr(PACKAGE_NAME), tr("Wallet is currently rescanning. Abort existing rescan or wait."));
        return;
    }
	pwallet->RescanFromTime(0, reserver, true);
	//QMessageBox::information(0, tr(PACKAGE_NAME), tr("Rescan complete."));
}

// Ring-fork: Key import helper
void WalletView::importPrivateKey()
{
    if (gArgs.GetBoolArg("-gen", DEFAULT_GENERATE)) {
        QMessageBox::critical(0, tr(PACKAGE_NAME), tr("Please stop mining before importing keys."));
        return;
    }

    if (clientModel->node().isInitialBlockDownload()) {
        QMessageBox::critical(0, tr(PACKAGE_NAME), tr("Please wait until wallet is synced before importing keys."));
        return;
    }

    bool ok;
    QString privKey = QInputDialog::getText(0, tr(PACKAGE_NAME), tr("Enter a private key from any supported chain to claim Ring into your wallet."), QLineEdit::Normal, "", &ok);
    if (ok && !privKey.isEmpty()) {
        std::shared_ptr<CWallet> wallet = GetWalletForQTKeyImport();
        CWallet* const pwallet = wallet.get();

        if(!pwallet) {
            QMessageBox::critical(0, tr(PACKAGE_NAME), tr("Couldn't select valid wallet."));
            return;
        }

        if (!EnsureWalletIsAvailable(pwallet, false)) {
            QMessageBox::critical(0, tr(PACKAGE_NAME), tr("Wallet isn't open."));
            return;
        }

        LOCK2(cs_main, pwallet->cs_wallet);

        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        if(!ctx.isValid())  // Unlock wallet was cancelled
            return;

        CKey key = DecodeSecret(privKey.toStdString());
        if (!key.IsValid()) {
            QMessageBox::critical(0, tr(PACKAGE_NAME), tr("That doesn't seem to be a valid private key."));
            return;
        }

        CPubKey pubkey = key.GetPubKey();
        assert(key.VerifyPubKey(pubkey));
        CKeyID vchAddress = pubkey.GetID();
        {
            pwallet->MarkDirty();
            pwallet->SetAddressBook(vchAddress, "", "receive");

            if (pwallet->HaveKey(vchAddress)) {
                QMessageBox::critical(0, tr(PACKAGE_NAME), tr("This key has already been added."));
                return;
            }

            pwallet->mapKeyMetadata[vchAddress].nCreateTime = 1;

            if (!pwallet->AddKeyPubKey(key, pubkey)) {
                QMessageBox::critical(0, tr(PACKAGE_NAME), tr("Error adding key to wallet."));
                return;
            }

            pwallet->UpdateTimeFirstKey(1); // Mark as rescan needed, even if we don't do it now (it'll happen next restart if not before)
            
            QMessageBox msgBox;
            msgBox.setText(tr("Key successfully added to wallet."));
            msgBox.setInformativeText(tr("Rescan now? (Select No if you have more keys to import)"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::No);
            
            if (msgBox.exec() == QMessageBox::Yes)
                boost::thread t{WalletView::doRescan, pwallet, 0};                
        }
        return;
    }
}