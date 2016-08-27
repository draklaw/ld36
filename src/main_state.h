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


#ifndef LD36_MAIN_STATE_H
#define LD36_MAIN_STATE_H


#include <lair/core/signal.h>
#include <lair/core/json.h>

#include <lair/utils/game_state.h>
#include <lair/utils/interp_loop.h>
#include <lair/utils/input.h>
#include <lair/utils/tile_map.h>

#include <lair/render_gl2/orthographic_camera.h>
#include <lair/render_gl2/render_pass.h>

#include <lair/ec/entity.h>
#include <lair/ec/entity_manager.h>
#include <lair/ec/sprite_component.h>
#include <lair/ec/bitmap_text_component.h>
#include <lair/ec/tile_layer_component.h>
#include <lair/ec/collision_component.h>


using namespace lair;


class Game;


typedef void (*LevelLogic)(HitEventQueue& hitQueue, EntityRef useEntity);
typedef std::unordered_map<std::string, LevelLogic> LevelLogicMap;


class MainState : public GameState {
public:
	MainState(Game* game);
	virtual ~MainState();

	virtual void initialize();
	virtual void shutdown();

	virtual void run();
	virtual void quit();

	Game* game();

	void startGame();
	void stopGame();

	void updateTick();
	void updateFrame();

	// Game functions

	bool isSolid(TileMap::TileIndex tile) const;
	Vector2i cellCoord(const Vector2& pos, float height) const;
	void computeCollisions();

	EntityRef createTrigger(EntityRef parent, const char* name, const Box2& box);

	// Stuff

	void resizeEvent();

	EntityRef loadEntity(const Path& path, EntityRef parent,
	                     const Path& cd = Path());

	RenderPass* renderPass() { return &_mainPass; }
	SpriteRenderer* spriteRenderer() { return &_spriteRenderer; }

protected:
	// More or less system stuff

	RenderPass                 _mainPass;

	EntityManager              _entities;

	SpriteRenderer             _spriteRenderer;
	SpriteComponentManager     _sprites;
	BitmapTextComponentManager _texts;
	TileLayerComponentManager  _tileLayers;
	CollisionComponentManager  _collisions;
//	AnimationComponentManager  _anims;
	InputManager               _inputs;

	SlotTracker _slotTracker;

	LevelLogicMap _levelLogicMap;

	OrthographicCamera _camera;

	bool       _initialized;
	bool       _running;
	InterpLoop _loop;
	int64      _fpsTime;
	unsigned   _fpsCount;
	uint64     _prevFrameTime;

	Input* _quitInput;
	Input* _restartInput;
	Input* _upInput;
	Input* _leftInput;
	Input* _downInput;
	Input* _rightInput;
	Input* _useInput;

	TileMapSP _tileMap;

	// Models
	EntityRef _models;
	EntityRef _playerModel;

	// Game entities
	EntityRef _world;
	EntityRef _baseLayer;
	EntityRef _player;

	// Game params
	float _playerSpeed;
};


#endif
