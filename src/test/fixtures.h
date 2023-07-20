#include "hdmint/hdmint.h"
#include "primitives/transaction.h"
#include "test/test_bitcoin.h"
#include "test/testutil.h"
#include "consensus/params.h"
#include "liblelantus/coin.h"

#include <boost/test/unit_test.hpp>

struct TestDerivation {
    std::string pub;
    std::string prv;
    unsigned int nChild;
};

struct TestVector {
    std::string strHexMaster;
    std::vector<TestDerivation> vDerive;

    TestVector(std::string strHexMasterIn) : strHexMaster(strHexMasterIn) {}

    TestVector& operator()(std::string pub, std::string prv, unsigned int nChild) {
        vDerive.push_back(TestDerivation());
        TestDerivation &der = vDerive.back();
        der.pub = pub;
        der.prv = prv;
        der.nChild = nChild;
        return *this;
    }
};

inline bool no_check( std::runtime_error const& ex ) { return true; }

struct ZerocoinTestingSetupBase : public TestingSetup {
    ZerocoinTestingSetupBase();
    ~ZerocoinTestingSetupBase();

    CScript scriptPubKey;
    CPubKey pubkey;

    CBlock CreateBlock(const CScript&);

    bool ProcessBlock(const CBlock&);

    // Create a new block with just given transactions, coinbase paying to
    // scriptPubKey, and try to add it to the current chain.
    CBlock CreateAndProcessBlock(const CScript&);

    void CreateAndProcessEmptyBlocks(size_t block_numbers, const CScript& script);

    std::vector<CTransaction> coinbaseTxns; // For convenience, coinbase transactionsl
    CKey coinbaseKey; // private/public key needed to spend coinbase transactions
};

struct ZerocoinTestingSetup200 : public ZerocoinTestingSetupBase {
        ZerocoinTestingSetup200();
};

struct ZerocoinTestingSetup109 : public ZerocoinTestingSetupBase {
        ZerocoinTestingSetup109();
};

struct MtpMalformedTestingSetup : public ZerocoinTestingSetupBase {
        MtpMalformedTestingSetup();

    CBlock CreateBlock(
            const CScript&, bool);

    CBlock CreateAndProcessBlock(
        const CScript&, bool);
};

struct LelantusTestingSetup : public TestChain100Setup {
public:
    LelantusTestingSetup();

public:
    CBlockIndex* GenerateBlock(std::vector<CMutableTransaction> const &txns = {}, CScript *script = nullptr);
    void GenerateBlocks(size_t blocks, CScript *script = nullptr);

    std::vector<lelantus::PrivateCoin> GenerateMints(
        std::vector<CAmount> const &amounts);

    std::vector<CHDMint> GenerateMints(
        std::vector<CAmount> const &amounts,
        std::vector<CMutableTransaction> &txs);

    std::vector<CHDMint> GenerateMints(
        std::vector<CAmount> const &amounts,
        std::vector<CMutableTransaction> &txs,
        std::vector<lelantus::PrivateCoin> &coins);

    CPubKey GenerateAddress();

    ~LelantusTestingSetup();

public:
    lelantus::Params const *params;
    CScript script;
};

// for the duration of the test set network type to testnet
class FakeTestnet {
    Consensus::Params &params;
    Consensus::Params oldParams;
public:
    FakeTestnet() : params(const_cast<Consensus::Params &>(Params().GetConsensus())) {
        oldParams = params;
        params.chainType = Consensus::chainTestnet;
    }

    ~FakeTestnet() {
        params = oldParams;
    }
};
