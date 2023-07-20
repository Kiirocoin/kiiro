// Copyright (c) 2019-2021 The Kiirocoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pcodemodel.h"
#include "../bip47/paymentcode.h"

#include "bitcoinunits.h"
#include "guiutil.h"
#include "optionsmodel.h"

#include "clientversion.h"
#include "streams.h"
#include "bip47/account.h"

#include <boost/foreach.hpp>

extern CCriticalSection cs_main;

namespace {
static void OnPcodeCreated_(PcodeModel *PcodeModel, bip47::CPaymentCodeDescription const & pcodeDescr)
{
    QMetaObject::invokeMethod(PcodeModel, "DisplayCreatedPcode", Qt::AutoConnection,
                            Q_ARG(bip47::CPaymentCodeDescription const &, pcodeDescr));
}
}

PcodeModel::PcodeModel(CWallet *wallet, WalletModel *parent) :
    QAbstractTableModel(parent),
    walletMain(*wallet),
    walletModel(parent)
{
    /* These columns must match the indices in the ColumnIndex enumeration */
    columns << tr("#") << tr("Label") << tr("RAP address");

    wallet->NotifyPcodeCreated.connect(boost::bind(OnPcodeCreated_, this, _1));
    items = wallet->ListPcodes();
}

PcodeModel::~PcodeModel()
{
    walletMain.NotifyPcodeCreated.disconnect(boost::bind(OnPcodeCreated_, this, _1));
}

std::vector<bip47::CPaymentCodeDescription> const & PcodeModel::getItems() const
{
    return items;
}

int PcodeModel::rowCount(const QModelIndex &) const
{
    return items.size();
}

int PcodeModel::columnCount(const QModelIndex &) const
{
    return columns.length();
}

QVariant PcodeModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid() || index.row() >= int(items.size()))
        return QVariant();

    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        bip47::CPaymentCodeDescription const & desc = items[index.row()];
        switch(ColumnIndex(index.column()))
        {
            case ColumnIndex::Number:
                return int(std::get<0>(desc));
            case ColumnIndex::Pcode:
                return std::get<1>(desc).toString().c_str();
            case ColumnIndex::Label:
                return std::get<2>(desc).c_str();
        }
    }
    else if (role == Qt::TextAlignmentRole)
    {
        if (ColumnIndex(index.column()) == ColumnIndex::Number)
            return int((Qt::AlignCenter|Qt::AlignVCenter));
    }
    return QVariant();
}

bool PcodeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid())
        return false;
    int const row = index.row();
    if(row >= items.size())
        return false;
    if(ColumnIndex(index.column()) != ColumnIndex::Label)
        return false;

    if(role == Qt::EditRole)
    {
        std::string const newLab = value.toString().toStdString();
        if(std::get<2>(items[row]) == newLab)
            return false;

        walletMain.LabelReceivingPcode(std::get<1>(items[row]), newLab);
        std::get<2>(items[row]) = newLab;
        Q_EMIT dataChanged(createIndex(row,  int(ColumnIndex::Label)), createIndex(row, int(ColumnIndex::Label)));
        return true;
    }
    return false;
}

QVariant PcodeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole && section < columns.size())
        {
            return columns[section];
        }
    }
    return QVariant();
}

QModelIndex PcodeModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return createIndex(row, column);
}

Qt::ItemFlags PcodeModel::flags(const QModelIndex & index) const
{
    if(!index.isValid())
        return 0;
    Qt::ItemFlags retval = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if(index.column() == int(ColumnIndex::Label))
    {
        retval |= Qt::ItemIsEditable;
    }
    return retval;
}

uint256 PcodeModel::sendNotificationTx(bip47::CPaymentCode const & paymentCode)
{
    LOCK2(cs_main, walletMain.cs_wallet);
    return walletMain.PrepareAndSendNotificationTx(paymentCode).GetHash();
}

