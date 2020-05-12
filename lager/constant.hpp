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

#include <lager/commit.hpp>
#include <lager/reader.hpp>

namespace lager {

namespace detail {

template <typename T>
class constant_node : public root_node<T, reader_node>
{
    using base_t = root_node<T, reader_node>;

public:
    using base_t::base_t;

    virtual void recompute() final {}
};

template <typename T>
auto make_constant_node(T&& v)
{
    return std::make_shared<constant_node<std::decay_t<T>>>(std::forward<T>(v));
}

} // namespace detail

//! @defgroup cursors
//! @{

template <typename T>
class constant : public reader_base<detail::constant_node<T>>
{
    using base_t = reader_base<detail::constant_node<T>>;

public:
    using value_type = T;

    constant(T v)
        : base_t{detail::make_constant_node(std::move(v))}
    {}

private:
    friend class detail::access;
    auto roots() const { return detail::access::node(*this); }
};

template <typename T>
auto make_constant(T&& v) -> constant<std::decay_t<T>>
{
    return std::forward<T>(v);
}

//! @}

} // namespace lager
