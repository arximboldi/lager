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
#include <lager/tags.hpp>

namespace lager {

namespace detail {

template <typename ParentT, typename FnT, typename TagT = transactional_tag>
class setter_node : public cursor_node<typename ParentT::value_type>
{
    using base_t = cursor_node<typename ParentT::value_type>;

    std::shared_ptr<ParentT> parent_;
    FnT setter_fn_;
    bool recomputed_ = false;

public:
    using value_type = typename ParentT::value_type;

    setter_node(std::shared_ptr<ParentT> p, FnT fn)
        : base_t{p->current()}
        , parent_{std::move(p)}
        , setter_fn_{std::move(fn)}
    {}

    void recompute() final
    {
        if (recomputed_)
            recomputed_ = false;
        else
            this->push_down(parent_->current());
    }

    void refresh() final {}

    void send_up(const value_type& value) override
    {
        setter_fn_(value);
        this->push_down(value);
        if constexpr (std::is_same_v<TagT, automatic_tag>) {
            recomputed_ = true;
            this->send_down();
            this->notify();
        }
    }

    void send_up(value_type&& value) override
    {
        setter_fn_(value);
        this->push_down(std::move(value));
        if constexpr (std::is_same_v<TagT, automatic_tag>) {
            recomputed_ = true;
            this->send_down();
            this->notify();
        }
    }
};

template <typename TagT = transactional_tag, typename ParentT, typename FnT>
auto make_setter_node(std::shared_ptr<ParentT> p, FnT&& fn)
{
    using node_t = setter_node<ParentT, std::decay_t<FnT>, TagT>;
    auto&& pv    = *p;
    auto n = std::make_shared<node_t>(std::move(p), std::forward<FnT>(fn));
    pv.link(n);
    return n;
}

} // namespace detail

template <typename TagT = transactional_tag, typename ReaderNode, typename FnT>
auto with_setter(reader_base<ReaderNode> r, FnT&& fn)
{
    auto node = detail::make_setter_node<TagT>(
        detail::access::node(std::move(r)), std::forward<FnT>(fn));
    using node_t = typename decltype(node)::element_type;
    return cursor_base<node_t>{std::move(node)};
}

template <typename ReaderT, typename FnT, typename TagT = transactional_tag>
auto with_setter(ReaderT&& r, FnT&& fn, TagT = {})
{
    return with_setter<TagT>(std::forward<ReaderT>(r), std::forward<FnT>(fn));
}

} // namespace lager
