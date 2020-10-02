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

template <typename ParentT, typename FnT>
class setter_node : public cursor_node<typename ParentT::value_type>
{
    using base_t = cursor_node<typename ParentT::value_type>;

    std::shared_ptr<ParentT> parent_;
    FnT setter_fn_;

public:
    using value_type = typename ParentT::value_type;

    setter_node(std::shared_ptr<ParentT> p, FnT fn)
        : base_t{p->current()}
        , parent_{std::move(p)}
        , setter_fn_{std::move(fn)}
    {}

    void recompute() final { this->push_down(parent_->current()); }
    void refresh() final { parent_->refresh(); }

    void send_up(const value_type& value) override
    {
        this->push_down(value);
        setter_fn_(value);
    }

    void send_up(value_type&& value) override
    {
        this->push_down(std::move(value));
        setter_fn_(value);
    }
};

template <typename ParentT, typename FnT>
auto make_setter_node(std::shared_ptr<ParentT> p, FnT&& fn)
{
    using node_t = setter_node<ParentT, std::decay_t<FnT>>;
    auto&& pv    = *p;
    auto n = std::make_shared<node_t>(std::move(p), std::forward<FnT>(fn));
    pv.link(n);
    return n;
}

} // namespace detail

template <typename ReaderNode, typename FnT>
auto with_setter(reader_base<ReaderNode> r, FnT&& fn)
{
    auto node    = make_setter_node(detail::access::node(std::move(r)),
                                 std::forward<FnT>(fn));
    using node_t = typename decltype(node)::element_type;
    return cursor_base<node_t>{std::move(node)};
}

} // namespace lager
