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

TEST_CASE("exception safety")
{
    auto loop = lager::with_manual_event_loop{};

    SECTION("basic")
    {
        CHECK_THROWS(loop.post([] { throw std::runtime_error{"noo!"}; }));

        auto called = 0;
        loop.post([&] { ++called; });
        CHECK(called == 1);
    }

    SECTION("recursive")
    {
        auto called_a = 0;
        auto called_b = 0;
        auto called_c = 0;
        CHECK_THROWS(loop.post([&] {
            loop.post([&] { ++called_a; });
            loop.post([&] { throw std::runtime_error{"noo!"}; });
            loop.post([&] { ++called_b; });
            CHECK(called_a == 0);
            CHECK(called_b == 0);
        }));

        CHECK(called_a == 1);
        CHECK(called_b == 0); // not called yet, queue was interrupted

        loop.post([&] { ++called_c; });
        CHECK(called_b == 1); // queue continued
        CHECK(called_c == 1);
        CHECK(called_a == 1);
    }
}
