/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tradetycoon_sl.h Savegame versions of Trade Tycoon.
 *
 * Trade Tycoon keeps the savegame version of upstream OpenTTD, so merging upstream never clashes with fork versions.
 * Its own changes are versioned separately in the TTVR chunk, which is saved before all other chunks.
 * Fields added to table chunks need no version: they are found by name.
 */

#ifndef SAVELOAD_TRADETYCOON_SL_H
#define SAVELOAD_TRADETYCOON_SL_H

/** Savegame versions of Trade Tycoon; 0 is a savegame without Trade Tycoon changes. */
enum class TTSaveVersion : uint16_t {
	None = 0, ///< Upstream OpenTTD savegame.
	WideCargoTypes, ///< Up to 240 cargo types: wider cargo masks and cargo monitor packing.
	CargoQuality, ///< Cargo packets carry the quality of Capitalism products.

	Current = CargoQuality, ///< Version written by this build.
};

extern TTSaveVersion _tt_savegame_version;

/**
 * Checks whether the loaded savegame predates a Trade Tycoon savegame version.
 * @param version The version to check against.
 * @return True if the savegame is older.
 */
inline bool IsTTSavegameVersionBefore(TTSaveVersion version)
{
	return _tt_savegame_version < version;
}

#endif /* SAVELOAD_TRADETYCOON_SL_H */
