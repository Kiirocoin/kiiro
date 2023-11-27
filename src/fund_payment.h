/*
 * Copyright (c) 2023 The Kiiro Core developers
 * Copyright (c) 2018 The Pigeon Core developers
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 * 
 * FundPayment.h
 *
 *  Created on: Jun 24, 2018
 *      Author: Tri Nguyen
 */

#ifndef SRC_FUND_PAYMENT_H_
#define SRC_FUND_PAYMENT_H_
#include <string>
#include "amount.h"
#include "primitives/transaction.h"
#include "script/standard.h"
#include <limits.h>

struct FundRewardStructure {
	int blockHeight;
	int rewardPercentage;
};

class FundPayment {
public:
	FundPayment(std::vector<FundRewardStructure> rewardStructures = {}, int startBlock = 0, const std::string &address = "") {
		this->fundAddress = address;
		this->startBlock = startBlock;
		this->rewardStructures = rewardStructures;
	}
	~FundPayment(){};
	CAmount getFundPaymentAmount(int blockHeight, CAmount blockReward);
	void FillFundPayment(CMutableTransaction& txNew, int nBlockHeight, CAmount blockReward, CTxOut& txoutFundRet);
	bool IsBlockPayeeValid(const CTransaction& txNew, const int height, const CAmount blockReward);
	int getStartBlock() {return this->startBlock;}
private:
	std::string fundAddress;
	int startBlock;
	std::vector<FundRewardStructure> rewardStructures;
};



#endif /* SRC_COMMUNITY_FUND_PAYMENT_H_ */
