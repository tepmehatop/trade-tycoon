/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file deposit.h Natural resource deposits owned by companies (Capitalism mechanic 12).
 *
 * A primary industry producing a natural resource that a company funded becomes a deposit:
 * it has a finite reserve and a quality, and extracts a fixed amount per day while a station serves it.
 * Industries nobody funded keep their normal production. See catalog/mechanics/12-natural-resources.md.
 */

#ifndef TRADETYCOON_DEPOSIT_H
#define TRADETYCOON_DEPOSIT_H

#include "../cargo_type.h"
#include "../economy_type.h"
#include "../industry_type.h"

struct Industry;

namespace TradeTycoon {

/** State of a deposit. */
struct Deposit {
	uint32_t initial = 0; ///< Reserve when the deposit was founded, in cargo units.
	uint32_t reserve = 0; ///< Remaining reserve, in cargo units.
	uint32_t fraction = 0; ///< Extracted cargo not yet added as a whole unit, in 1/1000 cargo units.
	uint8_t quality = 0; ///< Quality of the extracted cargo, 1..100.
	uint8_t level = 1; ///< Level of the extraction unit, 1..9.
};

/** All deposits, by industry. */
using DepositMap = std::map<IndustryID, Deposit>;

DepositMap &GetDeposits();
const Deposit *GetDeposit(IndustryID index);
bool IsManagedDeposit(const Industry *i);
void OnIndustryFounded(Industry *i);
void OnIndustryDeleted(IndustryID index);
void OnCargoNotShipped(Industry *i, CargoType cargo, uint amount);
Money GetDepositPurchaseCost(IndustryType type);
std::string GetDepositInfoString(const Industry *i);

} // namespace TradeTycoon

#endif /* TRADETYCOON_DEPOSIT_H */
