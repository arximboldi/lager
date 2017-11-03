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
#include <lager/util.hpp>
#include <lager/debug/cereal/struct.hpp>

namespace counter {

struct model
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

model update(model c, action action)
{
    return std::visit(lager::visitor{
            [&] (increment_action) {
                return model{ c.value + 1 };
            },
            [&] (decrement_action) {
                return model{ c.value - 1 };
            },
            [&] (reset_action a) {
                return model{ a.new_value };
            },
        }, action);
}

LAGER_CEREAL_STRUCT(model, (value));
LAGER_CEREAL_STRUCT(increment_action);
LAGER_CEREAL_STRUCT(decrement_action);
LAGER_CEREAL_STRUCT(reset_action, (new_value));

} // namespace counter
