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


#ifndef LD36_LEVEL_H
#define LD36_LEVEL_H


#include <map>

#include <lair/core/lair.h>
#include <lair/core/path.h>

#include <lair/utils/tile_map.h>

#include <lair/ec/entity.h>
#include <lair/ec/collision_component.h>


using namespace lair;


class MainState;


bool isSolid(TileMap::TileIndex tile);
Vector2i cellCoord(const Vector2& pos, float height);
void updatePenetration(CollisionComponent* comp, const Box2& objBox, const Box2& otherBox);

Box2 flipY(const Box2& box, float height);


class Level {
public:
	typedef std::unordered_multimap<std::string, EntityRef> EntityMap;
	struct EntityRange;

public:
	Level(MainState* mainState, const Path& path);
	Level(const Level&)  = delete;
	Level(      Level&&) = default;
	virtual ~Level() = default;

	Level& operator=(const Level&)  = delete;
	Level& operator=(      Level&&) = default;

	void preload();
	void initialize();

	void start();
	void stop();

	Box2 objectBox(const Json::Value& obj) const;

	EntityRef createLayer(unsigned index, const char* name);
	EntityRef createTrigger(const Json::Value& obj, const std::string& name);
	EntityRef createItem(const Json::Value& obj, const std::string& name);
	EntityRef createDoor(const Json::Value& obj, const std::string& name);

	const Path& path() { return _path; }
	TileMapSP   tileMap() { return _tileMap; }
	EntityRef   root() { return _levelRoot; }
	EntityRef   entity(const std::string& name);
	EntityRange entities(const std::string& name);

	void computeCollisions();

protected:
	MainState* _mainState;
	Path       _path;
	TileMapSP  _tileMap;

	EntityRef  _levelRoot;
	EntityRef  _baseLayer;
	EntityMap  _entityMap;

	Vector2    _spawnPoint;

public:
	struct EntityRange {
		struct EntityIterator;

		inline EntityRange(EntityMap& map, const std::string& key)
			: _key(key), _begin(this), _end(this) {
			if(!map.bucket_count()) {
				_begin._it = EntityMap::local_iterator();
				_end  ._it = _begin._it;
			}
			else {
				int bucket = map.bucket(_key);
				_begin._it = map.begin(bucket);
				_end  ._it = map.end  (bucket);
				_begin.skip();
			}
		}

		inline EntityIterator begin() { return _begin; }
		inline EntityIterator end()   { return _end; }

		struct EntityIterator {
			typedef EntityRef  value_type;
			typedef ptrdiff_t  difference_type;
			typedef size_t     size_type;
			typedef EntityRef& reference;
			typedef EntityRef* pointer;
			typedef std::forward_iterator_tag iterator_category;

			inline EntityIterator(EntityRange* range)
				: _range(range) {}

			inline void skip() {
				while (_it != _range->_end._it && _it->first != _range->_key)
					++_it;
			}

			inline bool operator==(const EntityIterator& other) const {
				return _range == other._range && _it == other._it;
			}
			inline bool operator!=(const EntityIterator& other) const {
				return !(*this == other);
			}

			inline EntityIterator& operator++() {
				++_it;
				skip();
				return *this;
			}
			inline EntityIterator operator++(int) {
				EntityIterator tmp(*this);
				++(*this);
				return tmp;
			}

			inline const EntityRef& operator*() const {
				return _it->second;
			}
			inline EntityRef& operator*() {
				return _it->second;
			}

			EntityRange*              _range;
			EntityMap::local_iterator _it;
		};

		std::string    _key;
		EntityIterator _begin;
		EntityIterator _end;
	};
};


#endif
