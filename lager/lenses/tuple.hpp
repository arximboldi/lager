#pragma once

#include <tuple>
#include <utility>

#include <zug/compose.hpp>

#include <lager/util.hpp>

namespace lager {
namespace lenses {
namespace detail {

// due to some obscure behavior on MSVC, this has to be implemented in a
// manually defined function object
template <size_t N>
struct element_t : zug::detail::pipeable {
    template <typename F>
    auto operator()(F&& f) const {
        return [f = std::forward<F>(f)](auto&& whole) {
            return f(std::get<N>(LAGER_FWD(whole)))([&](auto&& part) {
                auto res = LAGER_FWD(whole);
                std::get<N>(res) = LAGER_FWD(part);
                return res;
            });
        };
    }
};
} // namespace detail

//! @defgroup lenses
//! @{

/**
 * N -> Lens<(P1, ..., Pn), PN>
 *
 * Note: works for pairs, tuples, arrays. don't use on variants.
 */
template <size_t N>
auto element = detail::element_t<N>{};
inline auto first = element<0>;
inline auto second = element<1>;

//! @}

} // namespace lenses
} // namespace lager

