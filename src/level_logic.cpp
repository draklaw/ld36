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


#include <lair/core/json.h>

#include "level_logic.h"


void fillLevelLogicMap(LevelLogicMap& map) {
	map["test"] = test_logic;
}


void test_logic(MainState& state, HitEventQueue& hitQueue, EntityRef useEntity) {
	if(useEntity.isValid() && std::strcmp(useEntity.name(), "test_trigger") == 0) {
		state.enqueueMessage("Sit possimus sed dignissimos necessitatibus quia asperiores. Soluta rem ut quia quia autem sequi nemo eius. Doloribus et recusandae exercitationem at laboriosam qui. Facere sed quas et quasi. Voluptates qui est omnis. Sit ut perferendis consequatur est distinctio et.");
		state.enqueueMessage("Incidunt mollitia eos molestias laudantium. Eos et ratione ullam commodi ratione ea. Inventore eius vel ut. Error hic minima laborum est numquam ut et. Quisquam deserunt repellendus et esse. Facilis iste animi explicabo.");
		state.enqueueMessage("Enim vel et rerum aut vero accusamus. Nam beatae perferendis nemo deserunt modi. Consequatur aut quia provident aliquid unde velit. Eligendi illo nesciunt ut quia dolores. Vero eius fugiat repellat et quas non. Quo non reprehenderit fugit illo occaecati.");
	}
}
