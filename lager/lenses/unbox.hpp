#pragma once

#include <utility>

#include <zug/compose.hpp>
#include <lager/util.hpp>

namespace lager {
namespace lenses {

/*!
 * `Lens<box<T>, T>`
 */
ZUG_INLINE_CONSTEXPR auto unbox = zug::comp([](auto&& f) {
    return [f](auto&& p) {
        return f(LAGER_FWD(p).get())([&](auto&& x) {
            return std::decay_t<decltype(p)>{LAGER_FWD(x)};
        });
    };
});


} // namespace lenses
} // namespace lager
