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

#include <variant>
#include "util.hpp"

namespace model {

struct counter
{
    int value = 0;
};

struct increment_action {};
struct decrement_action {};
struct reset_action { int new_value = 0; };

using action = std::variant<
    increment_action,
    decrement_action,
    reset_action>;

counter update(counter c, action action)
{
    return std::visit(util::visitor{
            [&] (increment_action) {
                return counter { c.value + 1 };
            },
            [&] (decrement_action) {
                return counter { c.value - 1 };
            },
            [&] (reset_action a) {
                return counter { a.new_value };
            },
        }, action);
}

} // namespace model
