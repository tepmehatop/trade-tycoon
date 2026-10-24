/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file economy_data.h Trade Tycoon economy parameters, loaded from tradetycoon_economy.json next to the products NewGRF. */

#ifndef TRADETYCOON_ECONOMY_DATA_H
#define TRADETYCOON_ECONOMY_DATA_H

#include "../cargo_type.h"

namespace TradeTycoon {

/** Number of unit levels. */
static constexpr uint8_t NUM_UNIT_LEVELS = 9;

/** Parameters of a natural resource (Capitalism Lab NATURAL_RESOURCES). */
struct ResourceSpec {
	CargoLabel label; ///< Cargo label of the extracted cargo.
	uint8_t speed = 0; ///< Extraction speed, cells per day at unit level 1.
	uint16_t res_qty = 0; ///< Reserve index; a deposit holds about res_qty * 100 cells.
	uint8_t res_cost = 0; ///< Price of the reserve, in percent of the product value.
	uint8_t quality_lo = 0; ///< Lowest quality of a deposit.
	uint8_t quality_hi = 0; ///< Highest quality of a deposit.
	double units_per_cell = 0; ///< Cargo units in one production cell.
};

/** All economy parameters. */
struct EconomyData {
	double cell_value = 0; ///< Value of one production cell, in Capitalism money.
	double price_scale = 0; ///< Conversion of Capitalism money to OpenTTD money.
	std::array<uint16_t, NUM_UNIT_LEVELS> level_productivity{}; ///< Productivity per unit level, in percent.
	std::array<uint16_t, NUM_UNIT_LEVELS> level_capacity{}; ///< Stock capacity per unit level, in cells.
	std::vector<ResourceSpec> resources; ///< Natural resources.

	const ResourceSpec *FindResource(CargoLabel label) const;
};

const EconomyData &GetEconomyData();

} // namespace TradeTycoon

#endif /* TRADETYCOON_ECONOMY_DATA_H */
