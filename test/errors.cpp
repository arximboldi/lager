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

#include <catch.hpp>

#include <lager/deps.hpp>
#include <lager/store.hpp>

#include "../example/counter/counter.hpp"

//
// Uncomment some of this to see what errors messages one gets
//
TEST_CASE("effect with bad multiple actions")
{
    // (void) lager::effect<counter::increment_action,
    // counter::decrement_action>{
    //     [](auto&& ctx) { ctx.dispatch(counter::increment_action{}); }};
}
