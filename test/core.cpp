//
// lager - library for functional interactive c++ programs
// Copyright (C) 2017 Juan Pedro Bolivar Puente
//
// This file is part of lager.
//
// lager is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// lager is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with lager.  If not, see <http://www.gnu.org/licenses/>.
//

#include <catch.hpp>

#include <lager/store.hpp>
#include <lager/event_loop/manual.hpp>

#include "../example/counter/counter.hpp"
#include <optional>

TEST_CASE("basic")
{
    auto viewed = std::optional<counter::model>{std::nullopt};
    auto view   = [&] (auto model) { viewed = model; };
    auto store  = lager::make_store<counter::action>(
        counter::model{},
        counter::update,
        view,
        lager::manual_event_loop{});

    CHECK(viewed);
    CHECK(viewed->value == 0);
    CHECK(store.current().value == 0);

    store.dispatch(counter::increment_action{});
    CHECK(viewed);
    CHECK(viewed->value == 1);
    CHECK(store.current().value == 1);
}
