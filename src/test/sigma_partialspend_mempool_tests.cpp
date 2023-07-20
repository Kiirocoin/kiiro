#include "util.h"

#include <stdint.h>
#include <vector>

#include "chainparams.h"
#include "key.h"
#include "validation.h"
#include "pubkey.h"
#include "txdb.h"
#include "txmempool.h"
#include "sigma.h"

#include "test/fixtures.h"
#include "test/testutil.h"

#include "wallet/db.h"
#include "wallet/wallet.h"
#include "wallet/walletexcept.h"

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>

using namespace std;

static bool addToMempool(const CWalletTx& tx) {
    CValidationState state;
    bool fMissingInputs;
    CAmount nMaxRawTxFee = maxTxFee;
    LOCK(cs_main);
    return AcceptToMemoryPool(mempool, state, tx.tx, false, &fMissingInputs, NULL, true, nMaxRawTxFee);
}

BOOST_FIXTURE_TEST_SUITE(sigma_partialspend_mempool_tests, ZerocoinTestingSetup200)

/*
* 1. Make two mint of denom pair
* 2. Create two spend transactions using same mint
* 3. Double spend with previous spend in last block
*/
BOOST_AUTO_TEST_CASE(partialspend)
{
    // Generate addresses
    CPubKey newKey1, newKey2;
    BOOST_CHECK_MESSAGE(pwalletMain->GetKeyFromPool(newKey1), "Fail to get new address");
    BOOST_CHECK_MESSAGE(pwalletMain->GetKeyFromPool(newKey2), "Fail to get new address");

    const CBitcoinAddress randomAddr1(newKey1.GetID());
    const CBitcoinAddress randomAddr2(newKey2.GetID());

    sigma::CSigmaState* sigmaState = sigma::CSigmaState::GetState();
    // Can't test denomination 0.1, because we're unable to pay the fees.
    std::vector<std::string> denominations = {"0.5", "1", "10", "25", "100"};

    // Get smallest denomination value
    std::vector<sigma::CoinDenomination> denoms;
    sigma::GetAllDenoms(denoms);
    CAmount smallestDenomAmount;
    sigma::DenominationToInteger(denoms.back(), smallestDenomAmount);


    // foreach denom from denominations
    for (const auto& denomination : denominations) {

        // Get denomination value
        sigma::CoinDenomination denomId;
        CAmount denomAmount;
        sigma::StringToDenomination(denomination, denomId);
        sigma::DenominationToInteger(denomId, denomAmount);
        std::string stringError;

        // Make sure that transactions get to mempool
        pwalletMain->SetBroadcastTransactions(true);

        // Verify Mint is successful
        std::vector<sigma::PrivateCoin> privCoins;
        const auto& sigmaParams = sigma::Params::get_default();
        privCoins.push_back(sigma::PrivateCoin(sigmaParams, denomId));
        privCoins.push_back(sigma::PrivateCoin(sigmaParams, denomId));

        vector<CHDMint> vDMints;
        auto vecSend = CWallet::CreateSigmaMintRecipients(privCoins, vDMints);
        CWalletTx wtx;
        stringError = pwalletMain->MintAndStoreSigma(vecSend, privCoins, vDMints, wtx);

        BOOST_CHECK_MESSAGE(stringError == "", "Mint Failed");

        // Verify Mint gets in the mempool
        BOOST_CHECK_MESSAGE(mempool.size() == 1, "Mint was not added to mempool");

        int previousHeight = chainActive.Height();
        CBlock b = CreateAndProcessBlock(scriptPubKey);
        BOOST_CHECK_MESSAGE(previousHeight + 1 == chainActive.Height(), "Block not added to chain");

        // Verify Mint is mined
        BOOST_CHECK_MESSAGE(mempool.size() == 0, "Mempool was not cleared");

        previousHeight = chainActive.Height();

        // Add 5 more blocks to makesure sigma coins can be spend
        CreateAndProcessEmptyBlocks(5, scriptPubKey);

        BOOST_CHECK_MESSAGE(previousHeight + 5 == chainActive.Height(), "Block not added to chain");

        // Create tx to do double spend before spend
        CWalletTx dtx;
        CAmount dFee;
        std::vector<CSigmaEntry> dSelected;
        std::vector<CHDMint> dChanges;

        CAmount denomAmount005;
        sigma::DenominationToInteger(sigma::CoinDenomination::SIGMA_DENOM_0_05, denomAmount005);

        // Make dtx is not identical to tx
        std::vector<CRecipient> dupRecipients = {
            {GetScriptForDestination(randomAddr2.Get()), denomAmount / 2, false},
            {GetScriptForDestination(randomAddr1.Get()), denomAmount / 2 - denomAmount005 - CENT, false},
        };
        bool fChangeAddedToFee;
        dtx = pwalletMain->CreateSigmaSpendTransaction(dupRecipients, dFee, dSelected, dChanges, fChangeAddedToFee);

        // Create partial spend transaction
        CWalletTx tx;

        std::vector<CRecipient> recipients = {
            {GetScriptForDestination(randomAddr1.Get()), denomAmount / 2, false},
            {GetScriptForDestination(randomAddr2.Get()), denomAmount / 2 - denomAmount005 - CENT, false},
        };

        // Create two spend transactions using the same mint.
        BOOST_CHECK_NO_THROW(pwalletMain->SpendSigma(recipients, tx));

        // Try to put two in the same block and it will fail, expect 1
        // And verify spend got into mempool
        BOOST_CHECK_MESSAGE(mempool.size() == 1, "Spend was not added to mempool");

        b = CreateBlock(scriptPubKey);
        previousHeight = chainActive.Height();
        BOOST_CHECK_MESSAGE(ProcessBlock(b), "ProcessBlock failed although valid spend inside");
        BOOST_CHECK_MESSAGE(previousHeight + 1 == chainActive.Height(), "Block not added to chain");

        BOOST_CHECK_MESSAGE(mempool.size() == 0, "Mempool was not cleared");

        BOOST_CHECK_NO_THROW(pwalletMain->SpendSigma(recipients, tx));

        //Verify spend got into mempool
        BOOST_CHECK_MESSAGE(mempool.size() == 1, "Spend was not added to mempool");

        b = CreateBlock(scriptPubKey);
        previousHeight = chainActive.Height();
        BOOST_CHECK_MESSAGE(ProcessBlock(b), "ProcessBlock failed although valid spend inside");
        BOOST_CHECK_MESSAGE(previousHeight + 1 == chainActive.Height(), "Block not added to chain");

        BOOST_CHECK_MESSAGE(mempool.size() == 0, "Mempool not cleared");

        // Test double spend with previous spend in last block
        BOOST_CHECK_MESSAGE(!addToMempool(dtx), "Spend created although double");
        // This confirms that double spend is blocked and cannot enter mempool
        BOOST_CHECK_MESSAGE(mempool.size() == 0, "Mempool not empty although mempool should reject double spend");

        // Temporary disable usedCoinSerials check to force double spend in mempool
        auto tempSerials = sigmaState->containers.usedCoinSerials;
        sigmaState->containers.usedCoinSerials.clear();

        // Add invalid transaction to mempool, this will pass because we have removed serials from state
        BOOST_CHECK_MESSAGE(addToMempool(dtx), "Spend created although double");
        BOOST_CHECK_MESSAGE(mempool.size() == 1, "Mempool not set");

        // Bring serials back to sigma state
        sigmaState->containers.usedCoinSerials = tempSerials;

        // CreateBlock throw exception because invalid transaction is in mempool
        BOOST_CHECK_EXCEPTION(CreateBlock(scriptPubKey), std::runtime_error, no_check);
        BOOST_CHECK_MESSAGE(mempool.size() == 1, "Mempool not set");

        // Add invalid tx too block manually
        // it will work be cause we remove serials from state and don't bring it back before create block like previous test
        tempSerials = sigmaState->containers.usedCoinSerials;
        sigmaState->containers.usedCoinSerials.clear();
        CreateBlock(scriptPubKey);

        // Bring serials back
        sigmaState->containers.usedCoinSerials = tempSerials;

        // Create new block, last block should be remove because it contain invalid spend tx
        mempool.clear();
        previousHeight = chainActive.Height();
        BOOST_CHECK_MESSAGE(ProcessBlock(b), "ProcessBlock failed");
        // This test confirms that a block containing a double spend is rejected and not added in the chain
        BOOST_CHECK_MESSAGE(previousHeight == chainActive.Height(), "Double spend - Block added to chain even though same spend in previous block");

        mempool.clear();
        sigmaState->Reset();
    }
}

