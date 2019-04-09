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

#include <lager/event_loop/manual.hpp>
#include <lager/store.hpp>

#include "../example/counter/counter.hpp"
#include <optional>

TEST_CASE("basic")
{
    auto viewed = std::optional<counter::model>{std::nullopt};
    auto view   = [&](auto model) { viewed = model; };
    auto store =
        lager::make_store<counter::action>(counter::model{},
                                           counter::update,
                                           view,
                                           lager::with_manual_event_loop{});

    CHECK(viewed);
    CHECK(viewed->value == 0);

    store.dispatch(counter::increment_action{});
    CHECK(viewed);
    CHECK(viewed->value == 1);
}

TEST_CASE("effect as a result")
{
    auto viewed = std::optional<int>{std::nullopt};
    auto view   = [&](auto model) { viewed = model; };
    auto called = 0;
    auto effect = [&](lager::context<int> ctx) { ++called; };
    auto store =
        lager::make_store<int>(0,
                               [=](int model, int action) {
                                   return std::pair{model + action, effect};
                               },
                               view,
                               lager::with_manual_event_loop{});

    store.dispatch(2);
    CHECK(viewed);
    CHECK(*viewed == 2);
    CHECK(called == 1);
}

TEST_CASE("store type erasure")
{
    auto viewed = std::optional<counter::model>{std::nullopt};
    auto view   = [&](auto model) { viewed = model; };
    lager::store<counter::action, counter::model> store =
        lager::make_store<counter::action>(counter::model{},
                                           counter::update,
                                           view,
                                           lager::with_manual_event_loop{});

    CHECK(viewed);
    CHECK(viewed->value == 0);

    store.dispatch(counter::increment_action{});
    CHECK(viewed);
    CHECK(viewed->value == 1);
}

namespace services {

struct foo
{
    int x = 0;
};

struct params
{
    const char* host = "bar";
};

} // namespace services

TEST_CASE("with deps enhancer")
{
    auto f     = services::foo{};
    auto store = lager::make_store<counter::action>(
        counter::model{},
        counter::update,
        [](auto) {},
        lager::with_manual_event_loop{},
        lager::with_deps(std::ref(f), services::params{"yeah"}));
    f.x = 42;

    auto ctx = store.get_context();
    CHECK(ctx.get<services::foo>().x == 42);
    CHECK(ctx.get<services::params>().host == std::string{"yeah"});
}

TEST_CASE("with deps, type erased, plus effects")
{
    auto called1 = 0;
    auto called2 = 0;
    auto called3 = 0;
    auto effect1 =
        [&](lager::context<counter::action, lager::deps<services::foo&>> ctx) {
            CHECK(ctx.get<services::foo>().x == 42);
            ++called1;
        };
    auto effect2 = [&](lager::context<counter::action,
                                      lager::deps<services::params>> ctx) {
        CHECK(ctx.get<services::params>().host == std::string{"yeah"});
        ++called2;
    };
    auto effect3 = [&](auto ctx) {
        CHECK(lager::get<services::foo>(ctx).x == 42);
        CHECK(lager::get<services::params>(ctx).host == std::string{"yeah"});
        ++called3;
    };

    auto f = services::foo{};
    lager::store<counter::action, counter::model> store =
        lager::make_store<counter::action>(
            counter::model{},
            [&](auto m, auto act) {
                return std::make_pair(counter::update(m, act), [&](auto ctx) {
                    effect1(ctx);
                    effect2(ctx);
                    effect3(ctx);
                });
            },
            [](auto) {},
            lager::with_manual_event_loop{},
            lager::with_deps(std::ref(f), services::params{"yeah"}));

    f.x = 42;
    store.dispatch(counter::increment_action{});
    CHECK(called1 == 1);
    CHECK(called2 == 1);
    CHECK(called3 == 1);
}

TEST_CASE("sequencing multiple effects with deps")
{
    auto called1 = 0;
    auto called2 = 0;
    auto called3 = 0;
    auto effect1 = lager::effect<counter::action, lager::deps<services::foo&>>{
        [&](lager::context<counter::action, lager::deps<services::foo&>> ctx) {
            CHECK(ctx.get<services::foo>().x == 42);
            ++called1;
        }};
    auto effect2 =
        lager::effect<counter::action, lager::deps<services::params>>{
            [&](lager::context<counter::action, lager::deps<services::params>>
                    ctx) {
                CHECK(ctx.get<services::params>().host == std::string{"yeah"});
                ++called2;
            }};
    auto effect3 = lager::effect<counter::action,
                                 lager::deps<services::foo&, services::params>>{
        [&](auto ctx) {
            CHECK(lager::get<services::foo>(ctx).x == 42);
            CHECK(lager::get<services::params>(ctx).host ==
                  std::string{"yeah"});
            ++called3;
        }};

    auto f     = services::foo{};
    auto store = lager::make_store<counter::action>(
        counter::model{},
        [&](auto m, auto act) {
            return std::make_pair(counter::update(m, act),
                                  lager::sequence(effect1, effect2, effect3));
        },
        [](auto) {},
        lager::with_manual_event_loop{},
        lager::with_deps(std::ref(f), services::params{"yeah"}));

    f.x = 42;
    store.dispatch(counter::increment_action{});
    CHECK(called1 == 1);
    CHECK(called2 == 1);
    CHECK(called3 == 1);
}
