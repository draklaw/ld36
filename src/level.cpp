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
#include "commands.h"

#include "level.h"


bool isSolid(TileMap::TileIndex tile) {
	unsigned x = (tile - 1) % TILE_SET_WIDTH;
	return x >= TILE_SET_WIDTH / 2;
}


Vector2i cellCoord(const Vector2& pos, float height) {
	return Vector2i(pos(0) / TILE_SIZE, height - pos(1) / TILE_SIZE);
}


void updatePenetration(CollisionComponent* comp, const Box2& objBox, const Box2& otherBox) {
	Vector2 offset = otherBox.center() - objBox.center();
	Vector2 hsizes = (objBox.sizes() + otherBox.sizes()) / 2;

	Box2 inter = objBox.intersection(otherBox);

	bool hitX = objBox  .max()(0) > otherBox.min()(0)
	         && otherBox.max()(0) > objBox  .min()(0);
	bool hitY = objBox  .max()(1) > otherBox.min()(1)
	         && otherBox.max()(1) > objBox  .min()(1);

	if(hitY && (inter.isEmpty() || inter.sizes()(0) < inter.sizes()(1))) {
		if(offset(0) < 0) {
			float p = hsizes(0) + offset(0);
			comp->setPenetration(LEFT,  std::max(comp->penetration(LEFT),  p));
		}
		else {
			float p = hsizes(0) - offset(0);
			comp->setPenetration(RIGHT, std::max(comp->penetration(RIGHT), p));
		}
	}
	if(hitX && (inter.isEmpty() || inter.sizes()(1) < inter.sizes()(0))) {
		if(offset(1) < 0) {
			float p = hsizes(1) + offset(1);
			comp->setPenetration(DOWN,  std::max(comp->penetration(DOWN),  p));
		}
		else {
			float p = hsizes(1) - offset(1);
			comp->setPenetration(UP,    std::max(comp->penetration(UP),    p));
		}
	}
}



Box2 flipY(const Box2& box, float height) {
	Vector2 min = box.min();
	Vector2 max = box.max();
	min(1) = height - min(1);
	max(1) = height - max(1);
	std::swap(min(1), max(1));
	return Box2(min, max);
}



Level::Level(MainState* mainState, const Path& path)
	: _mainState(mainState)
	, _path(path)
{
}


void Level::preload() {
	_mainState->loader()->load<TileMapLoader>(_path);
}


void Level::initialize() {
	dbgLogger.info("Initialize level ", _path);

	AssetSP asset = _mainState->assets()->getAsset(_path);
	lairAssert(asset);

	TileMapAspectSP aspect = asset->aspect<TileMapAspect>();
	lairAssert(aspect);

	_tileMap = aspect->get();
	lairAssert(_tileMap);

	_entityMap.clear();
	if(_levelRoot.isValid())
		_levelRoot.destroy();
	_levelRoot = _mainState->_entities.createEntity(_mainState->_world, _path.utf8CStr());
	_levelRoot.setEnabled(false);

	_baseLayer = createLayer(0, "layer_base");

	for(int oli = 0; oli < _tileMap->nObjectLayer(); ++oli) {
		for(const Json::Value& obj: _tileMap->objectLayer(oli)["objects"]) {
			std::string type = obj.get("type", "<no_type>").asString();
			std::string name = obj.get("name", "<no_name>").asString();

			EntityRef entity;
			if(type == "trigger") {
				entity = createTrigger(obj, name);
			}
			else if(type == "item") {
				entity = createItem(obj, name);
			}
			else if(type == "door") {
				entity = createDoor(obj, name);
			}
			else if(type == "spawn") {
				_spawnPoint = objectBox(obj).center();
			}

			if(!entity.isValid() && type != "spawn")
				dbgLogger.warning(_path, ": Failed to load entity \"", name, "\" of type \"", type, "\"");
			else
				_entityMap.emplace(name, entity);
		}
	}
}


void Level::start() {
	dbgLogger.info("Start level ", _path);
	_levelRoot.setEnabled(true);
	_mainState->_player.place((Vector3() << _spawnPoint, .1).finished());
	_mainState->_playerDir = UP;
	_mainState->_playerAnim = 0;

	HitEventQueue hitQueue;
	_mainState->_collisions.findCollisions(hitQueue);
	_mainState->updateTriggers(hitQueue, EntityRef(), true);
}


void Level::stop() {
	dbgLogger.info("Stop level ", _path);
	_levelRoot.setEnabled(false);
}


Box2 Level::objectBox(const Json::Value& obj) const {
	try {
		Json::Value props = obj["properties"];

		Vector2 min(obj["x"].asFloat(),
		            obj["y"].asFloat());
		Vector2 max(min(0) + obj["width"] .asFloat(),
		            min(1) + obj["height"].asFloat());

		float height = _tileMap->height(0) * TILE_SIZE;
		return flipY(Box2(min, max), height);
	}
	catch(Json::Exception& e) {
		dbgLogger.error(_path, ": Json error while loading: ", e.what());
		return Box2(Vector2(0, 0), Vector2(0, 0));
	}
}