/*
* 1. Create two mints with denomination 1 in same transaction
* 2. Spend all old coin and expect new coin from remint
* 3. Spend reminted coins
*/
BOOST_AUTO_TEST_CASE(partialspend_remint) {

    // Generate addresses
    CPubKey newKey1, newKey2;
    BOOST_CHECK_MESSAGE(pwalletMain->GetKeyFromPool(newKey1), "Fail to get new address");
    BOOST_CHECK_MESSAGE(pwalletMain->GetKeyFromPool(newKey2), "Fail to get new address");

    const CBitcoinAddress randomAddr1(newKey1.GetID());
    const CBitcoinAddress randomAddr2(newKey2.GetID());

    sigma::CSigmaState* sigmaState = sigma::CSigmaState::GetState();

    CAmount denomAmount1;
    CAmount denomAmount01;
    sigma::DenominationToInteger(sigma::CoinDenomination::SIGMA_DENOM_1, denomAmount1);
    sigma::DenominationToInteger(sigma::CoinDenomination::SIGMA_DENOM_0_1, denomAmount01);

    std::string stringError;

    // Make sure that transactions get to mempool
    pwalletMain->SetBroadcastTransactions(true);

    // Verify Mint is successful
    std::vector<sigma::PrivateCoin> privCoins;
    const auto& sigmaParams = sigma::Params::get_default();
    privCoins.push_back(sigma::PrivateCoin(sigmaParams, sigma::CoinDenomination::SIGMA_DENOM_1));
    privCoins.push_back(sigma::PrivateCoin(sigmaParams, sigma::CoinDenomination::SIGMA_DENOM_1));

    vector<CHDMint> vDMints;
    auto vecSend = CWallet::CreateSigmaMintRecipients(privCoins, vDMints);
    CWalletTx wtx;
    stringError = pwalletMain->MintAndStoreSigma(vecSend, privCoins, vDMints, wtx);

    BOOST_CHECK_MESSAGE(stringError == "", "Mint Failed");

    // Verify Mint gets in the mempool
    BOOST_CHECK_MESSAGE(mempool.size() == 1, "Mint was not added to mempool");

    int previousHeight = chainActive.Height();
    CBlock b = CreateAndProcessBlock(scriptPubKey);
    BOOST_CHECK_MESSAGE(previousHeight + 1 == chainActive.Height(), "Block not added to chain");

    BOOST_CHECK_MESSAGE(mempool.size() == 0, "Mempool was not cleared");

    // Fees are expected to be less than 0.2 kiirocoin.
    std::vector<CRecipient> recipients = {
        {GetScriptForDestination(randomAddr1.Get()), denomAmount1 , false},
        {GetScriptForDestination(randomAddr2.Get()), denomAmount1 - 4 * denomAmount01, false},
    };

    previousHeight = chainActive.Height();
    // Add 5 more blocks and verify that Mint can not be spent until 6 blocks verification
    CreateAndProcessEmptyBlocks(5, scriptPubKey);

    BOOST_CHECK_MESSAGE(previousHeight + 5 == chainActive.Height(), "Block not added to chain");

    // spend all and should remints 2 coins of denomination 0.1
    CWalletTx tx;
    BOOST_CHECK_NO_THROW(pwalletMain->SpendSigma(recipients, tx));

    // Try to put two in the same block and it will fail, expect 1
    // And verify spend got into mempool
    BOOST_CHECK_MESSAGE(mempool.size() == 1, "Spend was not added to mempool");

    previousHeight = chainActive.Height();
    CreateAndProcessBlock(scriptPubKey);
    BOOST_CHECK_MESSAGE(previousHeight + 1 == chainActive.Height(), "Block not added to chain");
    BOOST_CHECK_MESSAGE(mempool.size() == 0, "Mempool was not cleared");

    previousHeight = chainActive.Height();
    // Add 5 more blocks and verify that re-minted coin can not be spent until 6 blocks verification
    CreateAndProcessEmptyBlocks(5, scriptPubKey);

    BOOST_CHECK_MESSAGE(previousHeight + 5 == chainActive.Height(), "Block not added to chain");

    // Can't use the other 0.1, it goes to fees.
    recipients = {
        {GetScriptForDestination(randomAddr1.Get()), denomAmount01 , false},
    };

    // Use remints
    BOOST_CHECK_NO_THROW(pwalletMain->SpendSigma(recipients, tx));

    mempool.clear();
    sigmaState->Reset();
}

