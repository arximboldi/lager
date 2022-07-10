#pragma once

#include <lager/config.hpp>
#include <lager/util.hpp>

#include <zug/compose.hpp>
#include <zug/meta/detected.hpp>

#include <optional>
#include <stdexcept>
#include <utility>

namespace lager {
namespace lenses {
namespace detail {

// detect if T satifsies the immer API for setting values
template <typename T, typename Key, typename V>
using set_t = std::decay_t<decltype(
    std::declval<T>().set(std::declval<Key>(), std::declval<V>()))>;

template <
    typename Whole,
    typename Part,
    typename Key,
    std::enable_if_t<!zug::meta::is_detected<set_t, Whole, Key, Part>::value,
                     int> = 0>
std::decay_t<Whole> at_or_setter_impl(Whole&& whole, Part&& part, Key&& key)
{
    auto r = std::forward<Whole>(whole);
    LAGER_TRY {
        r.at(std::forward<Key>(key)) = std::forward<Part>(part);
    } LAGER_CATCH(std::out_of_range const&) {}
    return r;
}

template <
    typename Whole,
    typename Part,
    typename Key,
    std::enable_if_t<zug::meta::is_detected<set_t, Whole, Key, Part>::value,
                     int> = 0>
std::decay_t<Whole> at_or_setter_impl(Whole&& whole, Part&& part, Key&& key)
{
    LAGER_TRY {
        (void) whole.at(std::forward<Key>(key));
        return std::forward<Whole>(whole).set(std::forward<Key>(key),
                                              std::forward<Part>(part));
    } LAGER_CATCH(std::out_of_range const&) {}
    return std::forward<Whole>(whole);
}

} // namespace detail

/*!
 * `Key -> Lens<{X}, X>`
 */
template <typename Key>
auto at_or(Key key)
{
    return zug::comp([key](auto&& f) {
        return [f = LAGER_FWD(f), &key](auto&& whole) {
            using Part = std::decay_t<decltype(whole.at(key))>;
            return f([&]() -> Part {
                LAGER_TRY {
                    return LAGER_FWD(whole).at(key);
                } LAGER_CATCH(std::out_of_range const&) {
                    return Part{};
                }
            }())([&](auto&& part) {
                return detail::at_or_setter_impl(
                    LAGER_FWD(whole), LAGER_FWD(part), key);
            });
        };
    });
}

/*!
 * `Key, Default -> Lens<{X}, X>`
 */
template <typename Key, typename Default>
auto at_or(Key key, Default&& def)
{
    return zug::comp([key, def = LAGER_FWD(def)](auto&& f) {
        return [f = LAGER_FWD(f), &key, &def](auto&& whole) {
            using Part = std::decay_t<decltype(whole.at(key))>;
            return f([&]() -> Part {
                LAGER_TRY {
                    return LAGER_FWD(whole).at(key);
                } LAGER_CATCH(std::out_of_range const&) {
                    return def;
                }
            }())([&](auto&& part) {
                return detail::at_or_setter_impl(
                    LAGER_FWD(whole), LAGER_FWD(part), key);
            });
        };
    });
}

} // namespace lenses
} // namespace lager
