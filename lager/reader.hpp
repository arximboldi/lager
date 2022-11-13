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
#include <lager/with.hpp>

#include <zug/meta/value_type.hpp>

namespace lager {

template <typename NodeT>
class cursor_base;

//! @defgroup cursors
//! @{

template <typename DerivT>
struct reader_mixin
{
    decltype(auto) get() const { return node_()->last(); }
    decltype(auto) operator*() const { return get(); }
    decltype(auto) operator->() const { return &get(); }

    template <typename T>
    auto operator[](T&& t) const
    {
        return with(deriv_())[std::forward<T>(t)];
    }

    template <typename Xform>
    auto xform(Xform&& xf) const
    {
        return with(deriv_()).xform(std::forward<Xform>(xf));
    }

    template <typename Lens>
    auto zoom(Lens&& l) const
    {
        return with(deriv_()).zoom(std::forward<Lens>(l));
    }

    template <typename TagT = transactional_tag, typename FnT>
    auto setter(FnT&& fn) const
    {
        return with_setter(deriv_(), std::forward<FnT>(fn), TagT{});
    }

    const DerivT& make() const& { return static_cast<const DerivT&>(*this); }
    DerivT&& make() && { return static_cast<DerivT&&>(*this); }

protected:
    ~reader_mixin() = default;

private:
    const DerivT& deriv_() const { return *static_cast<const DerivT*>(this); }

    auto node_() const
    {
        if(auto node = detail::access::node(*static_cast<const DerivT*>(this))) {
            return node;
        }
        throw std::runtime_error("Accessing uninitialized reader");
    }
};

template <typename NodeT>
class reader_base
    : public reader_mixin<reader_base<NodeT>>
    , public xform_mixin<reader_base<NodeT>>
    , public watchable_base<NodeT>
{
    template <typename T>
    friend class reader_base;
    friend class detail::access;

    using base_t = watchable_base<NodeT>;

public:
    using value_type = zug::meta::value_t<NodeT>;

    reader_base() = default;

    template <typename T,
              std::enable_if_t<std::is_same_v<zug::meta::value_t<NodeT>,
                                              zug::meta::value_t<T>>,
                               int> = 0>
    reader_base(reader_base<T> x)
        : base_t{std::move(x)}
    {}

    template <typename T,
              std::enable_if_t<std::is_same_v<zug::meta::value_t<NodeT>,
                                              zug::meta::value_t<T>>,
                               int> = 0>
    reader_base(cursor_base<T> x)
        : base_t{std::move(x)}
    {}

    template <typename NodeT2>
    reader_base(std::shared_ptr<NodeT2> n)
        : base_t{std::move(n)}
    {}
};

/*!
 * Provides access to reading values of type `T`.
 */
template <typename T>
class reader : public reader_base<detail::reader_node<T>>
{
    using base_t = reader_base<detail::reader_node<T>>;

public:
    using base_t::base_t;
};

//! @}

} // namespace lager
