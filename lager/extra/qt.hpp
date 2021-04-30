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

#ifndef Q_MOC_RUN
#include <lager/cursor.hpp>
#include <lager/reader.hpp>
#include <lager/watch.hpp>
#endif

namespace lager {

namespace detail {

struct qt_helper
{};

} // namespace detail

/*!
 * Return the name of the underlying value supporting a FunQ
 * property.
 */
#define LAGER_QT(name) lager_qt__##name##__

/*!
 * Use in a QObject class declaration to define a property supported
 * by a single member that is not expected to change.
 */
#define LAGER_QT_CONST(type, name)                                             \
    type LAGER_QT(name);                                                       \
    Q_PROPERTY(type name READ name CONSTANT)                                   \
    const type& name() const { return LAGER_QT(name); }                        \
    /**/

#define LAGER_QT_SIGNAL_DETAIL(type, name)                                     \
    Q_SIGNAL void name##Changed(const type& name);                             \
    ::lager::detail::qt_helper funq__##name##__initHelper__ = [this] {         \
        ::lager::watch(LAGER_QT(name), [this](const type& curr) {              \
            this->name##Changed(curr);                                         \
        });                                                                    \
        return ::lager::detail::qt_helper{};                                   \
    }() /**/

/*!
 * Use in a QObject class declaration to define a property supported
 * by a lager::reader<> value.
 */
#define LAGER_QT_READER(type, name)                                            \
    ::lager::reader<type> LAGER_QT(name);                                      \
    Q_PROPERTY(type name READ name NOTIFY name##Changed)                       \
    const type& name() const { return LAGER_QT(name).get(); }                  \
    LAGER_QT_SIGNAL_DETAIL(type, name)                                         \
    /**/

/*!
 * Use in a QObject class declaration to define a property supported
 * by a lager::writer<> value.
 */
#define LAGER_QT_CURSOR(type, name)                                            \
    ::lager::cursor<type> LAGER_QT(name);                                      \
    Q_PROPERTY(type name READ name WRITE set##name NOTIFY name##Changed)       \
    const type& name() const { return LAGER_QT(name).get(); }                  \
    void set##name(const type& val) { LAGER_QT(name).set(val); }               \
    LAGER_QT_SIGNAL_DETAIL(type, name)                                         \
    /**/

} // namespace lager
