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

#include "cerealize.hpp"

#include <lager/extra/cereal/immer_table.hpp>

#include <catch2/catch.hpp>

struct entry_t
{
    size_t id;
    size_t value;

    bool operator==(const entry_t& other) const
    {
        return id == other.id && value == other.value;
    }

    template <typename Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(id), CEREAL_NVP(value));
    }
};

TEST_CASE("basic")
{
    auto x = immer::table<entry_t>{{.id = 1, .value = 2},
                                   {.id = 3, .value = 4},
                                   {.id = 12, .value = 42},
                                   {.id = 0, .value = 5}};
    auto y = cerealize(x);
    CHECK(x == y);
}
