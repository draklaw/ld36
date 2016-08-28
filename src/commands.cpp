/*
 *  Copyright (C) 2016 the authors (see AUTHORS)
 *
 *  This file is part of ld36.
 *
 *  lair is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  lair is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with lair.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <sstream>

#include "main_state.h"
#include "level.h"

#include "components.h"


int echoCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	std::ostringstream out;
	out << argv[0];
	for(int i = 1; i < argc; ++i)
		out << " " << argv[i];
	dbgLogger.info(out.str());
}


bool isDoorOpen(MainState* state, EntityRef door) {
	SpriteComponent* sc = state->_sprites.get(door);
	return sc && !sc->tileIndex();
}


void setDoorOpen(MainState* state, EntityRef door, bool open) {
	SpriteComponent*    sc = state->_sprites.get(door);
	CollisionComponent* cc = state->_collisions.get(door);
	if(sc && cc) {
		sc->setTileIndex(open? 0: 1);
		cc->setEnabled(!open);
	}
	else {
		dbgLogger.warning("setDoorOpen: ", door.name(), " do not look like a door.");
	}
}


int switchDoorCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	if(argc != 2) {
		dbgLogger.warning("Command ", argv[0], ": Invalid number of arguments.");
		return -2;
	}

	auto targets = state->_level->entities(argv[1]);
	for(EntityRef entity: targets) {
		setDoorOpen(state, entity, !isDoorOpen(state, entity));
	}
}
