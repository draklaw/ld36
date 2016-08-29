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

#include <lair/sys_sdl2/audio_module.h>

#include "game.h"
#include "main_state.h"
#include "splash_state.h"
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

	state->playSound("door.wav");

	auto targets = state->_level->entities(argv[1]);
	for(EntityRef entity: targets) {
		setDoorOpen(state, entity, !isDoorOpen(state, entity));
	}

	return 0;
}


int setDoorCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	if(argc != 3) {
		dbgLogger.warning("Command ", argv[0], ": Invalid number of arguments.");
		return -2;
	}

	state->playSound("door.wav");

	auto targets = state->_level->entities(argv[1]);
	bool open = std::atoi(argv[2]);
	for(EntityRef entity: targets) {
		setDoorOpen(state, entity, open);
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

	state->playSound("footstep.wav");
	int item = sc->tileIndex();
	state->addToInventory(Item(item));

	self.setEnabled(false);

	return 0;
}


int messageCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	if(argc < 2) {
		dbgLogger.warning("messageCommand: wrong number of argument.");
		return -2;
	}

	state->popupMessage(argv[1]);

	if(argc > 2)
		state->setPostCommand(argc - 2, argv + 2);
	else
		state->setPostCommand("continue");

	return 0;
}


int nextLevelCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	if(argc != 2 && argc != 3) {
		dbgLogger.warning("nextLevelCommand: wrong number of argument.");
		return -2;
	}

	if(argc == 2)
		state->startLevel(argv[1]);
	else
		state->startLevel(argv[1], argv[2]);

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
	state->playSound("tp.wav");

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
		state->playSound("footstep.wav");
		state->removeFromInventory(item);
		state->exec(argc - 2, argv + 2, self);
	}

	return 0;
}


int playSoundCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	if(argc != 2) {
		dbgLogger.warning("playSoundCommand: wrong number of argument.");
		return -2;
	}

	state->playSound(argv[1]);

	return 0;
}


int continueCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	if(argc != 1) {
		dbgLogger.warning("playSoundCommand: wrong number of argument.");
		return -2;
	}

	state->setState(STATE_PLAY);

	return 0;
}


int fadeInCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	if(argc < 1) {
		dbgLogger.warning("fadeInCommand: wrong number of argument.");
		return -2;
	}

	state->setState(STATE_FADE_IN);

	if(argc > 1) {
		state->setPostCommand(argc - 1, argv + 1);
	}

	return 0;
}


int fadeOutCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	if(argc < 1) {
		dbgLogger.warning("fadeOutCommand: wrong number of argument.");
		return -2;
	}

	state->setState(STATE_FADE_OUT);

	if(argc > 1) {
		state->setPostCommand(argc - 1, argv + 1);
	}

	return 0;
}


int disableCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	if(argc != 1) {
		dbgLogger.warning(argv[0], ": wrong number of argument.");
		return -2;
	}

	if(!self.isValid()) {
		dbgLogger.warning(argv[0], ": self is not set.");
		return -2;
	}

	self.setEnabled(false);

	return 0;
}


int bocalCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	if(argc != 1) {
		dbgLogger.warning(argv[0], ": wrong number of argument.");
		return -2;
	}

	if(state->_endingState == END_BOCAL_OFF) {
		state->_sprites.get(state->_level->entity("left"))->setTileIndex(1);
		state->_sprites.get(state->_level->entity("right"))->setTileIndex(1);
		state->popupMessage("lvl_f_bocal_tout");
		state->_endingState = END_BOCAL_ON;
	}

	return 0;
}

int bocalKillCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	if(argc != 1) {
		dbgLogger.warning(argv[0], ": wrong number of argument.");
		return -2;
	}

	if(state->_endingState == END_BOCAL_ON) {
		state->_sprites.get(state->_level->entity("bocal"))->setTileIndex(2);
		state->popupMessage("lvl_f_bocal_kill");
		state->_endingState = END_KILL;
	}

	return 0;
}

int bocalSaveCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	if(argc != 1) {
		dbgLogger.warning(argv[0], ": wrong number of argument.");
		return -2;
	}

	if(state->_endingState == END_BOCAL_ON) {
		if(state->hasItem(ITEM_ARTEFACT) && state->hasItem(ITEM_CHIP)) {
			state->removeFromInventory(ITEM_ARTEFACT);
			state->removeFromInventory(ITEM_CHIP);
			state->_sprites.get(state->_level->entity("bocal"))->setTileIndex(1);
			state->_level->entity("alien").setEnabled(true);
			state->popupMessage("lvl_f_bocal_save");
			state->_endingState = END_SAVE;
		}
	}

	return 0;
}

int letsFlyCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	if(argc != 1) {
		dbgLogger.warning(argv[0], ": wrong number of argument.");
		return -2;
	}

	if(state->hasItem(ITEM_MAN) && state->hasItem(ITEM_CABLE) && state->hasItem(ITEM_GROUPE)) {
		state->removeFromInventory(ITEM_MAN);
		state->removeFromInventory(ITEM_CABLE);
		state->removeFromInventory(ITEM_GROUPE);
		state->setState(STATE_FADE_OUT);
		state->setPostCommand("lets_fly_2");
	}

	return 0;
}

int letsFly2Command(MainState* state, EntityRef self, int argc, const char** argv) {
	if(argc != 1) {
		dbgLogger.warning(argv[0], ": wrong number of argument.");
		return -2;
	}

	if(state->_endingState == END_SAVE)
		state->popupMessage("lvl_f_swth_ship");
	else
		state->popupMessage("lvl_f_noswth_ship");

	state->setPostCommand("credits");

	return 0;
}

int letsQuitCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	if(argc != 1) {
		dbgLogger.warning(argv[0], ": wrong number of argument.");
		return -2;
	}

	if(state->_endingState == END_SAVE)
		state->popupMessage("lvl_f_swth_noship");
	else
		state->popupMessage("lvl_f_noswth_noship");

	state->setPostCommand("credits");

	return 0;
}

int creditsCommand(MainState* state, EntityRef self, int argc, const char** argv) {
	if(argc != 1) {
		dbgLogger.warning(argv[0], ": wrong number of argument.");
		return -2;
	}

	state->game()->splashState()->setup(nullptr, "tileset.png");
	state->game()->setNextState(state->game()->splashState());
	state->quit();

	return 0;
}
