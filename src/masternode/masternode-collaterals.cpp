/*
 * Copyright (c) 2023 The Kiirocoin developers
 * Distributed under the MIT/X11 software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 *      Author: tri
 */

#include "masternode-collaterals.h"
#include <limits.h>

CMasternodeCollaterals::CMasternodeCollaterals(std::vector<Collateral> collaterals, std::vector<RewardPercentage> rewardPercentages) {
	this->collaterals = collaterals;
	this->rewardPercentages = rewardPercentages;
	for (auto& it : this->collaterals) {
		collateralsHeightMap.insert(std::make_pair(it.amount, it.height));
	}

}

CAmount CMasternodeCollaterals::getCollateral(int height) const {
	for (auto& it : this->collaterals) {
		if(it.height == INT_MAX || height < it.height) {
			return it.amount;
		}
	}
	return 0;
}

int CMasternodeCollaterals::getRewardPercentage(int height) const {
	for (auto& it : this->rewardPercentages) {
		if(it.height == INT_MAX || height < it.height) {
			return it.percentage;
		}
	}
	return 0;
}

CMasternodeCollaterals::~CMasternodeCollaterals() {
	this->collaterals.clear();
}

bool CMasternodeCollaterals::isValidCollateral(CAmount collateralAmount) const {
	auto it = collateralsHeightMap.find(collateralAmount);
	return it != collateralsHeightMap.end();
}

bool CMasternodeCollaterals::isPayableCollateral(int height, CAmount collateralAmount) const {
	if(!this->isValidCollateral(collateralAmount)) {
		return false;
	}
	int collateralEndHeight = this->collateralsHeightMap.at(collateralAmount);
	return collateralEndHeight == INT_MAX || (height + 1) < collateralEndHeight;
}
