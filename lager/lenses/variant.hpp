#pragma once

#include <lager/util.hpp>
#include <zug/compose.hpp>

#include <optional>
#include <utility>
#include <variant>

namespace lager {
namespace lenses {
namespace detail {

// due to some obscure behavior on MSVC, this has to be implemented in a
// manually defined function object
template <typename T>
struct alternative_t : zug::detail::pipeable
{
    using Part = std::optional<T>;
    template <typename F>
    auto operator()(F&& f) const
    {
        return [f = std::forward<F>(f)](auto&& p) {
            using Whole = std::decay_t<decltype(p)>;
            return f([&]() -> Part {
                if (std::holds_alternative<T>(p)) {
                    return std::get<T>(LAGER_FWD(p));
                } else {
                    return std::nullopt;
                }
            }())([&](Part x) -> Whole {
                if (x.has_value() && std::holds_alternative<T>(p)) {
                    return std::move(x).value();
                } else {
                    return LAGER_FWD(p);
                }
            });
        };
    }
};
} // namespace detail

//! @defgroup lenses
//! @{

template <typename T>
inline auto alternative = detail::alternative_t<T>{};

//! @}

} // namespace lenses
} // namespace lager
