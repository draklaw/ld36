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


#include <functional>

#include <lair/core/json.h>

#include <lair/utils/tile_map.h>

#include "game.h"
#include "splash_state.h"
#include "level.h"
#include "commands.h"

#include "main_state.h"


void dumpEntities(EntityRef entity, int level) {
	dbgLogger.log(std::string(2*level, ' '), entity.name(), ", ", entity.worldTransform()(2, 3));
	EntityRef e = entity.firstChild();
	while(e.isValid()) {
		dumpEntities(e, level + 1);
		e = e.nextSibling();
	}
}


MainState::MainState(Game* game)
	: GameState(game),

      _mainPass(renderer()),

      _entities(log()),

      _spriteRenderer(renderer()),
      _sprites(assets(), loader(), &_mainPass, &_spriteRenderer),
      _texts(loader(), &_mainPass, &_spriteRenderer),
      _tileLayers(&_mainPass, &_spriteRenderer),
      _collisions(),

      _triggers(),

      _inputs(sys(), &log()),

      _camera(),

      _initialized(false),
      _running(false),
      _loop(sys()),
      _fpsTime(0),
      _fpsCount(0),
      _prevFrameTime(0),

      _quitInput    (nullptr),
      _restartInput (nullptr),
      _upInput      (nullptr),
      _leftInput    (nullptr),
      _downInput    (nullptr),
      _rightInput   (nullptr),
      _useInput     (nullptr),

      _playerSpeed(8),
      _playerAnimSpeed(5),
      _fadeTime(.5)
{
	// Register the component manager by decreasing importance. The first ones
	// will be more easily accessible.
	_entities.registerComponentManager(&_sprites);
	_entities.registerComponentManager(&_collisions);
	_entities.registerComponentManager(&_triggers);
	_entities.registerComponentManager(&_texts);
	_entities.registerComponentManager(&_tileLayers);

	_commands["switch"]      = switchDoorCommand;
	_commands["set_door"]    = setDoorCommand;
	_commands["pickup_item"] = pickupItemCommand;
	_commands["message"]     = messageCommand;
	_commands["next_level"]  = nextLevelCommand;
	_commands["teleport"]    = teleportCommand;
	_commands["use_object"]  = useObjectCommand;
	_commands["play_sound"]  = playSoundCommand;
	_commands["continue"]    = continueCommand;
	_commands["fade_in"]     = fadeInCommand;
	_commands["fade_out"]    = fadeOutCommand;
}


MainState::~MainState() {
}


