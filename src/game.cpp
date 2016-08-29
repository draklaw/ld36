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


#include "main_state.h"
#include "splash_state.h"

#include "game.h"


#ifndef CMAKE_PROJECT_NAME
#define CMAKE_PROJECT_NAME "Lair"
#endif


Game::Game(int argc, char** argv)
    : GameBase(argc, argv),
      _mainState(),
      _splashState(),
      _firstLevel("lvl_init.json") {

	if(argc == 2) {
		_firstLevel = argv[1];
	}
}


Game::~Game() {
}


void Game::initialize() {
	GameBase::initialize();

	window()->setUtf8Title(CMAKE_PROJECT_NAME);
//	window()->resize(1920 / 4, 1080 / 4);
//	window()->setFullscreen(true);

	_splashState.reset(new SplashState(this));
	_mainState.reset(new MainState(this));

	_splashState->initialize();
	_splashState->setup(_mainState.get(), "titlescreen.png", 3);

	_mainState->initialize();
	_mainState->startGame(_firstLevel);

//	AssetSP music = _loader->loadAsset<MusicLoader>("shapeout.ogg");
//	_loader->waitAll();
//	audio()->playMusic(music);
}


void Game::shutdown() {
	_mainState->shutdown();
	_splashState->shutdown();

	// Required to ensure that everything is freed
	_mainState.reset();
	_splashState.reset();

	GameBase::shutdown();
}


MainState* Game::mainState() {
	return _mainState.get();
}


SplashState* Game::splashState() {
	return _splashState.get();
}


const Path& Game::firstLevel() {
	return _firstLevel;
}
