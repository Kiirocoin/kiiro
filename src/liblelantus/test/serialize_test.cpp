#include "../lelantus_prover.h"
#include "../lelantus_verifier.h"

#include "streams.h"

#include "lelantus_test_fixture.h"

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(lelantus_serialize_tests, lelantus::LelantusTestingSetup)

BOOST_AUTO_TEST_CASE(serialize)
{
    auto params = lelantus::Params::get_default();
    std::map<uint32_t, std::vector<lelantus::PublicCoin>> anonymity_sets;
    int N = 100;

    std::vector<std::pair<lelantus::PrivateCoin, uint32_t>> Cin;
    lelantus::PrivateCoin input_coin1(params, 5);
    Cin.emplace_back(std::make_pair(input_coin1, 0));
    std::vector <size_t> indexes;
    indexes.push_back(0);

    std::vector <lelantus::PublicCoin> anonymity_set;
    anonymity_set.reserve(N);
    anonymity_set.push_back(Cin[0].first.getPublicCoin());
    for(int i = 0; i < N; ++i){
          secp_primitives::GroupElement coin;
          coin.randomize();
           anonymity_set.emplace_back(lelantus::PublicCoin(coin));
     }

    anonymity_sets[0] = anonymity_set;

    secp_primitives::Scalar Vin(uint64_t(5));
    secp_primitives::Scalar Vout(uint64_t(6));
    std::vector <lelantus::PrivateCoin> Cout;
    Cout.push_back(lelantus::PrivateCoin(params, 2));
    Cout.push_back(lelantus::PrivateCoin(params, 1));
    secp_primitives::Scalar f(uint64_t(1));

    lelantus::LelantusProof initial_proof;
    lelantus::SchnorrProof qkSchnorrProof;

    lelantus::LelantusProver prover(params, LELANTUS_TX_VERSION_4_5);
    prover.proof(anonymity_sets, {}, Vin, Cin, indexes, {}, Vout, Cout, f,  initial_proof, qkSchnorrProof);

    CDataStream serialized(SER_NETWORK, PROTOCOL_VERSION);
    serialized << initial_proof;

    lelantus::LelantusProof resulted_proof;
    serialized >> resulted_proof;

    for(size_t i = 0; i <  initial_proof.sigma_proofs.size(); ++i){
        BOOST_CHECK(initial_proof.sigma_proofs[i].B_ == resulted_proof.sigma_proofs[i].B_);
        BOOST_CHECK(initial_proof.sigma_proofs[i].A_ == resulted_proof.sigma_proofs[i].A_);
        BOOST_CHECK(initial_proof.sigma_proofs[i].C_ == resulted_proof.sigma_proofs[i].C_);
        BOOST_CHECK(initial_proof.sigma_proofs[i].D_ == resulted_proof.sigma_proofs[i].D_);
        BOOST_CHECK(initial_proof.sigma_proofs[i].f_ == resulted_proof.sigma_proofs[i].f_);
        BOOST_CHECK(initial_proof.sigma_proofs[i].ZA_ == resulted_proof.sigma_proofs[i].ZA_);
        BOOST_CHECK(initial_proof.sigma_proofs[i].ZC_ == resulted_proof.sigma_proofs[i].ZC_);
        BOOST_CHECK(initial_proof.sigma_proofs[i].Gk_ == resulted_proof.sigma_proofs[i].Gk_);
        BOOST_CHECK(initial_proof.sigma_proofs[i].zV_ == resulted_proof.sigma_proofs[i].zV_);
        BOOST_CHECK(initial_proof.sigma_proofs[i].zR_ == resulted_proof.sigma_proofs[i].zR_);
    }

    BOOST_CHECK(initial_proof.bulletproofs.A == resulted_proof.bulletproofs.A);
    BOOST_CHECK(initial_proof.bulletproofs.S == resulted_proof.bulletproofs.S);
    BOOST_CHECK(initial_proof.bulletproofs.T1 == resulted_proof.bulletproofs.T1);
    BOOST_CHECK(initial_proof.bulletproofs.T2 == resulted_proof.bulletproofs.T2);
    BOOST_CHECK(initial_proof.bulletproofs.T_x1 == resulted_proof.bulletproofs.T_x1);
    BOOST_CHECK(initial_proof.bulletproofs.T_x2 == resulted_proof.bulletproofs.T_x2);
    BOOST_CHECK(initial_proof.bulletproofs.u == resulted_proof.bulletproofs.u);
    BOOST_CHECK(initial_proof.bulletproofs.innerProductProof.a_ == resulted_proof.bulletproofs.innerProductProof.a_);
    BOOST_CHECK(initial_proof.bulletproofs.innerProductProof.b_ == resulted_proof.bulletproofs.innerProductProof.b_);
    BOOST_CHECK(initial_proof.bulletproofs.innerProductProof.c_ == resulted_proof.bulletproofs.innerProductProof.c_);
    BOOST_CHECK(initial_proof.bulletproofs.innerProductProof.L_ == resulted_proof.bulletproofs.innerProductProof.L_);
    BOOST_CHECK(initial_proof.bulletproofs.innerProductProof.R_ == resulted_proof.bulletproofs.innerProductProof.R_);

        BOOST_CHECK(initial_proof.schnorrProof.u == resulted_proof.schnorrProof.u);
        BOOST_CHECK(initial_proof.schnorrProof.P1 == resulted_proof.schnorrProof.P1);
        BOOST_CHECK(initial_proof.schnorrProof.T1 == resulted_proof.schnorrProof.T1);
}

BOOST_AUTO_TEST_SUITE_END()
