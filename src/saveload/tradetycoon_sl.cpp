/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tradetycoon_sl.cpp Code handling saving and loading of Trade Tycoon state. */

#include "../stdafx.h"

#include "saveload.h"
#include "saveload_error.hpp"
#include "tradetycoon_sl.h"

#include "../tradetycoon/deposit.h"
#include "../tradetycoon/factory.h"

#include "table/strings.h"

#include "../safeguards.h"

TTSaveVersion _tt_savegame_version = TTSaveVersion::None; ///< Trade Tycoon version of the savegame being loaded.

/** Storage of the Trade Tycoon savegame version. */
struct TTVRStorage {
	uint16_t version; ///< The version.
};

static const SaveLoad _version_desc[] = {
	SaveLoad::Variable<VarFileType::U16>("version", SLE_OBJECT_ADDRESS(TTVRStorage, version)),
};

/** Trade Tycoon savegame version; saved first, so other chunks can depend on it while loading. */
struct TTVRChunkHandler : ChunkHandler {
	TTVRChunkHandler() : ChunkHandler("TTVR", ChunkType::Table) {}

	void Save() const override
	{
		SlTableHeader(_version_desc);
		TTVRStorage storage{to_underlying(TTSaveVersion::Current)};
		SlSetArrayIndex(0);
		SlObject(&storage, _version_desc);
	}

	void Load() const override
	{
		const std::vector<SaveLoad> slt = SlTableHeader(_version_desc);
		TTVRStorage storage{};
		while (SlIterateArray() != -1) SlObject(&storage, slt);
		if (storage.version > to_underlying(TTSaveVersion::Current)) SlError(STR_GAME_SAVELOAD_ERROR_TOO_NEW_SAVEGAME);
		_tt_savegame_version = static_cast<TTSaveVersion>(storage.version);
	}
};

/** Deposit with its industry, as stored in the savegame. */
struct DepositStorage {
	IndustryID industry; ///< Industry of the deposit.
	TradeTycoon::Deposit deposit; ///< Deposit state.
};

static const SaveLoad _deposit_desc[] = {
	SaveLoad::Variable<VarFileType::U16>("industry", SLE_OBJECT_ADDRESS(DepositStorage, industry)),
	SaveLoad::Variable<VarFileType::U32>("initial", SLE_OBJECT_ADDRESS(DepositStorage, deposit.initial)),
	SaveLoad::Variable<VarFileType::U32>("reserve", SLE_OBJECT_ADDRESS(DepositStorage, deposit.reserve)),
	SaveLoad::Variable<VarFileType::U32>("fraction", SLE_OBJECT_ADDRESS(DepositStorage, deposit.fraction)),
	SaveLoad::Variable<VarFileType::U8>("quality", SLE_OBJECT_ADDRESS(DepositStorage, deposit.quality)),
	SaveLoad::Variable<VarFileType::U8>("level", SLE_OBJECT_ADDRESS(DepositStorage, deposit.level)),
};

/** Natural resource deposits owned by companies. */
struct TTDPChunkHandler : ChunkHandler {
	TTDPChunkHandler() : ChunkHandler("TTDP", ChunkType::Table) {}

	void Save() const override
	{
		SlTableHeader(_deposit_desc);

		int index = 0;
		for (const auto &[industry, deposit] : TradeTycoon::GetDeposits()) {
			DepositStorage storage{industry, deposit};
			SlSetArrayIndex(index++);
			SlObject(&storage, _deposit_desc);
		}
	}

	void Load() const override
	{
		const std::vector<SaveLoad> slt = SlTableHeader(_deposit_desc);

		TradeTycoon::DepositMap &deposits = TradeTycoon::GetDeposits();
		deposits.clear();
		DepositStorage storage{};
		while (SlIterateArray() != -1) {
			SlObject(&storage, slt);
			deposits[storage.industry] = storage.deposit;
		}
	}
};

/** Factory with its industry, as stored in the savegame. */
struct FactoryStorage {
	IndustryID industry; ///< Industry of the factory.
	TradeTycoon::Factory factory; ///< Factory state.
};

static const SaveLoad _factory_desc[] = {
	SaveLoad::Variable<VarFileType::U16>("industry", SLE_OBJECT_ADDRESS(FactoryStorage, industry)),
	SaveLoad::Variable<VarFileType::U16>("recipe", SLE_OBJECT_ADDRESS(FactoryStorage, factory.recipe)),
	SaveLoad::Variable<VarFileType::U8>("level", SLE_OBJECT_ADDRESS(FactoryStorage, factory.level)),
	SaveLoad::Variable<VarFileType::U8>("output_quality", SLE_OBJECT_ADDRESS(FactoryStorage, factory.output_quality)),
	SaveLoad::Array<VarFileType::U8, TradeTycoon::MAX_RECIPE_INPUTS>("input_quality", SLE_OBJECT_ADDRESS(FactoryStorage, factory.input_quality)),
	SaveLoad::Array<VarFileType::U32, TradeTycoon::MAX_RECIPE_INPUTS>("input_fraction", SLE_OBJECT_ADDRESS(FactoryStorage, factory.input_fraction)),
	SaveLoad::Variable<VarFileType::U32>("output_fraction", SLE_OBJECT_ADDRESS(FactoryStorage, factory.output_fraction)),
};

/** Factories owned by companies. */
struct TTFCChunkHandler : ChunkHandler {
	TTFCChunkHandler() : ChunkHandler("TTFC", ChunkType::Table) {}

	void Save() const override
	{
		SlTableHeader(_factory_desc);

		int index = 0;
		for (const auto &[industry, factory] : TradeTycoon::GetFactories()) {
			FactoryStorage storage{industry, factory};
			SlSetArrayIndex(index++);
			SlObject(&storage, _factory_desc);
		}
	}

	void Load() const override
	{
		const std::vector<SaveLoad> slt = SlTableHeader(_factory_desc);

		TradeTycoon::FactoryMap &factories = TradeTycoon::GetFactories();
		factories.clear();
		FactoryStorage storage{};
		while (SlIterateArray() != -1) {
			SlObject(&storage, slt);
			factories[storage.industry] = storage.factory;
		}
	}
};

static const TTVRChunkHandler TTVR;
static const TTDPChunkHandler TTDP;
static const TTFCChunkHandler TTFC;
static const ChunkHandlerRef tradetycoon_chunk_handlers[] = {
	TTVR,
	TTDP,
	TTFC,
};

extern const ChunkHandlerTable _tradetycoon_chunk_handlers(tradetycoon_chunk_handlers);
