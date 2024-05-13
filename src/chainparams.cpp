// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"
#include "consensus/consensus.h"
#include "kiirocoin_params.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"
#include "bitcoin_bignum/bignum.h"
#include "blacklists.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"
#include "arith_uint256.h"

using namespace secp_primitives;

// If some feature is enabled at block intervalStart and its duration is intervalLength halving distance between blocks
// causes the end to happen sooner in real time. This function adjusts the end block number so the approximate ending time
// is left intact
static constexpr int AdjustEndingBlockNumberAfterSubsidyHalving(int intervalStart, int intervalLength, int halvingPoint) {
    if (halvingPoint < intervalStart || halvingPoint >= intervalStart + intervalLength)
        // halving occurs outside of interval
        return intervalStart + intervalLength;
    else
        return halvingPoint + (intervalStart + intervalLength - halvingPoint)*2;
}

static CBlock CreateGenesisBlock(const char *pszTimestamp, const CScript &genesisOutputScript, uint32_t nTime, uint32_t nNonce,
        uint32_t nBits, int32_t nVersion, const CAmount &genesisReward,
        std::vector<unsigned char> extraNonce) {
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 504365040 << CBigNum(4).getvch() << std::vector < unsigned char >
    ((const unsigned char *) pszTimestamp, (const unsigned char *) pszTimestamp + strlen(pszTimestamp)) << extraNonce;
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}


static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount &genesisReward,
                   std::vector<unsigned char> extraNonce) {
    //btzc: firo timestamp
    const char *pszTimestamp = "Times 2014/10/31 Maine Judge Says Nurse Must Follow Ebola Quarantine for Now";
    const CScript genesisOutputScript = CScript();
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward,
                              extraNonce);
}

// this one is for testing only
static Consensus::LLMQParams llmq5_60 = {
        .type = Consensus::LLMQ_5_60,
        .name = "llmq_5_60",
        .size = 5,
        .minSize = 3,
        .threshold = 3,

        .dkgInterval = 24, // one DKG per hour
        .dkgPhaseBlocks = 2,
        .dkgMiningWindowStart = 10, // dkgPhaseBlocks * 5 = after finalization
        .dkgMiningWindowEnd = 18,
        .dkgBadVotesThreshold = 8,

        .signingActiveQuorumCount = 2, // just a few ones to allow easier testing

        .keepOldConnections = 3,
};

// to use on testnet
static Consensus::LLMQParams llmq10_70 = {
        .type = Consensus::LLMQ_10_70,
        .name = "llmq_10_70",
        .size = 10,
        .minSize = 8,
        .threshold = 7,

        .dkgInterval = 24, // one DKG per hour
        .dkgPhaseBlocks = 2,
        .dkgMiningWindowStart = 10, // dkgPhaseBlocks * 5 = after finalization
        .dkgMiningWindowEnd = 18,
        .dkgBadVotesThreshold = 8,

        .signingActiveQuorumCount = 2, // just a few ones to allow easier testing

        .keepOldConnections = 3,
};

static Consensus::LLMQParams llmq50_60 = {
        .type = Consensus::LLMQ_50_60,
        .name = "llmq_50_60",
        .size = 50,
        .minSize = 40,
        .threshold = 30,

        .dkgInterval = 18, // one DKG per 90 minutes
        .dkgPhaseBlocks = 2,
        .dkgMiningWindowStart = 10, // dkgPhaseBlocks * 5 = after finalization
        .dkgMiningWindowEnd = 16,
        .dkgBadVotesThreshold = 40,

        .signingActiveQuorumCount = 16, // a full day worth of LLMQs

        .keepOldConnections = 17,
};

static Consensus::LLMQParams llmq400_60 = {
        .type = Consensus::LLMQ_400_60,
        .name = "llmq_400_60",
        .size = 400,
        .minSize = 300,
        .threshold = 240,

        .dkgInterval = 12 * 12, // one DKG every 12 hours
        .dkgPhaseBlocks = 4,
        .dkgMiningWindowStart = 20, // dkgPhaseBlocks * 5 = after finalization
        .dkgMiningWindowEnd = 28,
        .dkgBadVotesThreshold = 300,

        .signingActiveQuorumCount = 4, // two days worth of LLMQs

        .keepOldConnections = 5,
};

// Used for deployment and min-proto-version signalling, so it needs a higher threshold
static Consensus::LLMQParams llmq400_85 = {
        .type = Consensus::LLMQ_400_85,
        .name = "llmq_400_85",
        .size = 400,
        .minSize = 350,
        .threshold = 340,

        .dkgInterval = 12 * 24, // one DKG every 24 hours
        .dkgPhaseBlocks = 4,
        .dkgMiningWindowStart = 20, // dkgPhaseBlocks * 5 = after finalization
        .dkgMiningWindowEnd = 48, // give it a larger mining window to make sure it is mined
        .dkgBadVotesThreshold = 300,

        .signingActiveQuorumCount = 4, // two days worth of LLMQs

        .keepOldConnections = 5,
};


