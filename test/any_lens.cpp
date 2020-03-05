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

#include <zug/compose.hpp>
#include <immer/vector.hpp>
#include <immer/box.hpp>
#include <lager/lenses.hpp>
#include <lager/any_lens.hpp>
#include <zug/util.hpp>

using val_pair = std::pair<size_t, size_t>;

struct tree
{
    size_t value;
    val_pair pair {0, 0};
    immer::vector<immer::box<tree>> children {};
};

auto unbox = zug::comp([](auto&& f) {
    return [f](auto&& p) {
        return f(std::forward<decltype(p)>(p).get())([&](auto&& x) {
            return decltype(p){std::forward<decltype(x)>(x)};
        });
    };
});

using namespace lager;
using namespace lager::lens;
using namespace zug;

TEST_CASE("type erased lenses, attr")
{
    using te_lens = any_lens<tree, size_t>;

    te_lens value = attr(&tree::value);
    te_lens first = attr(&tree::pair) | attr(&val_pair::first);

    auto t1 = tree{42, {256, 1115}};
    CHECK(view(value.get(), t1) == 42);
    CHECK(view(first.get(), t1) == 256);

    auto p2 = set(first.get(), t1, 6);
    CHECK(p2.pair.first == 6);
    CHECK(view(first.get(), p2) == 6);

    auto p3 = over(first.get(), t1, [](auto x) { return --x; });
    CHECK(view(first.get(), p3) == 255);
    CHECK(p3.pair.first == 255);
}

TEST_CASE("type erased lenses, at")
{
    auto first_vll = attr(&tree::children) | at(0);
    any_lens<tree, std::optional<immer::box<tree>>> first_child = first_vll;
    any_lens<tree, std::optional<size_t>> first_value =
            first_child.get() | optlift(unbox | attr(&tree::value));

    auto vll = first_value.get();

    auto t1 = tree{42, {256, 1115}, {}};
    CHECK(view(vll, t1) == std::nullopt);
    // CHECK(view(vll, set(first_vll, t1, t1)) == std::nullopt);

    // t1.push_back({{}, "foo"});
    // CHECK(view(first_name, t1) == "foo");
    // CHECK(view(first_name, set(at(0), t1, person{{}, "bar"})) == "bar");
    // CHECK(view(first_name, set(first_name, t1, "bar")) == "bar");
}

// template <typename Member>
// auto attr2(Member member)
// {
//     return getset(
//         [=](auto&& x) -> decltype(auto) {
//             return std::forward<decltype(x)>(x).*member;
//         },
//         [=](auto x, auto&& v) {
//             x.*member = std::forward<decltype(v)>(v);
//             return x;
//         });
// };

// TEST_CASE("type erased lenses, attr2")
// {
//     auto name = attr2(&person::name);
//     auto birthday_month = attr2(&person::birthday) | attr2(&yearday::month);

//     auto p1 = person{{5, 4}, "juanpe"};
//     CHECK(view(name, p1) == "juanpe");
//     CHECK(view(birthday_month, p1) == 4);

//     auto p2 = set(birthday_month, p1, 6);
//     CHECK(p2.birthday.month == 6);
//     CHECK(view(birthday_month, p2) == 6);

//     auto p3 = over(birthday_month, p1, [](auto x) { return --x; });
//     CHECK(view(birthday_month, p3) == 3);
//     CHECK(p3.birthday.month == 3);
// }

// TEST_CASE("type erased lenses, fallback")
// {
//     auto first      = at_i(0);
//     auto first_name = first | optlift(attr(&person::name)) | fallback("NULL");

//     auto v1 = immer::vector<person>{};
//     CHECK(view(first_name, v1) == "NULL");
//     CHECK(view(first_name, set(at_i(0), v1, person{{}, "foo"})) == "NULL");

//     v1 = v1.push_back({{}, "foo"});
//     CHECK(view(first_name, v1) == "foo");
//     CHECK(view(first_name, set(at_i(0), v1, person{{}, "bar"})) == "bar");
//     CHECK(view(first_name, set(first_name, v1, "bar")) == "bar");
// }

// TEST_CASE("type erased lenses, optlift")
// {
//     auto first          = at_i(0);
//     auto birthday       = attr(&person::birthday);
//     auto month          = attr(&yearday::month);
//     auto birthday_month = birthday | month;

//     SECTION("lifting composed type erased lenses") {
//         auto first_month = first
//                 | optlift(birthday_month);

//         auto p1 = person{{5, 4}, "juanpe"};

//         auto v1 = immer::vector<person>{};
//         CHECK(view(first_month, v1) == std::nullopt);
//         CHECK(view(first_month, set(at_i(0), v1, p1)) == std::nullopt);

//         v1 = v1.push_back(p1);
//         CHECK(view(first_month, v1) == 4);
//         p1.birthday.month = 6;
//         CHECK(view(first_month, set(at_i(0), v1, p1)) == 6);
//         CHECK(view(first_month, set(first_month, v1, 8)) == 8);
//     }

//     SECTION("composing lifted type erased lenses") {
//         auto first_month = first
//                 | optlift(birthday)
//                 | optlift(month);

//         auto p1 = person{{5, 4}, "juanpe"};

//         auto v1 = immer::vector<person>{};
//         CHECK(view(first_month, v1) == std::nullopt);
//         CHECK(view(first_month, set(at_i(0), v1, p1)) == std::nullopt);

//         v1 = v1.push_back(p1);
//         CHECK(view(first_month, v1) == 4);
//         p1.birthday.month = 6;
//         CHECK(view(first_month, set(at_i(0), v1, p1)) == 6);
//         CHECK(view(first_month, set(first_month, v1, 8)) == 8);
//     }
// }
