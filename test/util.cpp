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

#include <catch2/catch.hpp>

#include <lager/util.hpp>

#include <variant>

using variant_t = std::variant<int, float, std::string>;

TEST_CASE("match")
{
    auto v = variant_t{42};
    auto y = lager::match(v)([](int x) { return x; }, [](auto) { return 0; });
    CHECK(y == 42);
}

struct deriv_t : variant_t
{
    using variant_t::variant_t;
};

TEST_CASE("match deriv")
{
    auto v = deriv_t{42};
    auto y = lager::match(v)([](int x) { return x; }, [](auto) { return 0; });
    CHECK(y == 42);
}
