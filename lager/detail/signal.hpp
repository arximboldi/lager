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

#include <boost/intrusive/list.hpp>

#include <memory>

namespace lager {
namespace detail {

template <typename... Args>
struct forwarder;

template <typename... Args>
class signal
{
    using slot_list_base = boost::intrusive::list_base_hook<
        boost::intrusive::link_mode<boost::intrusive::auto_unlink>>;

public:
    using forwarder_type = forwarder<Args...>;

    struct slot_base : slot_list_base
    {
        virtual ~slot_base()             = default;
        virtual void operator()(Args...) = 0;
    };

    template <typename Fn>
    class slot : public slot_base
    {
        Fn fn_;

    public:
        slot(Fn fn)
            : fn_{std::move(fn)}
        {}
        void operator()(Args... args) final { fn_(args...); }
    };

    struct connection
    {
        std::unique_ptr<slot_base> slot_;

    public:
        connection(std::unique_ptr<slot_base> s)
            : slot_{std::move(s)}
        {}
    };

    template <typename Fn>
    connection connect(Fn&& fn)
    {
        using slot_t = slot<std::decay_t<Fn>>;
        auto s       = std::make_unique<slot_t>(std::forward<Fn>(fn));
        slots_.push_back(*s);
        return {std::move(s)};
    }

    void add(slot_base& slot) { slots_.push_back(slot); }

    template <typename... Args2>
    void operator()(Args2&&... args)
    {
        for (auto&& s : slots_)
            s(std::forward<Args2>(args)...);
    }

    bool empty() const { return slots_.empty(); }

private:
    using slot_list =
        boost::intrusive::list<slot_base,
                               boost::intrusive::constant_time_size<false>>;

    slot_list slots_;
};

template <typename... Args>
struct forwarder
    : signal<Args...>::slot_base
    , signal<Args...>
{
    void operator()(Args... args) override
    {
        signal<Args...>::operator()(args...);
    }
};

} // namespace detail
} // namespace lager
