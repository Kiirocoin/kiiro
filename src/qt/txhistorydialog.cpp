// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txhistorydialog.h"
#include "ui_txhistorydialog.h"

#include "elysium_qtutils.h"

#include "clientmodel.h"
#include "guiutil.h"
#include "walletmodel.h"
#include "platformstyle.h"

#include "elysium/fetchwallettx.h"
#include "elysium/elysium.h"
#include "elysium/pending.h"
#include "elysium/rpc.h"
#include "elysium/rpctxobject.h"
#include "elysium/sp.h"
#include "elysium/tx.h"
#include "elysium/utilsbitcoin.h"
#include "elysium/walletcache.h"
#include "elysium/wallettxs.h"

#include "init.h"
#include "validation.h"
#include "primitives/transaction.h"
#include "sync.h"
#include "txdb.h"
#include "uint256.h"
#include "wallet/wallet.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <stdint.h>
#include <list>
#include <map>
#include <string>
#include <vector>

#include <QAbstractItemModel>
#include <QAction>
#include <QDateTime>
#include <QDialog>
#include <QHeaderView>
#include <QIcon>
#include <QMenu>
#include <QModelIndex>
#include <QPoint>
#include <QResizeEvent>
#include <QSortFilterProxyModel>
#include <QString>
#include <QTableWidgetItem>
#include <QWidget>

using std::string;
using namespace elysium;

TXHistoryDialog::TXHistoryDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::txHistoryDialog),
    clientModel(0),
    walletModel(0)
{
    ui->setupUi(this);
    // setup
    ui->txHistoryTable->setColumnCount(7);
    ui->txHistoryTable->setHorizontalHeaderItem(2, new QTableWidgetItem(" "));
    ui->txHistoryTable->setHorizontalHeaderItem(3, new QTableWidgetItem("Date"));
    ui->txHistoryTable->setHorizontalHeaderItem(4, new QTableWidgetItem("Type"));
    ui->txHistoryTable->setHorizontalHeaderItem(5, new QTableWidgetItem("Address"));
    ui->txHistoryTable->setHorizontalHeaderItem(6, new QTableWidgetItem("Amount"));
    // borrow ColumnResizingFixer again
    borrowedColumnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(ui->txHistoryTable, 100, 100, this);
    // allow user to adjust - go interactive then manually set widths
    #if QT_VERSION < 0x050000
       ui->txHistoryTable->horizontalHeader()->setResizeMode(2, QHeaderView::Fixed);
       ui->txHistoryTable->horizontalHeader()->setResizeMode(3, QHeaderView::Interactive);
       ui->txHistoryTable->horizontalHeader()->setResizeMode(4, QHeaderView::Interactive);
       ui->txHistoryTable->horizontalHeader()->setResizeMode(5, QHeaderView::Interactive);
       ui->txHistoryTable->horizontalHeader()->setResizeMode(6, QHeaderView::Interactive);
   #else
       ui->txHistoryTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
       ui->txHistoryTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
       ui->txHistoryTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Interactive);
       ui->txHistoryTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Interactive);
       ui->txHistoryTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Interactive);
    #endif
    ui->txHistoryTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->txHistoryTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->txHistoryTable->verticalHeader()->setVisible(false);
    ui->txHistoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->txHistoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->txHistoryTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->txHistoryTable->setTabKeyNavigation(false);
    ui->txHistoryTable->setContextMenuPolicy(Qt::CustomContextMenu);
    // set alternating row colors via styling instead of manually
    ui->txHistoryTable->setAlternatingRowColors(true);
    // Actions
    QAction *copyAddressAction = new QAction(tr("Copy address"), this);
    QAction *copyAmountAction = new QAction(tr("Copy amount"), this);
    QAction *copyTxIDAction = new QAction(tr("Copy transaction ID"), this);
    QAction *showDetailsAction = new QAction(tr("Show transaction details"), this);
    contextMenu = new QMenu();
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyAmountAction);
    contextMenu->addAction(copyTxIDAction);
    contextMenu->addAction(showDetailsAction);
    // Connect actions
    connect(ui->txHistoryTable, &QWidget::customContextMenuRequested, this, &TXHistoryDialog::contextualMenu);
    connect(ui->txHistoryTable, &QAbstractItemView::doubleClicked, this, &TXHistoryDialog::showDetails);
    connect(ui->txHistoryTable->horizontalHeader(), &QHeaderView::sectionClicked, this, &TXHistoryDialog::checkSort);
    connect(copyAddressAction, &QAction::triggered, this, &TXHistoryDialog::copyAddress);
    connect(copyAmountAction, &QAction::triggered, this, &TXHistoryDialog::copyAmount);
    connect(copyTxIDAction, &QAction::triggered, this, &TXHistoryDialog::copyTxID);
    connect(showDetailsAction, &QAction::triggered, this, &TXHistoryDialog::showDetails);
    // Perform initial population and update of history table

    UpdateHistory();
    // since there is no model we can't do this before we add some data, so now let's do the resizing
    // *after* we've populated the initial content for the table
    ui->txHistoryTable->setColumnWidth(2, 23);
    ui->txHistoryTable->resizeColumnToContents(3);
    ui->txHistoryTable->resizeColumnToContents(4);
    ui->txHistoryTable->resizeColumnToContents(6);
    ui->txHistoryTable->setColumnHidden(0, true); // hidden txid for displaying transaction details
    ui->txHistoryTable->setColumnHidden(1, true); // hideen sort key
    borrowedColumnResizingFixer->stretchColumnWidth(5);
    ui->txHistoryTable->setSortingEnabled(true);
    ui->txHistoryTable->horizontalHeader()->setSortIndicator(1, Qt::DescendingOrder); // sort by hidden sort key
}