/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";

        consensus.chainType = Consensus::chainMain;

        consensus.nSubsidyHalvingFirst = 30;
        consensus.nSubsidyHalvingSecond = AdjustEndingBlockNumberAfterSubsidyHalving(30, 420000, 250); // =958655
        consensus.nSubsidyHalvingInterval = 420000*2;
        consensus.nSubsidyHalvingStopBlock = AdjustEndingBlockNumberAfterSubsidyHalving(0, 3646849, 250);  // =6807477

        consensus.stage2DevelopmentFundShare = 15;
        consensus.stage2ZnodeShare = 50;
        consensus.stage2DevelopmentFundAddress = "KKFbnqPdzBwGFP4PvCsLrW8e7mBKwyhhfx";

        consensus.stage3StartTime = 1689499903; // Thursday, 16 June 2022 12:00:00 UTC
        consensus.stage3StartBlock = 250;
        consensus.stage3DevelopmentFundShare = 10;
        consensus.stage3CommunityFundShare = 10;
        consensus.stage3MasternodeShare = 60;
        consensus.stage3DevelopmentFundAddress = "KWTco92wURX5Jwu3mMdWrs36j574meAvew";
        consensus.stage3CommunityFundAddress = "KDW8CeScVpWFzekvZm4f37qs5GxByEGSKE";

        consensus.nStartCollateralChange = 75000;

        std::vector<FundRewardStructure> rewardStructures = { {190000, 10}, {INT_MAX, 9} }; // 9% dev/community fee forever
        std::vector<FundRewardStructure> rewardStructuresDataMining = { {INT_MAX, 8} }; // 8% data mining fee forever
        consensus.nDevelopmentFundPayment = FundPayment(rewardStructures, 30, "KWTco92wURX5Jwu3mMdWrs36j574meAvew");
        consensus.nCommunityFundPayment = FundPayment(rewardStructures, 30,"KDW8CeScVpWFzekvZm4f37qs5GxByEGSKE");
        consensus.nDataMiningFundPayment = FundPayment(rewardStructuresDataMining, 190000,"KVibEVgfWA8qtiwdNNfH9n7tW3uL1ZFcRj");
        
        consensus.nCollaterals = CMasternodeCollaterals(
          { {75000, 1000 * COIN}, // Block 0 - 74999 Collateral 1000
            {125000, 2500 * COIN}, // Block 75000 - 124999 Collateral 2500
            {175000, 3000 * COIN}, // Block 125000 - 174999 Collateral 3000
            {INT_MAX, 4000 * COIN} // Block 175000 - Infinity Collateral 4000
          },
          { {190000, 60},{INT_MAX, 50} }
        );        

        consensus.nStartBlacklist = 29399;
        consensus.nStartDuplicationCheck = 29352;

        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.nMinNFactor = 10;
        consensus.nMaxNFactor = 30;
        consensus.nChainStartTime = 1689495903;
        consensus.BIP34Height = 22793;
        consensus.BIP34Hash = uint256S("0x000000000000024b89b42a942fe0d9fea3bb44ab7bd1b19115dd6a759c0808b8");
        consensus.BIP65Height = INT_MAX;
        consensus.BIP66Height = INT_MAX;
        consensus.powLimit = uint256S("00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 60 * 60; // 60 minutes between retargets
        consensus.nPowTargetSpacing = 10 * 60; // 10 minute blocks
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1689495903; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1699495903; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1689495903; // May 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1699495903; // May 1st, 2017

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1689495903; // November 15th, 2016.
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1699495903; // November 15th, 2017.

        // Deployment of MTP
        consensus.vDeployments[Consensus::DEPLOYMENT_MTP].bit = 12;
        consensus.vDeployments[Consensus::DEPLOYMENT_MTP].nStartTime = SWITCH_TO_MTP_BLOCK_HEADER - 2*60; // 2 hours leeway
        consensus.vDeployments[Consensus::DEPLOYMENT_MTP].nTimeout = SWITCH_TO_MTP_BLOCK_HEADER + consensus.nMinerConfirmationWindow*2 * 5*60;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000708f98bf623f02e");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x "); //184200

        consensus.nCheckBugFixedAtBlock = ZC_CHECK_BUG_FIXED_AT_BLOCK;
        consensus.nZnodePaymentsBugFixedAtBlock = ZC_ZNODE_PAYMENT_BUG_FIXED_AT_BLOCK;
	    consensus.nSpendV15StartBlock = ZC_V1_5_STARTING_BLOCK;
	    consensus.nSpendV2ID_1 = ZC_V2_SWITCH_ID_1;
	    consensus.nSpendV2ID_10 = ZC_V2_SWITCH_ID_10;
	    consensus.nSpendV2ID_25 = ZC_V2_SWITCH_ID_25;
	    consensus.nSpendV2ID_50 = ZC_V2_SWITCH_ID_50;
	    consensus.nSpendV2ID_100 = ZC_V2_SWITCH_ID_100;
	    consensus.nModulusV2StartBlock = ZC_MODULUS_V2_START_BLOCK;
        consensus.nModulusV1MempoolStopBlock = ZC_MODULUS_V1_MEMPOOL_STOP_BLOCK;
	    consensus.nModulusV1StopBlock = ZC_MODULUS_V1_STOP_BLOCK;
        consensus.nMultipleSpendInputsInOneTxStartBlock = ZC_MULTIPLE_SPEND_INPUT_STARTING_BLOCK;
        consensus.nDontAllowDupTxsStartBlock = 89700;

        // znode params
        consensus.nZnodePaymentsStartBlock = HF_ZNODE_PAYMENT_START; // not true, but it's ok as long as it's less then nZnodePaymentsIncreaseBlock
        // consensus.nZnodePaymentsIncreaseBlock = 680000; // actual historical value // not used for now, probably later
        // consensus.nZnodePaymentsIncreasePeriod = 576*30; // 17280 - actual historical value // not used for now, probably later
        // consensus.nSuperblockStartBlock = 614820;
        // consensus.nBudgetPaymentsStartBlock = 328008; // actual historical value
        // consensus.nBudgetPaymentsCycleBlocks = 16616; // ~(60*24*30)/2.6, actual number of blocks per month is 200700 / 12 = 16725
        // consensus.nBudgetPaymentsWindowBlocks = 100;

        // evo znodes
        consensus.DIP0003Height = 127; // Approximately June 22 2020, 12:00 UTC
        consensus.DIP0003EnforcementHeight = 128; // Approximately July 13 2020, 12:00 UTC
        consensus.DIP0003EnforcementHash = uint256S("0x");
        consensus.DIP0008Height = 134; // Approximately Jan 28 2021, 11:00 UTC
        consensus.nEvoZnodeMinimumConfirmations = 15;

        // long living quorum params
        consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
        consensus.llmqs[Consensus::LLMQ_400_60] = llmq400_60;
        consensus.llmqs[Consensus::LLMQ_400_85] = llmq400_85;
        consensus.nLLMQPowTargetSpacing = 5*60;
        consensus.llmqChainLocks = Consensus::LLMQ_400_60;
        consensus.llmqForInstantSend = Consensus::LLMQ_50_60;
        consensus.nInstantSendConfirmationsRequired = 2;
        consensus.nInstantSendKeepLock = 24;
        consensus.nInstantSendBlockFilteringStartHeight = 42115;   // Approx Nov 2 2021 06:00:00 GMT+0000

        consensus.nMTPSwitchTime = SWITCH_TO_MTP_BLOCK_HEADER;
        consensus.nMTPStartBlock = 150;
        consensus.nMTPFiveMinutesStartBlock = SWITCH_TO_MTP_5MIN_BLOCK;
        consensus.nMTPStripDataTime = 1689899903;   // December 08, 2021 09:00 UTC

        consensus.nDifficultyAdjustStartBlock = 100;
        consensus.nFixedDifficulty = 0x2000ffff;
        consensus.nPowTargetSpacingMTP = 5*60;
        consensus.nInitialMTPDifficulty = 0x1c021e57;
        consensus.nMTPRewardReduction = 2;

        consensus.nDisableZerocoinStartBlock = 57000;

        nMaxTipAge = 6 * 60 * 60; // ~144 blocks behind -> 2 x fork detection time, was 24 * 60 * 60 in bitcoin

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 60*60; // fulfilled requests expire in 1 hour
        strZnodePaymentsPubKey = "04549ac134f694c0243f503e8c8a9a986f5de6610049c40b07816809b0d1d06a21b07be27b9bb555931773f62ba6cf35a25fd52f694d4e1106ccd237a7bb899fdd";

        
        
        //btzc: update kiirocoin pchMessage
        pchMessageStart[0] = 0x4b; // K
        pchMessageStart[1] = 0x49; // I
        pchMessageStart[2] = 0x49; // I
        pchMessageStart[3] = 0x52; // R
        nDefaultPort = 8999;
        nPruneAfterHeight = 100000;
         
		 
		
        std::vector<unsigned char> extraNonce(4);
        extraNonce[0] = 0x82;
        extraNonce[1] = 0x3f;
        extraNonce[2] = 0x00;
        extraNonce[3] = 0x00; 
		
	
		 
		 
        genesis = CreateGenesisBlock(ZC_GENESIS_BLOCK_TIME, 142392, 0x1e0ffff0, 2, 0 * COIN, extraNonce);
        const std::string s = genesis.GetHash().ToString();
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x4381deb85b1b2c9843c222944b616d997516dcbd6a964e1eaf0def0830695233"));
        assert(genesis.hashMerkleRoot == uint256S("0x365d2aa75d061370c9aefdabac3985716b1e3b4bb7c4af4ed54f25e5aaa42783"));
        vSeeds.push_back(CDNSSeedData("168.119.163.26", "168.119.163.26", false));
        vSeeds.push_back(CDNSSeedData("37.27.16.153", "37.27.16.153", false));
        vSeeds.push_back(CDNSSeedData("5.75.160.251", "5.75.160.251", false));
        vSeeds.push_back(CDNSSeedData("seed.kiirocoin.org", "seed.kiirocoin.org", false));
        vSeeds.push_back(CDNSSeedData("seed1.kiirocoin.org", "seed1.kiirocoin.org", false));
        vSeeds.push_back(CDNSSeedData("seed2.kiirocoin.org", "seed2.kiirocoin.org", false));
        vSeeds.push_back(CDNSSeedData("seed3.kiirocoin.org", "seed3.kiirocoin.org", false));
        vSeeds.push_back(CDNSSeedData("seed4.kiirocoin.org", "seed4.kiirocoin.org", false));
        // Note that of those with the service bits flag, most only support a subset of possible options
        base58Prefixes[PUBKEY_ADDRESS] = std::vector < unsigned char > (1, 45);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector < unsigned char > (1, 7);
        base58Prefixes[SECRET_KEY] = std::vector < unsigned char > (1, 210);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container < std::vector < unsigned char > > ();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container < std::vector < unsigned char > > ();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand =false;
        fAllowMultiplePorts = false;

        checkpointData = (CCheckpointData) {
                boost::assign::map_list_of
                (0, uint256S("0x4381deb85b1b2c9843c222944b616d997516dcbd6a964e1eaf0def0830695233"))
		(2000, uint256S("0xcd9d43c024f717734d4b5931e227314b4f5750fa3cb84f484a308b60ade5b61f"))
		(51578, uint256S("0xc5bb7521f82bdd4e711791e8bcef02e1a2d1db84dbcbecf99ca0b60f477118ad"))
		(123394, uint256S("0x95792a4e6d6f59f4eeaddd41684e0abd0fa073805b0365e1ceb2914373eaeca8"))
		(171754, uint256S("0xb6a2eff04e1555c3f63f816468c973761ddc87d6709f38097a994e0648afd8fd"))
                
        };

        chainTxData = ChainTxData{
                1697443237, // * UNIX timestamp of last checkpoint block
                121940,     // * total number of transactions between genesis and last checkpoint
                            //   (the tx=... number in the SetBestChain debug.log lines)
                0.014       // * estimated number of transactions per second after checkpoint
        };
        consensus.nSpendV15StartBlock = ZC_V1_5_STARTING_BLOCK;
        consensus.nSpendV2ID_1 = ZC_V2_SWITCH_ID_1;
        consensus.nSpendV2ID_10 = ZC_V2_SWITCH_ID_10;
        consensus.nSpendV2ID_25 = ZC_V2_SWITCH_ID_25;
        consensus.nSpendV2ID_50 = ZC_V2_SWITCH_ID_50;
        consensus.nSpendV2ID_100 = ZC_V2_SWITCH_ID_100;
        consensus.nModulusV2StartBlock = ZC_MODULUS_V2_START_BLOCK;
        consensus.nModulusV1MempoolStopBlock = ZC_MODULUS_V1_MEMPOOL_STOP_BLOCK;
        consensus.nModulusV1StopBlock = ZC_MODULUS_V1_STOP_BLOCK;

        // Sigma related values.
        consensus.nSigmaStartBlock = ZC_SIGMA_STARTING_BLOCK;
        consensus.nSigmaPaddingBlock = ZC_SIGMA_PADDING_BLOCK;
        consensus.nDisableUnpaddedSigmaBlock = ZC_SIGMA_DISABLE_UNPADDED_BLOCK;
        consensus.nStartSigmaBlacklist = 29370;
        consensus.nRestartSigmaWithBlacklistCheck = 29690;
        consensus.nOldSigmaBanBlock = ZC_OLD_SIGMA_BAN_BLOCK;
        consensus.nLelantusStartBlock = ZC_LELANTUS_STARTING_BLOCK;
        consensus.nLelantusFixesStartBlock = ZC_LELANTUS_FIXES_START_BLOCK;
        consensus.nZerocoinV2MintMempoolGracefulPeriod = ZC_V2_MINT_GRACEFUL_MEMPOOL_PERIOD;
        consensus.nZerocoinV2MintGracefulPeriod = ZC_V2_MINT_GRACEFUL_PERIOD;
        consensus.nZerocoinV2SpendMempoolGracefulPeriod = ZC_V2_SPEND_GRACEFUL_MEMPOOL_PERIOD;
        consensus.nZerocoinV2SpendGracefulPeriod = ZC_V2_SPEND_GRACEFUL_PERIOD;
        consensus.nMaxSigmaInputPerBlock = ZC_SIGMA_INPUT_LIMIT_PER_BLOCK;
        consensus.nMaxValueSigmaSpendPerBlock = ZC_SIGMA_VALUE_SPEND_LIMIT_PER_BLOCK;
        consensus.nMaxSigmaInputPerTransaction = ZC_SIGMA_INPUT_LIMIT_PER_TRANSACTION;
        consensus.nMaxValueSigmaSpendPerTransaction = ZC_SIGMA_VALUE_SPEND_LIMIT_PER_TRANSACTION;
        consensus.nMaxLelantusInputPerBlock = ZC_LELANTUS_INPUT_LIMIT_PER_BLOCK;
        consensus.nMaxValueLelantusSpendPerBlock = ZC_LELANTUS_VALUE_SPEND_LIMIT_PER_BLOCK;
        consensus.nMaxLelantusInputPerTransaction = ZC_LELANTUS_INPUT_LIMIT_PER_TRANSACTION;
        consensus.nMaxValueLelantusSpendPerTransaction = ZC_LELANTUS_VALUE_SPEND_LIMIT_PER_TRANSACTION;
        consensus.nMaxValueLelantusMint = ZC_LELANTUS_MAX_MINT;
        consensus.nZerocoinToSigmaRemintWindowSize = 50000;

        for (const auto& str : lelantus::lelantus_blacklist) {
            GroupElement coin;
            try {
                coin.deserialize(ParseHex(str).data());
            } catch (...) {
                continue;
            }
            consensus.lelantusBlacklist.insert(coin);
        }

        for (const auto& str : sigma::sigma_blacklist) {
            GroupElement coin;
            try {
                coin.deserialize(ParseHex(str).data());
            } catch (...) {
                continue;
            }
            consensus.sigmaBlacklist.insert(coin);
        }

        consensus.evoSporkKeyID = "KBdNFgEK8ywqUT7N8quKScCKEkzPCnH6VZ";
        consensus.nEvoSporkStartBlock = ZC_LELANTUS_STARTING_BLOCK;
        consensus.nEvoSporkStopBlock = AdjustEndingBlockNumberAfterSubsidyHalving(ZC_LELANTUS_STARTING_BLOCK, 3*24*12*365, 486200);  // =818275, three years after lelantus
        consensus.nEvoSporkStopBlockExtensionVersion = 14090;
        consensus.nEvoSporkStopBlockPrevious = ZC_LELANTUS_STARTING_BLOCK + 1*24*12*365; // one year after lelantus
        consensus.nEvoSporkStopBlockExtensionGracefulPeriod = 24*12*14; // two weeks

        // reorg
        consensus.nMaxReorgDepth = 5;
        consensus.nMaxReorgDepthEnforcementBlock = 33800;

        // whitelist
        consensus.txidWhitelist.insert(uint256S("0x"));

        // Dandelion related values.
        consensus.nDandelionEmbargoMinimum = DANDELION_EMBARGO_MINIMUM;
        consensus.nDandelionEmbargoAvgAdd = DANDELION_EMBARGO_AVG_ADD;
        consensus.nDandelionMaxDestinations = DANDELION_MAX_DESTINATIONS;
        consensus.nDandelionShuffleInterval = DANDELION_SHUFFLE_INTERVAL;
        consensus.nDandelionFluff = DANDELION_FLUFF;

        // Bip39
        consensus.nMnemonicBlock = 22240;

        // moving lelantus data to v3 payload
        consensus.nLelantusV3PayloadStartBlock = 70000;
        
        // ProgPow
        consensus.nPPSwitchTime = 1686473210;           // Tue Oct 26 2021 06:00:00 GMT+0000
        consensus.nPPBlockNumber = 100;
        consensus.nInitialPPDifficulty = 0x1d016e81;    // 10MH/s
    }
    virtual bool SkipUndoForBlock(int nHeight) const
    {
        return nHeight == 29350;
    }
    virtual bool ApplyUndoForTxout(int nHeight, uint256 const & txid, int n) const
    {
        // We only apply first 23 tx inputs UNDOs for the tx 7702 in block 293526
        if (!SkipUndoForBlock(nHeight)) {
            return true;
        }
        static std::map<uint256, int> const txs = { {uint256S("0x"), 22} };
        std::map<uint256, int>::const_iterator const itx = txs.find(txid);
        if (itx == txs.end()) {
            return false;
        }
        if (n <= itx->second) {
            return true;
        }
        return false;
    }
};

