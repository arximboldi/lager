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

#include <type_traits>
#include <utility>

namespace lager {

namespace detail {

/*!
 * Provides access to the underlying nodes of different
 * entities. This encapsulates acess to the implementation of
 * node-based objects: don't make access to nodes public, instead
 * friend this class.
 */
class access
{
public:
    /*!
     * Returns a smart pointer to the underlying root node or nodes
     * of an object, if exist.
     */
    template <typename T>
    static decltype(auto) roots(T&& object)
    {
        return std::forward<T>(object).roots();
    }

    /*!
     * Returns a pointer to the underlying node of an object, if
     * exists.
     */
    template <typename T>
    static decltype(auto) node(T&& object)
    {
        return std::forward<T>(object).node();
    }

    /*!
     * Returns an optional boost.signal to the specific watchers of
     * the underlying signal of an object.
     */
    template <typename T>
    static decltype(auto) watchers(T&& object)
    {
        return std::forward<T>(object).watchers();
    }
};

/*!
 * Returns the node type for an object
 */
template <typename ObjectT>
struct node_type
{
    using type = std::decay_t<typename std::decay_t<decltype(
        access::node(std::declval<ObjectT>()))>::element_type>;
};

template <typename ObjectT>
using node_type_t = typename node_type<ObjectT>::type;

} // namespace detail

} // namespace lager
