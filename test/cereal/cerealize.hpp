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

#include <cereal/archives/json.hpp>
#include <sstream>

template <typename T>
T cerealize(const T& x)
{
    auto os = std::ostringstream{};
    {
        auto ar = cereal::JSONOutputArchive{os};
        ar(x);
    }
    auto is = std::istringstream{os.str()};
    {
        T xx;
        auto ar = cereal::JSONInputArchive{is};
        ar(xx);
        return xx;
    }
}
