/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file factory.h Factories owned by companies (Capitalism mechanics 2 and 3, simplified).
 *
 * A company funds a "Company Factory" industry and chooses a recipe; the factory then accepts the inputs of
 * the recipe and makes the product daily, with a quality derived from the input qualities and the technology.
 * See catalog/mechanics/02-manufacturing.md and 03-quality.md.
 */

#ifndef TRADETYCOON_FACTORY_H
#define TRADETYCOON_FACTORY_H

#include "../cargo_type.h"
#include "../industry_type.h"

struct Industry;

namespace TradeTycoon {

/** Maximum number of inputs of a recipe. */
static constexpr size_t MAX_RECIPE_INPUTS = 3;

/** State of a company factory. */
struct Factory {
	uint16_t recipe = 0; ///< Chosen recipe, 1-based index into EconomyData::recipes; 0 for none.
	uint8_t level = 1; ///< Level of the manufacturing unit, 1..9.
	uint8_t output_quality = 0; ///< Quality of the product made last.
	std::array<uint8_t, MAX_RECIPE_INPUTS> input_quality{}; ///< Average quality of the stock of each input.
	std::array<uint32_t, MAX_RECIPE_INPUTS> input_fraction{}; ///< Input used but not yet taken from the stock, in 1/1000 cargo units.
	uint32_t output_fraction = 0; ///< Product made but not yet added as a whole cargo unit, in 1/1000 cargo units.
};

/** All company factories, by industry. */
using FactoryMap = std::map<IndustryID, Factory>;

FactoryMap &GetFactories();
const Factory *GetFactory(IndustryID index);
bool IsCompanyFactoryType(IndustryType type);
bool IsCompanyFactory(const Industry *i);
void OnFactoryFounded(Industry *i);
void OnFactoryDeleted(IndustryID index);
void OnFactoryCargoDelivered(Industry *i, CargoType cargo, uint amount, uint8_t quality);
void OnFactoryCargoNotShipped(Industry *i, CargoType cargo, uint amount);
uint8_t GetIndustryCargoQuality(const Industry *i, CargoType cargo);
std::vector<uint16_t> GetAvailableRecipes();
std::string GetRecipeName(uint16_t recipe);
std::string GetFactoryInfoString(const Industry *i);

} // namespace TradeTycoon

#endif /* TRADETYCOON_FACTORY_H */