void MainState::initialize() {
	// Set to true to debug OpenGL calls
	renderer()->context()->setLogCalls(false);

	_loop.reset();
	_loop.setTickDuration(  ONE_SEC / TICKRATE);
	_loop.setFrameDuration( ONE_SEC / FRAMERATE);
	_loop.setMaxFrameDuration(_loop.frameDuration() * 3);
	_loop.setFrameMargin(     _loop.frameDuration() / 2);

	window()->onResize.connect(std::bind(&MainState::resizeEvent, this))
	        .track(_slotTracker);

	_quitInput    = _inputs.addInput("quit");
	_restartInput = _inputs.addInput("restart");
	_upInput      = _inputs.addInput("up");
	_leftInput    = _inputs.addInput("left");
	_downInput    = _inputs.addInput("down");
	_rightInput   = _inputs.addInput("right");
	_useInput     = _inputs.addInput("skip");

	_inputs.mapScanCode(_quitInput,    SDL_SCANCODE_ESCAPE);
	_inputs.mapScanCode(_restartInput, SDL_SCANCODE_F5);
	_inputs.mapScanCode(_upInput,      SDL_SCANCODE_W);
	_inputs.mapScanCode(_leftInput,    SDL_SCANCODE_A);
	_inputs.mapScanCode(_downInput,    SDL_SCANCODE_S);
	_inputs.mapScanCode(_rightInput,   SDL_SCANCODE_D);
	_inputs.mapScanCode(_upInput,      SDL_SCANCODE_UP);
	_inputs.mapScanCode(_leftInput,    SDL_SCANCODE_LEFT);
	_inputs.mapScanCode(_downInput,    SDL_SCANCODE_DOWN);
	_inputs.mapScanCode(_rightInput,   SDL_SCANCODE_RIGHT);
	_inputs.mapScanCode(_useInput,     SDL_SCANCODE_SPACE);
	_inputs.mapScanCode(_useInput,     SDL_SCANCODE_RETURN);
	_inputs.mapScanCode(_useInput,     SDL_SCANCODE_RETURN2);
	_inputs.mapScanCode(_useInput,     SDL_SCANCODE_E);
	_inputs.mapScanCode(_useInput,     SDL_SCANCODE_LCTRL);
	_inputs.mapScanCode(_useInput,     SDL_SCANCODE_RCTRL);

	parseJson(_messages, loader()->realFromLogic("text.json"), "text.json", dbgLogger);

	_models = _entities.createEntity(_entities.root(), "models");
	_models.setEnabled(false);

	loader()->load<BitmapFontLoader>("font.json");

	loader()->load<TileMapLoader>("lvl_0.json");

	loader()->load<ImageLoader>("dialog_box.png");

	preloadSound("button.wav");
	preloadSound("door.wav");
	preloadSound("footstep.wav");
	preloadSound("menu.wav");
	preloadSound("radio.wav");
	preloadSound("tp.wav");

	_playerModel = loadEntity("player.json", _models);
	_collisions.get(_playerModel)->setHitMask(HIT_PLAYER_FLAG | HIT_SOLID_FLAG);

	_itemModel = loadEntity("item.json", _models);
	_collisions.get(_itemModel)->setHitMask(HIT_USE_FLAG);

	_itemHudModel = loadEntity("item_hud.json", _models);

	_doorHModel = loadEntity("door_h.json", _models);
	_collisions.get(_doorHModel)->setHitMask(HIT_SOLID_FLAG);

	_doorVModel = loadEntity("door_v.json", _models);
	_collisions.get(_doorVModel)->setHitMask(HIT_SOLID_FLAG);

	loader()->waitAll();

	renderer()->uploadPendingTextures();

	Mix_Volume(-1, 64);

	_initialized = true;
}


void MainState::shutdown() {
	_slotTracker.disconnectAll();

	_initialized = false;
}


void MainState::run() {
	lairAssert(_initialized);

	log().log("Starting main state...");
	_running = true;
	_loop.start();
	_fpsTime  = sys()->getTimeNs();
	_fpsCount = 0;

	do {
		switch(_loop.nextEvent()) {
		case InterpLoop::Tick:
			updateTick();
			break;
		case InterpLoop::Frame:
			updateFrame();
			break;
		}
	} while (_running);
	_loop.stop();
}


void MainState::quit() {
	_running = false;
}


Game* MainState::game() {
	return static_cast<Game*>(_game);
}


void MainState::registerLevel(const Path& path) {
	LevelSP level = std::make_shared<Level>(this, path);
	level->preload();
	_levels.emplace(path, level);
}


void MainState::exec(const std::string& cmd, EntityRef self) {
#define MAX_CMD_ARGS 32

	std::string tokens = cmd;
	unsigned    size   = tokens.size();
	for(int ci = 0; ci < size; ) {
		int   argc = 0;
		const char* argv[MAX_CMD_ARGS];
		while(ci < size) {
			bool endLine = false;
			while(ci < size && std::isspace(tokens[ci])) {
				endLine = tokens[ci] == '\n';
				tokens[ci] = '\0';
				++ci;
			}
			if(endLine)
				break;

			argv[argc] = tokens.data() + ci;
			++argc;

			while(ci < size && !std::isspace(tokens[ci])) {
				++ci;
			}
		}

		if(argc) {
			exec(argc, argv, self);
		}
	}
}


int MainState::exec(int argc, const char** argv, EntityRef self) {
	lairAssert(argc > 0);
	echoCommand(this, self, argc, argv);
	auto cmd = _commands.find(argv[0]);
	if(cmd == _commands.end()) {
		dbgLogger.warning("Unknown command \"", argv[0], "\"");
		return -1;
	}
	return cmd->second(this, self, argc, argv);
}