BOOST_AUTO_TEST_CASE(same_serial_in_a_transaction) {
    // Generate addresses
    CPubKey newKey1, newKey2;
    BOOST_CHECK_MESSAGE(pwalletMain->GetKeyFromPool(newKey1), "Fail to get new address");
    BOOST_CHECK_MESSAGE(pwalletMain->GetKeyFromPool(newKey2), "Fail to get new address");

    const CBitcoinAddress randomAddr1(newKey1.GetID());
    const CBitcoinAddress randomAddr2(newKey2.GetID());

    sigma::CSigmaState* sigmaState = sigma::CSigmaState::GetState();

    CAmount denomAmount01;
    sigma::DenominationToInteger(sigma::CoinDenomination::SIGMA_DENOM_0_1, denomAmount01);

    CAmount denomAmount;
    sigma::DenominationToInteger(sigma::CoinDenomination::SIGMA_DENOM_1, denomAmount);

    // Make sure that transactions get to mempool
    pwalletMain->SetBroadcastTransactions(true);

    // mint 2 coins
    // Verify Mint is successful
    std::vector<sigma::PrivateCoin> privCoins;
    const auto& sigmaParams = sigma::Params::get_default();
    privCoins.push_back(sigma::PrivateCoin(sigmaParams, sigma::CoinDenomination::SIGMA_DENOM_1));
    privCoins.push_back(sigma::PrivateCoin(sigmaParams, sigma::CoinDenomination::SIGMA_DENOM_1));

    vector<CHDMint> vDMints;
    auto vecSend = CWallet::CreateSigmaMintRecipients(privCoins, vDMints);
    CWalletTx wtx;
    string stringError = pwalletMain->MintAndStoreSigma(vecSend, privCoins, vDMints, wtx);

    BOOST_CHECK_MESSAGE(stringError == "", "Mint Failed");

    BOOST_CHECK_MESSAGE(mempool.size() == 1, "Mint was not added to mempool " + stringError);
    CreateAndProcessBlock(scriptPubKey);
    BOOST_CHECK_MESSAGE(mempool.size() == 0, "Mempool was not cleared");

    // Add 5 more blocks and verify that Mint can not be spent until 6 blocks verification
    CreateAndProcessEmptyBlocks(5, scriptPubKey);

    // - denomAmount01 stands for fees. We'll normally payt a fee of 0.1.
    std::vector<CRecipient> recipients = {
        {GetScriptForDestination(randomAddr1.Get()), denomAmount , false},
        {GetScriptForDestination(randomAddr2.Get()), denomAmount - denomAmount01 - CENT, false}, // reserve a CENT to pay fee
    };

    // Create tx
    CAmount fee;
    std::vector<CSigmaEntry> selected;
    std::vector<CHDMint> changes;
    bool fChangeAddedToFee;
    auto tx = pwalletMain->CreateSigmaSpendTransaction(recipients, fee, selected, changes, fChangeAddedToFee);

    // Expect 2 spends
    BOOST_CHECK_EQUAL(tx.tx->vin.size(), 2);

    // construct double spend transaction
    auto extendedScript = CScript();
    extendedScript.insert(extendedScript.end(), tx.tx->vin[0].scriptSig.begin(), tx.tx->vin[0].scriptSig.end());

    // append string "test" to make scriptSigs in vin 1 and 2 difference. but still share same serial.
    // vin[1].scriptSig = vin[0].scriptSig + "test"
    const std::string extended = "test";
    extendedScript.insert(extendedScript.end(), extended.begin(), extended.end());

    CMutableTransaction txCopy = *tx.tx;
    txCopy.vin[1].scriptSig = extendedScript;
    CWalletTx wtxCopy = tx;
    wtxCopy.SetTx(MakeTransactionRef(txCopy));

    // Add invalid transaction to mempool,
    BOOST_CHECK_MESSAGE(!addToMempool(wtxCopy), "Double spend transaction have been accepted");
    BOOST_CHECK_MESSAGE(mempool.size() == 0, "Mempool accept invalid transaction");

    sigmaState->Reset();
}

