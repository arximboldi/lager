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

#pragma once

#include <lager/extra/struct.hpp>

#include <string>
#include <variant>

namespace todo {

struct item
{
    bool done = false;
    std::string text;
};

struct toggle_item_action
{};
struct remove_item_action
{};
using item_action = std::variant<toggle_item_action, remove_item_action>;

item update(item m, item_action a);

} // namespace todo

LAGER_STRUCT(todo, item, done, text);