void MainState::startGame(const Path& firstLevel) {
	if(_world.isValid()) {
		stopGame();
	}

	_messageQueue.clear();
	_postCommand.clear();
	_inventorySlots.clear();

	_world = _entities.createEntity(_entities.root(), "world");

	_player = _entities.cloneEntity(_playerModel, _world);
	_playerDir  = UP;
	_playerAnim = 0;

	for(auto item: _levels) {
		item.second->initialize();
	}

	_hud = _entities.createEntity(_entities.root(), "hud");

	_dialogBox = _entities.createEntity(_hud, "dialog_box");
	_dialogBox.setEnabled(false);
	SpriteComponent* dialogSprite = _sprites.addComponent(_dialogBox);
	dialogSprite->setTexture("dialog_box.png");
	dialogSprite->setAnchor(Vector2(.5, 0));
	dialogSprite->setBlendingMode(BLEND_ALPHA);
	dialogSprite->setTextureFlags(Texture::TRILINEAR | Texture::CLAMP);

	int margin = 30;
	_dialogText = _entities.createEntity(_dialogBox, "dialog_text");
	_dialogText.place(Vector3(margin - 570, 300 - margin - 5, .1));
	BitmapTextComponent* dialogText = _texts.addComponent(_dialogText);
	dialogText->setFont("font.json");
	dialogText->setAnchor(Vector2(0, 1));
	dialogText->setColor(Vector4(.32, .295, .16, 1));
	dialogText->setSize(Vector2i(1140 - 2 * margin, 300 - 2 * margin));

	_overlay = _entities.createEntity(_hud, "overlay");
	SpriteComponent* sc = _sprites.addComponent(_overlay);
	sc->setTexture("white.png");
	sc->setAnchor(Vector2(.5, .5));
	sc->setColor(Vector4(0, 0, 0, 1));
	sc->setBlendingMode(BLEND_ALPHA);

	startLevel(firstLevel);

	dbgLogger.info("Entity count: ", _entities.nEntities(), " (", _entities.nZombieEntities(), " zombies)");
}


void MainState::startLevel(const Path& level, const std::string& spawn) {
	if(_levels.count(level) == 0) {
		registerLevel(level);
		loader()->waitAll();

		AssetSP asset = assets()->getAsset(level);
		TileMapAspectSP aspect = asset? asset->aspect<TileMapAspect>(): nullptr;
		if(!aspect || !aspect->get()) {
			_levels.erase(level);
			dbgLogger.error("Failed to load \"", level, "\".");
			return;
		}

		_levels[level]->initialize();
	}

	if(_level)
		_level->stop();

	_level = _levels[level];
	_level->start(spawn);
}


void MainState::stopGame() {
	_world.destroy();
	_hud.destroy();

	_world.release();
	_player.release();
	_hud.release();
	_dialogBox.release();
	_dialogText.release();
	for(EntityRef& e: _inventorySlots)
		e.release();
}


