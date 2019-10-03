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

#include <immer/flex_vector.hpp>

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/comparison.hpp>

using namespace boost::fusion::operators;

struct todo
{
    bool done = false;
    std::string text;

    bool operator==(const todo& x) const
    {
        return done == x.done && text == x.text;
    }

    bool operator!=(const todo& x) const { return !(*this == x); }
};

struct model
{
    std::string name;
    std::vector<todo> todos;
};

BOOST_FUSION_ADAPT_STRUCT(model, name, todos)
