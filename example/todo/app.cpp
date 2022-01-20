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

#include "app.hpp"

#include <iostream>

namespace todo {

app_result update(app s, app_action a)
{
    return lager::match(std::move(a))(
        [&](save_action&& a) -> app_result {
            s.path   = a.file.replace_extension("todo");
            auto eff = [m = s.doc, f = s.path](auto&& ctx) {
                try {
                    std::cout << "saving file: " << f << std::endl;
                    save(f, m);
                } catch (std::exception const& err) {
                    std::cerr << "error saving file: " << err.what()
                              << std::endl;
                    lager::get<logger>(ctx).error("Could not save file: " +
                                                  f.string());
                }
            };
            return {std::move(s), eff};
        },
        [&](load_action&& a) -> app_result {
            auto eff = [f = std::move(a.file)](auto&& ctx) {
                std::cout << "loading file: " << f << std::endl;
                try {
                    auto m = load(f);
                    ctx.dispatch(load_result_action{f, std::move(m)});
                } catch (std::exception const& err) {
                    std::cerr << "error loading file: " << err.what()
                              << std::endl;
                    lager::get<logger>(ctx).error("Could not load file: " +
                                                  f.string());
                }
            };
            return {std::move(s), eff};
        },
        [&](load_result_action&& a) -> app_result {
            s.doc  = std::move(a.doc);
            s.path = std::move(a.file);
            return std::move(s);
        },
        [&](model_action&& a) -> app_result {
            s.doc = update(std::move(s.doc), std::move(a));
            return std::move(s);
        });
}

} // namespace todo
