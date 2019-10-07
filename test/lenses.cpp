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

#include <lager/lenses.hpp>
#include <zug/util.hpp>

struct yearday
{
    int day;
    int month;
};

struct person
{
    yearday birthday;
    std::string name;
};

using namespace lager;

TEST_CASE("Minimal example")
{
    auto month = [](auto&& f) {
        return [=](auto&& p) {
            return f(p.month)([&](auto&& x) {
                auto r  = std::forward<decltype(p)>(p);
                r.month = x;
                return r;
            });
        };
    };

    auto birthday = [](auto&& f) {
        return [=](auto&& p) {
            return f(p.birthday)([&](auto&& x) {
                auto r     = std::forward<decltype(p)>(p);
                r.birthday = x;
                return r;
            });
        };
    };

    auto name = [](auto&& f) {
        return [=](auto&& p) {
            return f(p.name)([&](auto&& x) {
                auto r = std::forward<decltype(p)>(p);
                r.name = x;
                return r;
            });
        };
    };

    auto birthday_month = zug::comp(birthday, month);

    auto p1 = person{{5, 4}, "juanpe"};
    CHECK(view(name, p1) == "juanpe");
    CHECK(view(birthday_month, p1) == 4);

    auto p2 = set(birthday_month, p1, 6);
    CHECK(p2.birthday.month == 6);
    CHECK(view(birthday_month, p2) == 6);

    auto p3 = over(birthday_month, p1, [](auto x) { return --x; });
    CHECK(view(birthday_month, p3) == 3);
    CHECK(p3.birthday.month == 3);
}

TEST_CASE("Attr")
{
    auto name = lens::attr(&person::name);
    auto birthday_month =
        zug::comp(lens::attr(&person::birthday), lens::attr(&yearday::month));

    auto p1 = person{{5, 4}, "juanpe"};
    CHECK(view(name, p1) == "juanpe");
    CHECK(view(birthday_month, p1) == 4);

    auto p2 = set(birthday_month, p1, 6);
    CHECK(p2.birthday.month == 6);
    CHECK(view(birthday_month, p2) == 6);

    auto p3 = over(birthday_month, p1, [](auto x) { return --x; });
    CHECK(view(birthday_month, p3) == 3);
    CHECK(p3.birthday.month == 3);
}

TEST_CASE("At")
{
    auto data = std::vector<std::string>{};
    CHECK(view(lens::at(0), data) == "");
    CHECK(set(lens::at(0), data, "foo") == (std::vector<std::string>{}));

    data.push_back("foo");
    CHECK(view(lens::at(0), data) == "foo");
    CHECK(set(lens::at(0), data, "bar") == (std::vector<std::string>{{"bar"}}));
}
