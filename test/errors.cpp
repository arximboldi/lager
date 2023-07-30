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

#include <lager/deps.hpp>
#include <lager/store.hpp>

#include "../example/counter/counter.hpp"

//
// Uncomment some of this to see what errors messages one gets
//
TEST_CASE("effect with bad multiple actions")
{
    //(void) lager::effect<counter::increment_action,
    // counter::decrement_action>{
    //    [](auto&& ctx) { ctx.dispatch(counter::increment_action{}); }};
}

TEST_CASE("allow context of void")
{
    auto ctx = lager::context<>{};
    // ctx.dispatch();
}

struct foo
{};

TEST_CASE("result conversion")
{
    // bad model:
    // lager::result<int>{lager::result<std::string>{""};}

    // bad actions:
    // lager::result<int>{lager::result<int, int>{0}};

    // bad deps:
    // lager::result<int>{lager::result<int, void, lager::deps<foo>>{0}};

    // from effect
    // lager::result<int>{0, lager::effect<int>{}};
    // lager::result<int>{0, lager::effect<void, lager::deps<int>>{0}};
}
