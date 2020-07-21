#pragma once

#include <lager/lenses.hpp>
#include <lager/util.hpp>

#include <zug/compose.hpp>
#include <zug/meta/detected.hpp>
#include <zug/meta/util.hpp>

#include <optional>
#include <type_traits>
#include <utility>

namespace lager {
namespace lenses {

namespace detail {

template <template <typename> typename PartMeta, typename Lens>
auto opt_impl(Lens&& lens)
{
    return zug::comp([lens = std::forward<Lens>(lens)](auto&& f) {
        return [&, f = LAGER_FWD(f)](auto&& whole) {
            using Part = typename PartMeta<std::decay_t<decltype(::lager::view(
                lens,
                std::declval<std::decay_t<decltype(whole.value())>>()))>>::type;

            if (whole.has_value()) {
                return f(Part{::lager::view(lens, LAGER_FWD(whole).value())})(
                    [&](Part part) {
                        if (part.has_value()) {
                            return std::decay_t<decltype(whole)>{
                                ::lager::set(lens,
                                             LAGER_FWD(whole).value(),
                                             std::move(part).value())};
                        } else {
                            return LAGER_FWD(whole);
                        }
                    });
            } else {
                return f(Part{std::nullopt})(
                    [&](auto&&) { return LAGER_FWD(whole); });
            }
        };
    });
}

template <typename T>
struct remove_opt
{
    using type = T;
};
template <typename T>
struct remove_opt<std::optional<T>>
{
    using type = T;
};
template <typename T>
using remove_opt_t = typename remove_opt<T>::type;

template <typename T>
struct to_opt
{
    using type = std::optional<remove_opt_t<std::decay_t<T>>>;
};

template <typename T>
struct add_opt
{
    using type = std::optional<std::decay_t<T>>;
};

} // namespace detail

//! @defgroup lenses
//! @{

/*!
 * `Lens<W, P> -> Lens<[W], [P]>`
 */
template <typename Lens>
auto map_opt(Lens&& lens)
{
    return detail::opt_impl<detail::add_opt>(std::forward<Lens>(lens));
}

/*!
 * `Lens<W, [P]> -> Lens<[W], [P]>`
 */
template <typename Lens>
auto bind_opt(Lens&& lens)
{
    return detail::opt_impl<zug::meta::identity>(std::forward<Lens>(lens));
}

/*!
 * `(Lens<W, P> | Lens<W, [P]>) -> Lens<[W], [P]>`
 */
template <typename Lens>
auto with_opt(Lens&& lens)
{
    return detail::opt_impl<detail::to_opt>(std::forward<Lens>(lens));
}

/*!
 * `X -> Lens<[X], X>`
 */
template <typename T>
auto value_or(T&& t)
{
    return zug::comp([t = std::forward<T>(t)](auto&& f) {
        return [&, f = LAGER_FWD(f)](auto&& whole) {
            return f(LAGER_FWD(whole).value_or(std::move(t)))(
                [&](auto&& x) { return LAGER_FWD(x); });
        };
    });
}

/*!
 * `() -> Lens<[X], X>`
 */
ZUG_INLINE_CONSTEXPR auto value_or()
{
    return zug::comp([](auto&& f) {
        return [&, f = LAGER_FWD(f)](auto&& whole) {
            using T = std::decay_t<decltype(whole.value())>;
            return f(LAGER_FWD(whole).value_or(T{}))(
                [&](auto&& x) { return LAGER_FWD(x); });
        };
    });
}

/*!
 * `() -> Lens<[X], X>`
 */
ZUG_INLINE_CONSTEXPR auto or_default = value_or();

/*!
 * `Lens<T, [T]>`
 */
ZUG_INLINE_CONSTEXPR auto force_opt = zug::comp([](auto&& f) {
    return [f = LAGER_FWD(f)](auto&& p) {
        using opt_t = std::optional<std::decay_t<decltype(p)>>;
        return f(opt_t{LAGER_FWD(p)})(
            [&](auto&& x) { return LAGER_FWD(x).value_or(LAGER_FWD(p)); });
    };
});

//! @}

} // namespace lenses
} // namespace lager
