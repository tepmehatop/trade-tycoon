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

		for (const auto &item : json.at("resources")) {
			ResourceSpec &r = data.resources.emplace_back();
			std::string label = item.at("label").get<std::string>();
			if (label.size() != 4) return false;
			std::copy_n(label.begin(), 4, r.label.begin());
			r.speed = item.at("speed").get<uint8_t>();
			r.res_qty = item.at("res_qty").get<uint16_t>();
			r.res_cost = item.at("res_cost").get<uint8_t>();
			r.quality_lo = item.at("quality_lo").get<uint8_t>();
			r.quality_hi = item.at("quality_hi").get<uint8_t>();
			r.units_per_cell = item.at("units_per_cell").get<double>();
			if (r.quality_hi < r.quality_lo || r.units_per_cell <= 0) return false;
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
