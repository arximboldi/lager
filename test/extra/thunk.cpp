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

#include <lager/event_loop/manual.hpp>
#include <lager/extra/thunk.hpp>
#include <lager/store.hpp>

#include "../../example/counter/counter.hpp"

TEST_CASE("dispatching actions and thunks with with_thunk enhancer")
{
    auto viewed = std::optional<counter::model>{std::nullopt};
    auto view   = [&](auto model) { viewed = model; };
    auto called = 0;
    auto effect = [&](lager::context<counter::action> ctx) { ++called; };
    auto store  = lager::make_store<counter::action>(
        counter::model{}, lager::with_manual_event_loop{}, lager::with_thunk());
    watch(store, [&](auto&& v) { view(v); });

    CHECK(!viewed);
    CHECK(store.get().value == 0);

    store.dispatch(counter::increment_action{});
    CHECK(viewed);
    CHECK(viewed->value == 1);
    CHECK(store.get().value == 1);

    store.dispatch(effect);
    CHECK(called == 1);

    store.dispatch([&](lager::context<counter::action> ctx) { ++called; });
    CHECK(called == 2);
}
