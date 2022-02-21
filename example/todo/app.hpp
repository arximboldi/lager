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

#pragma once

#include "model.hpp"

#include <lager/effect.hpp>

namespace todo {

struct app
{
    model doc;
    std::filesystem::path path;
};

struct save_action
{
    std::filesystem::path file;
};
struct load_action
{
    std::filesystem::path file;
};
struct load_result_action
{
    std::filesystem::path file;
    model doc;
};
using app_action =
    std::variant<model_action, save_action, load_action, load_result_action>;

struct logger
{
    std::function<void(const std::string& text)> error;
};

using app_result = lager::result<app, app_action, lager::deps<logger&>>;

app_result update(app, app_action);

} // namespace todo

LAGER_STRUCT(todo, app, doc, path);