TXHistoryDialog::~TXHistoryDialog()
{
    delete ui;
}

void TXHistoryDialog::ReinitTXHistoryTable()
{
    ui->txHistoryTable->setRowCount(0);
    txHistoryMap.clear();
    UpdateHistory();
}

void TXHistoryDialog::focusTransaction(const uint256& txid)
{
    QAbstractItemModel* historyAbstractModel = ui->txHistoryTable->model();
    QSortFilterProxyModel historyProxy;
    historyProxy.setSourceModel(historyAbstractModel);
    historyProxy.setFilterKeyColumn(0);
    historyProxy.setFilterFixedString(QString::fromStdString(txid.GetHex()));
    QModelIndex rowIndex = historyProxy.mapToSource(historyProxy.index(0,0));
    if(rowIndex.isValid()) {
        ui->txHistoryTable->scrollTo(rowIndex);
        ui->txHistoryTable->setCurrentIndex(rowIndex);
        ui->txHistoryTable->setFocus();
    }
}

void TXHistoryDialog::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if (model != NULL) {
        connect(model, &ClientModel::refreshElysiumBalance, this, &TXHistoryDialog::UpdateHistory);
        connect(model, &ClientModel::numBlocksChanged, this, &TXHistoryDialog::UpdateConfirmations);
        connect(model, &ClientModel::reinitElysiumState, this, &TXHistoryDialog::ReinitTXHistoryTable);
    }
}

void TXHistoryDialog::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if (model != NULL) { } // do nothing, signals from walletModel no longer needed
}