void MainState::updateTick() {
	_inputs.sync();

	if(_quitInput->justPressed()) {
		quit();
		return;
	}
	if(_restartInput->justPressed()) {
		startGame(game()->firstLevel());
	}
	if (sys()->getKeyState(SDL_SCANCODE_F1)) {
		renderer()->context()->setLogCalls(true);
	}

	_entities.setPrevWorldTransforms();

	if(_state == STATE_PLAY) {
		// Player movement
		Vector2 offset(0, 0);
		if(_upInput->isPressed()) {
			offset(1) += 1;
			_playerDir  = UP;
		}
		if(_leftInput->isPressed()) {
			offset(0) -= 1;
			_playerDir  = LEFT;
		}
		if(_downInput->isPressed()) {
			offset(1) -= 1;
			_playerDir  = DOWN;
		}
		if(_rightInput->isPressed()) {
			offset(0) += 1;
			_playerDir  = RIGHT;
		}

		Vector2 lastPlayerPos = _player.translation2();
		float playerSpeed = _playerSpeed * float(TILE_SIZE) / float(TICKRATE);
		if(!offset.isApprox(Vector2::Zero())) {
			_player.translation2() += offset.normalized() * playerSpeed;
		}

		_level->computeCollisions();

		// Level logic
		HitEventQueue hitQueue;
		_collisions.findCollisions(hitQueue);
//		for(const HitEvent& hit: hitQueue)
//			dbgLogger.debug("hit: ", hit.entities[0].name(), ", ", hit.entities[1].name());

		EntityRef useEntity;
		if(_useInput->justPressed()) {
			std::deque<EntityRef> useQueue;
			Vector2 pos = _player.worldTransform().translation().head<2>();
			_collisions.hitTest(useQueue, pos, HIT_USE_FLAG);

			if(useQueue.empty()) {
				float o = 28;
				Vector2 offset((_playerDir == LEFT)? -o: (_playerDir == RIGHT)? o: 0,
				               (_playerDir == DOWN)? -o: (_playerDir == UP   )? o: 0);
				_collisions.hitTest(useQueue, pos + offset, HIT_USE_FLAG);
			}

			if(!useQueue.empty()) {
				useEntity = useQueue.front();
				dbgLogger.debug("use: ", useEntity.name());
			}
		}

		updateTriggers(hitQueue, useEntity);

		// Bump player
		for(HitEvent& hit: hitQueue) {
			CollisionComponent* cc = _collisions.get(hit.entities[1]);
			if(cc && hit.entities[0] == _player && cc->hitMask() & HIT_SOLID_FLAG) {
				CollisionComponent* pcc = _collisions.get(_player);
				updatePenetration(pcc, pcc->worldAlignedBox(), cc->worldAlignedBox());
			}
		}

		CollisionComponent* pColl = _collisions.get(_player);
		Vector2  bump(0, 0);
		bump(0) += std::max(0.01f + pColl->penetration(LEFT),  0.f);
		bump(0) -= std::max(0.01f + pColl->penetration(RIGHT), 0.f);
		bump(1) += std::max(0.01f + pColl->penetration(DOWN),  0.f);
		bump(1) -= std::max(0.01f + pColl->penetration(UP),    0.f);
		_player.translate(bump);

		if(!_player.translation2().isApprox(lastPlayerPos)) {
			float prevAnim = _playerAnim;
			_playerAnim += _playerAnimSpeed / float(TICKRATE);
			orientPlayer(_playerDir, 1 + int(_playerAnim) % 2);

			if(int(prevAnim) % 2 != int(_playerAnim) % 2)
				playSound("footstep.wav");
		}
		else {
			_playerAnim = 0;
			orientPlayer(_playerDir);
		}
	}
	else if(_state == STATE_FADE_IN || _state == STATE_FADE_OUT) {
		_fadeAnim += 1. / (_fadeTime * float(TICKRATE));
		if(_fadeAnim >= 1)
			setState(STATE_PLAY);
	}

	if(!_messageQueue.empty()) {
		if(_useInput->justPressed()) {
			playSound("menu.wav");
			nextMessage();
		}
	}

	// WARNING: returning early might skip updateWorldTransform.
	_entities.updateWorldTransforms();
}


