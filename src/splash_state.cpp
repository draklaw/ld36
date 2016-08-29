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

#include "game.h"
#include "main_state.h"

#include "splash_state.h"


#define ONE_SEC (1000000000)


SplashState::SplashState(Game* game)
	: GameState(game),

      _entities(log()),
      _renderPass(renderer()),
      _spriteRenderer(renderer()),
      _sprites(assets(), loader(), &_renderPass, &_spriteRenderer),
      _texts(loader(), &_renderPass, &_spriteRenderer),

      _inputs(sys(), &log()),

      _camera(),

      _initialized(false),
      _running(false),
      _loop(sys()),
      _fpsTime(0),
      _fpsCount(0),

      _skipInput(nullptr),

      _skipTime(1),
      _nextState(nullptr) {

	_entities.registerComponentManager(&_sprites);
	_entities.registerComponentManager(&_texts);
}


SplashState::~SplashState() {
}


void SplashState::initialize() {
	_loop.reset();
	_loop.setTickDuration(    ONE_SEC /  60);
	_loop.setFrameDuration(   ONE_SEC /  60);
	_loop.setMaxFrameDuration(_loop.frameDuration() * 3);
	_loop.setFrameMargin(     _loop.frameDuration() / 2);

	window()->onResize.connect(std::bind(&SplashState::resizeEvent, this))
	        .track(_slotTracker);

	_skipInput = _inputs.addInput("skip");
	_inputs.mapScanCode(_skipInput, SDL_SCANCODE_ESCAPE);

	_splash = _entities.createEntity(_entities.root(), "splash");
	_sprites.addComponent(_splash);
	_splash.place(Vector3(0, 0, 0));

//	EntityRef text = loadEntity("text.json", _entities.root());
//	text.place(Vector3(160, 90, .5));

//	loader()->load<SoundLoader>("sound.ogg");
//	loader()->load<MusicLoader>("shapeout.ogg");

	loader()->waitAll();

	// Set to true to debug OpenGL calls
	renderer()->context()->setLogCalls(false);

	_initialized = true;
}


void SplashState::shutdown() {
	_slotTracker.disconnectAll();

	_initialized = false;
}


void SplashState::run() {
	lairAssert(_initialized);

	log().log("Starting splash state...");
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


void SplashState::quit() {
	game()->setNextState(_nextState);
	_running = false;
}


Game* SplashState::game() {
	return static_cast<Game*>(_game);
}


void SplashState::setup(GameState* nextState, const Path& splashImage, float skipTime) {
	_skipTime = skipTime;
	_nextState = nextState;
	_sprites.get(_splash)->setTexture(splashImage);
	loader()->waitAll();
}


void SplashState::updateTick() {
	_inputs.sync();

	_skipTime -= float(_loop.tickDuration()) / float(ONE_SEC);

	if (_skipTime <= 0
	|| _skipInput->justPressed()
//	|| sys()->getKeyState(SDL_SCANCODE_SPACE)
	|| sys()->getKeyState(SDL_SCANCODE_RETURN)) {
		// ESC quite the game.
		if(sys()->getKeyState(SDL_SCANCODE_ESCAPE))
			_nextState = nullptr;
		quit();
	}

	_entities.updateWorldTransforms();
}


void SplashState::updateFrame() {
	renderer()->uploadPendingTextures();

	// Rendering
	Context* glc = renderer()->context();

	glc->clear(gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT);

	_renderPass.clear();
	_spriteRenderer.clear();

	_sprites.render(_entities.root(), _loop.frameInterp(), _camera);
	_texts.render(  _entities.root(), _loop.frameInterp(), _camera);

	_renderPass.render();

	window()->swapBuffers();
	glc->setLogCalls(false);

	uint64 now = sys()->getTimeNs();
	++_fpsCount;
	if(_fpsCount == 60) {
		log().info("Fps: ", _fpsCount * float(ONE_SEC) / (now - _fpsTime));
		_fpsTime  = now;
		_fpsCount = 0;
	}
}


void SplashState::resizeEvent() {
	Box3 viewBox(Vector3::Zero(),
	             Vector3(1080 * window()->width() / window()->height(),
	                     1080,
	                     1));
	_camera.setViewBox(viewBox);
	renderer()->context()->viewport(0, 0, window()->width(), window()->height());
}


EntityRef SplashState::loadEntity(const Path& path, EntityRef parent, const Path& cd) {
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
