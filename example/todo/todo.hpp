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

#include <lager/debug/cereal/immer_flex_vector.hpp>
#include <lager/debug/cereal/struct.hpp>

#include <immer/flex_vector.hpp>

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/comparison.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>

#include <variant>

namespace todo {

struct item
{
    bool done = false;
    std::string text;
};
LAGER_CEREAL_STRUCT(todo::item, (done)(text));

struct toggle_item_action
{};
struct remove_item_action
{};
using item_action = std::variant<toggle_item_action, remove_item_action>;

item update_item(item m, item_action a);

struct model
{
    std::string name;
    immer::flex_vector<item> todos;
};
LAGER_CEREAL_STRUCT(todo::model, (name)(todos));

struct add_todo_action
{
    std::string text;
};

using action =
    std::variant<add_todo_action, std::pair<std::size_t, item_action>>;

model update(model m, action a);

model save(const std::string& fname, model todos);
model load(const std::string& fname);

using boost::fusion::operators::operator==;
using boost::fusion::operators::operator!=;

} // namespace todo

BOOST_FUSION_ADAPT_STRUCT(todo::item, done, text);
BOOST_FUSION_ADAPT_STRUCT(todo::model, name, todos);
