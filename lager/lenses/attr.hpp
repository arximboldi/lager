#pragma once

#include <lager/util.hpp>
#include <zug/compose.hpp>

#include <utility>

namespace lager {
namespace lenses {

//! @defgroup lenses
//! @{

/*!
 * `(Part Whole::*) -> Lens<Whole, Part>`
 */
template <typename Member>
auto attr(Member member)
{
    return zug::comp([member](auto&& f) {
        return [&, f = LAGER_FWD(f)](auto&& p) {
            return f(LAGER_FWD(p).*member)([&](auto&& x) {
                auto r    = LAGER_FWD(p);
                r.*member = LAGER_FWD(x);
                return r;
            });
        };
    });
}

//! @}

} // namespace lenses
} // namespace lager
