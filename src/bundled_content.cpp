/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file bundled_content.cpp Default NewGRF selection for the content shipped in bundled/. */

#include "stdafx.h"
#include "debug.h"
#include "string_func.h"
#include "bundled_content.h"

#include "safeguards.h"

/**
 * NewGRFs enabled for new games when the config has no [newgrf] section yet.
 * Order matters: houses, industries, vehicles, stations. Keep in sync with bundled/content.json.
 */
static const GrfID _bundled_newgrfs[] = {
	GrfID{"\x54\x54\x03\x01"}, // ITL Houses (Improved Town Layouts)
	GrfID{"\xF1\x25\x00\x09"}, // FIRS Industries 5
	GrfID{"\x43\x41\x12\x23"}, // Iron Horse 4 (Trains)
	GrfID{"\x97\x87\xEA\xFE"}, // Road Hog (Buses, Trucks, Trams)
	GrfID{"\x43\x48\x50\x53"}, // CHIPS Station Set 1
};

/**
 * Append the bundled NewGRFs to a NewGRF selection; used when the config has no [newgrf] section yet.
 * Must be called after the NewGRF scan, which is the case when the config is (re)loaded.
 * @param list The selection to append to.
 */
void AppendBundledNewGRFDefaults(GRFConfigList &list)
{
	for (const GrfID &grfid : _bundled_newgrfs) {
		const GRFConfig *c = FindGRFConfig(grfid, FindGRFConfigMode::NewestValid);
		if (c == nullptr) {
			Debug(Facility::Grf, Severity::Error, "Bundled NewGRF {} not found; run bundled/fetch_content.py", FormatArrayAsHex(grfid));
			continue;
		}
		AppendToGRFConfigList(list, std::make_unique<GRFConfig>(*c));
	}
}
