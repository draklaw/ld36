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

#include "main_state.h"


MainState::MainState(Game* game)
	: GameState(game),

      _mainPass(renderer()),

      _entities(log()),

      _spriteRenderer(renderer()),
      _sprites(assets(), loader(), &_mainPass, &_spriteRenderer),
      _texts(loader(), &_mainPass, &_spriteRenderer),
      _tileLayers(&_mainPass, &_spriteRenderer),
      _collisions(),

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

      _playerSpeed(5)
{
	// Register the component manager by decreasing importance. The first ones
	// will be more easily accessible.
	_entities.registerComponentManager(&_sprites);
	_entities.registerComponentManager(&_collisions);
	_entities.registerComponentManager(&_texts);
	_entities.registerComponentManager(&_tileLayers);
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

	registerLevel("lvl_0.json");

	_models = _entities.createEntity(_entities.root(), "models");
	_models.setEnabled(false);

	loader()->load<BitmapFontLoader>("droid_sans_24.json");

	loader()->load<TileMapLoader>("lvl_0.json");

	loader()->load<ImageLoader>("dialog_box.png");

	_playerModel = loadEntity("player.json", _models);
	_collisions.get(_playerModel)->setHitMask(HIT_PLAYER_FLAG);

	_itemModel = loadEntity("item.json", _models);
	_collisions.get(_itemModel)->setHitMask(HIT_PLAYER_FLAG | HIT_TRIGGER_FLAG | HIT_USE_FLAG);

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


void MainState::startGame() {
	if(_world.isValid()) {
		stopGame();
	}

	_messageQueue.clear();
	_inventorySlots.clear();

	_world = _entities.createEntity(_entities.root(), "world");

	_player = _entities.cloneEntity(_playerModel, _world);
	_player.place(Vector3(70, 70, .1));

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
	dialogText->setFont("droid_sans_24.json");
	dialogText->setAnchor(Vector2(0, 1));
	dialogText->setColor(Vector4(.32, .295, .16, 1));
	dialogText->setSize(Vector2i(1140 - 2 * margin, 300 - 2 * margin));

	startLevel("lvl_0.json");
}


void MainState::startLevel(const Path& level) {
	if(_level)
		_level->stop();

	_level = _levels[level];
	_level->start();
}


void MainState::stopGame() {
	_world.destroy();

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
		startGame();
	}

	if(_messageQueue.empty()) {
		// Player movement
		Vector2 offset(0, 0);
		if(_upInput->isPressed())
			offset(1) += 1;
		if(_leftInput->isPressed())
			offset(0) -= 1;
		if(_downInput->isPressed())
			offset(1) -= 1;
		if(_rightInput->isPressed())
			offset(0) += 1;

		float playerSpeed = _playerSpeed * float(TILE_SIZE) / float(TICKRATE);
		if(!offset.isApprox(Vector2::Zero())) {
			_player.translation2() += offset.normalized() * playerSpeed;
		}

		_level->computeCollisions();

		CollisionComponent* pColl = _collisions.get(_player);
		Vector2  bump(0, 0);
		bump(0) += std::max(0.01f + pColl->penetration(LEFT),  0.f);
		bump(0) -= std::max(0.01f + pColl->penetration(RIGHT), 0.f);
		bump(1) += std::max(0.01f + pColl->penetration(DOWN),  0.f);
		bump(1) -= std::max(0.01f + pColl->penetration(UP),    0.f);
		_player.translate(bump);

		// WARNING: returning early might skip updateWorldTransform.
		// Update world transform before level logic so that collision match
		_entities.updateWorldTransform();

		// Level logic
		HitEventQueue hitQueue;
		_collisions.findCollisions(hitQueue);
	//	for(const HitEvent& hit: hitQueue)
	//		dbgLogger.debug("hit: ", hit.entities[0].name(), ", ", hit.entities[1].name());

		EntityRef useEntity;
		if(_useInput->justPressed()) {
			std::deque<EntityRef> useQueue;
			_collisions.hitTest(useQueue, _player.worldTransform().translation().head<2>(), HIT_USE_FLAG);

			if(!useQueue.empty()) {
				useEntity = useQueue.front();
				dbgLogger.debug("use: ", useEntity.name());
			}
		}
	}
	else {
		if(_useInput->justPressed())
			nextMessage();

		_entities.updateWorldTransform();
	}
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
	_hud.resetPrevWorldTransform();

	_dialogBox.place(Vector3(hudWidth / 2, 40, .8));
	_dialogBox.resetPrevWorldTransform();

	_dialogText.updateWorldTransform();
	_dialogText.resetPrevWorldTransform();

	for(int i=0; i < _inventorySlots.size(); ++i) {
		EntityRef item = _inventorySlots[i];
		item.place(Vector3(48 + 80 * i, hudHeight - 48, 0.8));
		item.updateWorldTransform();
		item.resetPrevWorldTransform();
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

	uint64 now = sys()->getTimeNs();
	++_fpsCount;
	if(_fpsCount == FRAMERATE) {
		log().info("FPS: ", _fpsCount * float(ONE_SEC) / (now - _fpsTime));
		_fpsTime  = now;
		_fpsCount = 0;
	}

	_prevFrameTime = _loop.frameTime();
}


void MainState::enqueueMessage(const std::string& message) {
	_messageQueue.push_back(message);
	if(_messageQueue.size() == 1) {
		_dialogBox.setEnabled(true);
		_texts.get(_dialogText)->setText(message);
	}
}


void MainState::nextMessage() {
	_messageQueue.pop_front();
	bool show = _messageQueue.size();
	_dialogBox.setEnabled(show);
	if(show)
		_texts.get(_dialogText)->setText(_messageQueue.front());
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
	EntityRef entity = _entities.cloneEntity(_itemModel, _hud);
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
