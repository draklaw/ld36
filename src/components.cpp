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

#include "components.h"


TriggerComponent::TriggerComponent(Manager* manager, _Entity* entity)
	: Component(manager, entity)
	, prevInside(false)
	, inside(false)
{
}


TriggerComponentManager::TriggerComponentManager()
	: DenseComponentManager<TriggerComponent>("trigger", 128)
{
}


TriggerComponent* TriggerComponentManager::addComponentFromJson(
        EntityRef entity, const Json::Value& json, const Path& cd) {
	TriggerComponent* comp = addComponent(entity);

	comp->onEnter = json.get("on_enter", "").asString();
	comp->onExit  = json.get("on_exit",  "").asString();
	comp->onUse   = json.get("on_use",   "").asString();

	return comp;
}


TriggerComponent* TriggerComponentManager::cloneComponent(EntityRef base, EntityRef entity) {
	TriggerComponent* baseComp = get(base);
	TriggerComponent* comp = _addComponent(entity, baseComp);

	comp->onEnter = baseComp->onEnter;
	comp->onExit  = baseComp->onExit;
	comp->onUse   = baseComp->onUse;

	return comp;
}
