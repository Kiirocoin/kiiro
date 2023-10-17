/*
 * Copyright (c) 2023 The Kiiro developers
 * Distributed under the MIT/X11 software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 *
 *      Author: tri
 */

#include "amount.h"
#include <vector>
#include <unordered_map>

#ifndef KIIRO_MASTERNODE_COLLATERALS_H_
#define KIIRO_MASTERNODE_COLLATERALS_H_
class CMasternodeCollaterals;
struct Collateral {
	int height;
	CAmount amount;
};

struct RewardPercentage {
	int height;
	int percentage;
};

class CMasternodeCollaterals {
protected:
	std::vector<Collateral> collaterals;
	std::vector<RewardPercentage> rewardPercentages;
	std::unordered_map<CAmount, int> collateralsHeightMap;

public:
	CMasternodeCollaterals(std::vector<Collateral> collaterals = {}, std::vector<RewardPercentage> rewardPercentages = {});
	CAmount getCollateral(int height) const;
	bool isValidCollateral(CAmount collateralAmount) const;
	bool isPayableCollateral(int height,CAmount collateralAmount) const;
	int getRewardPercentage(int height) const;
	virtual ~CMasternodeCollaterals();
};

#endif /* KIIRO_MASTERNODE_COLLATERALS_H_ */
