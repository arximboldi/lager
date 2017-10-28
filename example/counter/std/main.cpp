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

#include "../model.hpp"
#include <lager/store.hpp>
#include <lager/event_loop/manual.hpp>
#include <iostream>

void draw(model::counter c)
{
    std::cout << "current value: " << c.value << '\n';
}

int main()
{
    auto store = lager::make_store<model::action>(
        model::counter{},
        model::update,
        draw,
        lager::manual_event_loop{});

    auto event = char{};
    while (std::cin >> event) {
        switch (event) {
        case '+':
            store.dispatch(model::increment_action{});
            break;
        case '-':
            store.dispatch(model::decrement_action{});
            break;
        case '.':
            store.dispatch(model::reset_action{});
            break;
        case 'q': return 0;
        default:  continue;
        }
    }
}
