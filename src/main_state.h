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

#include <lair/sys_sdl2/audio_module.h>

#include <lair/render_gl2/orthographic_camera.h>
#include <lair/render_gl2/render_pass.h>

#include <lair/ec/entity.h>
#include <lair/ec/entity_manager.h>
#include <lair/ec/sprite_component.h>
#include <lair/ec/bitmap_text_component.h>
#include <lair/ec/tile_layer_component.h>
#include <lair/ec/collision_component.h>

#include "components.h"


#define ONE_SEC (1000000000)

#define SCREEN_WIDTH  1920
#define SCREEN_HEIGHT 1080

#define FRAMERATE 60
#define TICKRATE  60

#define TILE_SIZE       48
#define TILE_SET_WIDTH  12
#define TILE_SET_HEIGHT 12

#define HIT_PLAYER_FLAG  0x01
#define HIT_TRIGGER_FLAG 0x02
#define HIT_USE_FLAG     0x04
#define HIT_SOLID_FLAG   0x08


using namespace lair;


class Game;
class MainState;
class Level;
typedef std::shared_ptr<Level> LevelSP;


enum Item {
	ITEM_MAN,
	ITEM_CARD,
	ITEM_ARTEFACT,
	ITEM_WRENCH,
	ITEM_CABLE,
	ITEM_CHIP,
	ITEM_GROUP,
	ITEM_RADIO,
	ITEM_COG,
	ITEM_CORKSCREW,
	ITEM_BG,
};

enum State {
	STATE_PLAY,
	STATE_MESSAGE,
	STATE_FADE_IN,
	STATE_FADE_OUT,
};

enum EndingState {
	END_BOCAL_OFF,
	END_BOCAL_ON,
	END_SAVE,
	END_KILL,
};


typedef std::unordered_map<Path, LevelSP, boost::hash<Path>> LevelMap;

typedef int (*Command)(MainState* state, EntityRef self, int argc, const char** argv);
typedef std::unordered_map<std::string, Command> CommandMap;

typedef std::unordered_map<Path, int, boost::hash<Path>> SoundMap;


class MainState : public GameState {
public:
	MainState(Game* game);
	virtual ~MainState();

	virtual void initialize();
	virtual void shutdown();

	virtual void run();
	virtual void quit();

	Game* game();

	void registerLevel(const Path& path);
	void exec(const std::string& cmd, EntityRef self = EntityRef());
	int exec(int argc, const char** argv, EntityRef self = EntityRef());

	void startGame(const Path& firstLevel);
	void startLevel(const Path& level, const std::string& spawn = "spawn");
	void stopGame();

	void updateTick();
	void updateFrame();

	void updateTriggers(HitEventQueue& hitQueue, EntityRef useEntity, bool disableCmds = false);

	// Game functions

	void setState(State state);

	void setPostCommand(const std::string& command);
	void setPostCommand(int argc, const char** argv);

	void popupMessage(const std::string& key);
	void enqueueMessage(const std::string& message);
	void nextMessage();

	bool hasItem(Item item);
	void addToInventory(Item item);
	void removeFromInventory(Item item);

	void setOverlay(float opacity, const Vector4& color = Vector4(0, 0, 0, 1));

	void preloadSound(const Path& sound);
	void playSound(const Path& sound);

	void orientPlayer(Direction dir, int frame = 0);

	EntityRef createTrigger(EntityRef parent, const char* name, const Box2& box);

	// Stuff

	void resizeEvent();

	EntityRef loadEntity(const Path& path, EntityRef parent,
	                     const Path& cd = Path());

	RenderPass* renderPass() { return &_mainPass; }
	SpriteRenderer* spriteRenderer() { return &_spriteRenderer; }

public:
	// More or less system stuff

	RenderPass                 _mainPass;

	EntityManager              _entities;

	SpriteRenderer             _spriteRenderer;
	SpriteComponentManager     _sprites;
	BitmapTextComponentManager _texts;
	TileLayerComponentManager  _tileLayers;
	CollisionComponentManager  _collisions;

	TriggerComponentManager    _triggers;
//	AnimationComponentManager  _anims;

	InputManager               _inputs;

	SlotTracker _slotTracker;

	State       _state;
	CommandMap  _commands;
	Json::Value _messages;
	std::string _postCommand;
	std::deque<std::string> _messageQueue;
	OrthographicCamera _camera;
	SoundMap _soundMap;

	EndingState _endingState;

	bool       _initialized;
	bool       _running;
	InterpLoop _loop;
	int64      _fpsTime;
	unsigned   _fpsCount;
	uint64     _prevFrameTime;
	float      _fadeAnim;

	Input* _quitInput;
	Input* _restartInput;
	Input* _upInput;
	Input* _leftInput;
	Input* _downInput;
	Input* _rightInput;
	Input* _useInput;

	LevelMap  _levels;
	LevelSP   _level;

	// Models
	EntityRef _models;
	EntityRef _playerModel;
	EntityRef _itemModel;
	EntityRef _itemHudModel;
	EntityRef _doorHModel;
	EntityRef _doorVModel;

	// Game entities
	EntityRef _world;
	EntityRef _player;
	Direction _playerDir;
	float     _playerAnim;

	// HUD entities
	EntityRef _hud;
	EntityRef _dialogBox;
	EntityRef _dialogText;
	std::vector<EntityRef> _inventorySlots;
	EntityRef _overlay;

	// Game params
	float _playerSpeed;
	float _playerAnimSpeed;
	float _fadeTime;
};


#endif
