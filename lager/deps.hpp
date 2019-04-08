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

#include <boost/hana/at_key.hpp>
#include <boost/hana/map.hpp>

namespace lager {

template <typename... Deps>
struct deps
{
private:
    boost::hana::map<boost::hana::pair<boost::hana::type<Deps>, Deps>...> impl_;

public:
    template <typename... Ts>
    deps(Ts... ts)
        : impl_{boost::hana::make_pair(boost::hana::type_c<Deps>,
                                       std::move(ts))...}
    {}

    template <typename D>
    auto get() -> decltype(this->impl_[boost::hana::type_c<D>])
    {
        return impl_[boost::hana::type_c<D>];
    }
};

} // namespace lager