void MainState::updateFrame() {
//	double time = double(_loop.frameTime()) / double(ONE_SEC);
	double etime = double(_loop.frameTime() - _prevFrameTime) / double(ONE_SEC);

	Vector2 playerPos = _player.interpTransform(_loop.frameInterp()).translation().head<2>();
	Vector2 viewSize(window()->width(), window()->height());
	Box3 viewBox((Vector3() << playerPos - viewSize / 2, 0).finished(),
	             (Vector3() << playerPos + viewSize / 2, 1).finished());
	_camera.setViewBox(viewBox);

	float hudHeight = SCREEN_HEIGHT;
	float hudScale = float(window()->height()) / hudHeight;
	float hudWidth = float(window()->width()) / hudScale;

	Transform hudTrans = Transform::Identity();
	hudTrans.translate(Vector3(viewBox.min()));
	hudTrans.scale(hudScale);
	_hud.place(hudTrans);
	_hud.setPrevWorldTransform();

	_dialogBox.place(Vector3(hudWidth / 2, 40, .8));
	_dialogBox.setPrevWorldTransform();

	_dialogText.updateWorldTransform();
	_dialogText.setPrevWorldTransform();

	_overlay.transform().setIdentity();
	_overlay.transform().translate(Vector3(hudWidth / 2, hudHeight / 2, .7));
	_overlay.transform().scale(Vector3(hudWidth + 2, hudHeight + 2, 1));
	_overlay.updateWorldTransform();
	_overlay.setPrevWorldTransform();

	if(_state == STATE_FADE_IN) {
		setOverlay(1 - _fadeAnim);
	}
	else if(_state == STATE_FADE_OUT) {
		setOverlay(_fadeAnim);
	}

	for(int i=0; i < _inventorySlots.size(); ++i) {
		EntityRef item = _inventorySlots[i];
		item.place(Vector3(48 + 80 * i, hudHeight - 48, 0.6));
		item.updateWorldTransform();
		item.setPrevWorldTransform();
	}

	renderer()->uploadPendingTextures();

	// Rendering
	Context* glc = renderer()->context();

	glc->clearColor(0, 0, 0, 1);
	glc->clear(gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT);

	_mainPass.clear();
	_spriteRenderer.clear();

	_sprites   .render(_entities.root(), _loop.frameInterp(), _camera);
	_texts     .render(_entities.root(), _loop.frameInterp(), _camera);
	_tileLayers.render(_entities.root(), _loop.frameInterp(), _camera);

	_mainPass.render();

	window()->swapBuffers();
	glc->setLogCalls(false);

//	dumpEntities(_entities.root(), 0);

	uint64 now = sys()->getTimeNs();
	++_fpsCount;
	if(_fpsCount == FRAMERATE) {
		log().info("FPS: ", _fpsCount * float(ONE_SEC) / (now - _fpsTime));
		_fpsTime  = now;
		_fpsCount = 0;
	}

	_prevFrameTime = _loop.frameTime();
}


void MainState::updateTriggers(HitEventQueue& hitQueue, EntityRef useEntity, bool disableCmds) {
	_triggers.compactArray();

	if(useEntity.isValid()) {
		TriggerComponent* tc = _triggers.get(useEntity);
		if(tc && !tc->onUse.empty())
			exec(tc->onUse, useEntity);
	}

	for(TriggerComponent& tc: _triggers) {
		if(tc.isEnabled() && tc.entity().isEnabledRec()) {
			tc.prevInside = tc.inside;
			tc.inside = false;
		}
	}

	for(HitEvent& hit: hitQueue) {
		if(hit.entities[1] == _player) {
			std::swap(hit.entities[0], hit.entities[1]);
			std::swap(hit.boxes[0],    hit.boxes[1]);
		}

		if(hit.entities[0] == _player) {
			TriggerComponent* tc = _triggers.get(hit.entities[1]);
			if(tc) {
				tc->inside = true;
			}
		}
	}

	if(!disableCmds) {
		for(TriggerComponent& tc: _triggers) {
			if(tc.isEnabled() && tc.entity().isEnabledRec()) {
				if(!tc.prevInside && tc.inside && !tc.onEnter.empty())
					exec(tc.onEnter, tc.entity());
				if(tc.prevInside && !tc.inside && !tc.onExit.empty())
					exec(tc.onExit, tc.entity());
			}
		}
	}
}


void MainState::enqueueMessage(const std::string& message) {
	_messageQueue.push_back(message);
	if(_messageQueue.size() == 1) {
		_dialogBox.setEnabled(true);
		_texts.get(_dialogText)->setText(message);
		setState(STATE_MESSAGE);
	}
}


