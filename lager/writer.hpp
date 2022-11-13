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
#include <lager/detail/smart_lens.hpp>
#include <lager/watch.hpp>

namespace lager {

template <typename NodeT>
class cursor_base;

//! @defgroup cursors
//! @{

template <typename DerivT>
struct writer_mixin
{
    template <typename T>
    void set(T&& value)
    {
        return node_()->send_up(std::forward<T>(value));
    }

    template <typename Fn>
    void update(Fn&& fn)
    {
        return node_()->send_up(std::forward<Fn>(fn)(node_()->current()));
    }

    template <typename T>
    auto operator[](T&& t) const
    {
        return with(deriv_())[std::forward<T>(t)];
    }

    template <typename Xform, typename Xform2>
    auto xform(Xform&& xf, Xform2&& xf2) const
    {
        return with(deriv_()).xform(std::forward<Xform>(xf),
                                    std::forward<Xform2>(xf2));
    }

    template <typename Lens>
    auto zoom(Lens&& l) const
    {
        return with(deriv_()).zoom(std::forward<Lens>(l));
    }

    const DerivT& make() const& { return static_cast<const DerivT&>(*this); }
    DerivT&& make() && { return static_cast<DerivT&&>(*this); }

protected:
    ~writer_mixin() = default;

private:
    const DerivT& deriv_() const { return *static_cast<const DerivT*>(this); }

    auto node_() const
    {
        if(auto node = detail::access::node(*static_cast<const DerivT*>(this))) {
            return node;
        }
        throw std::runtime_error("Accessing uninitialized writer");
    }
};

template <typename NodeT>
class writer_base
    : public writer_mixin<writer_base<NodeT>>
    , public xform_mixin<writer_base<NodeT>>
{
    template <typename T>
    friend class writer_base;
    friend class detail::access;

    using node_ptr_t = std::shared_ptr<NodeT>;
    node_ptr_t node_;

    const node_ptr_t& node() const& { return node_; }
    node_ptr_t&& node() && { return std::move(node_); }

public:
    using value_type = zug::meta::value_t<NodeT>;

    writer_base() = default;

    template <typename T>
    writer_base(writer_base<T> x)
        : node_(std::move(x.node_))
    {}

    template <typename T>
    writer_base(cursor_base<T> x)
        : node_(detail::access::node(std::move(x)))
    {}

    writer_base(node_ptr_t sig)
        : node_(std::move(sig))
    {}
};

/*!
 * Provides access to writing values of type `T`.
 */
template <typename T>
class writer : public writer_base<detail::cursor_node<T>>
{
    using base_t = writer_base<detail::cursor_node<T>>;

public:
    using base_t::base_t;
};

//! @}

} // namespace lager
