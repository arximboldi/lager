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

#include "item.hpp"

#include <lager/util.hpp>

namespace todo {

item update(item s, item_action a)
{
    return lager::match(std::move(a))(
        [&](toggle_item_action&& a) {
            s.done = !s.done;
            return std::move(s);
        },
        [&](remove_item_action&& a) { return std::move(s); });
}

} // namespace todo
