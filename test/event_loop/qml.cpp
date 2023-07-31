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

#include <lager/event_loop/qml.hpp>
#include <lager/store.hpp>

#include "example/counter/counter.hpp"

namespace {

class TestApp : public lager::event_loop_quick_item
{
    using base_t  = lager::event_loop_quick_item;
    using store_t = lager::store<counter::action, counter::model>;

    store_t store_ =
        lager::make_store<counter::action, lager::transactional_tag>(
            counter::model{}, lager::with_qml_event_loop{*this});

public:
    TestApp(QQuickItem* parent = nullptr)
        : base_t{parent}
    {}

    ~TestApp() = default;

    void step() override
    {
        base_t::step();
        commit(store_);
    }
};

} // namespace

TEST_CASE("instantiation")
{
    auto test = TestApp{nullptr};
    (void) test;
    CHECK(true);
}
