//
// lager - library for functional interactive c++ programs
//
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

#include "../counter.hpp"
#include <lager/store.hpp>
#include <lager/event_loop/manual.hpp>
#include <iostream>

void draw(counter::model c)
{
    std::cout << "current value: " << c.value << '\n';
}

std::optional<counter::action> intent(char event)
{
    switch (event) {
    case '+':
        return counter::increment_action{};
    case '-':
        return counter::decrement_action{};
    case '.':
        return counter::reset_action{};
    default:
        return std::nullopt;
    }
}

int main()
{
    auto store = lager::make_store<counter::action>(
        counter::model{},
        counter::update,
        draw,
        lager::manual_event_loop{});

    auto event = char{};
    while (std::cin >> event) {
        if (auto act = intent(event))
            store.dispatch(*act);
    }
}
