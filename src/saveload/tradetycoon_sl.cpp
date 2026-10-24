/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tradetycoon_sl.cpp Code handling saving and loading of Trade Tycoon state. */

#include "../stdafx.h"

#include "saveload.h"

#include "../tradetycoon/deposit.h"

#include "../safeguards.h"

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

static const TTDPChunkHandler TTDP;
static const ChunkHandlerRef tradetycoon_chunk_handlers[] = {
	TTDP,
};

extern const ChunkHandlerTable _tradetycoon_chunk_handlers(tradetycoon_chunk_handlers);
