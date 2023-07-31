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
#include <lager/store.hpp>

#include "../example/counter/counter.hpp"
#include <optional>

TEST_CASE("automatic")
{
    auto viewed = std::optional<counter::model>{std::nullopt};
    auto view   = [&](auto model) { viewed = model; };
    auto store  = lager::make_store<counter::action>(
        counter::model{}, lager::with_manual_event_loop{});
    watch(store, [&](auto&& v) { view(v); });

    CHECK(!viewed);
    CHECK(store.get().value == 0);

    store.dispatch(counter::increment_action{});
    CHECK(viewed);
    CHECK(viewed->value == 1);
    CHECK(store.get().value == 1);
}

TEST_CASE("basic")
{
    auto viewed = std::optional<counter::model>{std::nullopt};
    auto view   = [&](auto model) { viewed = model; };
    auto store  = lager::make_store<counter::action, lager::transactional_tag>(
        counter::model{}, lager::with_manual_event_loop{});
    watch(store, [&](auto&& v) { view(v); });

    CHECK(!viewed);
    CHECK(store.get().value == 0);

    store.dispatch(counter::increment_action{});
    CHECK(!viewed);
    CHECK(store.get().value == 0);

    lager::commit(store);
    CHECK(viewed);
    CHECK(viewed->value == 1);
    CHECK(store.get().value == 1);
}

TEST_CASE("with reducer enhancer")
{
    auto reducer = [](counter::model m, counter::action) { return m; };
    auto store =
        lager::make_store<counter::action>(counter::model{},
                                           lager::with_manual_event_loop{},
                                           lager::with_reducer(reducer));

    CHECK(store.get().value == 0);
    store.dispatch(counter::increment_action{});
    CHECK(store.get().value == 0);
}

TEST_CASE("effect as a result")
{
    auto viewed = std::optional<int>{std::nullopt};
    auto view   = [&](auto model) { viewed = model; };
    auto called = 0;
    auto effect = [&](lager::context<int> ctx) { ++called; };
    auto store =
        lager::make_store<int>(0,
                               lager::with_manual_event_loop{},
                               lager::with_reducer([=](int model, int action) {
                                   return std::pair{model + action, effect};
                               }));
    watch(store, [&](auto&& v) { view(v); });

    store.dispatch(2);
    CHECK(viewed);
    CHECK(*viewed == 2);
    CHECK(called == 1);
}

TEST_CASE("effects see updated world")
{
    auto called = 0;
    auto store  = std::optional<lager::store<int, int>>{};
    auto effect = [&](lager::context<int> ctx) {
        CHECK(store->get() == 2);
        ++called;
    };
    store =
        lager::make_store<int>(0,
                               lager::with_manual_event_loop{},
                               lager::with_reducer([=](int model, int action) {
                                   return std::pair{model + action, effect};
                               }));

    store->dispatch(2);
    CHECK(called == 1);
    CHECK(store->get() == 2);
}

TEST_CASE("store type erasure")
{
    auto viewed = std::optional<counter::model>{std::nullopt};
    auto view   = [&](auto model) { viewed = model; };
    lager::store<counter::action, counter::model> store =
        lager::make_store<counter::action>(counter::model{},
                                           lager::with_manual_event_loop{});
    watch(store, [&](auto&& v) { view(v); });
    CHECK(!viewed);

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
        lager::with_manual_event_loop{},
        lager::with_deps(std::ref(f), services::params{"yeah"}));
    f.x = 42;

    CHECK(lager::get<services::foo>(store).x == 42);
    CHECK(lager::get<services::params>(store).host == std::string{"yeah"});
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
            lager::with_manual_event_loop{},
            lager::with_deps(std::ref(f), services::params{"yeah"}),
            lager::with_reducer([&](auto m, auto act) {
                return std::make_pair(counter::update(m, act), [&](auto ctx) {
                    effect1(ctx);
                    effect2(ctx);
                    effect3(ctx);
                });
            }));

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
        lager::with_manual_event_loop{},
        lager::with_deps(std::ref(f), services::params{"yeah"}),
        lager::with_reducer([&](auto m, auto act) {
            return std::make_pair(counter::update(m, act),
                                  lager::sequence(effect1, effect2, effect3));
        }));

    f.x = 42;
    store.dispatch(counter::increment_action{});
    CHECK(called1 == 1);
    CHECK(called2 == 1);
    CHECK(called3 == 1);
}

namespace {

struct child1_action
{};
struct child2_action
{};
struct child3_action
{};
using parent_action = std::variant<child1_action, child2_action, child3_action>;

} // namespace

TEST_CASE("sequencing hierarchical actions")
{
    auto eff1called = 0;
    auto eff2called = 0;
    auto eff1 = lager::effect<child1_action>{[&](auto&&) { ++eff1called; }};
    auto eff2 = lager::effect<child2_action>{[&](auto&&) { ++eff2called; }};
    auto eff3 = lager::effect<parent_action>{lager::sequence(eff1, eff2)};
    eff3(lager::context<parent_action>{});
    CHECK(eff1called == 1);
    CHECK(eff2called == 1);
}

TEST_CASE("subsetting context actions")
{
    auto eff1 = lager::effect<lager::actions<child1_action, child3_action>>{
        [](auto&& ctx) {
            ctx.dispatch(child1_action{});
            ctx.dispatch(child3_action{});
        }};
    auto eff2 = lager::effect<
        lager::actions<child1_action, child2_action, child3_action>>{eff1};
    auto eff3 = lager::effect<parent_action>{eff2};

    auto dispatch_count = 0;
    auto dispatcher     = [&](auto) {
        ++dispatch_count;
        return lager::future{};
    };
    auto loop = lager::with_manual_event_loop{};
    auto ctx  = lager::context<parent_action>{dispatcher, loop, {}};

    eff3(ctx);
    CHECK(dispatch_count == 2);

    eff2(ctx);
    CHECK(dispatch_count == 4);

    eff1(ctx);
    CHECK(dispatch_count == 6);
}

using parent2_action =
    std::variant<std::pair<int, child1_action>, child2_action, child3_action>;

TEST_CASE("composing context with converter")
{
    auto store = lager::make_store<parent2_action>(
        0,
        lager::with_manual_event_loop{},
        lager::with_reducer([=](int model, parent2_action action) {
            return std::visit(
                lager::visitor{
                    [](std::pair<int, child1_action> p) { return p.first; },
                    [](auto x) { return 0; }},
                action);
        }));

    auto ctx1 = lager::context<child1_action>{
        store, [](auto&& act) { return std::make_pair(1, child1_action{}); }};
    auto ctx2 = lager::context<child1_action>{
        store, [](auto&& act) { return std::make_pair(2, child1_action{}); }};

    ctx1.dispatch(child1_action{});
    CHECK(*store == 1);

    ctx2.dispatch(child1_action{});
    CHECK(*store == 2);
}
