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

#include <vector>

#include <zug/compose.hpp>
#include <immer/vector.hpp>
#include <immer/box.hpp>
#include <immer/algorithm.hpp>
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

auto force_opt = zug::comp([](auto&& f) {
    return [f = LAGER_FWD(f)](auto&& p) {
        using opt_t = std::optional<std::decay_t<decltype(p)>>;
        return f(opt_t{LAGER_FWD(p)})([&](auto&& x) {
            return std::forward<decltype(x)>(x)
                .value_or(LAGER_FWD(p));
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
    CHECK(view(value, t1) == 42);
    CHECK(view(first, t1) == 256);

    auto p2 = set(first, t1, 6);
    CHECK(p2.pair.first == 6);
    CHECK(view(first, p2) == 6);

    auto p3 = over(first, t1, [](auto x) { return --x; });
    CHECK(view(first, p3) == 255);
    CHECK(p3.pair.first == 255);
}

TEST_CASE("type erased lenses, at")
{
    auto children = attr(&tree::children);
    auto first_child = children | at_i(0);
    any_lens<tree, std::optional<immer::box<tree>>> te_first_child = first_child;
    any_lens<tree, std::optional<size_t>> te_first_value =
            te_first_child | optlift(unbox | attr(&tree::value));

    auto t1 = tree{42};
    CHECK(view(te_first_value, t1) == std::nullopt);
    CHECK(view(te_first_value, set(first_child, t1, t1)) == std::nullopt);
    CHECK(view(te_first_value, set(te_first_child, t1, t1)) == std::nullopt);

    t1 = over(children, t1, [](auto vec) {
        return vec.push_back(tree{1});
    });

    CHECK(view(te_first_value, t1) == 1);
    CHECK(view(te_first_value, set(first_child, t1, tree{2})) == 2);
    CHECK(view(te_first_value, set(te_first_child, t1, tree{3})) == 3);
    CHECK(view(te_first_value, set(te_first_value, t1, 4)) == 4);
}

template <typename Member>
auto attr2(Member member)
{
    return getset(
        [=](auto&& x) -> decltype(auto) {
            return std::forward<decltype(x)>(x).*member;
        },
        [=](auto x, auto&& v) {
            x.*member = std::forward<decltype(v)>(v);
            return x;
        });
};

TEST_CASE("type erased lenses, attr2")
{
    using te_lens = any_lens<tree, size_t>;

    te_lens value = attr2(&tree::value);
    te_lens first = attr2(&tree::pair) | attr2(&val_pair::first);

    auto t1 = tree{42, {256, 1115}};
    CHECK(view(value, t1) == 42);
    CHECK(view(first, t1) == 256);

    auto p2 = set(first, t1, 6);
    CHECK(p2.pair.first == 6);
    CHECK(view(first, p2) == 6);

    auto p3 = over(first, t1, [](auto x) { return --x; });
    CHECK(view(first, p3) == 255);
    CHECK(p3.pair.first == 255);
}

using lens_list = std::vector<any_lens<tree, std::optional<size_t>>>;

lens_list all_values(tree t) {
    size_t idx = 0;
    lens_list res {attr(&tree::value) | force_opt};
    for (auto child : t.children) {
        auto child_lens = attr(&tree::children)
                        | at_i(idx++)
                        | optmap(unbox);
        for (auto lens : all_values(child.get())) {
            res.push_back(child_lens | optbind(lens));
        }
    }
    return res;
}

TEST_CASE("type erased lenses, nesting")
{
    auto children = attr(&tree::children);
    auto t1 = over(children, tree{1}, [](auto vec) {
        return vec.push_back(tree{2})
                  .push_back(tree{3});
    });
    auto t2 = t1;
    t1 = over(children, t1, [=](auto vec) {
        return vec.push_back(t1);
    });

    auto lenses = all_values(t1);

    size_t expected1[] = {1, 2, 3, 1, 2, 3};
    for (size_t idx = 0; idx < lenses.size(); ++idx) {
        CHECK(view(lenses[idx], t1) == expected1[idx]);
    }
    
    std::optional<size_t> expected2[] =
        {1, 2, 3, std::nullopt, std::nullopt, std::nullopt};
    for (size_t idx = 0; idx < lenses.size(); ++idx) {
        CHECK(view(lenses[idx], t2) == expected2[idx]);
    }
    
    size_t expected3[] = {4, 6, 48, 3, 5, 16};
    for (size_t idx = 0; idx < lenses.size(); ++idx) {
        auto lens = lenses[idx];
        auto expected = expected1[idx];
        CHECK(view(lens, set(lens, t1, expected)) == expected);
    }
}
