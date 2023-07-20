#include "specialtx.h"
#include "base58.h"
#include "chainparams.h"
#include "validation.h"
#include "spork.h"
#include "hash.h"
#include "messagesigner.h"
#include "univalue.h"

bool CheckSporkTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state) {
    const Consensus::Params &chainParams = Params().GetConsensus();

    if (tx.nType != TRANSACTION_SPORK) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-type");
    }

    CSporkTx sporkTx;
    if (!GetTxPayload(tx, sporkTx)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-payload");
    }

    if (sporkTx.nVersion == 0 || sporkTx.nVersion > CSporkTx::CURRENT_VERSION) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-version");
    }
    if (sporkTx.actions.empty()) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-empty");
    }

    CKeyID sporkKeyID;
    if (!CBitcoinAddress(chainParams.evoSporkKeyID).GetKeyID(sporkKeyID)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-sig", false);
    }

    std::string strError;
    if (!CHashSigner::VerifyHash(::SerializeHash(sporkTx), sporkKeyID, sporkTx.vchSig, strError)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-sig", false, strError);
    }

    uint256 inputsHash = CalcTxInputsHash(tx);
    if (inputsHash != sporkTx.inputsHash) {
        return state.DoS(100, false, REJECT_INVALID, "bad-protx-inputs-hash");
    }

    for (const CSporkAction &action: sporkTx.actions) {
        if (action.nEnableAtHeight != 0 &&
                    !(action.nEnableAtHeight >= chainParams.nEvoSporkStartBlock && action.nEnableAtHeight < chainParams.nEvoSporkStopBlock)) {
            return state.DoS(100, false, REJECT_INVALID, "bad-protx-height");
        }
    }

    return true;
}

static bool IsTransactionAllowed(const CTransaction &tx, const ActiveSporkMap &sporkMap, CValidationState &state)
{
    if (tx.IsLelantusTransaction()) {
        if (sporkMap.count(CSporkAction::featureLelantus) > 0)
            return state.DoS(100, false, REJECT_CONFLICT, "txn-lelantus-disabled", false, "Lelantus transactions are disabled at the moment");

        if (tx.IsLelantusJoinSplit()) {
            const auto &limitSpork = sporkMap.find(CSporkAction::featureLelantusTransparentLimit);
            if (limitSpork != sporkMap.cend()) {
                if (lelantus::GetSpendTransparentAmount(tx) > (CAmount)limitSpork->second.second)
                    return state.DoS(100, false, REJECT_CONFLICT, "txn-lelantus-disabled", false, "Lelantus transaction is over the transparent limit");
            }
        }
    }
    return true;
}

void CSporkTx::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.push_back(Pair("version", nVersion));

    UniValue jsonActions;
    jsonActions.setArray();
    for (const CSporkAction &action: actions) {
        UniValue jsonAction;
        jsonAction.setObject();
        switch (action.actionType) {
        case CSporkAction::sporkDisable:
            jsonAction.push_back(Pair("actiontype", "disable"));
            if (action.nEnableAtHeight != 0)
                jsonAction.push_back(Pair("enableAtHeight", (int)action.nEnableAtHeight));
            break;

        case CSporkAction::sporkEnable:
            jsonAction.push_back(Pair("actiontype", "enable"));
            break;

        case CSporkAction::sporkLimit:
            jsonAction.push_back(Pair("actiontype", "limit"));
            jsonAction.push_back(Pair("parameter", action.parameter));
            if (action.nEnableAtHeight != 0)
                jsonAction.push_back(Pair("enableAtHeight", (int)action.nEnableAtHeight));
            break;
        }
        jsonAction.push_back(Pair("feature", action.feature));
        jsonActions.push_back(jsonAction);
    }

    obj.push_back(Pair("actions", jsonActions));
}

// CSporkManager

CSporkManager *CSporkManager::sharedSporkManager = new CSporkManager();

bool CSporkManager::BlockConnected(const CBlock &block, CBlockIndex *pindex)
{
    return UpdateActiveSporkMap(pindex->activeDisablingSporks, pindex->pprev->activeDisablingSporks, pindex->nHeight, block.vtx);
}

