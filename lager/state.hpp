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
#include <lager/cursor.hpp>
#include <lager/detail/access.hpp>
#include <lager/detail/nodes.hpp>

#include <lager/tags.hpp>
#include <lager/util.hpp>

namespace lager {

namespace detail {

template <typename T, typename TagT = transactional_tag>
class state_node : public cursor_node<T>
{
public:
    using value_type = T;

    using cursor_node<T>::cursor_node;

    void send_up(const value_type& value) final
    {
        this->push_down(value);
        if constexpr (std::is_same_v<TagT, automatic_tag>) {
            this->send_down();
            this->notify();
        }
    }

    void send_up(value_type&& value) final
    {
        this->push_down(std::move(value));
        if constexpr (std::is_same_v<TagT, automatic_tag>) {
            this->send_down();
            this->notify();
        }
    }
};

template <typename TagT = transactional_tag, typename T>
auto make_state_node(T&& value)
{
    return std::make_shared<state_node<std::decay_t<T>, TagT>>(
        std::forward<T>(value));
}

} // namespace detail

template <typename T, typename TagT = transactional_tag>
class state : public cursor_base<detail::state_node<T, TagT>>
{
    using base_t = cursor_base<detail::state_node<T, TagT>>;

    friend class detail::access;
    auto roots() const { return detail::access::node(*this); }

public:
    using value_type = T;
    using base_t::base_t;

    state()
        : base_t{detail::make_state_node<TagT>(T())}
    {}
    state(T value)
        : base_t{detail::make_state_node<TagT>(std::move(value))}
    {}

    state& operator=(const state&) = delete;
    state(const state&)            = delete;

    state(state&&) = default;
    state& operator=(state&&) = default;
};

template <typename T>
state<T> make_state(T value)
{
    return value;
}

template <typename T, typename TagT>
state<T, TagT> make_state(T value, TagT)
{
    return value;
}

} // namespace lager