int TXHistoryDialog::PopulateHistoryMap()
{
    // TODO: locks may not be needed here -- looks like wallet lock can be removed
    if (NULL == pwalletMain) return 0;
    // try and fix intermittent freeze on startup and while running by only updating if we can get required locks
    TRY_LOCK(cs_main,lckMain);
    if (!lckMain) return 0;
    TRY_LOCK(pwalletMain->cs_wallet, lckWallet);
    if (!lckWallet) return 0;

    int64_t nProcessed = 0; // counter for how many transactions we've added to history this time

    // obtain a sorted list of Elysium layer wallet transactions (including STO receipts and pending) - default last 65535
    std::map<std::string,uint256> walletTransactions = FetchWalletElysiumTransactions(GetArg("-elysiumuiwalletscope", 65535L));

    // reverse iterate over (now ordered) transactions and populate history map for each one
    for (std::map<std::string,uint256>::reverse_iterator it = walletTransactions.rbegin(); it != walletTransactions.rend(); it++) {
        uint256 txHash = it->second;
        // check historyMap, if this tx exists and isn't pending don't waste resources doing anymore work on it
        HistoryMap::iterator hIter = txHistoryMap.find(txHash);
        if (hIter != txHistoryMap.end()) { // the tx is in historyMap, if it's a confirmed transaction skip it
            HistoryTXObject *temphtxo = &(hIter->second);
            if (temphtxo->blockHeight != 0) continue;
            {
                LOCK(cs_main);
                PendingMap::iterator pending_it = my_pending.find(txHash);
                if (pending_it != my_pending.end()) continue; // transaction is still pending, do nothing
            }

            // pending transaction has confirmed, remove temp pending object from map and allow it to be readded as an Elysium transaction
            txHistoryMap.erase(hIter);
            ui->txHistoryTable->setSortingEnabled(false); // disable sorting temporarily while we update the table (leaving enabled gives unexpected results)
            QAbstractItemModel* historyAbstractModel = ui->txHistoryTable->model(); // get a model to work with
            QSortFilterProxyModel historyProxy;
            historyProxy.setSourceModel(historyAbstractModel);
            historyProxy.setFilterKeyColumn(0);
            historyProxy.setFilterFixedString(QString::fromStdString(txHash.GetHex()));
            QModelIndex rowIndex = historyProxy.mapToSource(historyProxy.index(0,0)); // map to the row in the actual table
            if(rowIndex.isValid()) ui->txHistoryTable->removeRow(rowIndex.row()); // delete the pending tx row, it'll be readded as a proper confirmed transaction
            ui->txHistoryTable->setSortingEnabled(true); // re-enable sorting
        }

        CTransactionRef wtx;
        uint256 blockHash;
        if (!GetTransaction(txHash, wtx, Params().GetConsensus(), blockHash, true)) continue;
        if (blockHash.IsNull() || NULL == GetBlockIndex(blockHash)) {
            // this transaction is unconfirmed, should be one of our pending transactions
            LOCK(cs_main);
            PendingMap::iterator pending_it = my_pending.find(txHash);
            if (pending_it == my_pending.end()) continue;
            const CMPPending& pending = pending_it->second;
            HistoryTXObject htxo;
            htxo.blockHeight = 0;
            if (it->first.length() == 16) htxo.blockByteOffset = atoi(it->first.substr(6)); // use wallet position from key in lieu of block position
            htxo.valid = true; // all pending transactions are assumed to be valid prior to confirmation (wallet would not send them otherwise)
            htxo.address = pending.src;
            htxo.amount = "-" + FormatShortMP(pending.prop, pending.amount) + getTokenLabel(pending.prop);
            bool fundsMoved = true;
            htxo.txType = shrinkTxType(pending.type, &fundsMoved);
            if (pending.type == ELYSIUM_TYPE_METADEX_CANCEL_PRICE || pending.type == ELYSIUM_TYPE_METADEX_CANCEL_PAIR ||
                pending.type == ELYSIUM_TYPE_METADEX_CANCEL_ECOSYSTEM || pending.type == ELYSIUM_TYPE_SEND_ALL) {
                htxo.amount = "N/A";
            }

            if (pending.type == ELYSIUM_TYPE_SIMPLE_SPEND) {
                if (pending.dest && IsMyAddress(pending.dest.get())) {
                    htxo.amount = FormatShortMP(pending.prop, pending.amount) + getTokenLabel(pending.prop);
                }
            }

            txHistoryMap.insert(std::make_pair(txHash, htxo));
            nProcessed++;
            continue;
        }

        // parse the transaction and setup the new history object
        CBlockIndex* pBlockIndex = GetBlockIndex(blockHash);
        if (NULL == pBlockIndex) continue;
        int blockHeight = pBlockIndex->nHeight;
        CMPTransaction mp_obj;
        int parseRC = ParseTransaction(*wtx, blockHeight, 0, mp_obj);
        HistoryTXObject htxo;
        if (it->first.length() == 16) {
            htxo.blockHeight = atoi(it->first.substr(0,6));
            htxo.blockByteOffset = atoi(it->first.substr(6));
        }

        // positive RC means payment, potential DEx purchase
        if (0 < parseRC) {
            string tmpBuyer;
            string tmpSeller;
            uint64_t total = 0;
            uint64_t tmpVout = 0;
            uint64_t tmpNValue = 0;
            uint64_t tmpPropertyId = 0;
            bool bIsBuy = false;
            int numberOfPurchases = 0;
            {
                LOCK(cs_main);
                p_txlistdb->getPurchaseDetails(txHash, 1, &tmpBuyer, &tmpSeller, &tmpVout, &tmpPropertyId, &tmpNValue);
            }
            bIsBuy = IsMyAddress(tmpBuyer);
            numberOfPurchases = p_txlistdb->getNumberOfSubRecords(txHash);
            if (0 >= numberOfPurchases) continue;
            for (int purchaseNumber = 1; purchaseNumber <= numberOfPurchases; purchaseNumber++) {
                LOCK(cs_main);
                p_txlistdb->getPurchaseDetails(txHash, purchaseNumber, &tmpBuyer, &tmpSeller, &tmpVout, &tmpPropertyId, &tmpNValue);
                total += tmpNValue;
            }
            if (!bIsBuy) {
                htxo.txType = "DEx Sell";
                htxo.address = tmpSeller;
            } else {
                htxo.txType = "DEx Buy";
                htxo.address = tmpBuyer;
            }
            htxo.valid = true; // only valid DEx payments are recorded in txlistdb
            htxo.amount = (!bIsBuy ? "-" : "") + FormatDivisibleShortMP(total) + getTokenLabel(tmpPropertyId);
            htxo.fundsMoved = true;
            txHistoryMap.insert(std::make_pair(txHash, htxo));
            nProcessed++;
            continue;
        }

        // handle Elysium transaction
        if (0 != parseRC) continue;
        if (!mp_obj.interpret_Transaction()) continue;
        int64_t amount = mp_obj.getAmount();
        int tmpBlock = 0;
        uint32_t type = 0;
        uint64_t amountNew = 0;
        htxo.valid = getValidMPTX(txHash, &tmpBlock, &type, &amountNew);
        if (htxo.valid && type == ELYSIUM_TYPE_TRADE_OFFER && amountNew > 0) amount = amountNew; // override for when amount for sale has been auto-adjusted

        if (htxo.valid && type == ELYSIUM_TYPE_SIMPLE_SPEND) { // override amount for spend
            amount = mp_obj.getSpendAmount();
        }

        if (htxo.valid && type == ELYSIUM_TYPE_SIMPLE_MINT) { // override amount for mint
            amount = mp_obj.getMintAmount();
        }

        std::string displayAmount = FormatShortMP(mp_obj.getProperty(), amount) + getTokenLabel(mp_obj.getProperty());
        htxo.fundsMoved = true;
        htxo.txType = shrinkTxType(mp_obj.getType(), &htxo.fundsMoved);
        if (!htxo.valid) htxo.fundsMoved = false; // funds never move in invalid txs
        if (htxo.txType == "Send" && !IsMyAddress(mp_obj.getSender())) htxo.txType = "Receive"; // still a send transaction, but avoid confusion for end users
        htxo.address = mp_obj.getSender();
        if (!IsMyAddress(mp_obj.getSender())) htxo.address = mp_obj.getReceiver();
        if (htxo.fundsMoved && IsMyAddress(mp_obj.getSender())) displayAmount = "-" + displayAmount;

        if (type == ELYSIUM_TYPE_SIMPLE_SPEND) {
            htxo.address = "Spend";
            if (htxo.fundsMoved && !IsMyAddress(mp_obj.getReceiver())) {
                displayAmount = "-" + displayAmount;
            }
        }

        // override - special case for property creation (getProperty cannot get ID as createdID not stored in obj)
        if (type == ELYSIUM_TYPE_CREATE_PROPERTY_FIXED || type == ELYSIUM_TYPE_CREATE_PROPERTY_VARIABLE || type == ELYSIUM_TYPE_CREATE_PROPERTY_MANUAL) {
            displayAmount = "N/A";
            if (htxo.valid) {
                uint32_t propertyId = _my_sps->findSPByTX(txHash);
                if (type == ELYSIUM_TYPE_CREATE_PROPERTY_FIXED) displayAmount = FormatShortMP(propertyId, getTotalTokens(propertyId)) + getTokenLabel(propertyId);
            }
        }

        // override - hide display amount for transaction types below as we can't display amount/property as no prop exists
        // - Unchanged balance sigma transactions
        // - Cancels transactions
        // - Unknown transactions
        if (type == ELYSIUM_TYPE_METADEX_CANCEL_PRICE || type == ELYSIUM_TYPE_METADEX_CANCEL_PAIR ||
            type == ELYSIUM_TYPE_METADEX_CANCEL_ECOSYSTEM || type == ELYSIUM_TYPE_SEND_ALL ||
            type == ELYSIUM_TYPE_CREATE_DENOMINATION || htxo.txType == "Unknown") {
            displayAmount = "N/A";
        }

        // override - display amount received not STO amount in packet (the total amount) for STOs I didn't send
        if (type == ELYSIUM_TYPE_SEND_TO_OWNERS && !IsMyAddress(mp_obj.getSender())) {
            UniValue receiveArray(UniValue::VARR);
            uint64_t tmpAmount = 0, stoFee = 0;
            LOCK(cs_main);
            s_stolistdb->getRecipients(txHash, "", &receiveArray, &tmpAmount, &stoFee);
            displayAmount = FormatShortMP(mp_obj.getProperty(), tmpAmount) + getTokenLabel(mp_obj.getProperty());
        }

        htxo.amount = displayAmount;
        txHistoryMap.insert(std::make_pair(txHash, htxo));
        nProcessed++;
    }

    return nProcessed;
}

