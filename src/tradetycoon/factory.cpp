/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file factory.cpp Factories owned by companies (Capitalism mechanics 2 and 3, simplified). */

#include "../stdafx.h"
#include "../debug.h"
#include "../cargotype.h"
#include "../command_func.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../industry.h"
#include "../newgrf_industries.h"
#include "../station_base.h"
#include "../station_func.h"
#include "../strings_func.h"
#include "../window_func.h"
#include "../timer/timer.h"
#include "../timer/timer_game_economy.h"
#include "economy_data.h"
#include "deposit.h"
#include "factory.h"
#include "factory_cmd.h"

#include "table/strings.h"

#include "../safeguards.h"

namespace TradeTycoon {

static FactoryMap _factories; ///< All company factories, saved in the TTFC chunk.

/** Quality of Capitalism products from industries that are no deposit or factory of a company. */
static constexpr uint8_t LOCAL_SUPPLIER_QUALITY = 50;

/**
 * Get all factories, e.g. for saving.
 * @return The factories.
 */
FactoryMap &GetFactories()
{
	return _factories;
}

/**
 * Get the factory state of an industry.
 * @param index Industry.
 * @return The factory, or nullptr if the industry is no company factory.
 */
const Factory *GetFactory(IndustryID index)
{
	auto it = _factories.find(index);
	return it == _factories.end() ? nullptr : &it->second;
}

/**
 * Whether an industry type is the company factory of the products NewGRF.
 * @param type Industry type.
 * @return True for the company factory.
 */
bool IsCompanyFactoryType(IndustryType type)
{
	const EconomyData &data = GetEconomyData();
	const IndustrySpec *indspec = GetIndustrySpec(type);
	return indspec->grf_prop.grfid == data.grfid && indspec->grf_prop.local_id == data.factory_local_id;
}

/**
 * Whether an industry is a factory of a company.
 * @param i Industry.
 * @return True for company factories.
 */
bool IsCompanyFactory(const Industry *i)
{
	return Company::IsValidID(i->founder) && _factories.contains(i->index);
}

/**
 * Get a recipe.
 * @param recipe 1-based recipe index.
 * @return The recipe, or nullptr for none.
 */
static const Recipe *GetRecipe(uint16_t recipe)
{
	const EconomyData &data = GetEconomyData();
	if (recipe == 0 || recipe > data.recipes.size()) return nullptr;
	return &data.recipes[recipe - 1];
}

/**
 * Register a newly built company factory; it has no cargo until a recipe is chosen.
 * @param i The new industry.
 */
void OnFactoryFounded(Industry *i)
{
	if (!Company::IsValidID(i->founder) || !IsCompanyFactoryType(i->type)) return;

	i->accepted.clear();
	i->produced.clear();
	_factories[i->index] = Factory{};
}

/**
 * Forget the factory state of a removed industry.
 * @param index Industry.
 */
void OnFactoryDeleted(IndustryID index)
{
	_factories.erase(index);
}

/**
 * Update the average input quality after cargo has been delivered.
 * @param i Industry, its waiting cargo already includes the delivery.
 * @param cargo Delivered cargo.
 * @param amount Delivered amount.
 * @param quality Quality of the delivered cargo; 0 counts as a local supplier.
 */
void OnFactoryCargoDelivered(Industry *i, CargoType cargo, uint amount, uint8_t quality)
{
	auto it = _factories.find(i->index);
	if (it == _factories.end() || amount == 0) return;

	auto a = i->GetCargoAccepted(cargo);
	if (a == std::end(i->accepted)) return;
	size_t index = std::distance(std::begin(i->accepted), a);
	if (index >= MAX_RECIPE_INPUTS) return;

	if (quality == 0) quality = LOCAL_SUPPLIER_QUALITY;
	uint8_t &stock_quality = it->second.input_quality[index];
	uint before = a->waiting - amount;
	stock_quality = static_cast<uint8_t>((stock_quality * before + quality * amount) / a->waiting);
}

/**
 * Keep product that no station took in the factory stock, like unsold stock of a Capitalism factory.
 * @param i Industry.
 * @param cargo Product.
 * @param amount Cargo units that were not moved to a station.
 */
void OnFactoryCargoNotShipped(Industry *i, CargoType cargo, uint amount)
{
	if (amount == 0 || !IsCompanyFactory(i)) return;
	auto p = i->GetCargoProduced(cargo);
	if (p == std::end(i->produced)) return;
	p->waiting = ClampTo<uint16_t>(p->waiting + amount);
	/* It is counted as production again when it leaves the stock. */
	auto &production = p->history[THIS_MONTH].production;
	production -= std::min<uint16_t>(production, amount);
}

/**
 * Quality of cargo leaving an industry.
 * @param i Industry.
 * @param cargo Cargo.
 * @return Quality 1..100, or 0 for cargo that is no Capitalism product.
 */
uint8_t GetIndustryCargoQuality(const Industry *i, CargoType cargo)
{
	if (const Factory *f = GetFactory(i->index); f != nullptr && IsCompanyFactory(i)) return f->output_quality;
	if (const Deposit *d = GetDeposit(i->index); d != nullptr && IsManagedDeposit(i)) return d->quality;
	return GetEconomyData().FindProduct(CargoSpec::Get(cargo)->label) != nullptr ? LOCAL_SUPPLIER_QUALITY : 0;
}

/**
 * Get the recipes whose cargo types all exist in this game.
 * @return 1-based recipe indices.
 */
std::vector<uint16_t> GetAvailableRecipes()
{
	const EconomyData &data = GetEconomyData();
	std::vector<uint16_t> list;
	for (size_t n = 0; n < data.recipes.size(); n++) {
		const Recipe &r = data.recipes[n];
		if (!IsValidCargoType(GetCargoTypeByLabel(r.output))) continue;
		if (std::ranges::any_of(r.inputs, [](const RecipeInput &in) { return !IsValidCargoType(GetCargoTypeByLabel(in.label)); })) continue;
		list.push_back(static_cast<uint16_t>(n + 1));
	}
	return list;
}

/**
 * Name of a recipe: the name of its product.
 * @param recipe 1-based recipe index.
 * @return The name.
 */
std::string GetRecipeName(uint16_t recipe)
{
	const Recipe *r = GetRecipe(recipe);
	if (r == nullptr) return {};
	return GetString(CargoSpec::Get(GetCargoTypeByLabel(r->output))->name);
}

/**
 * Batches the factory can make per day (CapMain.exe j_mft.cpp 005c1310, method.cpp 005f3590):
 * productivity(level) * SPACE * max(1, PROD_SPEED / 100 * cell_items / OQTY), i.e. about one cell of product per day.
 * @param f Factory.
 * @param r Its recipe.
 * @return Batches per day.
 */
static double BatchesPerDay(const Factory &f, const Recipe &r)
{
	const EconomyData &data = GetEconomyData();
	const ProductSpec *out = data.FindProduct(r.output);
	uint8_t level = Clamp<uint8_t>(f.level, 1, NUM_UNIT_LEVELS);
	double speed = std::max(1.0, r.prod_speed / 100.0 * out->cell_items / r.oqty);
	return data.level_productivity[level - 1] / 100.0 * data.factory_space / 100.0 * speed;
}

/**
 * Text for the industry window: recipe, inputs and product of a company factory.
 * @param i Industry.
 * @return The text, or empty if the industry is no company factory.
 */
std::string GetFactoryInfoString(const Industry *i)
{
	const Factory *f = GetFactory(i->index);
	if (f == nullptr) return {};

	const Recipe *r = GetRecipe(f->recipe);
	if (r == nullptr) return GetString(STR_TT_FACTORY_NO_RECIPE, i->founder);

	const ProductSpec *out = GetEconomyData().FindProduct(r->output);
	uint per_day = static_cast<uint>(BatchesPerDay(*f, *r) * r->oqty / out->items_per_unit);
	std::string text = GetString(STR_TT_FACTORY_PRODUCT, i->founder, GetCargoTypeByLabel(r->output), per_day, f->output_quality, f->level);
	for (size_t n = 0; n < r->inputs.size() && n < i->accepted.size(); n++) {
		const auto &a = i->accepted[n];
		text += GetString(STR_TT_FACTORY_INPUT, a.cargo, a.waiting, f->input_quality[n], r->inputs[n].iqua);
	}
	return text;
}

/**
 * Daily production of all company factories (CapMain.exe j_mft.cpp 005c10a0, 005c1310, 005c2600).
 * quality = sum(IQUA * input quality) / 100 + min(max, max * tech ratio), max = 100 - sum(IQUA).
 */
static const IntervalTimer<TimerGameEconomy> _economy_factories_daily({TimerGameEconomy::Trigger::Day, TimerGameEconomy::Priority::None}, [](auto)
{
	const EconomyData &data = GetEconomyData();

	for (auto &[index, f] : _factories) {
		Industry *i = Industry::GetIfValid(index);
		const Recipe *r = GetRecipe(f.recipe);
		if (i == nullptr || r == nullptr || !Company::IsValidID(i->founder)) continue;
		if (i->produced.empty() || i->accepted.size() < r->inputs.size()) continue;

		const ProductSpec *out = data.FindProduct(r->output);
		auto &p = i->produced[0];
		uint8_t level = Clamp<uint8_t>(f.level, 1, NUM_UNIT_LEVELS);

		/* Limited by capacity, the stock of every input and the room in the product stock. */
		double batches = BatchesPerDay(f, *r);
		for (size_t n = 0; n < r->inputs.size(); n++) {
			const ProductSpec *in = data.FindProduct(r->inputs[n].label);
			double stock_units = i->accepted[n].waiting - f.input_fraction[n] / 1000.0;
			batches = std::min(batches, std::max(0.0, stock_units) * in->items_per_unit / r->inputs[n].iqty);
		}
		double room_units = data.level_capacity[level - 1] * out->cell_items / out->items_per_unit - p.waiting;
		batches = std::min(batches, std::max(0.0, room_units) * out->items_per_unit / r->oqty);
		if (batches <= 0) continue;

		uint quality = 0;
		for (size_t n = 0; n < r->inputs.size(); n++) {
			const ProductSpec *in = data.FindProduct(r->inputs[n].label);
			uint64_t used = static_cast<uint64_t>(batches * r->inputs[n].iqty / in->items_per_unit * 1000.0) + f.input_fraction[n];
			uint whole = static_cast<uint>(std::min<uint64_t>(used / 1000, i->accepted[n].waiting));
			i->accepted[n].waiting -= whole;
			f.input_fraction[n] = static_cast<uint32_t>(used - whole * 1000ULL);
			quality += r->inputs[n].iqua * f.input_quality[n];
		}
		uint8_t max_production = r->MaxProductionQuality();
		quality = quality / 100 + static_cast<uint>(max_production * std::min(1.0, data.base_tech_ratio));
		f.output_quality = static_cast<uint8_t>(Clamp<uint>(quality, 1, 100));

		uint64_t made = static_cast<uint64_t>(batches * r->oqty / out->items_per_unit * 1000.0) + f.output_fraction;
		p.waiting = ClampTo<uint16_t>(p.waiting + made / 1000);
		f.output_fraction = static_cast<uint32_t>(made % 1000);
		SetWindowDirty(WindowClass::IndustryView, index);
	}
});

/** Monthly debug output of deposits and factories (-d misc=5). */
static const IntervalTimer<TimerGameEconomy> _economy_tradetycoon_debug_monthly({TimerGameEconomy::Trigger::Month, TimerGameEconomy::Priority::None}, [](auto)
{
	if (!IsVisibleSeverity(Facility::Misc, Severity::Info)) return;
	for (const auto &[index, d] : GetDeposits()) {
		Debug(Facility::Misc, Severity::Info, "TT deposit {}: reserve {}/{} quality {} level {}", index, d.reserve, d.initial, d.quality, d.level);
	}
	for (const auto &[index, f] : _factories) {
		const Industry *i = Industry::GetIfValid(index);
		if (i == nullptr) continue;
		std::string inputs;
		for (size_t n = 0; n < i->accepted.size() && n < MAX_RECIPE_INPUTS; n++) inputs += fmt::format(" in{}={}(q{})", n, i->accepted[n].waiting, f.input_quality[n]);
		uint16_t stock = i->produced.empty() ? 0 : i->produced[0].waiting;
		Debug(Facility::Misc, Severity::Info, "TT factory {}: recipe {} stock {} quality {}{}", index, f.recipe, stock, f.output_quality, inputs);
	}
});

} // namespace TradeTycoon

