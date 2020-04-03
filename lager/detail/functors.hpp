#pragma once

#include <utility>

namespace lager {
namespace detail {

template <typename T>
struct const_functor;

template <typename T>
auto make_const_functor(T&& x) -> const_functor<T> {
    return {std::forward<T>(x)};
}

template <typename T>
struct const_functor {
    T value;

    template <typename Fn>
    const_functor operator()(Fn&&) && {
        return std::move(*this);
    }
};

template <typename T>
struct identity_functor;

template <typename T>
auto make_identity_functor(T&& x) -> identity_functor<T> {
    return {std::forward<T>(x)};
}

template <typename T>
struct identity_functor {
    T value;

    template <typename Fn>
    auto operator()(Fn&& f) && {
        return make_identity_functor(
            std::forward<Fn>(f)(std::forward<T>(value)));
    }
};

} // namespace detail
} // namespace lager