bool CSporkManager::UpdateActiveSporkMap(ActiveSporkMap &sporkMap, const ActiveSporkMap &previousSporkMap, int nHeight, const std::vector<CTransactionRef> &sporkTransactions)
{
    const Consensus::Params &chainParams = Params().GetConsensus();

    if (!(nHeight >= chainParams.nEvoSporkStartBlock && nHeight < chainParams.nEvoSporkStopBlock))
        return true;

    sporkMap.clear();
    for (const auto &prevIndexSpork: previousSporkMap) {
        // check if spork is expired at new block height
        if (!(prevIndexSpork.second.first != 0 && nHeight >= prevIndexSpork.second.first))
            sporkMap.insert(prevIndexSpork);
    }

    for (CTransactionRef ptx: sporkTransactions) {
        if (ptx->nVersion >= 3 && ptx->nType == TRANSACTION_SPORK) {
            CSporkTx sporkTx;
            if (!GetTxPayload<CSporkTx>(*ptx, sporkTx))
                return false;

            for (const CSporkAction &action: sporkTx.actions) {
                switch (action.actionType) {
                case CSporkAction::sporkEnable:
                    // enable immediately
                    sporkMap.erase(action.feature);
                    break;
                
                case CSporkAction::sporkDisable:
                case CSporkAction::sporkLimit:
                    if (action.nEnableAtHeight == 0 || (action.nEnableAtHeight != 0 && nHeight < action.nEnableAtHeight))
                        sporkMap[action.feature] = std::pair<int, int64_t>(action.nEnableAtHeight, action.parameter);
                    break;
                }
            }
        }
    }

    return true;
}

bool CSporkManager::IsFeatureEnabled(const std::string &featureName, const CBlockIndex *pindex)
{
    return pindex->activeDisablingSporks.count(featureName) == 0;
}

bool CSporkManager::IsTransactionAllowed(const CTransaction &tx, const ActiveSporkMap &sporkMap, CValidationState &state)
{
    return ::IsTransactionAllowed(tx, sporkMap, state);
}

bool CSporkManager::IsBlockAllowed(const CBlock &block, const CBlockIndex *pindex, CValidationState &state) {
    if (pindex->activeDisablingSporks.count(CSporkAction::featureLelantusTransparentLimit) > 0) {
        // limit total transparent output of lelantus joinsplit
        int64_t limit = pindex->activeDisablingSporks.at(CSporkAction::featureLelantusTransparentLimit).second;
        CAmount totalTransparentOutput = 0;

        for (const auto &tx: block.vtx) {
            if (!tx->IsLelantusJoinSplit())
                continue;

            totalTransparentOutput += lelantus::GetSpendTransparentAmount(*tx);
        }

        return totalTransparentOutput <= CAmount(limit) ? true :
                state.DoS(100, false, REJECT_CONFLICT, "txn-lelantus-disabled", false, "Block is over the transparent output limit because of existing spork");
    }

    return true;
}

// CMempoolSporkManager

bool CMempoolSporkManager::AcceptSporkToMemoryPool(const CTransaction &sporkTx)
{
    int chainHeight = chainActive.Height();

    // remove expired sporks from the mempool
    auto it = mempoolSporks.begin();
    while (it != mempoolSporks.end()) {
        if (it->second.first != 0 && chainHeight >= it->second.first)
            mempoolSporks.erase(it++);
        else
            it++;
    }

    if (sporkTx.nVersion >= 3 && sporkTx.nType == TRANSACTION_SPORK) {
        CSporkTx sporkTxPayload;
        if (!GetTxPayload<CSporkTx>(sporkTx, sporkTxPayload))
            return false;
    
        for (const CSporkAction &action: sporkTxPayload.actions) {
            if ((action.actionType == CSporkAction::sporkDisable || action.actionType == CSporkAction::sporkLimit) &&
                        (action.nEnableAtHeight == 0 || action.nEnableAtHeight > chainHeight))
                mempoolSporks[action.feature] = std::pair<int, int64_t>(action.nEnableAtHeight, action.parameter);
        }
    }

    return true;       
}

void CMempoolSporkManager::RemovedFromMemoryPool(const CTransaction &sporkTx)
{
    if (sporkTx.nVersion >= 3 && sporkTx.nType == TRANSACTION_SPORK) {
        CSporkTx sporkTxPayload;
        if (GetTxPayload<CSporkTx>(sporkTx, sporkTxPayload)) {
            // remove it from the mempool spork manager. If the removal is due to inclusion into the block
            // IsFeatureEnabled() will later check the last block index and block the feature
            for (const CSporkAction &action: sporkTxPayload.actions)
                mempoolSporks.erase(action.feature);
        }
    }
}

bool CMempoolSporkManager::IsFeatureEnabled(const std::string &feature) const
{
    if (mempoolSporks.count(feature) > 0)
        return false;

    const CBlockIndex *chainTip = chainActive.Tip();
    if (!chainTip)
        return true;

    return CSporkManager::GetSporkManager()->IsFeatureEnabled(feature, chainTip);
}

bool CMempoolSporkManager::IsTransactionAllowed(const CTransaction &tx, CValidationState &state) const
{
    if (!::IsTransactionAllowed(tx, mempoolSporks, state))
        return false;

    const CBlockIndex *chainTip = chainActive.Tip();
    if (!chainTip)
        return true;

    return ::IsTransactionAllowed(tx, chainTip->activeDisablingSporks, state);
}
