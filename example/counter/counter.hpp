//
// lager - library for functional interactive c++ programs
// Copyright (C) 2017 Juan Pedro Bolivar Puente
//
// This file is part of lager.
//
// lager is free software: you can redistribute it and/or modify
// it under the terms of the MIT License, as detailed in the LICENSE
// file located at the root of this source code distribution,
// or here: <https://github.com/arximboldi/lager/blob/master/LICENSE>
//

#include <lager/debug/cereal/struct.hpp>
#include <lager/util.hpp>

#include <variant>

namespace counter {

struct model
{
    int value = 0;
};

struct increment_action
{};
struct decrement_action
{};
struct reset_action
{
    int new_value = 0;
};

using action = std::variant<increment_action, decrement_action, reset_action>;

inline model update(model c, action action)
{
    return std::visit(lager::visitor{
                          [&](increment_action) { return model{c.value + 1}; },
                          [&](decrement_action) { return model{c.value - 1}; },
                          [&](reset_action a) { return model{a.new_value}; },
                      },
                      action);
}

LAGER_CEREAL_STRUCT(model, (value));
LAGER_CEREAL_STRUCT(increment_action);
LAGER_CEREAL_STRUCT(decrement_action);
LAGER_CEREAL_STRUCT(reset_action, (new_value));

} // namespace counter