void TXHistoryDialog::UpdateConfirmations()
{
    int chainHeight = GetHeight(); // get the chain height
    int rowCount = ui->txHistoryTable->rowCount();
    for (int row = 0; row < rowCount; row++) {
        int confirmations = 0;
        bool valid = false;
        uint256 txid;
        txid.SetHex(ui->txHistoryTable->item(row,0)->text().toStdString());
        // lookup transaction in the map and grab validity and blockheight details
        HistoryMap::iterator hIter = txHistoryMap.find(txid);
        if (hIter != txHistoryMap.end()) {
            HistoryTXObject *temphtxo = &(hIter->second);
            if (temphtxo->blockHeight>0) confirmations = (chainHeight+1) - temphtxo->blockHeight;
            valid = temphtxo->valid;
        }
        int txBlockHeight = ui->txHistoryTable->item(row,1)->text().toInt();
        if (txBlockHeight>0) confirmations = (chainHeight+1) - txBlockHeight;
        // setup the appropriate icon
        QIcon ic = QIcon(":/icons/transaction_0");
        switch(confirmations) {
            case 1: ic = QIcon(":/icons/transaction_1"); break;
            case 2: ic = QIcon(":/icons/transaction_2"); break;
            case 3: ic = QIcon(":/icons/transaction_3"); break;
            case 4: ic = QIcon(":/icons/transaction_4"); break;
            case 5: ic = QIcon(":/icons/transaction_5"); break;
        }
        if (confirmations > 5) ic = QIcon(":/icons/transaction_confirmed");
        if (!valid) ic = QIcon(":/icons/transaction_conflicted");
        QTableWidgetItem *iconCell = new QTableWidgetItem;
//        ic = platformStyle->SingleColorIcon(ic);
        iconCell->setIcon(ic);
        ui->txHistoryTable->setItem(row, 2, iconCell);
    }
}

