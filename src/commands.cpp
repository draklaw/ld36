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

	return 0;
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
		door.transform()(2, 3) = open? .2: .09;
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

	return 0;
}


int pickupItemCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	if(!self.isValid()) {
		dbgLogger.warning("pickupItemCommand: self is not set.");
		return -2;
	}
	if(argc != 1) {
		dbgLogger.warning("pickupItemCommand: wrong number of argument.");
		return -2;
	}

	SpriteComponent* sc = state->_sprites.get(self);
	if(!sc) {
		dbgLogger.warning("pickupItemCommand: ", self.name(), " do not look like an item.");
		return -2;
	}

	int item = sc->tileIndex();
	state->addToInventory(Item(item));

	self.setEnabled(false);

	return 0;
}


int messageCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	if(argc != 2) {
		dbgLogger.warning("messageCommand: wrong number of argument.");
		return -2;
	}

	const Json::Value& messages = state->_messages.get(argv[1], Json::nullValue);
	if(!messages.isArray()) {
		dbgLogger.warning("messageCommand: invalid message identifier \"", argv[1], "\"");
		return -2;
	}

	for(const Json::Value& msg: messages)
		state->enqueueMessage(msg.asString());

	return 0;
}


int nextLevelCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	if(argc != 2) {
		dbgLogger.warning("nextLevelCommand: wrong number of argument.");
		return -2;
	}

	state->startLevel(argv[1]);

	return 0;
}


int teleportCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	if(argc != 2) {
		dbgLogger.warning("teleportCommand: wrong number of argument.");
		return -2;
	}

	EntityRef target = state->_level->entity(argv[1]);
	if(!target.isValid()) {
		dbgLogger.warning("teleportCommand: target \"", target.name(), "\" not found.");
		return -2;
	}

	float depth = state->_player.transform()(2, 3);
	state->_player.place((Vector3() << target.translation2(), depth).finished());

	TriggerComponent* tc = state->_triggers.get(target);
	if(tc) {
		tc->inside = true;
		tc->prevInside = true;
	}

	return 0;
}


int useObjectCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	if(argc < 3) {
		dbgLogger.warning("useObjectCommand: wrong number of argument.");
		return -2;
	}

	Item item = Item(std::atoi(argv[1]));
	if(state->hasItem(item)) {
		state->removeFromInventory(item);
		state->exec(argc - 2, argv + 2, self);
	}

	return 0;
}
