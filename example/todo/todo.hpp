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

#include <fstream>

namespace todo {

struct entry
{
    bool done = false;
    std::string text;
};

using entries = immer::flex_vector<entry>;

struct model
{
    std::string name;
    entries todos;
};

LAGER_CEREAL_STRUCT(todo::entry, (done)(text));
LAGER_CEREAL_STRUCT(todo::model, (name)(todos));

} // namespace todo

BOOST_FUSION_ADAPT_STRUCT(todo::entry, done, text);
BOOST_FUSION_ADAPT_STRUCT(todo::model, name, todos);

namespace todo {

using boost::fusion::operators::operator==;
using boost::fusion::operators::operator!=;

struct action
{};

model update(model m, action a);

void save(const std::string& fname, model todos);
model load(const std::string& fname);

} // namespace todo