EntityRef Level::createLayer(unsigned index, const char* name) {
	EntityRef layer = _mainState->_entities.createEntity(_levelRoot, name);
	TileLayerComponent* lc = _mainState->_tileLayers.addComponent(layer);
	lc->setTileMap(_tileMap);
	lc->setLayerIndex(index);
	layer.place(Vector3(0, 0, .01 * index));
	return layer;
}


EntityRef Level::createTrigger(const Json::Value &obj, const std::string& name) {
	Json::Value props = obj.get("properties", Json::Value());

	Box2 box = objectBox(obj);
	float margin = props.get("margin", 0).asFloat();
	Vector2 half = box.sizes() / 2 + Vector2(margin, margin);
	Box2 hitBox(-half, half);

	EntityRef entity = _mainState->createTrigger(_levelRoot, name.c_str(), hitBox);
	entity.place((Vector3() << box.center(), 0.08).finished());


	TriggerComponent* tc = _mainState->_triggers.addComponent(entity);
	tc->onEnter = props.get("on_enter", "").asString();
	tc->onExit  = props.get("on_exit",  "").asString();
	tc->onUse   = props.get("on_use",   "").asString();

	std::string sprite = props.get("sprite", "").asString();
	if(!sprite.empty()) {
		SpriteComponent* sc = _mainState->_sprites.addComponent(entity);
		sc->setTexture(sprite);
		sc->setTileIndex(props.get("tile_index", 0).asInt());
		
		int tileH = props.get("tile_h", 4).asInt();
		int tileV = props.get("tile_v", 2).asInt();
		sc->setTileGridSize(Vector2i(tileH, tileV));
		sc->setAnchor(Vector2(.5, .5));
		sc->setBlendingMode(BLEND_ALPHA);
	}

	return entity;
}


EntityRef Level::createItem(const Json::Value& obj, const std::string& name) {
	Json::Value props = obj.get("properties", Json::Value());
	Box2 box  = objectBox(obj);
	int  item = props.get("item", 0).asInt();

	EntityRef entity = _mainState->_entities.cloneEntity(_mainState->_itemModel, _levelRoot, name.c_str());
	entity.place((Vector3() << box.center(), .09).finished());

	SpriteComponent* sc = _mainState->_sprites.get(entity);
	sc->setTileIndex(item);

	return entity;
}


EntityRef Level::createDoor(const Json::Value& obj, const std::string& name) {
	Json::Value props = obj.get("properties", Json::Value());
	Box2 box = objectBox(obj);
	bool horizontal = props.get("horizontal", true).asBool();
	bool open = props.get("open", false).asBool();

	EntityRef model = horizontal? _mainState->_doorHModel: _mainState->_doorVModel;
	EntityRef entity = _mainState->_entities.cloneEntity(model, _levelRoot, name.c_str());

	entity.place((Vector3() << box.min(), .2).finished());
	setDoorOpen(_mainState, entity, open);

	return entity;
}


EntityRef Level::entity(const std::string& name) {
	EntityRange range = entities(name);
	if(range.begin() == range.end()) {
		dbgLogger.warning("Level::entity(\"", name, "\"): Entity not found.");
		return EntityRef();
	}
	auto it = range.begin();
	++it;
	if(it != range.end())
		dbgLogger.warning("Level::entity(\"", name, "\"): More than one entity found.");
	return *range.begin();
}


Level::EntityRange Level::entities(const std::string& name) {
	return EntityRange(_entityMap, name);
}


void Level::computeCollisions() {
	// FIXME: The character can be stuck while sliding against a wall. Compute
	// collision against thin walls.

	EntityRef entity = _mainState->_player;

	CollisionComponent* cc = _mainState->_collisions.get(entity);
	if(!cc->isEnabled())
		return;
	lairAssert(cc && cc->shape() && cc->shape()->type() == SHAPE_ALIGNED_BOX);

	TileLayerComponent* tlc = _mainState->_tileLayers.get(_baseLayer);
	lairAssert(tlc && tlc->tileMap());
	unsigned layer = tlc->layerIndex();

	for(int i = 0; i < N_DIRECTIONS; ++i)
		cc->setPenetration(Direction(i), -TILE_SIZE);

	Vector2 pos  = entity.computeWorldTransform().translation().head<2>();
	Box2    realBox(pos + cc->shape()->point(0), pos + cc->shape()->point(1));
	Box2i box(cellCoord(realBox.corner(Box2::TopLeft),     _tileMap->height(layer)),
	          cellCoord(realBox.corner(Box2::BottomRight), _tileMap->height(layer)));

//	Vector2 lpos = _baseLayer.worldTransform().translation().head<2>();
	int width  = _tileMap->width(layer);
	int height = _tileMap->height(layer);
	int beginX = std::max(box.min()(0) - 1, 0);
	int endX   = std::min(box.max()(0) + 2, width);
	int beginY = std::max(box.min()(1) - 1, 0);
	int endY   = std::min(box.max()(1) + 2, height);
	for(int y = beginY; y < endY; ++y) {
		for(int x = beginX; x < endX; ++x) {
			if(isSolid(_tileMap->tile(x, y, layer))) {
				Box2 tileBox(Vector2(x, height - y - 1) * TILE_SIZE,
				             Vector2(x + 1, height - y) * TILE_SIZE);
				updatePenetration(cc, realBox, tileBox);
			}
		}
	}
}