static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";

        consensus.chainType = Consensus::chainTestnet;

        consensus.nSubsidyHalvingFirst = 10;
        consensus.nSubsidyHalvingSecond = 150000;
        consensus.nSubsidyHalvingInterval = 150000;
        consensus.nSubsidyHalvingStopBlock = 1000000;

        consensus.stage2DevelopmentFundShare = 15;
        consensus.stage2ZnodeShare = 35;
        consensus.stage2DevelopmentFundAddress = "TUuKypsbbnHHmZ2auC2BBWfaP1oTEnxjK2";

        consensus.stage3StartTime = 1653409800;  // May 24th 2022 04:30 UTC
        consensus.stage3StartBlock = 0;
        consensus.stage3DevelopmentFundShare = 15;
        consensus.stage3CommunityFundShare = 10;
        consensus.stage3MasternodeShare = 50;
        consensus.stage3DevelopmentFundAddress = "TWDxLLKsFp6qcV1LL4U2uNmW4HwMcapmMU";
        consensus.stage3CommunityFundAddress = "TCkC4uoErEyCB4MK3d6ouyJELoXnuyqe9L";

        consensus.nStartCollateralChange = 50;

        std::vector<FundRewardStructure> rewardStructures = { {250, 10}, {INT_MAX, 9} }; // 9% dev/community fee forever
        std::vector<FundRewardStructure> rewardStructuresDataMining = { { INT_MAX, 8} }; // 8% data mining fee forever
        consensus.nDevelopmentFundPayment = FundPayment(rewardStructures, 1, "TWDxLLKsFp6qcV1LL4U2uNmW4HwMcapmMU");
        consensus.nCommunityFundPayment = FundPayment(rewardStructures, 1,"TCkC4uoErEyCB4MK3d6ouyJELoXnuyqe9L");
        consensus.nDataMiningFundPayment = FundPayment(rewardStructuresDataMining, 250,"TCkC4uoErEyCB4MK3d6ouyJELoXnuyqe9L");
        consensus.nCollaterals = CMasternodeCollaterals(
          { {250, 50 * COIN},
            {450, 150 * COIN},
            {650, 250 * COIN},
            {INT_MAX, 400 * COIN}
          },
          { {250, 60}, {INT_MAX, 50} }
        );        

        consensus.nStartBlacklist = 0;
        consensus.nStartDuplicationCheck = 0;
        consensus.nMajorityEnforceBlockUpgrade = 51;
        consensus.nMajorityRejectBlockOutdated = 75;
        consensus.nMajorityWindow = 100;
        consensus.nMinNFactor = 10;
        consensus.nMaxNFactor = 30;
        consensus.nChainStartTime = 1389306217;
        consensus.BIP34Height = 21111;
        consensus.BIP34Hash = uint256S("0x0000000023b3a96d3484e5abb3755c413e7d41500f8e2a5c3f0dd01299cd8ef8");
        consensus.powLimit = uint256S("00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 60 * 60; // 60 minutes between retargets
        consensus.nPowTargetSpacing = 5 * 60; // 5 minute blocks
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1456790400; // March 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1493596800; // May 1st, 2017

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1462060800; // May 1st 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1493596800; // May 1st 2017

        // Deployment of MTP
        consensus.vDeployments[Consensus::DEPLOYMENT_MTP].bit = 12;
        consensus.vDeployments[Consensus::DEPLOYMENT_MTP].nStartTime = 1539172800 - 2*60;
        consensus.vDeployments[Consensus::DEPLOYMENT_MTP].nTimeout = 1539172800 + consensus.nMinerConfirmationWindow*2 * 5*60;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000708f98bf623f02e");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("3825896ac39b8b27220e7bfaed81c5f979ca11dc874e564c5e70756ad06077b0 "); // 50000

        consensus.nSpendV15StartBlock = 5000;
        consensus.nCheckBugFixedAtBlock = 1;
        consensus.nZnodePaymentsBugFixedAtBlock = 1;

        consensus.nSpendV2ID_1 = ZC_V2_TESTNET_SWITCH_ID_1;
        consensus.nSpendV2ID_10 = ZC_V2_TESTNET_SWITCH_ID_10;
        consensus.nSpendV2ID_25 = ZC_V2_TESTNET_SWITCH_ID_25;
        consensus.nSpendV2ID_50 = ZC_V2_TESTNET_SWITCH_ID_50;
        consensus.nSpendV2ID_100 = ZC_V2_TESTNET_SWITCH_ID_100;
        consensus.nModulusV2StartBlock = ZC_MODULUS_V2_TESTNET_START_BLOCK;
        consensus.nModulusV1MempoolStopBlock = ZC_MODULUS_V1_TESTNET_MEMPOOL_STOP_BLOCK;
        consensus.nModulusV1StopBlock = ZC_MODULUS_V1_TESTNET_STOP_BLOCK;
        consensus.nMultipleSpendInputsInOneTxStartBlock = 1;
        consensus.nDontAllowDupTxsStartBlock = 1;

        // Znode params testnet
        consensus.nZnodePaymentsStartBlock = 2200;
        //consensus.nZnodePaymentsIncreaseBlock = 360; // not used for now, probably later
        //consensus.nZnodePaymentsIncreasePeriod = 650; // not used for now, probably later
        //consensus.nSuperblockStartBlock = 61000;
        //consensus.nBudgetPaymentsStartBlock = 60000;
        //consensus.nBudgetPaymentsCycleBlocks = 50;
        //consensus.nBudgetPaymentsWindowBlocks = 10;
        nMaxTipAge = 0x7fffffff; // allow mining on top of old blocks for testnet

        // evo znodes
        consensus.DIP0003Height = 100;
        consensus.DIP0003EnforcementHeight = 101;
        consensus.DIP0003EnforcementHash.SetNull();

        consensus.DIP0008Height = 103;
        consensus.nEvoZnodeMinimumConfirmations = 0;

        // long living quorum params
        consensus.llmqs[Consensus::LLMQ_10_70] = llmq10_70;
        consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
        consensus.llmqs[Consensus::LLMQ_400_60] = llmq400_60;
        consensus.llmqs[Consensus::LLMQ_400_85] = llmq400_85;
        consensus.nLLMQPowTargetSpacing = 20;
        consensus.llmqChainLocks = Consensus::LLMQ_10_70;
        consensus.llmqForInstantSend = Consensus::LLMQ_10_70;
        consensus.nInstantSendConfirmationsRequired = 2;
        consensus.nInstantSendKeepLock = 6;
        consensus.nInstantSendBlockFilteringStartHeight = 48136;

        consensus.nMTPSwitchTime = 1539172800;
        consensus.nMTPStartBlock = 1;
        consensus.nMTPFiveMinutesStartBlock = 0;
        consensus.nMTPStripDataTime = 1636362000;      // November 08 2021, 09:00 UTC

        consensus.nDifficultyAdjustStartBlock = 100;
        consensus.nFixedDifficulty = 0x2000ffff;
        consensus.nPowTargetSpacingMTP = 5*60;
        consensus.nInitialMTPDifficulty = 0x2000ffff;  // !!!! change it to the real value
        consensus.nMTPRewardReduction = 2;

        consensus.nDisableZerocoinStartBlock = 1;

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes
        strZnodePaymentsPubKey = "046f78dcf911fbd61910136f7f0f8d90578f68d0b3ac973b5040fb7afb501b5939f39b108b0569dca71488f5bbf498d92e4d1194f6f941307ffd95f75e76869f0e";

        pchMessageStart[0] = 0xcf;
        pchMessageStart[1] = 0xfc;
        pchMessageStart[2] = 0xbe;
        pchMessageStart[3] = 0xea;
        nDefaultPort = 18200;
        nPruneAfterHeight = 1000;
        /**
         * btzc: testnet params
         * nTime: 1414776313
         * nNonce: 1620571
         */
        std::vector<unsigned char> extraNonce(4);
       extraNonce[0] = 0x09;
        extraNonce[1] = 0x00;
        extraNonce[2] = 0x00;
        extraNonce[3] = 0x00;

        genesis = CreateGenesisBlock(ZC_GENESIS_BLOCK_TIME, 3577337, 0x1e0ffff0, 2, 0 * COIN, extraNonce);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock ==
                uint256S("0xaa22adcc12becaf436027ffe62a8fb21b234c58c23865291e5dc52cf53f64fca"));
        assert(genesis.hashMerkleRoot ==
                uint256S("0xf70dba2d976778b985de7b5503ede884988d78fbb998d6969e4f676b40b9a741"));
        vFixedSeeds.clear();
        vSeeds.clear();
        // kiirocoin test seeds

        vSeeds.push_back(CDNSSeedData("EVO1", "evo1.kiirocoin.org", false));
        vSeeds.push_back(CDNSSeedData("EVO2", "evo2.kiirocoin.org", false));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector < unsigned char > (1, 65);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector < unsigned char > (1, 178);
        base58Prefixes[SECRET_KEY] = std::vector < unsigned char > (1, 185);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container < std::vector < unsigned char > > ();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container < std::vector < unsigned char > > ();
        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fAllowMultiplePorts = true;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            (0, uint256S("0x"))
        };

        chainTxData = ChainTxData{
            1414776313,
            0,
            0.001
        };

        consensus.nSpendV15StartBlock = ZC_V1_5_TESTNET_STARTING_BLOCK;
        consensus.nSpendV2ID_1 = ZC_V2_TESTNET_SWITCH_ID_1;
        consensus.nSpendV2ID_10 = ZC_V2_TESTNET_SWITCH_ID_10;
        consensus.nSpendV2ID_25 = ZC_V2_TESTNET_SWITCH_ID_25;
        consensus.nSpendV2ID_50 = ZC_V2_TESTNET_SWITCH_ID_50;
        consensus.nSpendV2ID_100 = ZC_V2_TESTNET_SWITCH_ID_100;
        consensus.nModulusV2StartBlock = ZC_MODULUS_V2_TESTNET_START_BLOCK;
        consensus.nModulusV1MempoolStopBlock = ZC_MODULUS_V1_TESTNET_MEMPOOL_STOP_BLOCK;
        consensus.nModulusV1StopBlock = ZC_MODULUS_V1_TESTNET_STOP_BLOCK;

        // Sigma related values.
        consensus.nSigmaStartBlock = 1;
        consensus.nSigmaPaddingBlock = 1;
        consensus.nDisableUnpaddedSigmaBlock = 1;
        consensus.nStartSigmaBlacklist = INT_MAX;
        consensus.nRestartSigmaWithBlacklistCheck = INT_MAX;
        consensus.nOldSigmaBanBlock = 1;

        consensus.nLelantusStartBlock = ZC_LELANTUS_TESTNET_STARTING_BLOCK;
        consensus.nLelantusFixesStartBlock = ZC_LELANTUS_TESTNET_FIXES_START_BLOCK;

        consensus.nZerocoinV2MintMempoolGracefulPeriod = ZC_V2_MINT_TESTNET_GRACEFUL_MEMPOOL_PERIOD;
        consensus.nZerocoinV2MintGracefulPeriod = ZC_V2_MINT_TESTNET_GRACEFUL_PERIOD;
        consensus.nZerocoinV2SpendMempoolGracefulPeriod = ZC_V2_SPEND_TESTNET_GRACEFUL_MEMPOOL_PERIOD;
        consensus.nZerocoinV2SpendGracefulPeriod = ZC_V2_SPEND_TESTNET_GRACEFUL_PERIOD;
        consensus.nMaxSigmaInputPerBlock = ZC_SIGMA_INPUT_LIMIT_PER_BLOCK;
        consensus.nMaxValueSigmaSpendPerBlock = ZC_SIGMA_VALUE_SPEND_LIMIT_PER_BLOCK;
        consensus.nMaxSigmaInputPerTransaction = ZC_SIGMA_INPUT_LIMIT_PER_TRANSACTION;
        consensus.nMaxValueSigmaSpendPerTransaction = ZC_SIGMA_VALUE_SPEND_LIMIT_PER_TRANSACTION;
        consensus.nMaxLelantusInputPerBlock = ZC_LELANTUS_INPUT_LIMIT_PER_BLOCK;
        consensus.nMaxValueLelantusSpendPerBlock = 1100 * COIN;
        consensus.nMaxLelantusInputPerTransaction = ZC_LELANTUS_INPUT_LIMIT_PER_TRANSACTION;
        consensus.nMaxValueLelantusSpendPerTransaction = 1001 * COIN;
        consensus.nMaxValueLelantusMint = 1001 * COIN;
        consensus.nZerocoinToSigmaRemintWindowSize = 0;

        for (const auto& str : lelantus::lelantus_testnet_blacklist) {
            GroupElement coin;
            try {
                coin.deserialize(ParseHex(str).data());
            } catch (...) {
                continue;
            }
            consensus.lelantusBlacklist.insert(coin);
        }

        consensus.evoSporkKeyID = "TWSEa1UsZzDHywDG6CZFDNdeJU6LzhbbBL";
        consensus.nEvoSporkStartBlock = 22000;
        consensus.nEvoSporkStopBlock = 40000;
        consensus.nEvoSporkStopBlockExtensionVersion = 0;

        // reorg
        consensus.nMaxReorgDepth = 4;
        consensus.nMaxReorgDepthEnforcementBlock = 25150;

        // whitelist
        consensus.txidWhitelist.insert(uint256S("44b3829117bd248544c71b430d585cb88b4ce156a7d4fdb9ef3ae96efa8f09d3"));

        // Dandelion related values.
        consensus.nDandelionEmbargoMinimum = DANDELION_TESTNET_EMBARGO_MINIMUM;
        consensus.nDandelionEmbargoAvgAdd = DANDELION_TESTNET_EMBARGO_AVG_ADD;
        consensus.nDandelionMaxDestinations = DANDELION_MAX_DESTINATIONS;
        consensus.nDandelionShuffleInterval = DANDELION_SHUFFLE_INTERVAL;
        consensus.nDandelionFluff = DANDELION_FLUFF;

        // Bip39
        consensus.nMnemonicBlock = 1;

        // moving lelantus data to v3 payload
        consensus.nLelantusV3PayloadStartBlock = 35000;
        
        // ProgPow
        consensus.nPPSwitchTime = 1630069200;           // August 27 2021, 13:00 UTC
        consensus.nPPBlockNumber = 37305;
        consensus.nInitialPPDifficulty = 0x1d016e81;    // 10MH/s
    }
};

