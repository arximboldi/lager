#pragma once

#include <lager/util.hpp>
#include <zug/compose.hpp>

#include <utility>

namespace lager {
namespace lenses {

//! @defgroup lenses
//! @{

/*!
 * `Lens<box<T>, T>`
 */
ZUG_INLINE_CONSTEXPR auto unbox = zug::comp([](auto&& f) {
    return [f](auto&& p) {
        return f(LAGER_FWD(p).get())(
            [&](auto&& x) { return std::decay_t<decltype(p)>{LAGER_FWD(x)}; });
    };
});

//! @}

} // namespace lenses
} // namespace lager