void TXHistoryDialog::UpdateHistory()
{
    // now moved to a new methodology where historical transactions are stored in a map in memory (effectively a cache) so we can compare our
    // history table against the cache.  This allows us to avoid reparsing transactions repeatedly and provides a diff to modify the table without
    // repopuplating all the rows top to bottom each refresh.

    // first things first, call PopulateHistoryMap to add in any missing (ie new) transactions
    int newTXCount = PopulateHistoryMap();
    // were any transactions added?
    if(newTXCount > 0) { // there are new transactions (or a pending shifted to confirmed), refresh the table adding any that are in the map but not in the table
        ui->txHistoryTable->setSortingEnabled(false); // disable sorting temporarily while we update the table (leaving enabled gives unexpected results)
        QAbstractItemModel* historyAbstractModel = ui->txHistoryTable->model(); // get a model to work with
        for(HistoryMap::iterator it = txHistoryMap.begin(); it != txHistoryMap.end(); ++it) {
            uint256 txid = it->first; // grab txid
            // search the history table for this transaction, here we're going to use a filter proxy because it's a little quicker
            QSortFilterProxyModel historyProxy;
            historyProxy.setSourceModel(historyAbstractModel);
            historyProxy.setFilterKeyColumn(0);
            historyProxy.setFilterFixedString(QString::fromStdString(txid.GetHex()));
            QModelIndex rowIndex = historyProxy.mapToSource(historyProxy.index(0,0));
            if(!rowIndex.isValid()) { // this transaction doesn't exist in the history table, add it
                HistoryTXObject htxo = it->second; // grab the tranaaction
                int workingRow = ui->txHistoryTable->rowCount();
                ui->txHistoryTable->insertRow(workingRow); // append a new row (sorting will take care of ordering)
                QDateTime txTime;
                QTableWidgetItem *dateCell = new QTableWidgetItem;
                if (htxo.blockHeight>0) {
                    LOCK(cs_main);
                    CBlockIndex* pBlkIdx = chainActive[htxo.blockHeight];
                    if (NULL != pBlkIdx) txTime.setTime_t(pBlkIdx->GetBlockTime());
                    dateCell->setData(Qt::DisplayRole, txTime);
                } else {
                    dateCell->setData(Qt::DisplayRole, QString::fromStdString("Unconfirmed"));
                }
                QTableWidgetItem *typeCell = new QTableWidgetItem(QString::fromStdString(htxo.txType));
                QTableWidgetItem *addressCell = new QTableWidgetItem(QString::fromStdString(htxo.address));
                QTableWidgetItem *amountCell = new QTableWidgetItem(QString::fromStdString(htxo.amount));
                QTableWidgetItem *iconCell = new QTableWidgetItem;
                QTableWidgetItem *txidCell = new QTableWidgetItem(QString::fromStdString(txid.GetHex()));
                std::string sortKey = strprintf("%06d%010d",htxo.blockHeight,htxo.blockByteOffset);
                if(htxo.blockHeight == 0) sortKey = strprintf("%06d%010D",999999,htxo.blockByteOffset); // spoof the hidden value to ensure pending txs are sorted top
                QTableWidgetItem *sortKeyCell = new QTableWidgetItem(QString::fromStdString(sortKey));
                addressCell->setTextAlignment(Qt::AlignLeft + Qt::AlignVCenter);
                addressCell->setForeground(QColor("#707070"));
                amountCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
                amountCell->setForeground(QColor("#00AA00"));
                if (htxo.amount.length() > 0) { // protect against an empty value
                    if (htxo.amount.substr(0,1) == "-") amountCell->setForeground(QColor("#EE0000")); // outbound
                }
                if (!htxo.fundsMoved) amountCell->setForeground(QColor("#404040"));
                ui->txHistoryTable->setItem(workingRow, 0, txidCell);
                ui->txHistoryTable->setItem(workingRow, 1, sortKeyCell);
                ui->txHistoryTable->setItem(workingRow, 2, iconCell);
                ui->txHistoryTable->setItem(workingRow, 3, dateCell);
                ui->txHistoryTable->setItem(workingRow, 4, typeCell);
                ui->txHistoryTable->setItem(workingRow, 5, addressCell);
                ui->txHistoryTable->setItem(workingRow, 6, amountCell);
            }
        }
        ui->txHistoryTable->setSortingEnabled(true); // re-enable sorting
    }
    UpdateConfirmations();
}