static CTestNetParams testNetParams;

/**
 * Devnet (testnet for experimental stuff)
 */
class CDevNetParams : public CChainParams {
public:
    CDevNetParams() {
        strNetworkID = "dev";

        consensus.chainType = Consensus::chainDevnet;

        consensus.nSubsidyHalvingFirst = 120;
        consensus.nSubsidyHalvingSecond = 100000;
        consensus.nSubsidyHalvingInterval = 100000;
        consensus.nSubsidyHalvingStopBlock = 1000000;

        consensus.stage2DevelopmentFundShare = 15;
        consensus.stage2ZnodeShare = 35;
        consensus.stage2DevelopmentFundAddress = "TixHByoJ21dmx5xfMAXTVC4V7k53U7RncU";

        consensus.stage3StartTime = 1653382800;
        consensus.stage3StartBlock = 1514;
        consensus.stage3DevelopmentFundShare = 15;
        consensus.stage3CommunityFundShare = 10;
        consensus.stage3MasternodeShare = 50;
        consensus.stage3DevelopmentFundAddress = "TepVKkmUo1N6sazuM2wWwV7aiG4m1BUShU";
        consensus.stage3CommunityFundAddress = "TZpbhfvQE61USHsxd55XdPpWBqu3SXB1EP";

        consensus.nStartCollateralChange = 200;

        std::vector<FundRewardStructure> rewardStructures = { {200, 10}, {INT_MAX, 9} }; // 9% dev/community fee forever
        std::vector<FundRewardStructure> rewardStructuresDataMining = { {INT_MAX, 8} }; // 8% Data Mining fee forever

        consensus.nDevelopmentFundPayment = FundPayment(rewardStructures, 30, "TepVKkmUo1N6sazuM2wWwV7aiG4m1BUShU");
        consensus.nCommunityFundPayment = FundPayment(rewardStructures, 30,"TZpbhfvQE61USHsxd55XdPpWBqu3SXB1EP");
        consensus.nDataMiningFundPayment = FundPayment(rewardStructuresDataMining, 200,"TZpbhfvQE61USHsxd55XdPpWBqu3SXB1EP");
        consensus.nCollaterals = CMasternodeCollaterals(
          { {200, 1000 * COIN},
            {INT_MAX, 5000 * COIN}
          },
          { {200, 60}, {INT_MAX, 50} }
        );

        consensus.nStartBlacklist = 0;
        consensus.nStartDuplicationCheck = 0;
        consensus.nMajorityEnforceBlockUpgrade = 51;
        consensus.nMajorityRejectBlockOutdated = 75;
        consensus.nMajorityWindow = 100;
        consensus.nMinNFactor = 10;
        consensus.nMaxNFactor = 30;
        consensus.nChainStartTime = 1389306217;
        consensus.BIP34Height = 21111;
        consensus.BIP34Hash = uint256S("0x0000000023b3a96d3484e5abb3755c413e7d41500f8e2a5c3f0dd01299cd8ef8");
        consensus.powLimit = uint256S("00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 60 * 60; // 60 minutes between retargets
        consensus.nPowTargetSpacing = 5 * 60; // 5 minute blocks
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1456790400; // March 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1493596800; // May 1st, 2017

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1462060800; // May 1st 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1493596800; // May 1st 2017

        // Deployment of MTP
        consensus.vDeployments[Consensus::DEPLOYMENT_MTP].bit = 12;
        consensus.vDeployments[Consensus::DEPLOYMENT_MTP].nStartTime = 1539172800 - 2*60;
        consensus.vDeployments[Consensus::DEPLOYMENT_MTP].nTimeout = 1539172800 + consensus.nMinerConfirmationWindow*2 * 5*60;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000708f98bf623f02e");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("3825896ac39b8b27220e7bfaed81c5f979ca11dc874e564c5e70756ad06077b0 "); // 50000

        consensus.nSpendV15StartBlock = 1;
        consensus.nCheckBugFixedAtBlock = 1;
        consensus.nZnodePaymentsBugFixedAtBlock = 1;

        consensus.nSpendV2ID_1 = 1;
        consensus.nSpendV2ID_10 = 1;
        consensus.nSpendV2ID_25 = 1;
        consensus.nSpendV2ID_50 = 1;
        consensus.nSpendV2ID_100 = 1;
        consensus.nModulusV2StartBlock = 1;
        consensus.nModulusV1MempoolStopBlock = 1;
        consensus.nModulusV1StopBlock = 1;
        consensus.nMultipleSpendInputsInOneTxStartBlock = 1;
        consensus.nDontAllowDupTxsStartBlock = 1;

        // Znode params testnet
        consensus.nZnodePaymentsStartBlock = 120;
        nMaxTipAge = 0x7fffffff; // allow mining on top of old blocks for testnet

        // evo znodes
        consensus.DIP0003Height = 800;
        consensus.DIP0003EnforcementHeight = 820;
        consensus.DIP0003EnforcementHash.SetNull();

        consensus.DIP0008Height = 850;
        consensus.nEvoZnodeMinimumConfirmations = 0;

        // long living quorum params
        consensus.llmqs[Consensus::LLMQ_5_60] = llmq5_60;
        consensus.llmqs[Consensus::LLMQ_10_70] = llmq10_70;
        consensus.nLLMQPowTargetSpacing = 20;
        consensus.llmqChainLocks = Consensus::LLMQ_5_60;
        consensus.llmqForInstantSend = Consensus::LLMQ_5_60;
        consensus.nInstantSendConfirmationsRequired = 2;
        consensus.nInstantSendKeepLock = 6;
        consensus.nInstantSendBlockFilteringStartHeight = 1000;

        consensus.nMTPSwitchTime = 0;
        consensus.nMTPStartBlock = 1;
        consensus.nMTPFiveMinutesStartBlock = 0;
        consensus.nMTPStripDataTime = INT_MAX;

        consensus.nDifficultyAdjustStartBlock = 800;
        consensus.nFixedDifficulty = 0x2000ffff;
        consensus.nPowTargetSpacingMTP = 5*60;
        consensus.nInitialMTPDifficulty = 0x2000ffff;  // !!!! change it to the real value
        consensus.nMTPRewardReduction = 2;

        consensus.nDisableZerocoinStartBlock = 1;

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

        pchMessageStart[0] = 0xcf;
        pchMessageStart[1] = 0xfc;
        pchMessageStart[2] = 0xbe;
        pchMessageStart[3] = 0xeb;
        nDefaultPort = 38200;
        nPruneAfterHeight = 1000;

        std::vector<unsigned char> extraNonce(4);
        extraNonce[0] = 0x0a;
        extraNonce[1] = 0x00;
        extraNonce[2] = 0x00;
        extraNonce[3] = 0x00;

        genesis = CreateGenesisBlock(ZC_GENESIS_BLOCK_TIME, 459834, 0x1e0ffff0, 2, 0 * COIN, extraNonce);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock ==
                uint256S("0x1fcfe26873831662874b5358c4a28611be641fbb997e62d8bf9c80f799f5caff"));
        assert(genesis.hashMerkleRoot ==
                uint256S("0x3a0d54ae5549a8d75cd8d0cb73c6e3577ae6be8d5706fc9411bdebbe75c97210"));
        vFixedSeeds.clear();
        vSeeds.clear();
        // kiirocoin test seeds

        vSeeds.push_back(CDNSSeedData("DEVNET1", "devnet1.kiirocoin.org", false));
        vSeeds.push_back(CDNSSeedData("DEVNET2", "devnet2.kiirocoin.org", false));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector < unsigned char > (1, 66);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector < unsigned char > (1, 179);
        base58Prefixes[SECRET_KEY] = std::vector < unsigned char > (1, 186);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xD0).convert_to_container < std::vector < unsigned char > > ();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x95).convert_to_container < std::vector < unsigned char > > ();
        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_dev, pnSeed6_dev + ARRAYLEN(pnSeed6_dev));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fAllowMultiplePorts = true;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            (0, uint256S("0x"))
        };

        chainTxData = ChainTxData{
            1414776313,
            0,
            0.001
        };

        // Sigma related values.
        consensus.nSigmaStartBlock = 1;
        consensus.nSigmaPaddingBlock = 1;
        consensus.nDisableUnpaddedSigmaBlock = 1;
        consensus.nStartSigmaBlacklist = INT_MAX;
        consensus.nRestartSigmaWithBlacklistCheck = INT_MAX;
        consensus.nOldSigmaBanBlock = 1;

        consensus.nLelantusStartBlock = 1;
        consensus.nLelantusFixesStartBlock = 1;

        consensus.nMaxSigmaInputPerBlock = ZC_SIGMA_INPUT_LIMIT_PER_BLOCK;
        consensus.nMaxValueSigmaSpendPerBlock = ZC_SIGMA_VALUE_SPEND_LIMIT_PER_BLOCK;
        consensus.nMaxSigmaInputPerTransaction = ZC_SIGMA_INPUT_LIMIT_PER_TRANSACTION;
        consensus.nMaxValueSigmaSpendPerTransaction = ZC_SIGMA_VALUE_SPEND_LIMIT_PER_TRANSACTION;
        consensus.nMaxLelantusInputPerBlock = ZC_LELANTUS_INPUT_LIMIT_PER_BLOCK;
        consensus.nMaxValueLelantusSpendPerBlock = 1100 * COIN;
        consensus.nMaxLelantusInputPerTransaction = ZC_LELANTUS_INPUT_LIMIT_PER_TRANSACTION;
        consensus.nMaxValueLelantusSpendPerTransaction = 1001 * COIN;
        consensus.nMaxValueLelantusMint = 1001 * COIN;
        consensus.nZerocoinToSigmaRemintWindowSize = 0;

        consensus.evoSporkKeyID = "TdxR3tfoHiQUkowcfjEGiMBfk6GXFdajUA";
        consensus.nEvoSporkStartBlock = 1;
        consensus.nEvoSporkStopBlock = 40000;
        consensus.nEvoSporkStopBlockExtensionVersion = 0;

        // reorg
        consensus.nMaxReorgDepth = 4;
        consensus.nMaxReorgDepthEnforcementBlock = 25150;

        // whitelist

        // Dandelion related values.
        consensus.nDandelionEmbargoMinimum = DANDELION_TESTNET_EMBARGO_MINIMUM;
        consensus.nDandelionEmbargoAvgAdd = DANDELION_TESTNET_EMBARGO_AVG_ADD;
        consensus.nDandelionMaxDestinations = DANDELION_MAX_DESTINATIONS;
        consensus.nDandelionShuffleInterval = DANDELION_SHUFFLE_INTERVAL;
        consensus.nDandelionFluff = DANDELION_FLUFF;

        // Bip39
        consensus.nMnemonicBlock = 1;

        // moving lelantus data to v3 payload
        consensus.nLelantusV3PayloadStartBlock = 1;

        // ProgPow
        consensus.nPPSwitchTime = 1631261566;           // immediately after network start
        consensus.nPPBlockNumber = 1;
        consensus.nInitialPPDifficulty = 0x2000ffff;
    }
};

