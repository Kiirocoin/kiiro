#include "../lelantus.h"
#include "../validation.h"

#include "automintmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "lelantusmodel.h"

#include <QDateTime>
#include <QTimer>

LelantusModel::LelantusModel(
    const PlatformStyle *platformStyle,
    CWallet *wallet,
    OptionsModel *optionsModel,
    QObject *parent)
    : QObject(parent),
    autoMintModel(0),
    wallet(wallet)
{
    autoMintModel = new AutoMintModel(this, optionsModel, wallet, this);

    connect(this, &LelantusModel::ackMintAll, autoMintModel, &AutoMintModel::ackMintAll);
}

LelantusModel::~LelantusModel()
{
    disconnect(this, &LelantusModel::ackMintAll, autoMintModel, &AutoMintModel::ackMintAll);

    delete autoMintModel;

    autoMintModel = nullptr;
}

CAmount LelantusModel::getMintableAmount()
{
    std::vector<std::pair<CAmount, std::vector<COutput>>> valueAndUTXO;
    {
        LOCK2(cs_main, wallet->cs_wallet);
        pwalletMain->AvailableCoinsForLMint(valueAndUTXO, nullptr);
    }

    CAmount s = 0;
    for (auto const &val : valueAndUTXO) {
        s += val.first;
    }

    return s;
}

AutoMintModel* LelantusModel::getAutoMintModel()
{
    return autoMintModel;
}

std::pair<CAmount, CAmount> LelantusModel::getPrivateBalance()
{
    size_t confirmed, unconfirmed;
    return pwalletMain->GetPrivateBalance(confirmed, unconfirmed);
}

std::pair<CAmount, CAmount> LelantusModel::getPrivateBalance(size_t &confirmed, size_t &unconfirmed)
{
    return pwalletMain->GetPrivateBalance(confirmed, unconfirmed);
}

bool LelantusModel::unlockWallet(SecureString const &passphase, size_t msecs)
{
    LOCK2(wallet->cs_wallet, cs);
    if (!wallet->Unlock(passphase)) {
        return false;
    }

    QTimer::singleShot(msecs, this, &LelantusModel::lock);
    return true;
}

void LelantusModel::lockWallet()
{
    LOCK2(wallet->cs_wallet, cs);
    wallet->Lock();
}

CAmount LelantusModel::mintAll()
{
    LOCK(wallet->cs_wallet);

    std::vector<std::pair<CWalletTx, CAmount>> wtxAndFee;
    std::vector<CHDMint> hdMints;

    auto str = wallet->MintAndStoreLelantus(0, wtxAndFee, hdMints, true);
    if (str != "") {
        throw std::runtime_error("Fail to mint all public balance, " + str);
    }

    CAmount s = 0;
    for (auto const &wtx : wtxAndFee) {
        for (auto const &out : wtx.first.tx->vout) {
            if (out.scriptPubKey.IsLelantusMint()) {
                s += out.nValue;
            }
        }
    }

    return s;
}

void LelantusModel::mintAll(AutoMintMode mode)
{
    Q_EMIT askMintAll(mode);
}

void LelantusModel::sendAckMintAll(AutoMintAck ack, CAmount minted, QString error)
{
    Q_EMIT ackMintAll(ack, minted, error);
}

void LelantusModel::lock()
{
    LOCK2(wallet->cs_wallet, cs);
    if (autoMintModel->isAnonymizing()) {
        QTimer::singleShot(MODEL_UPDATE_DELAY, this, &LelantusModel::lock);
        return;
    }

    if (wallet->IsCrypted() && !wallet->IsLocked()) {
        lockWallet();
    }
}