bool PcodeModel::getNotificationTxid(bip47::CPaymentCode const & paymentCode, uint256 & txid)
{
    bool result = false;
    LOCK(walletMain.cs_wallet);
    walletMain.GetBip47Wallet()->enumerateSenders(
        [&paymentCode, &result, &txid](bip47::CAccountSender const & sender)
        {
            if (sender.getTheirPcode() == paymentCode && !sender.getNotificationTxId().IsNull()) {
                txid = sender.getNotificationTxId();
                result = true;
                return false;
            }
            return true;
        }
    );
    return result;
}

void PcodeModel::generateTheirNextAddress(std::string const & pcode)
{
    try {
        walletMain.GenerateTheirNextAddress(pcode);
    } catch (std::runtime_error const &) {
        LogBip47("Cannot convert to pcode: %s", pcode.c_str());
    }
}

void PcodeModel::reconsiderBip47Tx(uint256 const & hash)
{
    LOCK(walletMain.cs_wallet);
    const CWalletTx * wtx = walletMain.GetWalletTx(hash);
    if (wtx && !wtx->IsCoinBase())
        walletMain.HandleBip47Transaction(*wtx);
}

bool PcodeModel::isBip47Transaction(uint256 const & hash) const
{
    const CWalletTx * wtx = walletMain.GetWalletTx(hash);
    if (wtx && !wtx->IsCoinBase())
    {
        for(unsigned int i = 0; i < wtx->tx->vout.size(); i++)
        {
            const CTxOut& txout = wtx->tx->vout[i];
            isminetype mine = walletMain.IsMine(txout);
            if (mine)
            {
                CTxDestination address;
                if(ExtractDestination(txout.scriptPubKey, address) && IsMine(walletMain, address))
                {
                    boost::optional<bip47::CPaymentCodeDescription> pcode = walletMain.FindPcode(address);
                    if (pcode)
                        return true;
                }
            }
        }
    }
    return false;
}

void PcodeModel::labelPcode(std::string const & pcode_, std::string const & label, bool remove)
{
    try {
        walletMain.LabelSendingPcode(pcode_, label, remove);
    } catch (std::runtime_error const &) {
        return;
    }
}

bool PcodeModel::hasSendingPcodes() const
{
    static bool result = false;
    if(!result) {
        LOCK(walletMain.cs_wallet);
        walletMain.GetBip47Wallet()->enumerateSenders(
            [](bip47::CAccountSender const & sender)
            {
                result = true;
                return false;
            });
    }
    return result;
}

void PcodeModel::sort(int column, Qt::SortOrder order)
{
    std::function<bool(bip47::CPaymentCodeDescription const &, bip47::CPaymentCodeDescription const &)> 
    sortPred = [&column, &order](bip47::CPaymentCodeDescription const & lhs, bip47::CPaymentCodeDescription const & rhs)
    {
        bip47::CPaymentCodeDescription const & cmp1 = (order == Qt::SortOrder::DescendingOrder ? lhs : rhs);
        bip47::CPaymentCodeDescription const & cmp2 = (order == Qt::SortOrder::DescendingOrder ? rhs : lhs);

        switch(ColumnIndex(column)) {
            case ColumnIndex::Number:
            default:
                return std::get<0>(cmp1) < std::get<0>(cmp2);
            case ColumnIndex::Pcode:
                return std::get<1>(cmp1).toString() < std::get<1>(cmp2).toString();
            case ColumnIndex::Label:
                return std::get<2>(cmp1) < std::get<2>(cmp2);
        }
    };
    qSort(items.begin(), items.end(), sortPred);
    Q_EMIT dataChanged(index(0, 0, QModelIndex()), index(items.size() - 1, int(ColumnIndex::NumberOfColumns) - 1, QModelIndex()));
}

void PcodeModel::DisplayCreatedPcode(bip47::CPaymentCodeDescription const & pcodeDescr)
{
    beginInsertRows(QModelIndex(), 0, 0);
    items.push_back(pcodeDescr);
    endInsertRows();
}
