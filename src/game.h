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


#ifndef LD36_GAME_H
#define LD36_GAME_H


#include <lair/utils/game_base.h>


using namespace lair;


class MainState;
class SplashState;


class Game : public GameBase {
public:
	Game(int argc, char** argv);
	Game(const Game&)  = delete;
	Game(      Game&&) = delete;
	~Game();

	Game& operator=(const Game&)  = delete;
	Game& operator=(      Game&&) = delete;

	void initialize();
	void shutdown();

	MainState*   mainState();
	SplashState* splashState();

	const Path& firstLevel();

protected:
	std::unique_ptr<SplashState> _splashState;
	std::unique_ptr<MainState>   _mainState;

	Path _firstLevel;
};


#endif
