#pragma once

#include <lager/lenses.hpp>
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
    return getset(
        [=](auto &&p) -> decltype(auto) {
            return LAGER_FWD(p).*member;
        },
        [=](auto p, auto &&x) {
            p.*member = LAGER_FWD(x);
            return p;
        });
}

//! @}

} // namespace lenses
} // namespace lager
