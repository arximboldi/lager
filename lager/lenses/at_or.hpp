#include <utility>
#include <stdexcept>
#include <optional>

#include <zug/compose.hpp>
#include <zug/meta/detected.hpp>

#include <lager/util.hpp>

namespace lager {
namespace lenses {
namespace detail {

// detect if T satifsies the immer API for setting values
template <typename T, typename Key, typename OptValue>
using set_t = std::decay_t<decltype(
    std::declval<T>().set(std::declval<Key>(), std::declval<OptValue>().value()))>;

template <typename Whole, typename Part, typename Key, std::enable_if_t<
        !zug::meta::is_detected<set_t, Whole, Key, Part>::value, int> = 0>
std::decay_t<Whole> at_or_setter_impl(Whole&& whole, Part&& part, Key&& key) {
    auto r = std::forward<Whole>(whole);
    if (part.has_value()) {
        try {
            r.at(std::forward<Key>(key)) = std::forward<Part>(part).value();
        } catch (std::out_of_range const&) {}
    }
    return r;
}

template <typename Whole, typename Part, typename Key, std::enable_if_t<
        zug::meta::is_detected<set_t, Whole, Key, Part>::value, int> = 0>
std::decay_t<Whole> at_or_setter_impl(Whole&& whole, Part&& part, Key&& key) {
    if (part.has_value()) {
        try {
            (void) whole.at(std::forward<Key>(key));
            return std::forward<Whole>(whole).set(
                std::forward<Key>(key), std::forward<Part>(part).value());
        } catch (std::out_of_range const&) {}
    }
    return std::forward<Whole>(whole);
}

} // namespace detail

/*!
 * `Key -> Lens<{X}, X>`
 */
template <typename Key>
auto at_or(Key key) {
    return zug::comp([key](auto&& f) {
        return [f = LAGER_FWD(f), &key](auto&& whole) {
            using Part = std::decay_t<decltype(whole.at(key))>;
            return f([&]() -> Part {
                try {
                    return LAGER_FWD(whole).at(key);
                } catch (std::out_of_range const&) { return Part{}; }
            }())([&](auto &&part) {
                return detail::at_or_setter_impl(LAGER_FWD(whole), LAGER_FWD(part), key);
            });
        };
    });
}

/*!
 * `Key, Default -> Lens<{X}, X>`
 */
template <typename Key, typename Default>
auto at_or(Key key, Default &&def) {
    return zug::comp([key, def = LAGER_FWD(def)](auto&& f) {
        return [f = LAGER_FWD(f), &key, &def](auto&& whole) {
            using Part = std::decay_t<decltype(whole.at(key))>;
            return f([&]() -> Part {
                try {
                    return LAGER_FWD(whole).at(key);
                } catch (std::out_of_range const&) { return def; }
            }())([&](auto &&part) {
                return detail::at_or_setter_impl(LAGER_FWD(whole), LAGER_FWD(part), key);
            });
        };
    });
}


} // namespace lenses
} // namespace lager
