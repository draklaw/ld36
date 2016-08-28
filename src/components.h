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


#ifndef LD36_COMPONENTS_H
#define LD36_COMPONENTS_H


#include <map>

#include <lair/core/lair.h>
#include <lair/core/path.h>

#include <lair/ec/entity.h>
#include <lair/ec/component.h>
#include <lair/ec/dense_component_manager.h>


using namespace lair;


class TriggerComponentManager;

class TriggerComponent : public Component {
public:
	typedef TriggerComponentManager Manager;

public:
	TriggerComponent(Manager* manager, _Entity* entity);
	TriggerComponent(const TriggerComponent&)  = delete;
	TriggerComponent(      TriggerComponent&&) = default;
	~TriggerComponent() = default;

	TriggerComponent& operator=(const TriggerComponent&)  = delete;
	TriggerComponent& operator=(      TriggerComponent&&) = default;

public:
	bool        prevInside;
	bool        inside;
	std::string onEnter;
	std::string onExit;
	std::string onUse;
};

class TriggerComponentManager : public DenseComponentManager<TriggerComponent> {
public:
	TriggerComponentManager();

	virtual TriggerComponent* addComponentFromJson(EntityRef entity, const Json::Value& json,
	                                  const Path& cd=Path());
	virtual TriggerComponent* cloneComponent(EntityRef base, EntityRef entity);
};


#endif
