// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Ring-fork: Hive

#include <wallet/wallet.h>
#include <wallet/fees.h>

#include <qt/hivedialog.h>
#include <qt/forms/ui_hivedialog.h>
#include <qt/clientmodel.h>
#include <qt/sendcoinsdialog.h>

#include <qt/addressbookpage.h>
#include <qt/addresstablemodel.h>
#include <qt/ringunits.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/receiverequestdialog.h>
#include <qt/hivetablemodel.h>
#include <qt/walletmodel.h>
#include <qt/tinypie.h>
#include <qt/qcustomplot.h>

#include <QAction>
#include <QCursor>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextDocument>

#include <validation.h>
#include <chainparams.h>

HiveDialog::HiveDialog(const PlatformStyle *_platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HiveDialog),
    columnResizingFixer(0),
    model(0),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);

    if (!_platformStyle->getImagesOnButtons())
        ui->createDwarvesButton->setIcon(QIcon());
    else
        ui->createDwarvesButton->setIcon(_platformStyle->SingleColorIcon(":/icons/hiveicon"));

    dwarfCost = totalCost = rewardsPaid = cost = profit = 0;
    immature = mature = dead = blocksFound = 0;
    lastGlobalCheckHeight = 0;
    potentialRewards = 0;
    currentBalance = 0;
    dwarfPopIndex = 0;

    ui->globalHiveSummaryError->hide();

    ui->dwarfPopIndexPie->foregroundCol = Qt::red;

    // Swap cols for hive weight pie
    QColor temp = ui->hiveWeightPie->foregroundCol;
    ui->hiveWeightPie->foregroundCol = ui->hiveWeightPie->backgroundCol;
    ui->hiveWeightPie->backgroundCol = temp;

    initGraph();
    ui->dwarfPopGraph->hide();
}

void HiveDialog::setClientModel(ClientModel *_clientModel) {
    this->clientModel = _clientModel;

    if (_clientModel) {
        connect(_clientModel, SIGNAL(numBlocksChanged(int,QDateTime,double,bool)), this, SLOT(updateData()));
        connect(_clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(updateData()));    // TODO: This may be too expensive to call here, and the only point is to update the hive status icon.
    }
}

void HiveDialog::setModel(WalletModel *_model) {
    this->model = _model;

    if(_model && _model->getOptionsModel())
    {
        _model->getHiveTableModel()->sort(HiveTableModel::Created, Qt::DescendingOrder);

        interfaces::WalletBalances balances = _model->wallet().getBalances();
        setBalance(balances);
        connect(_model, &WalletModel::balanceChanged, this, &HiveDialog::setBalance);
        connect(_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &HiveDialog::updateDisplayUnit);
        updateDisplayUnit();
        
        if (_model->getEncryptionStatus() != WalletModel::Locked)
            ui->releaseSwarmButton->hide();
        connect(_model, SIGNAL(encryptionStatusChanged(int)), this, SLOT(setEncryptionStatus(int)));

        QTableView* tableView = ui->currentHiveView;

        tableView->verticalHeader()->hide();
        tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        tableView->setModel(_model->getHiveTableModel());
        tableView->setAlternatingRowColors(true);
        tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        tableView->setSelectionMode(QAbstractItemView::ContiguousSelection);
        tableView->setColumnWidth(HiveTableModel::Created, CREATED_COLUMN_WIDTH);
        tableView->setColumnWidth(HiveTableModel::Count, COUNT_COLUMN_WIDTH);
        tableView->setColumnWidth(HiveTableModel::Status, STATUS_COLUMN_WIDTH);
        tableView->setColumnWidth(HiveTableModel::EstimatedTime, TIME_COLUMN_WIDTH);
        tableView->setColumnWidth(HiveTableModel::Cost, COST_COLUMN_WIDTH);
        tableView->setColumnWidth(HiveTableModel::Rewards, REWARDS_COLUMN_WIDTH);
        //tableView->setColumnWidth(HiveTableModel::Profit, PROFIT_COLUMN_WIDTH);

        // Last 2 columns are set by the columnResizingFixer, when the table geometry is ready.
        //columnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(tableView, PROFIT_COLUMN_WIDTH, HIVE_COL_MIN_WIDTH, this);
        columnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(tableView, REWARDS_COLUMN_WIDTH, HIVE_COL_MIN_WIDTH, this);

        // Populate initial data
        updateData(true);
    }
}

