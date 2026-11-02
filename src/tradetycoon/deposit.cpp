/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file deposit.cpp Natural resource deposits owned by companies (Capitalism mechanic 12). */

#include "../stdafx.h"
#include "../cargotype.h"
#include "../company_base.h"
#include "../core/random_func.hpp"
#include "../industry.h"
#include "../news_func.h"
#include "../station_base.h"
#include "../strings_func.h"
#include "../window_func.h"
#include "../timer/timer.h"
#include "../timer/timer_game_economy.h"
#include "economy_data.h"
#include "deposit.h"

#include "table/strings.h"

#include "../safeguards.h"

namespace TradeTycoon {

static DepositMap _deposits; ///< All deposits, saved in the TTDP chunk.

/**
 * Get all deposits, e.g. for saving.
 * @return The deposits.
 */
DepositMap &GetDeposits()
{
	return _deposits;
}

/**
 * Get the deposit of an industry.
 * @param index Industry.
 * @return The deposit, or nullptr if the industry is no deposit.
 */
const Deposit *GetDeposit(IndustryID index)
{
	auto it = _deposits.find(index);
	return it == _deposits.end() ? nullptr : &it->second;
}

/**
 * Find the natural resource an industry type extracts.
 * @param type Industry type.
 * @return The resource and the cargo type, or nullptr if the type extracts no natural resource.
 */
static std::pair<const ResourceSpec *, CargoType> FindResource(IndustryType type)
{
	const IndustrySpec *indspec = GetIndustrySpec(type);
	if (!indspec->IsRawIndustry()) return {nullptr, INVALID_CARGO};

	for (CargoType cargo : indspec->produced_cargo) {
		if (!IsValidCargoType(cargo)) continue;
		const ResourceSpec *r = GetEconomyData().FindResource(CargoSpec::Get(cargo)->label);
		if (r != nullptr) return {r, cargo};
	}
	return {nullptr, INVALID_CARGO};
}

/**
 * Whether the production of an industry is managed as a deposit instead of by the industry (set).
 * @param i Industry.
 * @return True for deposits of a company.
 */
bool IsManagedDeposit(const Industry *i)
{
	return Company::IsValidID(i->founder) && _deposits.contains(i->index);
}

/**
 * Turn a newly built industry into a deposit if a company funded it and it extracts a natural resource.
 * Reserve: res_qty * 100 cells, +-20 %; quality: quality_lo .. quality_hi (CapMain.exe rawres.cpp 006295c0).
 * @param i The new industry.
 */
void OnIndustryFounded(Industry *i)
{
	if (!Company::IsValidID(i->founder)) return;

	auto [resource, cargo] = FindResource(i->type);
	if (resource == nullptr) return;

	Deposit d;
	double cells = resource->res_qty * (80 + RandomRange(40)) / 100.0 * 100.0;
	d.initial = d.reserve = static_cast<uint32_t>(cells * resource->units_per_cell);
	d.quality = resource->quality_lo + RandomRange(resource->quality_hi - resource->quality_lo + 1);
	_deposits[i->index] = d;
}

/**
 * Forget the deposit of a removed industry.
 * @param index Industry.
 */
void OnIndustryDeleted(IndustryID index)
{
	_deposits.erase(index);
}

/**
 * Return extracted cargo that no station took back to the reserve, like unsold stock of a Capitalism mine.
 * @param i Industry.
 * @param cargo Extracted cargo.
 * @param amount Cargo units that were not moved to a station.
 */
void OnCargoNotShipped(Industry *i, CargoType cargo, uint amount)
{
	if (amount == 0) return;
	auto it = _deposits.find(i->index);
	if (it == _deposits.end()) return;
	Deposit &d = it->second;
	d.reserve = static_cast<uint32_t>(std::min<uint64_t>(d.initial, static_cast<uint64_t>(d.reserve) + amount));

	/* Cargo back in the ground was not produced. */
	auto p = i->GetCargoProduced(cargo);
	if (p == std::end(i->produced)) return;
	auto &production = p->history[THIS_MONTH].production;
	production -= std::min<uint16_t>(production, amount);
}

/**
 * Price a company pays for the reserve when it funds an industry of a type, based on the average deposit.
 * cost = value of the reserve * RES_COST % * quality % (CapMain.exe rawres.cpp 0062b160, without the scarcity premium).
 * @param type Industry type.
 * @return The cost, or zero if the type extracts no natural resource.
 */
Money GetDepositPurchaseCost(IndustryType type)
{
	auto [resource, cargo] = FindResource(type);
	if (resource == nullptr) return 0;

	const EconomyData &data = GetEconomyData();
	double average_quality = (resource->quality_lo + resource->quality_hi) / 2.0;
	double reserve_value = resource->res_qty * 100.0 * data.cell_value;
	return static_cast<Money>(reserve_value * resource->res_cost / 100.0 * average_quality / 100.0 * data.price_scale);
}

/**
 * Text for the industry window: reserve and quality of a deposit.
 * @param i Industry.
 * @return The text, or empty if the industry is no deposit.
 */
std::string GetDepositInfoString(const Industry *i)
{
	const Deposit *d = GetDeposit(i->index);
	if (d == nullptr) return {};

	auto [resource, cargo] = FindResource(i->type);
	if (resource == nullptr) return {};

	uint percent = d->initial == 0 ? 0 : static_cast<uint>(static_cast<uint64_t>(d->reserve) * 100 / d->initial);
	return GetString(STR_TT_DEPOSIT_INFO, i->founder, cargo, d->reserve, percent, d->quality, d->level);
}

/**
 * Whether a station near the industry picks up the cargo, i.e. a vehicle has tried to load it there.
 * Without such a station a deposit is idle, like a Capitalism mine that has nobody to sell to.
 * @param i Industry.
 * @param cargo Extracted cargo.
 * @return True if the cargo is picked up.
 */
static bool IsCargoPickedUp(const Industry *i, CargoType cargo)
{
	return std::ranges::any_of(i->stations_near, [cargo](const Station *st) { return st->goods[cargo].HasVehicleEverTriedLoading(); });
}

/**
 * Close a deposit that has run out of reserves.
 * @param i Industry.
 */
static void ExhaustDeposit(Industry *i)
{
	i->prod_level = PRODLEVEL_CLOSURE;
	AddTileNewsItem(GetEncodedString(STR_TT_NEWS_DEPOSIT_EXHAUSTED, i->index), NewsType::IndustryClose, i->location.tile + TileDiffXY(1, 1));
	_deposits.erase(i->index);
}

/**
 * Daily extraction (CapMain.exe j_raw.cpp 005cbb00):
 * cells per day = speed * productivity(level) / 100, while the stock is below capacity(level) cells.
 * A deposit whose cargo nobody picks up is idle, so its reserve is not lost.
 */
static const IntervalTimer<TimerGameEconomy> _economy_deposits_daily({TimerGameEconomy::Trigger::Day, TimerGameEconomy::Priority::None}, [](auto)
{
	const EconomyData &data = GetEconomyData();

	for (auto it = _deposits.begin(); it != _deposits.end(); /* advanced inside */) {
		IndustryID index = it->first;
		Deposit &d = it->second;
		++it; // ExhaustDeposit may erase the current element.

		Industry *i = Industry::GetIfValid(index);
		if (i == nullptr || !Company::IsValidID(i->founder)) continue;

		auto [resource, cargo] = FindResource(i->type);
		if (resource == nullptr || !IsCargoPickedUp(i, cargo)) continue;
		auto p = std::ranges::find(i->produced, cargo, &Industry::ProducedCargo::cargo);
		if (p == i->produced.end()) continue;

		uint8_t level = Clamp<uint8_t>(d.level, 1, NUM_UNIT_LEVELS);
		uint32_t capacity = static_cast<uint32_t>(data.level_capacity[level - 1] * resource->units_per_cell);
		if (p->waiting >= capacity) continue;

		uint64_t milli = static_cast<uint64_t>(resource->speed * data.level_productivity[level - 1] * resource->units_per_cell * 10.0) + d.fraction;
		uint32_t amount = static_cast<uint32_t>(milli / 1000);
		d.fraction = static_cast<uint32_t>(milli % 1000);
		amount = std::min({amount, d.reserve, capacity - p->waiting});

		p->waiting += amount;
		d.reserve -= amount;
		if (d.reserve == 0) ExhaustDeposit(i);
		SetWindowDirty(WindowClass::IndustryView, index);
	}
});

} // namespace TradeTycoon
