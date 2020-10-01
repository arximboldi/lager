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

#include <lager/detail/access.hpp>
#include <lager/detail/nodes.hpp>

#include <lager/cursor.hpp>

namespace lager {

namespace detail {

template <typename T>
class setter_node : public cursor_node<T>
{
    using base_t    = cursor_node<T>;
    using parent_t  = reader_node<T>;
    using setter_fn = std::function<void(T)>;

    std::shared_ptr<parent_t> parent_;
    setter_fn setter_fn_;

public:
    setter_node(std::shared_ptr<parent_t> p, setter_fn fn)
        : base_t{p->current()}
        , parent_{std::move(p)}
        , setter_fn_{std::move(fn)}
    {}

    void recompute() final { this->push_down(parent_->current()); }
    void refresh() final { parent_->refresh(); }

    void send_up(const T& value) override
    {
        this->push_down(value);
        setter_fn_(value);
    }

    void send_up(T&& value) override
    {
        this->push_down(std::move(value));
        setter_fn_(value);
    }
};

template <typename ParentT, typename FnT>
std::shared_ptr<setter_node<typename ParentT::value_type>>
make_setter_node(std::shared_ptr<ParentT> p, FnT fn)
{
    auto&& pv = *p;
    auto n    = std::make_shared<setter_node<typename ParentT::value_type>>(
        std::move(p), std::move(fn));
    pv.link(n);
    return n;
}

} // namespace detail

template <typename T>
struct setter : cursor_base<detail::setter_node<T>>
{
    using base_t = cursor_base<detail::setter_node<T>>;

public:
    using setter_fn = std::function<void(T)>;

    setter(reader<T> t, setter_fn fn)
        : base_t{detail::make_setter_node(detail::access::node(t), fn)}
    {}
};

} // namespace lager
