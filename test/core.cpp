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

#include <lager/store.hpp>
#include <lager/event_loop/manual.hpp>

#include "../example/counter/counter.hpp"
#include <optional>

TEST_CASE("basic")
{
    auto viewed = std::optional<counter::model>{std::nullopt};
    auto view   = [&] (auto model) { viewed = model; };
    auto store  = lager::make_store<counter::action>(
        counter::model{},
        counter::update,
        view,
        lager::manual_event_loop{});

    CHECK(viewed);
    CHECK(viewed->value == 0);
    CHECK(store.current().value == 0);

    store.dispatch(counter::increment_action{});
    CHECK(viewed);
    CHECK(viewed->value == 1);
    CHECK(store.current().value == 1);
}

TEST_CASE("effect as a result")
{
    auto called = 0;
    auto effect = [&] (lager::context<int> ctx) { ++called; };
    auto store  = lager::make_store<int>(
        0,
        [=] (int model, int action) {
            return std::pair{model + action, effect};
        },
        lager::noop,
        lager::manual_event_loop{});

    store.dispatch(2);
    CHECK(store.current() == 2);
    CHECK(called == 1);
}