void TXHistoryDialog::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->txHistoryTable->indexAt(point);
    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}

void TXHistoryDialog::copyAddress()
{
    GUIUtil::setClipboard(ui->txHistoryTable->item(ui->txHistoryTable->currentRow(),5)->text());
}

void TXHistoryDialog::copyAmount()
{
    GUIUtil::setClipboard(ui->txHistoryTable->item(ui->txHistoryTable->currentRow(),6)->text());
}

void TXHistoryDialog::copyTxID()
{
    GUIUtil::setClipboard(ui->txHistoryTable->item(ui->txHistoryTable->currentRow(),0)->text());
}

void TXHistoryDialog::checkSort(int column)
{
    // a header has been clicked on the tx history table, see if it's the status column and override the sort if necessary
    // we may wish to be a bit smarter about this longer term, so we can spoof a sort indicator display and perhaps allow both
    // directions, but for now will provide the core functionality needed
    if (column == 2) { // ignore any other column sorts and allow them to progress normally
        ui->txHistoryTable->horizontalHeader()->setSortIndicator(1, Qt::DescendingOrder);
    }
}

void TXHistoryDialog::showDetails()
{
    UniValue txobj(UniValue::VOBJ);
    uint256 txid;
    txid.SetHex(ui->txHistoryTable->item(ui->txHistoryTable->currentRow(),0)->text().toStdString());
    std::string strTXText;

    if (!txid.IsNull()) {
        // grab extended transaction details via the RPC populator
        int rc = populateRPCTransactionObject(txid, txobj, "", true);
        if (rc >= 0) strTXText = txobj.write(true);
    }

    if (!strTXText.empty()) {
        PopulateSimpleDialog(strTXText, "Transaction Information", "Transaction Information");
    }
}

