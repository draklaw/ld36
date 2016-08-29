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


#ifndef LD36_COMMANDS_H
#define LD36_COMMANDS_H


#include <map>

#include <lair/core/lair.h>
#include <lair/core/log.h>

#include <lair/ec/entity.h>


using namespace lair;


class MainState;


int echoCommand(MainState* state, EntityRef self, int argc, const char** argv);

bool isDoorOpen(MainState* state, EntityRef door);
void setDoorOpen(MainState* state, EntityRef door, bool open);
int switchDoorCommand(MainState* state, EntityRef self, int argc, const char** argv);
int setDoorCommand(MainState* state, EntityRef self, int argc, const char** argv);

int pickupItemCommand(MainState* state, EntityRef self, int argc, const char** argv);

int messageCommand(MainState* state, EntityRef self, int argc, const char** argv);

int nextLevelCommand(MainState* state, EntityRef self, int argc, const char** argv);

int teleportCommand(MainState* state, EntityRef self, int argc, const char** argv);

int useObjectCommand(MainState* state, EntityRef self, int argc, const char** argv);

int playSoundCommand(MainState* state, EntityRef self, int argc, const char** argv);

int continueCommand(MainState* state, EntityRef self, int argc, const char** argv);

int fadeInCommand(MainState* state, EntityRef self, int argc, const char** argv);
int fadeOutCommand(MainState* state, EntityRef self, int argc, const char** argv);

int disableCommand(MainState* state, EntityRef self, int argc, const char** argv);

int bocalCommand(MainState* state, EntityRef self, int argc, const char** argv);
int bocalKillCommand(MainState* state, EntityRef self, int argc, const char** argv);
int bocalSaveCommand(MainState* state, EntityRef self, int argc, const char** argv);
int letsFlyCommand(MainState* state, EntityRef self, int argc, const char** argv);
int letsFly2Command(MainState* state, EntityRef self, int argc, const char** argv);
int letsQuitCommand(MainState* state, EntityRef self, int argc, const char** argv);

int creditsCommand(MainState* state, EntityRef self, int argc, const char** argv);

#endif