HiveDialog::~HiveDialog() {
    delete ui;
}

void HiveDialog::setBalance(const interfaces::WalletBalances& balances)
{
    currentBalance = balances.balance;
}

void HiveDialog::setEncryptionStatus(int status) {
    switch(status) {
        case WalletModel::Unencrypted:
        case WalletModel::Unlocked:
            ui->releaseSwarmButton->hide();
            break;
        case WalletModel::Locked:
            ui->releaseSwarmButton->show();
            break;
    }
    updateData();
}

void HiveDialog::setAmountField(QLabel *field, CAmount value) {
    field->setText(
        RingUnits::format(model->getOptionsModel()->getDisplayUnit(), value)
        + " "
        + RingUnits::shortName(model->getOptionsModel()->getDisplayUnit())
    );
}

QString HiveDialog::formatLargeNoLocale(int i) {
    QString i_str = QString::number(i);

    // Use SI-style thin space separators as these are locale independent and can't be confused with the decimal marker.
    QChar thin_sp(THIN_SP_CP);
    int i_size = i_str.size();
    for (int i = 3; i < i_size; i += 3)
        i_str.insert(i_size - i, thin_sp);

    return i_str;
}

void HiveDialog::updateData(bool forceGlobalSummaryUpdate) {
    if(IsInitialBlockDownload() || chainActive.Height() == 0) {
        ui->globalHiveFrame->hide();
        ui->globalHiveSummaryError->show();
        return;
    }
    
    const Consensus::Params& consensusParams = Params().GetConsensus();

    if(model && model->getHiveTableModel()) {
        {
            model->getHiveTableModel()->updateDCTs(ui->includeDeadDwarvesCheckbox->isChecked());
            model->getHiveTableModel()->getSummaryValues(immature, mature, dead, blocksFound, cost, rewardsPaid, profit);
        }

        // Update labels
        setAmountField(ui->rewardsPaidLabel, rewardsPaid);
        setAmountField(ui->costLabel, cost);
        setAmountField(ui->profitLabel, profit);
        ui->matureLabel->setText(formatLargeNoLocale(mature));
        ui->immatureLabel->setText(formatLargeNoLocale(immature));
        ui->blocksFoundLabel->setText(QString::number(blocksFound));

        if(dead == 0) {
            ui->deadLabel->hide();
            ui->deadTitleLabel->hide();
            ui->deadLabelSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        } else {
            ui->deadLabel->setText(formatLargeNoLocale(dead));
            ui->deadLabel->show();
            ui->deadTitleLabel->show();
            ui->deadLabelSpacer->changeSize(ui->immatureLabelSpacer->geometry().width(), 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        }

        // Set icon and tooltip for tray icon
        QString tooltip, icon;
        if (clientModel && clientModel->getNumConnections() == 0) {
            tooltip = "Not hivemining: Wallet is not connected";
            icon = ":/icons/hivestatus_disabled";
        } else {
            if (mature + immature == 0) {
                tooltip = "Not hivemining: No live dwarves currently in wallet";
                icon = ":/icons/hivestatus_clear";
            } else if (mature == 0) {
                tooltip = "Not hivemining: Only immature dwarves currently in wallet";
                icon = ":/icons/hivestatus_orange";
            } else {
                if (model->getEncryptionStatus() == WalletModel::Locked) {
                    tooltip = "WARNING: Dwarves mature but not mining because wallet is locked";
                    icon = ":/icons/hivestatus_red";
                } else {
                    tooltip = "Hivemining: Dwarves mature and mining";
                    icon = ":/icons/hivestatus_green";
                }
            }
        }
        // Now update bitcoingui
        Q_EMIT hiveStatusIconChanged(icon, tooltip);
    }

    dwarfCost = GetDwarfCost(chainActive.Tip()->nHeight, consensusParams);
    setAmountField(ui->dwarfCostLabel, dwarfCost);
    updateTotalCostDisplay();

    if (forceGlobalSummaryUpdate || chainActive.Tip()->nHeight >= lastGlobalCheckHeight + 10) { // Don't update global summary every block
        int globalImmatureDwarves, globalImmatureDCTs, globalMatureDwarves, globalMatureDCTs;
        if (!GetNetworkHiveInfo(globalImmatureDwarves, globalImmatureDCTs, globalMatureDwarves, globalMatureDCTs, potentialRewards, consensusParams, true)) {
            ui->globalHiveFrame->hide();
            ui->globalHiveSummaryError->show();
        } else {
            ui->globalHiveSummaryError->hide();
            ui->globalHiveFrame->show();
            if (globalImmatureDwarves == 0)
                ui->globalImmatureLabel->setText("0");
            else
                ui->globalImmatureLabel->setText(formatLargeNoLocale(globalImmatureDwarves) + " (" + QString::number(globalImmatureDCTs) + " transactions)");

            if (globalMatureDwarves == 0)
                ui->globalMatureLabel->setText("0");
            else
                ui->globalMatureLabel->setText(formatLargeNoLocale(globalMatureDwarves) + " (" + QString::number(globalMatureDCTs) + " transactions)");

            updateGraph();
        }

        setAmountField(ui->potentialRewardsLabel, potentialRewards);

        double hiveWeight = mature / (double)globalMatureDwarves;
        ui->localHiveWeightLabel->setText((mature == 0 || globalMatureDwarves == 0) ? "0" : QString::number(hiveWeight, 'f', 3));
        ui->hiveWeightPie->setValue(hiveWeight);

        dwarfPopIndex = ((dwarfCost * globalMatureDwarves) / (double)potentialRewards) * 100.0;
        if (dwarfPopIndex > 200) dwarfPopIndex = 200;
        ui->dwarfPopIndexLabel->setText(QString::number(floor(dwarfPopIndex)));
        ui->dwarfPopIndexPie->setValue(dwarfPopIndex / 100);
        
        lastGlobalCheckHeight = chainActive.Tip()->nHeight;
    }

    ui->blocksTillGlobalRefresh->setText(QString::number(10 - (chainActive.Tip()->nHeight - lastGlobalCheckHeight)));
}

void HiveDialog::updateDisplayUnit() {
    if(model && model->getOptionsModel()) {
        setAmountField(ui->dwarfCostLabel, dwarfCost);
        setAmountField(ui->rewardsPaidLabel, rewardsPaid);
        setAmountField(ui->costLabel, cost);
        setAmountField(ui->profitLabel, profit);
        setAmountField(ui->potentialRewardsLabel, potentialRewards);
        setAmountField(ui->currentBalance, currentBalance);
        setAmountField(ui->totalCostLabel, totalCost);
    }

    updateTotalCostDisplay();
}

void HiveDialog::updateTotalCostDisplay() {    
    totalCost = dwarfCost * ui->dwarfCountSpinner->value();

    if(model && model->getOptionsModel()) {
        setAmountField(ui->totalCostLabel, totalCost);
        
        interfaces::WalletBalances balances = model->wallet().getBalances();
        if (totalCost > balances.balance)
            ui->dwarfCountSpinner->setStyleSheet("QSpinBox{background:#FF8080;}");
        else
            ui->dwarfCountSpinner->setStyleSheet("QSpinBox{background:white;}");
    }
}

void HiveDialog::on_dwarfCountSpinner_valueChanged(int i) {
    updateTotalCostDisplay();
}

void HiveDialog::on_includeDeadDwarvesCheckbox_stateChanged() {
    updateData();
}

void HiveDialog::on_showAdvancedStatsCheckbox_stateChanged() {
    if(ui->showAdvancedStatsCheckbox->isChecked()) {
        ui->dwarfPopGraph->show();
        ui->walletHiveStatsFrame->hide();
    } else {
        ui->dwarfPopGraph->hide();
        ui->walletHiveStatsFrame->show();
    }
}

void HiveDialog::on_retryGlobalSummaryButton_clicked() {
    updateData(true);
}

void HiveDialog::on_refreshGlobalSummaryButton_clicked() {
    updateData(true);
}

void HiveDialog::on_releaseSwarmButton_clicked() {
    if(model)
        model->requestUnlock(true);
}

void HiveDialog::on_createDwarvesButton_clicked() {
    if (model) {
        interfaces::WalletBalances balances = model->wallet().getBalances();
        if (totalCost > balances.balance) {
            QMessageBox::critical(this, tr("Error"), tr("Insufficient balance to create dwarves."));
            return;
        }
		WalletModel::UnlockContext ctx(model->requestUnlock());
		if(!ctx.isValid())
			return;     // Unlock wallet was cancelled
        model->createDwarves(ui->dwarfCountSpinner->value(), ui->donateCommunityFundCheckbox->isChecked(), this, dwarfPopIndex);
    }
}

void HiveDialog::initGraph() {
    ui->dwarfPopGraph->addGraph();
    ui->dwarfPopGraph->graph(0)->setLineStyle(QCPGraph::lsLine);
    ui->dwarfPopGraph->graph(0)->setPen(QPen(Qt::blue));
    QColor color(42, 67, 182);
    color.setAlphaF(0.35);
    ui->dwarfPopGraph->graph(0)->setBrush(QBrush(color));

    ui->dwarfPopGraph->addGraph();
    ui->dwarfPopGraph->graph(1)->setLineStyle(QCPGraph::lsLine);
    ui->dwarfPopGraph->graph(1)->setPen(QPen(Qt::black));
    QColor color1(42, 182, 67);
    color1.setAlphaF(0.35);
    ui->dwarfPopGraph->graph(1)->setBrush(QBrush(color1));

    QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
    dateTicker->setTickStepStrategy(QCPAxisTicker::TickStepStrategy::tssMeetTickCount);
    dateTicker->setTickCount(8);
    dateTicker->setDateTimeFormat("ddd d MMM");
    ui->dwarfPopGraph->xAxis->setTicker(dateTicker);

    ui->dwarfPopGraph->yAxis->setLabel("Dwarves");

    giTicker = QSharedPointer<QCPAxisTickerGI>(new QCPAxisTickerGI);
    ui->dwarfPopGraph->yAxis2->setTicker(giTicker);
    ui->dwarfPopGraph->yAxis2->setLabel("Global index");
    ui->dwarfPopGraph->yAxis2->setVisible(true);

    ui->dwarfPopGraph->xAxis->setTickLabelFont(QFont(QFont().family(), 8));
    ui->dwarfPopGraph->xAxis2->setTickLabelFont(QFont(QFont().family(), 8));
    ui->dwarfPopGraph->yAxis->setTickLabelFont(QFont(QFont().family(), 8));
    ui->dwarfPopGraph->yAxis2->setTickLabelFont(QFont(QFont().family(), 8));

    connect(ui->dwarfPopGraph->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->dwarfPopGraph->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->dwarfPopGraph->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->dwarfPopGraph->yAxis2, SLOT(setRange(QCPRange)));
    connect(ui->dwarfPopGraph, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(onMouseMove(QMouseEvent*)));

    globalMarkerLine = new QCPItemLine(ui->dwarfPopGraph);
    globalMarkerLine->setPen(QPen(Qt::blue, 1, Qt::DashLine));
    
    graphTracerImmature = new QCPItemTracer(ui->dwarfPopGraph);
    graphTracerImmature->setGraph(ui->dwarfPopGraph->graph(0));
    graphTracerMature = new QCPItemTracer(ui->dwarfPopGraph);
    graphTracerMature->setGraph(ui->dwarfPopGraph->graph(1));

    graphMouseoverText = new QCPItemText(ui->dwarfPopGraph);
}

void HiveDialog::updateGraph() {
    const Consensus::Params& consensusParams = Params().GetConsensus();

    ui->dwarfPopGraph->graph()->data()->clear();
    double now = QDateTime::currentDateTime().toTime_t();
    int totalLifespan = consensusParams.dwarfGestationBlocks + consensusParams.dwarfLifespanBlocks;
    QVector<QCPGraphData> dataMature(totalLifespan);
    QVector<QCPGraphData> dataImmature(totalLifespan);
    for (int i = 0; i < totalLifespan; i++) {
        dataImmature[i].key = now + consensusParams.nPowTargetSpacing / 2 * i;
        dataImmature[i].value = (double)dwarfPopGraph[i].immaturePop;

        dataMature[i].key = dataImmature[i].key;
        dataMature[i].value = (double)dwarfPopGraph[i].maturePop;
    }
    ui->dwarfPopGraph->graph(0)->data()->set(dataImmature);
    ui->dwarfPopGraph->graph(1)->data()->set(dataMature);

    double global100 = (double)potentialRewards / dwarfCost;
    globalMarkerLine->start->setCoords(now, global100);
    globalMarkerLine->end->setCoords(now + consensusParams.nPowTargetSpacing / 2 * totalLifespan, global100);
    giTicker->global100 = global100;
    ui->dwarfPopGraph->rescaleAxes();
    ui->dwarfPopGraph->replot();
}

void HiveDialog::onMouseMove(QMouseEvent *event) {
    QCustomPlot* customPlot = qobject_cast<QCustomPlot*>(sender());

    int x = (int)customPlot->xAxis->pixelToCoord(event->pos().x());
    int y = (int)customPlot->yAxis->pixelToCoord(event->pos().y());

    graphTracerImmature->setGraphKey(x);
    graphTracerMature->setGraphKey(x);
    int dwarfCountImmature = (int)graphTracerImmature->position->value();
    int dwarfCountMature = (int)graphTracerMature->position->value();      

    QDateTime xDateTime = QDateTime::fromTime_t(x);
    int global100 = (int)((double)potentialRewards / dwarfCost);
    QColor traceColMature = dwarfCountMature >= global100 ? Qt::red : Qt::black;
    QColor traceColImmature = dwarfCountImmature >= global100 ? Qt::red : Qt::black;

    graphTracerImmature->setPen(QPen(traceColImmature, 1, Qt::DashLine));    
    graphTracerMature->setPen(QPen(traceColMature, 1, Qt::DashLine));

    graphMouseoverText->setText(xDateTime.toString("ddd d MMM") + " " + xDateTime.time().toString() + ":\n" + formatLargeNoLocale(dwarfCountMature) + " mature dwarves\n" + formatLargeNoLocale(dwarfCountImmature) + " immature dwarves");
    graphMouseoverText->setColor(traceColMature);
    graphMouseoverText->position->setCoords(QPointF(x, y));
    QPointF pixelPos = graphMouseoverText->position->pixelPosition();

    int xoffs, yoffs;
    if (ui->dwarfPopGraph->height() > 150) {
        graphMouseoverText->setFont(QFont(font().family(), 10));
        xoffs = 80;
        yoffs = 30;
    } else {
        graphMouseoverText->setFont(QFont(font().family(), 8));
        xoffs = 70;
        yoffs = 20;
    }

    if (pixelPos.y() > ui->dwarfPopGraph->height() / 2)
        pixelPos.setY(pixelPos.y() - yoffs);
    else
        pixelPos.setY(pixelPos.y() + yoffs);

    if (pixelPos.x() > ui->dwarfPopGraph->width() / 2)
        pixelPos.setX(pixelPos.x() - xoffs);
    else
        pixelPos.setX(pixelPos.x() + xoffs);

    
    graphMouseoverText->position->setPixelPosition(pixelPos);

    customPlot->replot();
}

void HiveDialog::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    columnResizingFixer->stretchColumnWidth(HiveTableModel::Rewards);
}
