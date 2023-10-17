/*
 * Copyright (c) 2023 The Kiiro Core developers
 * Copyright (c) 2018 The Pigeon Core developers
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 * 
 * CommunityFund_Payment.cpp
 *
 *  Created on: Jun 24, 2018
 *      Author: Tri Nguyen
 */

#include "fund_payment.h"

#include "util.h"
#include "chainparams.h"
#include <boost/foreach.hpp>
#include "base58.h"

CAmount FundPayment::getFundPaymentAmount(int blockHeight, CAmount blockReward) {
	 if (blockHeight < startBlock){
		 return 0;
	 }
	 for(int i = 0; i < rewardStructures.size(); i++) {
		 FundRewardStructure rewardStructure = rewardStructures[i];
		 if(rewardStructure.blockHeight == INT_MAX || blockHeight <= rewardStructure.blockHeight) {
			 return blockReward * rewardStructure.rewardPercentage / 100;
		 }
	 }
	 return 0;
}

void FundPayment::FillFundPayment(CMutableTransaction& txNew, int nBlockHeight, CAmount blockReward, CTxOut& txoutFundRet) {
    // make sure it's not filled yet
	CAmount fundPayment = getFundPaymentAmount(nBlockHeight, blockReward);
	txoutFundRet = CTxOut();
    CScript payee;
    // fill payee with the found FundRewardStrcuture
    CBitcoinAddress cbAddress(fundAddress);
	payee = GetScriptForDestination(cbAddress.Get());

    // split reward between miner ...
    txNew.vout[0].nValue -= fundPayment;
    txoutFundRet = CTxOut(fundPayment, payee);
    txNew.vout.push_back(txoutFundRet);
    LogPrintf("FundPayment::FillFundPayment -- Fund payment %lld to %s\n", fundPayment, fundAddress.c_str());
}

bool FundPayment::IsBlockPayeeValid(const CTransaction& txNew, const int height, const CAmount blockReward) {
	CScript payee;
	// fill payee with the development fund address
	payee = GetScriptForDestination(CBitcoinAddress(fundAddress).Get());
	const CAmount fundReward = getFundPaymentAmount(height, blockReward);

	BOOST_FOREACH(const CTxOut& out, txNew.vout) {
		if(out.scriptPubKey == payee && out.nValue >= fundReward) {
			return true;
		}
	}

	return false;
}