/**
 * Choose what a company factory makes.
 * @param flags Type of operation.
 * @param industry The factory.
 * @param recipe 1-based recipe index; 0 stops production.
 * @return Empty cost or an error.
 */
CommandCost CmdSetFactoryRecipe(DoCommandFlags flags, IndustryID industry, uint16_t recipe)
{
	using namespace TradeTycoon;

	Industry *i = Industry::GetIfValid(industry);
	if (i == nullptr || !IsCompanyFactory(i)) return CMD_ERROR;

	CommandCost ret = CheckOwnership(i->founder);
	if (ret.Failed()) return ret;

	const Recipe *r = GetRecipe(recipe);
	if (recipe != 0) {
		auto available = GetAvailableRecipes();
		if (r == nullptr || std::ranges::find(available, recipe) == available.end()) return CMD_ERROR;
	}

	if (flags.Test(DoCommandFlag::Execute)) {
		Factory &f = _factories[industry];
		f.recipe = recipe;
		f.output_quality = 0;
		f.input_quality = {};
		f.input_fraction = {};
		f.output_fraction = 0;

		i->accepted.clear();
		i->produced.clear();
		if (r != nullptr) {
			for (const RecipeInput &in : r->inputs) i->accepted.emplace_back().cargo = GetCargoTypeByLabel(in.label);
			i->produced.emplace_back().cargo = GetCargoTypeByLabel(r->output);
		}

		/* Stations only deliver to industries that accept cargo, so rebuild their lists.
		 * RecomputeCatchment changes stations_near, so iterate over a copy. */
		std::vector<Station *> stations(i->stations_near.begin(), i->stations_near.end());
		for (Station *st : stations) {
			st->RecomputeCatchment();
			UpdateStationAcceptance(st, false);
		}
		SetWindowDirty(WindowClass::IndustryView, industry);
		InvalidateWindowData(WindowClass::IndustryDirectory, 0, IDIWD_FORCE_REBUILD);
	}
	return CommandCost();
}