void MainState::setState(State state) {
	std::string cmd;
	if(_state != STATE_PLAY) {
		// exec can set post command.
		cmd = _postCommand;
		_postCommand.clear();
	}

	dbgLogger.info("Set state: ", state);
	_state = state;

	if(_state == STATE_FADE_IN || _state == STATE_FADE_OUT)
		_fadeAnim = 0;
	if(_state == STATE_PLAY) {
		_overlay.setEnabled(false);
	}

	if(!cmd.empty())
		exec(cmd);
}


void MainState::setPostCommand(const std::string& command) {
	dbgLogger.info("Set post command: ", command);
	_postCommand = command;
}


void MainState::setPostCommand(int argc, const char** argv) {
	if(argc == 0) {
		_postCommand.clear();
	}
	else {
		std::ostringstream out;
		out << argv[0];
		for(int i = 1; i < argc; ++i)
			out << " " << argv[i];
		setPostCommand(out.str());
	}
}


void MainState::nextMessage() {
	_messageQueue.pop_front();
	bool show = _messageQueue.size();
	_dialogBox.setEnabled(show);
	if(show)
		_texts.get(_dialogText)->setText(_messageQueue.front());
	else {
		setState(STATE_PLAY);
	}
}


bool MainState::hasItem(Item item) {
	for(auto it = _inventorySlots.begin(); it != _inventorySlots.end(); ++it) {
		EntityRef entity = *it;
		if(_sprites.get(entity)->tileIndex() == item) {
			return true;
		}
	}
	return false;
}


void MainState::addToInventory(Item item) {
	dbgLogger.info("Add item ", item);
	EntityRef entity = _entities.cloneEntity(_itemHudModel, _hud);
	_sprites.get(entity)->setTileIndex(item);
	_inventorySlots.push_back(entity);
}


void MainState::removeFromInventory(Item item) {
	for(auto it = _inventorySlots.begin(); it != _inventorySlots.end(); ++it) {
		EntityRef entity = *it;
		if(_sprites.get(entity)->tileIndex() == item) {
			dbgLogger.info("Remove item ", item);
			entity.destroy();
			_inventorySlots.erase(it);
			return;
		}
	}
	lairAssert(false);
}


void MainState::setOverlay(float opacity, const Vector4& color) {
	_overlay.setEnabled(opacity > 0.001);
	Vector4 c = color;
	c(3) = opacity;

	SpriteComponent* overlaySprite = _sprites.get(_overlay);
	overlaySprite->setColor(c);
}


void MainState::preloadSound(const Path& sound) {
	loader()->load<SoundLoader>(sound);
	_soundMap.emplace(sound, _soundMap.size());
}


void MainState::playSound(const Path& sound) {
	int chann = 0;
	if(_soundMap.count(sound))
		chann = _soundMap[sound];
	audio()->playSound(assets()->getAsset(sound), 0, chann);
}


void MainState::orientPlayer(Direction dir, int frame) {
	static int playerTileMap[] = { 9, 3, 0, 6 };

	SpriteComponent* playerSprite = _sprites.get(_player);
	playerSprite->setTileIndex(playerTileMap[dir] + frame);
}


EntityRef MainState::createTrigger(EntityRef parent, const char* name, const Box2& box) {
	EntityRef entity = _entities.createEntity(parent, name);

	CollisionComponent* cc = _collisions.addComponent(entity);
	cc->setShape(Shape::newAlignedBox(box));
	cc->setHitMask(HIT_PLAYER_FLAG | HIT_TRIGGER_FLAG | HIT_USE_FLAG);
	cc->setIgnoreMask(HIT_TRIGGER_FLAG);

	return entity;
}


void MainState::resizeEvent() {
	renderer()->context()->viewport(0, 0, window()->width(), window()->height());
}


EntityRef MainState::loadEntity(const Path& path, EntityRef parent, const Path& cd) {
	Path localPath = make_absolute(cd, path);
	log().info("Load entity \"", localPath, "\"");

	Json::Value json;
	Path realPath = game()->dataPath() / localPath;
	if(!parseJson(json, realPath, localPath, log())) {
		return EntityRef();
	}

	EntityRef entity = _entities.createEntity(parent);
	_entities.initializeFromJson(entity, json, localPath.dir());

	return entity;
}
