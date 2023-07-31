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

#include "test/cereal/cerealize.hpp"
#include <catch2/catch.hpp>

#include <lager/extra/cereal/enum.hpp>
#include <lager/extra/enum.hpp>

namespace ns {
enum class kinds
{
    good,
    bad
};
} // namespace ns

LAGER_ENUM(ns, kinds, good, bad);

TEST_CASE("basic")
{
    {
        auto x = ns::kinds::good;
        auto y = cerealize(x);
        CHECK(x == y);
        CHECK(x == ns::kinds::good);
    }
    {
        auto x = ns::kinds::bad;
        auto y = cerealize(x);
        CHECK(x == y);
        CHECK(x == ns::kinds::bad);
    }
}

TEST_CASE("to_string")
{
    CHECK(lager::to_string(ns::kinds::good) == std::string{"good"});
    CHECK(lager::to_string(ns::kinds::bad) == std::string{"bad"});
}

TEST_CASE("to_enum")
{
    CHECK(lager::to_enum<ns::kinds>("good") == ns::kinds::good);
    CHECK(lager::to_enum<ns::kinds>("bad") == ns::kinds::bad);
}
