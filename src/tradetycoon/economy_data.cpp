/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file economy_data.cpp Loading of tradetycoon_economy.json. */

#include "../stdafx.h"
#include "../debug.h"
#include "../fileio_func.h"
#include "../string_func.h"
#include "economy_data.h"

#include "../3rdparty/nlohmann/json.hpp"

#include "../safeguards.h"

namespace TradeTycoon {

/** File with the economy parameters, generated together with the products NewGRF (tools/products_grf/build.py). */
static const std::string_view ECONOMY_DATA_FILE = "tradetycoon_economy.json";

/**
 * Find the parameters of a natural resource.
 * @param label Cargo label of the resource.
 * @return The resource, or nullptr if the cargo is no natural resource.
 */
const ResourceSpec *EconomyData::FindResource(CargoLabel label) const
{
	for (const ResourceSpec &r : this->resources) {
		if (r.label == label) return &r;
	}
	return nullptr;
}

/**
 * Find the units of a product.
 * @param label Cargo label of the product.
 * @return The product, or nullptr if the cargo is no Capitalism product.
 */
const ProductSpec *EconomyData::FindProduct(CargoLabel label) const
{
	for (const ProductSpec &p : this->products) {
		if (p.label == label) return &p;
	}
	return nullptr;
}

/**
 * Quality share of the production technology: 100 minus the shares of the inputs.
 * @return The maximum production quality.
 */
uint8_t Recipe::MaxProductionQuality() const
{
	int share = 100;
	for (const RecipeInput &in : this->inputs) share -= in.iqua;
	return static_cast<uint8_t>(std::max(0, share));
}

/**
 * Convert a 4 character string into a cargo label.
 * @param text The label text.
 * @param[out] label The label.
 * @return Whether the text was a valid label.
 */
static bool ParseLabel(const std::string &text, CargoLabel &label)
{
	if (text.size() != 4) return false;
	std::copy_n(text.begin(), 4, label.begin());
	return true;
}

/**
 * Read the economy parameters from disk.
 * @param[out] data Parameters to fill.
 * @return Whether the file was found and valid.
 */
static bool LoadEconomyData(EconomyData &data)
{
	size_t size;
	auto f = FioFOpenFile(ECONOMY_DATA_FILE, "rb", Subdirectory::NewGrf, &size);
	if (!f.has_value()) {
		Debug(Facility::Misc, Severity::Error, "{} not found; Capitalism mechanics are disabled (run tools/products_grf/build.py)", ECONOMY_DATA_FILE);
		return false;
	}

	std::string text(size, '\0');
	if (fread(text.data(), 1, size, *f) != size) return false;

	try {
		auto json = nlohmann::json::parse(text);
		data.cell_value = json.at("cell_value").get<double>();
		data.price_scale = json.at("price_scale").get<double>();
		auto productivity = json.at("level_productivity").get<std::vector<uint16_t>>();
		auto capacity = json.at("level_capacity").get<std::vector<uint16_t>>();
		if (productivity.size() != NUM_UNIT_LEVELS || capacity.size() != NUM_UNIT_LEVELS) return false;
		std::copy(productivity.begin(), productivity.end(), data.level_productivity.begin());
		std::copy(capacity.begin(), capacity.end(), data.level_capacity.begin());

		if (!ConvertHexToBytes(json.at("grfid").get<std::string>(), data.grfid)) return false;
		data.factory_local_id = json.at("factory_local_id").get<uint16_t>();
		data.factory_space = json.at("factory_space").get<uint16_t>();
		data.base_tech_ratio = json.at("base_tech_ratio").get<double>();

		for (const auto &item : json.at("resources")) {
			ResourceSpec &r = data.resources.emplace_back();
			if (!ParseLabel(item.at("label").get<std::string>(), r.label)) return false;
			r.speed = item.at("speed").get<uint8_t>();
			r.res_qty = item.at("res_qty").get<uint16_t>();
			r.res_cost = item.at("res_cost").get<uint8_t>();
			r.quality_lo = item.at("quality_lo").get<uint8_t>();
			r.quality_hi = item.at("quality_hi").get<uint8_t>();
			r.units_per_cell = item.at("units_per_cell").get<double>();
			if (r.quality_hi < r.quality_lo || r.units_per_cell <= 0) return false;
		}

		for (const auto &item : json.at("products")) {
			ProductSpec &p = data.products.emplace_back();
			if (!ParseLabel(item.at("label").get<std::string>(), p.label)) return false;
			p.cell_items = item.at("cell_items").get<double>();
			p.items_per_unit = item.at("items_per_unit").get<double>();
			if (p.cell_items <= 0 || p.items_per_unit <= 0) return false;
		}

		for (const auto &item : json.at("recipes")) {
			Recipe &r = data.recipes.emplace_back();
			if (!ParseLabel(item.at("output").get<std::string>(), r.output)) return false;
			r.oqty = item.at("oqty").get<uint16_t>();
			r.prod_speed = item.at("prod_speed").get<uint16_t>();
			for (const auto &in : item.at("inputs")) {
				RecipeInput &ri = r.inputs.emplace_back();
				if (!ParseLabel(in.at("label").get<std::string>(), ri.label)) return false;
				ri.iqty = in.at("iqty").get<uint16_t>();
				ri.iqua = in.at("iqua").get<uint8_t>();
			}
			if (r.oqty == 0 || r.inputs.empty() || r.inputs.size() > 3) return false;
			if (std::ranges::any_of(r.inputs, [&data](const RecipeInput &in) { return data.FindProduct(in.label) == nullptr || in.iqty == 0; })) return false;
			if (data.FindProduct(r.output) == nullptr) return false;
		}
	} catch (const nlohmann::json::exception &e) {
		Debug(Facility::Misc, Severity::Error, "Invalid {}: {}", ECONOMY_DATA_FILE, e.what());
		return false;
	}
	return true;
}

/**
 * Get the economy parameters, loading them on first use.
 * @return The parameters; empty when the data file is missing or invalid.
 */
const EconomyData &GetEconomyData()
{
	static const EconomyData data = []() {
		EconomyData d;
		if (!LoadEconomyData(d)) d = EconomyData{};
		return d;
	}();
	return data;
}

} // namespace TradeTycoon