BOOST_AUTO_TEST_CASE(double_mint_into_mempool) {
        sigma::CSigmaState *sigmaState = sigma::CSigmaState::GetState();
        string denomination;
        std::vector<string> denominations = {"0.05", "0.1", "0.5", "1", "10", "25", "100"};
        const auto& sigmaParams = sigma::Params::get_default();

        // foreach denom from denominations
        for(auto denomination : denominations)
        {
            // Make sure that transactions get to mempool
            pwalletMain->SetBroadcastTransactions(true);
            sigma::CoinDenomination denom;
            sigma:: StringToDenomination(denomination, denom);
            std::vector<sigma::PrivateCoin> privCoins;
            privCoins.push_back(sigma::PrivateCoin(sigmaParams, denom));

            vector<CHDMint> vDMints;
            auto vecSend = CWallet::CreateSigmaMintRecipients(privCoins, vDMints);

            CWalletTx wtx;
            string stringError = pwalletMain->MintAndStoreSigma(vecSend, privCoins, vDMints, wtx);
            BOOST_CHECK_MESSAGE(stringError == "", "Mint Failed");

            BOOST_CHECK_MESSAGE(mempool.size() == 1, "Mint not added into mempool");
            //Try mint with the same coin
            pwalletMain->MintAndStoreSigma(vecSend, privCoins, vDMints, wtx);
            //Second mint should not be added into mempool
            BOOST_CHECK_MESSAGE(mempool.size() == 1, "Double mint added into mempool");

            mempool.clear();
            sigmaState->Reset();
        }
}

BOOST_AUTO_TEST_SUITE_END()