static CDevNetParams devNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";

        consensus.chainType = Consensus::chainRegtest;

        // To be changed for specific tests
        consensus.nSubsidyHalvingFirst = 1500;
        consensus.nSubsidyHalvingSecond = 2500;
        consensus.nSubsidyHalvingInterval = 1000;
        consensus.nSubsidyHalvingStopBlock = 10000;

        consensus.nStartBlacklist = 0;
        consensus.nStartDuplicationCheck = 0;
        consensus.stage2DevelopmentFundShare = 15;
        consensus.stage2ZnodeShare = 35;

        consensus.stage3StartTime = INT_MAX;
        consensus.stage3StartBlock = 0;
        consensus.stage3DevelopmentFundShare = 15;
        consensus.stage3CommunityFundShare = 10;
        consensus.stage3MasternodeShare = 50;
        consensus.stage3DevelopmentFundAddress = "TGEGf26GwyUBE2P2o2beBAfE9Y438dCp5t";  // private key cMrz8Df36VR9TvZjtvSqLPhUQR7pcpkXRXaLNYUxfkKsRuCzHpAN
        consensus.stage3CommunityFundAddress = "TJmPzeJF4DECrBwUftc265U7rTPxKmpa4F";  // private key cTyPWqTMM1CgT5qy3K3LSgC1H6Q2RHvnXZHvjWtKB4vq9qXqKmMu

        consensus.nStartCollateralChange = 200;

        std::vector<FundRewardStructure> rewardStructures = { {200, 10}, {INT_MAX, 9} }; // 9% dev/community fee forever
        std::vector<FundRewardStructure> rewardStructuresDataMining = { {INT_MAX, 8} }; // 8% Data Mining fee forever
        consensus.nDevelopmentFundPayment = FundPayment(rewardStructures, 1, "TGEGf26GwyUBE2P2o2beBAfE9Y438dCp5t");
        consensus.nCommunityFundPayment = FundPayment(rewardStructures, 1,"TJmPzeJF4DECrBwUftc265U7rTPxKmpa4F");
        consensus.nDataMiningFundPayment = FundPayment(rewardStructuresDataMining, 200,"TZpbhfvQE61USHsxd55XdPpWBqu3SXB1EP");
        consensus.nCollaterals = CMasternodeCollaterals(
          { {200, 1000 * COIN},
            {INT_MAX, 5000 * COIN}
          },
          { {200, 60}, {INT_MAX, 50} }
        );  

        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.BIP34Height = -1; // BIP34 has not necessarily activated on regtest
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in rpc activation tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in rpc activation tests)
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 60 * 60 * 1000; // 60 minutes between retargets
        consensus.nPowTargetSpacing = 1; // 10 minute blocks
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nZnodePaymentsStartBlock = 120;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = INT_MAX;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_MTP].bit = 12;
        consensus.vDeployments[Consensus::DEPLOYMENT_MTP].nStartTime = INT_MAX;
        consensus.vDeployments[Consensus::DEPLOYMENT_MTP].nTimeout = 999999999999ULL;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");
        // Znode code
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes
        nMaxTipAge = 6 * 60 * 60; // ~144 blocks behind -> 2 x fork detection time, was 24 * 60 * 60 in bitcoin

        consensus.nCheckBugFixedAtBlock = 120;
        consensus.nZnodePaymentsBugFixedAtBlock = 1;
        consensus.nSpendV15StartBlock = 1;
        consensus.nSpendV2ID_1 = 2;
        consensus.nSpendV2ID_10 = 3;
        consensus.nSpendV2ID_25 = 3;
        consensus.nSpendV2ID_50 = 3;
        consensus.nSpendV2ID_100 = 3;
        consensus.nModulusV2StartBlock = 130;
        consensus.nModulusV1MempoolStopBlock = 135;
        consensus.nModulusV1StopBlock = 140;
        consensus.nMultipleSpendInputsInOneTxStartBlock = 1;
        consensus.nDontAllowDupTxsStartBlock = 1;

        // evo znodes
        consensus.DIP0003Height = 500;
        consensus.DIP0003EnforcementHeight = 550;
        consensus.DIP0003EnforcementHash.SetNull();

        consensus.DIP0008Height = 550;
        consensus.nEvoZnodeMinimumConfirmations = 1;

        // long living quorum params
        consensus.llmqs[Consensus::LLMQ_5_60] = llmq5_60;
        consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
        consensus.llmqs[Consensus::LLMQ_400_60] = llmq400_60;
        consensus.llmqs[Consensus::LLMQ_400_85] = llmq400_85;
        consensus.nLLMQPowTargetSpacing = 1;
        consensus.llmqChainLocks = Consensus::LLMQ_5_60;
        consensus.llmqForInstantSend = Consensus::LLMQ_5_60;
        consensus.nInstantSendConfirmationsRequired = 2;
        consensus.nInstantSendKeepLock = 6;
        consensus.nInstantSendBlockFilteringStartHeight = 800;

        consensus.nMTPSwitchTime = INT_MAX;
        consensus.nMTPStartBlock = 0;
        consensus.nMTPFiveMinutesStartBlock = 0;
        consensus.nMTPStripDataTime = INT_MAX;

        consensus.nDifficultyAdjustStartBlock = 5000;
        consensus.nFixedDifficulty = 0x207fffff;
        consensus.nPowTargetSpacingMTP = 5*60;
        consensus.nInitialMTPDifficulty = 0x2070ffff;  // !!!! change it to the real value
        consensus.nMTPRewardReduction = 2;

        consensus.nDisableZerocoinStartBlock = INT_MAX;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18444;
        nPruneAfterHeight = 1000;

        /**
          * btzc: testnet params
          * nTime: 1414776313
          * nNonce: 1620571
          */
		  
		  std::vector<unsigned char> extraNonce(4);
        extraNonce[0] = 0x08;
        extraNonce[1] = 0x00;
        extraNonce[2] = 0x00;
        extraNonce[3] = 0x00;
        
        genesis = CreateGenesisBlock(ZC_GENESIS_BLOCK_TIME, 414098459, 0x207fffff, 1, 0 * COIN, extraNonce);
        consensus.hashGenesisBlock = genesis.GetHash();

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fAllowMultiplePorts = true;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            (0, uint256S("0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"))
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };
        base58Prefixes[PUBKEY_ADDRESS] = std::vector < unsigned char > (1, 65);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector < unsigned char > (1, 178);
        base58Prefixes[SECRET_KEY] = std::vector < unsigned char > (1, 239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container < std::vector < unsigned char > > ();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container < std::vector < unsigned char > > ();

        nSpendV15StartBlock = ZC_V1_5_TESTNET_STARTING_BLOCK;
        nSpendV2ID_1 = ZC_V2_TESTNET_SWITCH_ID_1;
        nSpendV2ID_10 = ZC_V2_TESTNET_SWITCH_ID_10;
        nSpendV2ID_25 = ZC_V2_TESTNET_SWITCH_ID_25;
        nSpendV2ID_50 = ZC_V2_TESTNET_SWITCH_ID_50;
        nSpendV2ID_100 = ZC_V2_TESTNET_SWITCH_ID_100;
        nModulusV2StartBlock = ZC_MODULUS_V2_TESTNET_START_BLOCK;
        nModulusV1MempoolStopBlock = ZC_MODULUS_V1_TESTNET_MEMPOOL_STOP_BLOCK;
        nModulusV1StopBlock = ZC_MODULUS_V1_TESTNET_STOP_BLOCK;

        // Sigma related values.
        consensus.nSigmaStartBlock = 100;
        consensus.nSigmaPaddingBlock = 1;
        consensus.nDisableUnpaddedSigmaBlock = 1;
        consensus.nStartSigmaBlacklist = INT_MAX;
        consensus.nRestartSigmaWithBlacklistCheck = INT_MAX;
        consensus.nOldSigmaBanBlock = 1;
        consensus.nLelantusStartBlock = 400;
        consensus.nLelantusFixesStartBlock = 400;
        consensus.nZerocoinV2MintMempoolGracefulPeriod = 1;
        consensus.nZerocoinV2MintGracefulPeriod = 1;
        consensus.nZerocoinV2SpendMempoolGracefulPeriod = 1;
        consensus.nZerocoinV2SpendGracefulPeriod = 1;
        consensus.nMaxSigmaInputPerBlock = ZC_SIGMA_INPUT_LIMIT_PER_BLOCK;
        consensus.nMaxValueSigmaSpendPerBlock = ZC_SIGMA_VALUE_SPEND_LIMIT_PER_BLOCK;
        consensus.nMaxSigmaInputPerTransaction = ZC_SIGMA_INPUT_LIMIT_PER_TRANSACTION;
        consensus.nMaxValueSigmaSpendPerTransaction = ZC_SIGMA_VALUE_SPEND_LIMIT_PER_TRANSACTION;
        consensus.nMaxLelantusInputPerBlock = ZC_LELANTUS_INPUT_LIMIT_PER_BLOCK;
        consensus.nMaxValueLelantusSpendPerBlock = ZC_LELANTUS_VALUE_SPEND_LIMIT_PER_BLOCK;
        consensus.nMaxLelantusInputPerTransaction = ZC_LELANTUS_INPUT_LIMIT_PER_TRANSACTION;
        consensus.nMaxValueLelantusSpendPerTransaction = ZC_LELANTUS_VALUE_SPEND_LIMIT_PER_TRANSACTION;
        consensus.nMaxValueLelantusMint = ZC_LELANTUS_MAX_MINT;
        consensus.nZerocoinToSigmaRemintWindowSize = 1000;

        // evo spork
        consensus.evoSporkKeyID = "TSpmHGzQT4KJrubWa4N2CRmpA7wKMMWDg4";  // private key is cW2YM2xaeCaebfpKguBahUAgEzLXgSserWRuD29kSyKHq1TTgwRQ
        consensus.nEvoSporkStartBlock = 1000;
        consensus.nEvoSporkStopBlock = 1500;
        consensus.nEvoSporkStopBlockExtensionVersion = 0;

        // reorg
        consensus.nMaxReorgDepth = 4;
        consensus.nMaxReorgDepthEnforcementBlock = 300;

        // Dandelion related values.
        consensus.nDandelionEmbargoMinimum = 0;
        consensus.nDandelionEmbargoAvgAdd = 1;
        consensus.nDandelionMaxDestinations = DANDELION_MAX_DESTINATIONS;
        consensus.nDandelionShuffleInterval = DANDELION_SHUFFLE_INTERVAL;
        consensus.nDandelionFluff = DANDELION_FLUFF;

        // Bip39
        consensus.nMnemonicBlock = 0;

        // moving lelantus data to v3 payload
        consensus.nLelantusV3PayloadStartBlock = 1000;
        
        // ProgPow
        // this can be overridden with either -ppswitchtime or -ppswitchtimefromnow flags
        consensus.nPPSwitchTime = INT_MAX;
        consensus.nPPBlockNumber = INT_MAX;
        consensus.nInitialPPDifficulty = 0x2000ffff;
    }

    void UpdateBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
    }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams& Params(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
            return mainParams;
    else if (chain == CBaseChainParams::TESTNET)
            return testNetParams;
    else if (chain == CBaseChainParams::DEVNET)
            return devNetParams;
    else if (chain == CBaseChainParams::REGTEST)
            return regTestParams;
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

void UpdateRegtestBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    regTestParams.UpdateBIP9Parameters(d, nStartTime, nTimeout);
}

