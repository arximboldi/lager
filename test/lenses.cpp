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

#include <immer/vector.hpp>
#include <iostream>
#include <zug/compose.hpp>
#include <zug/util.hpp>

#include <lager/lenses.hpp>
#include <lager/lenses/at.hpp>
#include <lager/lenses/at_or.hpp>
#include <lager/lenses/attr.hpp>
#include <lager/lenses/optional.hpp>
#include <lager/lenses/variant.hpp>
#include <lager/lenses/tuple.hpp>

#include <array>

struct yearday
{
    int day;
    int month;

    friend bool operator==(const yearday &lhs, const yearday &rhs) {
        return lhs.day == rhs.day && lhs.month == rhs.month;
    }

    friend bool operator!=(const yearday &lhs, const yearday &rhs) {
        return !(lhs == rhs);
    }
};

struct person
{
    yearday birthday;
    std::string name;
    std::vector<std::string> things{};
};

using namespace lager;
using namespace lager::lenses;
using namespace zug;


TEST_CASE("lenses, minimal example")
{
    auto month = zug::comp([](auto&& f) {
        return [=](auto&& p) {
            return f(p.month)([&](auto&& x) {
                auto r  = std::forward<decltype(p)>(p);
                r.month = x;
                return r;
            });
        };
    });

    auto birthday = zug::comp([](auto&& f) {
        return [=](auto&& p) {
            return f(p.birthday)([&](auto&& x) {
                auto r     = std::forward<decltype(p)>(p);
                r.birthday = x;
                return r;
            });
        };
    });

    auto name = zug::comp([](auto&& f) {
        return [=](auto&& p) {
            return f(p.name)([&](auto&& x) {
                auto r = std::forward<decltype(p)>(p);
                r.name = x;
                return r;
            });
        };
    });

    auto birthday_month = birthday | month;

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

TEST_CASE("lenses, attr")
{
    auto name           = attr(&person::name);
    auto birthday_month = attr(&person::birthday) | attr(&yearday::month);

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

TEST_CASE("lenses, attr, references")
{
    auto name           = attr(&person::name);
    auto birthday_month = attr(&person::birthday) | attr(&yearday::month);

    auto p1       = person{{5, 4}, "juanpe", {{"foo"}, {"bar"}}};
    const auto p2 = p1;

    CHECK(&view(name, p1) == &p1.name);
    CHECK(&view(birthday_month, p1) == &p1.birthday.month);
    CHECK(&view(name, p2) == &p2.name);
    CHECK(&view(birthday_month, p2) == &p2.birthday.month);

    {
        [[maybe_unused]] int& x       = view(birthday_month, p1);
        [[maybe_unused]] int&& y      = view(birthday_month, std::move(p1));
        [[maybe_unused]] const int& z = view(birthday_month, p2);
    }
}

TEST_CASE("lenses, at")
{
    auto first      = at(0);
    auto first_name = first | with_opt(attr(&person::name));

    auto v1 = std::vector<person>{};
    CHECK(view(first_name, v1) == std::nullopt);
    CHECK(view(first_name, set(at(0), v1, person{{}, "foo"})) == std::nullopt);

    v1.push_back({{}, "foo"});
    CHECK(view(first_name, v1) == "foo");
    CHECK(view(first_name, set(at(0), v1, person{{}, "bar"})) == "bar");
    CHECK(view(first_name, set(first_name, v1, "bar")) == "bar");
}

// This is an alternative definition of lager::lenses::attr using
// lager::lens::getset.  The standard definition is potentially more efficient
// whene the whole lens can not be optimized away, because there is only one
// capture of member, as opposed to two.  However, getset is still an
// interesting device, since it provides an easier way to define lenses for
// people not used to the pattern.
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

TEST_CASE("lenses, attr2")
{
    auto name           = attr2(&person::name);
    auto birthday_month = attr2(&person::birthday) | attr2(&yearday::month);

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

TEST_CASE("lenses, attr2, references")
{
    auto name           = attr2(&person::name);
    auto birthday_month = attr2(&person::birthday) | attr2(&yearday::month);

    auto p1       = person{{5, 4}, "juanpe", {{"foo"}, {"bar"}}};
    const auto p2 = p1;

    CHECK(&view(name, p1) == &p1.name);
    CHECK(&view(birthday_month, p1) == &p1.birthday.month);
    CHECK(&view(name, p2) == &p2.name);
    CHECK(&view(birthday_month, p2) == &p2.birthday.month);

    {
        [[maybe_unused]] int& x       = view(birthday_month, p1);
        [[maybe_unused]] int&& y      = view(birthday_month, std::move(p1));
        [[maybe_unused]] const int& z = view(birthday_month, p2);
    }
}

template <typename Member>
auto attr3(Member member, int *numGetCalls)
{
    return getset(
        [=](auto&& x) -> decltype(auto) {
            (*numGetCalls)++;
            return std::forward<decltype(x)>(x).*member;
        },
        [=](auto x, auto&& v) {
            x.*member = std::forward<decltype(v)>(v);
            return x;
        });
};

TEST_CASE("lenses, no getter on set")
{
    int numGetCalls = 0;
    auto name = attr3(&person::name, &numGetCalls);
    auto p1 = person{{5, 4}, "juanpe"};
    CHECK(view(name, p1) == "juanpe");
    CHECK(numGetCalls == 1);

    p1 = set(name, p1, "ncopernicus");
    CHECK(numGetCalls == 1);

    CHECK(view(name, p1) == "ncopernicus");
    CHECK(numGetCalls == 2);
}

TEST_CASE("lenses, no getter on nested set")
{
    int numGetCallsOnBirthDay = 0;
    int numGetCallsOnMonth = 0;
    auto birthday = attr3(&person::birthday, &numGetCallsOnBirthDay);
    auto month = attr3(&yearday::month, &numGetCallsOnMonth);
    auto p1 = person{{5, 4}, "juanpe"};

    CHECK(view(birthday, p1) == yearday{5, 4});
    CHECK(numGetCallsOnBirthDay == 1);
    CHECK(numGetCallsOnMonth == 0);
    numGetCallsOnBirthDay = 0;
    numGetCallsOnMonth = 0;

    CHECK(view(birthday | month, p1) == 4);
    CHECK(numGetCallsOnBirthDay == 1);
    CHECK(numGetCallsOnMonth == 1);
    numGetCallsOnBirthDay = 0;
    numGetCallsOnMonth = 0;

    p1 = set(birthday | month, p1, 6);
    CHECK(numGetCallsOnBirthDay == 1);
    CHECK(numGetCallsOnMonth == 0);
    numGetCallsOnBirthDay = 0;
    numGetCallsOnMonth = 0;

    CHECK(view(birthday | month, p1) == 6);
    CHECK(numGetCallsOnBirthDay == 1);
    CHECK(numGetCallsOnMonth == 1);
    numGetCallsOnBirthDay = 0;
    numGetCallsOnMonth = 0;
}


struct copy_info
{
    copy_info(std::string _debugName) : debug_name(std::move(_debugName)) {};

    bool operator== (const std::tuple<int, int, int, int> &rhs) const {
        return num_copy_constructions == std::get<0>(rhs) &&
               num_copy_assignments == std::get<1>(rhs) &&
               num_move_constructions == std::get<2>(rhs) &&
               num_move_assignments == std::get<3>(rhs);
    }

    bool operator!= (const std::tuple<int, int, int, int> &rhs) const {
        return !(*this == rhs);
    }

    void reset() {
        num_copy_constructions = 0;
        num_copy_assignments = 0;
        num_move_constructions = 0;
        num_move_assignments = 0;
    }

    int num_copy_constructions = 0;
    int num_copy_assignments = 0;
    int num_move_constructions = 0;
    int num_move_assignments = 0;
    std::string debug_name;
};

std::ostream& operator << (std::ostream &out, const copy_info &info)
{
    out << "copy_info(";
    out << "cc:" << info.num_copy_constructions << " ";
    out << "ca:" << info.num_copy_assignments << " ";
    out << "mc:" << info.num_move_constructions << " ";
    out << "ma:" << info.num_move_assignments;
    out << ")";
    return out;
}

struct copy_tracker
{
    copy_tracker(copy_info &info)
        : info(info)
    {
    }

    copy_tracker(const copy_tracker &rhs)
        : info(rhs.info)
    {
        assert(!rhs.moved_from_here);
        info.num_copy_constructions++;
    }

    copy_tracker(copy_tracker &&rhs)
        : info(rhs.info)
    {
        assert(!rhs.moved_from_here);
        rhs.moved_from_here = true;
        info.num_move_constructions++;
    }

    copy_tracker& operator=(const copy_tracker &rhs)
    {
        assert(!rhs.moved_from_here);
        info = rhs.info;
        info.num_copy_assignments++;

        // the object becomes valid after it has been moved into
        moved_from_here = false;
        return *this;
    }

    copy_tracker& operator=(copy_tracker &&rhs)
    {
        assert(!rhs.moved_from_here);
        info = rhs.info;
        rhs.moved_from_here = true;
        info.num_move_assignments++;

        // the object becomes valid after it has been moved into
        moved_from_here = false;
        return *this;
    }

    copy_info &info;
    bool moved_from_here = false;
};

struct debug_yearday
{
    debug_yearday(copy_info &info)
        : tracker(info)
    {
    }

    bool valid() {
        return !tracker.moved_from_here;
    }

    copy_tracker tracker;
    int day {0};
    int month {0};
};

struct debug_person
{
    debug_person(copy_info &person_copy_info, copy_info &birthday_copy_info)
        : tracker(person_copy_info)
        , birthday(birthday_copy_info)

    {
    }

    bool valid() {
        return !tracker.moved_from_here &&
               !birthday.tracker.moved_from_here;
    }

    copy_tracker tracker;
    debug_yearday birthday;
    std::string name;
    std::vector<std::string> things{};
};

struct debug_freelancer
{
    debug_freelancer(copy_info &freelancer_copy_info, copy_info &person_copy_info, copy_info &birthday_copy_info)
        : tracker(freelancer_copy_info)
        , person(person_copy_info, birthday_copy_info)
    {
    }

    bool valid() {
        return !tracker.moved_from_here &&
               !person.tracker.moved_from_here &&
               !person.birthday.tracker.moved_from_here;
    }

    copy_tracker tracker;
    debug_person person;
    int tax_id {};
};

TEST_CASE("getset copy: view lvalue")
{
    copy_info person_copy_info("person");
    copy_info birthday_copy_info("birthday");
    auto birthday_lens = attr2(&debug_person::birthday);

    auto p1 = debug_person(person_copy_info, birthday_copy_info);

    CHECK(person_copy_info == std::make_tuple(0, 0, 0, 0));
    CHECK(birthday_copy_info == std::make_tuple(0, 0, 0, 0));

    auto birthday = view(birthday_lens, p1);

    CHECK(person_copy_info == std::make_tuple(0, 0, 0, 0));
    CHECK(birthday_copy_info == std::make_tuple(1, 0, 0, 0));

    CHECK(p1.valid());
    CHECK(birthday.valid());
}

TEST_CASE("getset copy: view rvalue")
{
    copy_info person_copy_info("person");
    copy_info birthday_copy_info("birthday");

    auto birthday_lens = attr2(&debug_person::birthday);

    auto birthday = view(birthday_lens, debug_person(person_copy_info, birthday_copy_info));

    CHECK(person_copy_info == std::make_tuple(0, 0, 0, 0));
    CHECK(birthday_copy_info == std::make_tuple(0, 0, 3, 0));

    CHECK(birthday.valid());
}


TEST_CASE("getset copy: set lvalue with rvalue")
{
    copy_info person_copy_info("person");
    copy_info birthday_copy_info("birthday");

    auto birthday_lens = attr2(&debug_person::birthday);

    auto p1 = debug_person(person_copy_info, birthday_copy_info);

    auto birthday =
        set(birthday_lens, p1, debug_yearday(birthday_copy_info));

    CHECK(person_copy_info == std::make_tuple(1, 0, 3, 0));
    CHECK(birthday_copy_info == std::make_tuple(1, 0, 5, 1));

    CHECK(p1.valid());
    CHECK(birthday.valid());
}

TEST_CASE("getset copy: set lvalue with lvalue")
{
    copy_info person_copy_info("person");
    copy_info birthday_copy_info("birthday");

    auto birthday_lens = attr2(&debug_person::birthday);

    auto p1 = debug_person(person_copy_info, birthday_copy_info);
    auto day1 = debug_yearday(birthday_copy_info);

    auto birthday = set(birthday_lens, p1, day1);

    CHECK(person_copy_info == std::make_tuple(1, 0, 3, 0));
    CHECK(birthday_copy_info == std::make_tuple(1, 1, 3, 0));

    CHECK(p1.valid());
    CHECK(day1.valid());
    CHECK(birthday.valid());
}

TEST_CASE("getset copy: set rvalue with lvalue")
{
    copy_info person_copy_info("person");
    copy_info birthday_copy_info("birthday");

    auto birthday_lens = attr2(&debug_person::birthday);

    auto day1 = debug_yearday(birthday_copy_info);

    auto birthday =
        set(birthday_lens, debug_person(person_copy_info, birthday_copy_info), day1);

    CHECK(person_copy_info == std::make_tuple(0, 0, 4, 0));
    CHECK(birthday_copy_info == std::make_tuple(0, 1, 4, 0));

    CHECK(day1.valid());
    CHECK(birthday.valid());
}

TEST_CASE("getset copy: set rvalue with rvalue")
{
    copy_info person_copy_info("person");
    copy_info birthday_copy_info("birthday");

    auto birthday_lens = attr2(&debug_person::birthday);

    auto birthday =
        set(birthday_lens,
            debug_person(person_copy_info, birthday_copy_info),
            debug_yearday(birthday_copy_info));

    CHECK(person_copy_info == std::make_tuple(0, 0, 4, 0));
    CHECK(birthday_copy_info == std::make_tuple(0, 0, 6, 1));

    CHECK(birthday.valid());
}

TEST_CASE("nested getset copy: view lvalue")
{
    copy_info freelancer_copy_info("freelancer");
    copy_info person_copy_info("person");
    copy_info birthday_copy_info("birthday");
    auto birthday_lens = attr2(&debug_person::birthday);
    auto person_lens = attr2(&debug_freelancer::person);
    auto freelancer_birthday_lens = person_lens | birthday_lens;

    auto e1 = debug_freelancer(freelancer_copy_info, person_copy_info, birthday_copy_info);

    CHECK(person_copy_info == std::make_tuple(0, 0, 0, 0));
    CHECK(birthday_copy_info == std::make_tuple(0, 0, 0, 0));

    auto birthday = view(freelancer_birthday_lens, e1);

    CHECK(freelancer_copy_info == std::make_tuple(0, 0, 0, 0));
    CHECK(person_copy_info == std::make_tuple(0, 0, 0, 0));
    CHECK(birthday_copy_info == std::make_tuple(1, 0, 0, 0));

    CHECK(e1.valid());
    CHECK(birthday.valid());
}

TEST_CASE("nested getset copy: view rvalue")
{
    copy_info freelancer_copy_info("freelancer");
    copy_info person_copy_info("person");
    copy_info birthday_copy_info("birthday");
    auto birthday_lens = attr2(&debug_person::birthday);
    auto person_lens = attr2(&debug_freelancer::person);
    auto freelancer_birthday_lens = person_lens | birthday_lens;

    CHECK(person_copy_info == std::make_tuple(0, 0, 0, 0));
    CHECK(birthday_copy_info == std::make_tuple(0, 0, 0, 0));

    auto birthday =
        view(freelancer_birthday_lens,
             debug_freelancer(freelancer_copy_info, person_copy_info, birthday_copy_info));

    CHECK(freelancer_copy_info == std::make_tuple(0, 0, 0, 0));
    CHECK(person_copy_info == std::make_tuple(0, 0, 0, 0));
    CHECK(birthday_copy_info == std::make_tuple(0, 0, 4, 0));

    CHECK(birthday.valid());
}

TEST_CASE("nested getset copy: set lvalue with rvalue")
{
    copy_info freelancer_copy_info("freelancer");
    copy_info person_copy_info("person");
    copy_info birthday_copy_info("birthday");

    auto birthday_lens = attr2(&debug_person::birthday);
    auto person_lens = attr2(&debug_freelancer::person);
    auto freelancer_birthday_lens = person_lens | birthday_lens;

    auto e1 = debug_freelancer(freelancer_copy_info, person_copy_info, birthday_copy_info);

    auto birthday =
        set(freelancer_birthday_lens,
            e1,
            debug_yearday(birthday_copy_info));

    CHECK(freelancer_copy_info == std::make_tuple(1, 0, 3, 0));

    /**
     * One copy is for passing a person into the setter of birthday_lens,
     * the other one is for passing the source enterpreneur into the
     * setter function of person_lens.
     */
    CHECK(person_copy_info == std::make_tuple(2, 0, 5, 1));
    CHECK(birthday_copy_info == std::make_tuple(2, 0, 7, 2));

    CHECK(e1.valid());
    CHECK(birthday.valid());
}

TEST_CASE("nested getset copy: set lvalue with lvalue")
{
    copy_info freelancer_copy_info("freelancer");
    copy_info person_copy_info("person");
    copy_info birthday_copy_info("birthday");

    auto birthday_lens = attr2(&debug_person::birthday);
    auto person_lens = attr2(&debug_freelancer::person);
    auto freelancer_birthday_lens = person_lens | birthday_lens;

    auto e1 = debug_freelancer(freelancer_copy_info, person_copy_info, birthday_copy_info);
    auto day1 = debug_yearday(birthday_copy_info);

    auto birthday =
        set(freelancer_birthday_lens,
            e1,
            day1);

    CHECK(freelancer_copy_info == std::make_tuple(1, 0, 3, 0));

    /**
     * One copy is for passing a person into the setter of birthday_lens,
     * the other one is for passing the source enterpreneur into the
     * setter function of person_lens.
     */
    CHECK(person_copy_info == std::make_tuple(2, 0, 5, 1));
    CHECK(birthday_copy_info == std::make_tuple(2, 1, 5, 1));

    CHECK(e1.valid());
    CHECK(day1.valid());
    CHECK(birthday.valid());
}

TEST_CASE("nested getset copy: set rvalue with lvalue")
{
    copy_info freelancer_copy_info("freelancer");
    copy_info person_copy_info("person");
    copy_info birthday_copy_info("birthday");

    auto birthday_lens = attr2(&debug_person::birthday);
    auto person_lens = attr2(&debug_freelancer::person);
    auto freelancer_birthday_lens = person_lens | birthday_lens;

    auto day1 = debug_yearday(birthday_copy_info);

    auto birthday =
        set(freelancer_birthday_lens,
            debug_freelancer(freelancer_copy_info, person_copy_info, birthday_copy_info),
            day1);

    CHECK(freelancer_copy_info == std::make_tuple(0, 0, 4, 0));

    /**
     * The only copy is to pass the person into birthday_lens. Use const-ref
     * for `whole` argument to avoid this copy.
     */
    CHECK(person_copy_info == std::make_tuple(1, 0, 6, 1));
    CHECK(birthday_copy_info == std::make_tuple(1, 1, 6, 1));

    CHECK(day1.valid());
    CHECK(birthday.valid());
}

TEST_CASE("nested getset copy: set rvalue with rvalue")
{
    copy_info freelancer_copy_info("freelancer");
    copy_info person_copy_info("person");
    copy_info birthday_copy_info("birthday");

    auto birthday_lens = attr2(&debug_person::birthday);
    auto person_lens = attr2(&debug_freelancer::person);
    auto freelancer_birthday_lens = person_lens | birthday_lens;

    auto birthday =
        set(freelancer_birthday_lens,
            debug_freelancer(freelancer_copy_info, person_copy_info, birthday_copy_info),
            debug_yearday(birthday_copy_info));

    CHECK(freelancer_copy_info == std::make_tuple(0, 0, 4, 0));

    /**
     * The only copy is to pass the person into birthday_lens. Use const-ref
     * for `whole` argument to avoid this copy.
     */
    CHECK(person_copy_info == std::make_tuple(1, 0, 6, 1));
    CHECK(birthday_copy_info == std::make_tuple(1, 0, 8, 2));

    CHECK(birthday.valid());
}

TEST_CASE("nested (attr | getset)")
{
    copy_info freelancer_copy_info("freelancer");
    copy_info person_copy_info("person");
    copy_info birthday_copy_info("birthday");

    auto birthday_lens = attr2(&debug_person::birthday);
    auto person_lens = attr(&debug_freelancer::person);
    auto freelancer_birthday_lens = person_lens | birthday_lens;

    auto birthday =
        set(freelancer_birthday_lens,
            debug_freelancer(freelancer_copy_info, person_copy_info, birthday_copy_info),
            debug_yearday(birthday_copy_info));

    CHECK(freelancer_copy_info == std::make_tuple(0, 0, 4, 0));

    /**
     * The only copy is to pass the person into birthday_lens. Use const-ref
     * for `whole` argument to avoid this copy.
     */
    CHECK(person_copy_info == std::make_tuple(1, 0, 6, 1));
    CHECK(birthday_copy_info == std::make_tuple(1, 0, 8, 2));

    CHECK(birthday.valid());
}

TEST_CASE("nested (getset | attr)")
{
    copy_info freelancer_copy_info("freelancer");
    copy_info person_copy_info("person");
    copy_info birthday_copy_info("birthday");

    auto birthday_lens = attr(&debug_person::birthday);
    auto person_lens = attr2(&debug_freelancer::person);
    auto freelancer_birthday_lens = person_lens | birthday_lens;

    auto birthday =
        set(freelancer_birthday_lens,
            debug_freelancer(freelancer_copy_info, person_copy_info, birthday_copy_info),
            debug_yearday(birthday_copy_info));

    CHECK(freelancer_copy_info == std::make_tuple(0, 0, 4, 0));

    /**
     * The only copy is to pass the person into birthday_lens. Use const-ref
     * for `whole` argument to avoid this copy.
     */
    CHECK(person_copy_info == std::make_tuple(1, 0, 6, 1));
    CHECK(birthday_copy_info == std::make_tuple(1, 0, 8, 2));

    CHECK(birthday.valid());
}

TEST_CASE("lenses, at immutable index")
{
    auto first      = at(0);
    auto first_name = first | with_opt(attr(&person::name));

    auto v1 = immer::vector<person>{};
    CHECK(view(first_name, v1) == std::nullopt);
    CHECK(view(first_name, set(at(0), v1, person{{}, "foo"})) == std::nullopt);
    CHECK(view(first_name, set(first_name, v1, "bar")) == std::nullopt);

    v1 = v1.push_back({{}, "foo"});
    CHECK(view(first_name, v1) == "foo");
    CHECK(view(first_name, set(at(0), v1, person{{}, "bar"})) == "bar");
    CHECK(view(first_name, set(first_name, v1, "bar")) == "bar");
}

TEST_CASE("lenses, at_or default")
{
    auto first      = at_or(0);
    auto first_name = first | attr(&person::name);

    auto v1 = immer::vector<person>{};
    CHECK(view(first_name, v1) == "");
    CHECK(view(first_name, set(at(0), v1, person{{}, "foo"})) == "");
    CHECK(view(first_name, set(first_name, v1, "bar")) == "");

    v1 = v1.push_back({{}, "foo"});
    CHECK(view(first_name, v1) == "foo");
    CHECK(view(first_name, set(at(0), v1, person{{}, "bar"})) == "bar");
    CHECK(view(first_name, set(first_name, v1, "bar")) == "bar");
}

TEST_CASE("lenses, at_or")
{
    auto first      = at_or(0, person{{}, "null"});
    auto first_name = first | attr(&person::name);

    auto v1 = immer::vector<person>{};
    CHECK(view(first_name, v1) == "null");
    CHECK(view(first_name, set(at(0), v1, person{{}, "foo"})) == "null");
    CHECK(view(first_name, set(first_name, v1, "bar")) == "null");

    v1 = v1.push_back({{}, "foo"});
    CHECK(view(first_name, v1) == "foo");
    CHECK(view(first_name, set(at(0), v1, person{{}, "bar"})) == "bar");
    CHECK(view(first_name, set(first_name, v1, "bar")) == "bar");
}

TEST_CASE("lenses, value_or")
{
    auto first      = at(0);
    auto first_name = first | with_opt(attr(&person::name)) | value_or("NULL");

    auto v1 = immer::vector<person>{};
    CHECK(view(first_name, v1) == "NULL");
    CHECK(view(first_name, set(at(0), v1, person{{}, "foo"})) == "NULL");
    CHECK(view(first_name, set(first_name, v1, "bar")) == "NULL");

    v1 = v1.push_back({{}, "foo"});
    CHECK(view(first_name, v1) == "foo");
    CHECK(view(first_name, set(at(0), v1, person{{}, "bar"})) == "bar");
    CHECK(view(first_name, set(first_name, v1, "bar")) == "bar");
}

TEST_CASE("lenses, alternative")
{
    auto the_person = alternative<person>;
    auto person_name =
        the_person | with_opt(attr(&person::name)) | value_or("NULL");

    auto v1 = std::variant<person, std::string>{"nonesuch"};
    CHECK(view(person_name, v1) == "NULL");
    CHECK(view(person_name, set(alternative<person>, v1, person{{}, "foo"})) ==
          "NULL");
    CHECK(view(person_name, set(person_name, v1, "bar")) == "NULL");

    v1 = person{{}, "foo"};
    CHECK(view(person_name, v1) == "foo");
    CHECK(view(person_name, set(alternative<person>, v1, person{{}, "bar"})) ==
          "bar");
    CHECK(view(person_name, set(person_name, v1, "bar")) == "bar");
}

TEST_CASE("lenses, with_opt")
{
    auto first          = at(0);
    auto birthday       = attr(&person::birthday);
    auto month          = attr(&yearday::month);
    auto birthday_month = birthday | month;

    SECTION("lifting composed lenses")
    {
        auto first_month = first | with_opt(birthday_month);

        auto p1 = person{{5, 4}, "juanpe"};

        auto v1 = immer::vector<person>{};
        CHECK(view(first_month, v1) == std::nullopt);
        CHECK(view(first_month, set(at(0), v1, p1)) == std::nullopt);

        v1 = v1.push_back(p1);
        CHECK(view(first_month, v1) == 4);
        p1.birthday.month = 6;
        CHECK(view(first_month, set(at(0), v1, p1)) == 6);
        CHECK(view(first_month, set(first_month, v1, 8)) == 8);
    }

    SECTION("composing lifted lenses")
    {
        auto first_month = first | with_opt(birthday) | with_opt(month);

        auto p1 = person{{5, 4}, "juanpe"};

        auto v1 = immer::vector<person>{};
        CHECK(view(first_month, v1) == std::nullopt);
        CHECK(view(first_month, set(at(0), v1, p1)) == std::nullopt);

        v1 = v1.push_back(p1);
        CHECK(view(first_month, v1) == 4);
        p1.birthday.month = 6;
        CHECK(view(first_month, set(at(0), v1, p1)) == 6);
        CHECK(view(first_month, set(first_month, v1, 8)) == 8);
    }
}

TEST_CASE("lenses, map_opt")
{
    auto first          = at(0);
    auto birthday       = attr(&person::birthday);
    auto month          = attr(&yearday::month);
    auto birthday_month = birthday | month;

    SECTION("mapping composed lenses")
    {
        auto first_month = first | map_opt(birthday_month);

        auto p1 = person{{5, 4}, "juanpe"};

        auto v1 = immer::vector<person>{};
        CHECK(view(first_month, v1) == std::nullopt);
        CHECK(view(first_month, set(at(0), v1, p1)) == std::nullopt);

        v1 = v1.push_back(p1);
        CHECK(view(first_month, v1) == 4);
        p1.birthday.month = 6;
        CHECK(view(first_month, set(at(0), v1, p1)) == 6);
        CHECK(view(first_month, set(first_month, v1, 8)) == 8);
    }

    SECTION("composing mapped lenses")
    {
        auto first_month = first | map_opt(birthday) | map_opt(month);

        auto p1 = person{{5, 4}, "juanpe"};

        auto v1 = immer::vector<person>{};
        CHECK(view(first_month, v1) == std::nullopt);
        CHECK(view(first_month, set(at(0), v1, p1)) == std::nullopt);

        v1 = v1.push_back(p1);
        CHECK(view(first_month, v1) == 4);
        p1.birthday.month = 6;
        CHECK(view(first_month, set(at(0), v1, p1)) == 6);
        CHECK(view(first_month, set(first_month, v1, 8)) == 8);
    }
}

TEST_CASE("lenses, bind_opt")
{
    [[maybe_unused]] auto wrong = bind_opt(attr(&person::name));
    std::optional<person> p1    = person{{5, 4}, "juanpe"};
    // CHECK(view(wrong, p1) == "juanpe"); // should not compile

    SECTION("composing bound lenses")
    {
        auto first       = bind_opt(at(0));
        auto first_first = first | first;

        std::optional<std::vector<std::vector<int>>> v1;

        v1 = {};
        CHECK(view(first, v1) == std::nullopt);
        CHECK(view(first_first, v1) == std::nullopt);
        CHECK(view(first, set(first_first, v1, 256)) == std::nullopt);
        CHECK(view(first_first, set(first_first, v1, 256)) == std::nullopt);

        v1 = {{std::vector<int>{}}};
        CHECK(view(first, v1) != std::nullopt);
        CHECK(view(first_first, v1) == std::nullopt);
        CHECK(view(first_first, set(first_first, v1, 256)) == std::nullopt);

        v1 = {{{42}}};
        CHECK(view(first, v1) != std::nullopt);
        CHECK(view(first_first, v1) == 42);
        CHECK(view(first_first, set(first_first, v1, 256)) == 256);
    }

    SECTION("binding composed bound lenses")
    {
        auto raw_first   = at(0);
        auto first       = bind_opt(at(0));
        auto first_first = bind_opt(raw_first | first);

        std::optional<std::vector<std::vector<int>>> v1;

        v1 = {};
        CHECK(view(first, v1) == std::nullopt);
        CHECK(view(first_first, v1) == std::nullopt);
        CHECK(view(first, set(first_first, v1, 256)) == std::nullopt);
        CHECK(view(first_first, set(first_first, v1, 256)) == std::nullopt);

        v1 = {{std::vector<int>{}}};
        CHECK(view(first, v1) != std::nullopt);
        CHECK(view(first_first, v1) == std::nullopt);
        CHECK(view(first_first, set(first_first, v1, 256)) == std::nullopt);

        v1 = {{{42}}};
        CHECK(view(first, v1) != std::nullopt);
        CHECK(view(first_first, v1) == 42);
        CHECK(view(first_first, set(first_first, v1, 256)) == 256);
    }
}

TEST_CASE("lenses::zip pair", "[lenses][zip][pair]")
{
    struct foo
    {
        int value;
    };

    std::pair<foo, int> baz{{42}, 256};
    auto zipped = zip(attr(&foo::value), lager::identity);

    baz = over(zipped, baz, [](auto x) {
        return std::pair{x.second, x.first};
    });

    CHECK(baz.first.value == 256);
    CHECK(baz.second == 42);
}

TEST_CASE("lenses::zip tuple", "[lenses][zip][tuple]")
{
    struct foo
    {
        int value;
    };
    struct bar
    {
        foo f;
    };

    std::tuple<foo, bar, int> baz{{42}, {{1115}}, 256};

    auto zipped = zip(
        attr(&foo::value), attr(&bar::f) | attr(&foo::value), lager::identity);

    baz = over(zipped, baz, [](auto x) {
        auto [a, b, c] = x;
        return std::tuple{b, c, a};
    });

    CHECK(view(zipped, baz) == std::tuple(1115, 256, 42));
}

TEST_CASE("lenses::fan", "[lenses][fan]")
{
    struct foo
    {
        int value;
    };
    struct bar
    {
        foo f;
        int value;
    };

    auto baz = bar{{42}, 256};

    auto exploded =
        lenses::fan(attr(&bar::f) | attr(&foo::value), attr(&bar::value));

    baz = over(exploded, baz, [](auto x) {
        auto [a, b] = x;
        return std::tuple{b, a};
    });

    CHECK(view(exploded, baz) == std::tuple(256, 42));
}

TEST_CASE("lenses::fan use after move edge case", "[lenses][fan]")
{
    using std::string;

    struct foo
    {
        string value;
    };
    struct bar
    {
        foo f;
        string value;
    };

    auto baz = bar{{"42"}, "256"};

    auto exploded =
        lenses::fan(attr(&bar::f) | attr(&foo::value), attr(&bar::value));

    baz = over(exploded, std::move(baz), [](auto x) {
        auto [a, b] = x;
        return std::tuple{b, a};
    });

    CHECK(view(exploded, baz) == std::tuple("256", "42"));
    CHECK(view(exploded, std::move(baz)) == std::tuple("256", "42"));
}

TEST_CASE("lenses::fan composed with lenses::zip", "[lenses][fan][zip]")
{
    struct foo
    {
        int value;
    };
    struct bar
    {
        foo f;
        int value;
    };

    auto baz = bar{{42}, 256};

    auto exploded = lenses::fan(attr(&bar::f), attr(&bar::value));
    auto zipped   = lenses::zip(attr(&foo::value), lager::identity);

    baz = over(exploded | zipped, baz, [](auto x) {
        auto [a, b] = x;
        return std::tuple{b, a};
    });

    CHECK(view(exploded | zipped, baz) == std::tuple(256, 42));
}

TEST_CASE("lenses::attr multiple", "[lenses][attr][fan]")
{
    struct foo
    {
        int value;
    };
    struct bar
    {
        foo f;
        int value;
    };

    auto baz = bar{{42}, 256};

    auto exploded = lenses::attr(&bar::f, &bar::value);
    auto zipped   = lenses::zip(attr(&foo::value), lager::identity);

    baz = over(exploded | zipped, baz, [](auto x) {
        auto [a, b] = x;
        return std::tuple{b, a};
    });

    CHECK(view(exploded | zipped, baz) == std::tuple(256, 42));
}

auto inline increment = [](auto x) { return x + 1; };

TEST_CASE("lenses::element tuple", "[lenses][element][tuple]")
{
    std::tuple<int, int, int> foo{1, 2, 3};

    CHECK(view(element<0>, foo) == 1);
    CHECK(view(element<1>, foo) == 2);
    CHECK(view(element<2>, foo) == 3);

    CHECK(view(first, foo) == 1);
    CHECK(view(second, foo) == 2);

    CHECK(over(element<1>, foo, increment) == std::tuple(1, 3, 3));
}

TEST_CASE("lenses::element pair", "[lenses][element][pair]")
{
    std::pair<int, int> foo{1, 2};

    CHECK(view(element<0>, foo) == 1);
    CHECK(view(element<1>, foo) == 2);

    CHECK(view(first, foo) == 1);
    CHECK(view(second, foo) == 2);

    CHECK(over(element<1>, foo, increment) == std::pair(1, 3));
}

TEST_CASE("lenses::element array", "[lenses][element][array]")
{
    std::array<int, 3> foo{1, 2, 3};

    CHECK(view(element<0>, foo) == 1);
    CHECK(view(element<1>, foo) == 2);
    CHECK(view(element<2>, foo) == 3);

    CHECK(view(first, foo) == 1);
    CHECK(view(second, foo) == 2);

    CHECK(over(element<1>, foo, increment) == (std::array{1, 3, 3}));
}
