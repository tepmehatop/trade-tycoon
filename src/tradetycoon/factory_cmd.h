/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file factory_cmd.h Command definitions for company factories. */

#ifndef TRADETYCOON_FACTORY_CMD_H
#define TRADETYCOON_FACTORY_CMD_H

#include "../command_type.h"
#include "../industry_type.h"

CommandCost CmdSetFactoryRecipe(DoCommandFlags flags, IndustryID industry, uint16_t recipe);

DEF_CMD_TRAIT(Commands::SetFactoryRecipe, CmdSetFactoryRecipe, {}, CommandType::OtherManagement)

#endif /* TRADETYCOON_FACTORY_CMD_H */
