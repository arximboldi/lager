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

#include "model.hpp"

#include <lager/util.hpp>

#include <lager/extra/cereal/immer_flex_vector.hpp>
#include <lager/extra/cereal/inline.hpp>
#include <lager/extra/cereal/struct.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>

#include <fstream>

namespace todo {

model update(model s, model_action a)
{
    return lager::match(std::move(a))(
        [&](add_todo_action&& a) {
            if (!a.text.empty())
                s.todos = std::move(s.todos).push_front({false, a.text});
            return std::move(s);
        },
        [&](std::pair<std::size_t, item_action>&& a) {
            if (a.first >= s.todos.size()) {
                std::cerr << "Invalid todo::item_action index!" << std::endl;
            } else {
                s.todos =
                    std::holds_alternative<remove_item_action>(a.second)
                        ? std::move(s.todos).erase(a.first)
                        : std::move(s.todos).update(a.first, [&](auto&& t) {
                              return update(t, a.second);
                          });
            }
            return std::move(s);
        });
}

void save(const std::string& fname, model todos)
{
    auto s = std::ofstream{fname};
    s.exceptions(std::fstream::badbit | std::fstream::failbit);
    {
        auto a = cereal::JSONOutputArchive{s};
        save_inline(a, todos);
    }
}

model load(const std::string& fname)
{
    auto s = std::ifstream{fname};
    s.exceptions(std::fstream::badbit);
    auto r = model{};
    {
        auto a = cereal::JSONInputArchive{s};
        load_inline(a, r);
    }
    return r;
}

} // namespace todo