void TXHistoryDialog::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    borrowedColumnResizingFixer->stretchColumnWidth(5);
}

std::string TXHistoryDialog::shrinkTxType(int txType, bool *fundsMoved)
{
    string displayType = "Unknown";
    switch (txType) {
        case ELYSIUM_TYPE_SIMPLE_SEND: displayType = "Send"; break;
        case ELYSIUM_TYPE_RESTRICTED_SEND: displayType = "Rest. Send"; break;
        case ELYSIUM_TYPE_SEND_TO_OWNERS: displayType = "Send To Owners"; break;
        case ELYSIUM_TYPE_SEND_ALL: displayType = "Send All"; break;
        case ELYSIUM_TYPE_SAVINGS_MARK: displayType = "Mark Savings"; *fundsMoved = false; break;
        case ELYSIUM_TYPE_SAVINGS_COMPROMISED: ; displayType = "Lock Savings"; break;
        case ELYSIUM_TYPE_RATELIMITED_MARK: displayType = "Rate Limit"; break;
        case ELYSIUM_TYPE_AUTOMATIC_DISPENSARY: displayType = "Auto Dispense"; break;
        case ELYSIUM_TYPE_TRADE_OFFER: displayType = "DEx Trade"; *fundsMoved = false; break;
        case ELYSIUM_TYPE_ACCEPT_OFFER_BTC: displayType = "DEx Accept"; *fundsMoved = false; break;
        case ELYSIUM_TYPE_METADEX_TRADE: displayType = "MetaDEx Trade"; *fundsMoved = false; break;
        case ELYSIUM_TYPE_METADEX_CANCEL_PRICE:
        case ELYSIUM_TYPE_METADEX_CANCEL_PAIR:
        case ELYSIUM_TYPE_METADEX_CANCEL_ECOSYSTEM:
            displayType = "MetaDEx Cancel"; *fundsMoved = false; break;
        case ELYSIUM_TYPE_CREATE_PROPERTY_FIXED: displayType = "Create Property"; break;
        case ELYSIUM_TYPE_CREATE_PROPERTY_VARIABLE: displayType = "Create Property"; *fundsMoved = false; break;
        case ELYSIUM_TYPE_PROMOTE_PROPERTY: displayType = "Promo Property"; break;
        case ELYSIUM_TYPE_CLOSE_CROWDSALE: displayType = "Close Crowdsale"; *fundsMoved = false; break;
        case ELYSIUM_TYPE_CREATE_PROPERTY_MANUAL: displayType = "Create Property"; *fundsMoved = false; break;
        case ELYSIUM_TYPE_GRANT_PROPERTY_TOKENS: displayType = "Grant Tokens"; break;
        case ELYSIUM_TYPE_REVOKE_PROPERTY_TOKENS: displayType = "Revoke Tokens"; break;
        case ELYSIUM_TYPE_CHANGE_ISSUER_ADDRESS: displayType = "Change Issuer"; *fundsMoved = false; break;
        case ELYSIUM_TYPE_SIMPLE_SPEND: displayType = "Sigma Spend"; break;
        case ELYSIUM_TYPE_CREATE_DENOMINATION: displayType = "Create Sigma Denomination"; *fundsMoved = false; break;
        case ELYSIUM_TYPE_SIMPLE_MINT: displayType = "Sigma Mint"; break;
    }
    return displayType;
}
