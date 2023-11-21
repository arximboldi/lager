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

// detect if T satisfies the immer API for setting values
template <typename T, typename Key, typename OptValue>
using set_opt_t = std::decay_t<decltype(std::declval<T>().set(
    std::declval<Key>(), std::declval<OptValue>().value()))>;

// detect if T satisfies the immer API for inserting values
template <typename T, typename OptValue>
using insert_opt_t = std::decay_t<decltype(std::declval<T>().insert(
    std::declval<OptValue>().value()))>;

template <typename T, typename K>
using has_count_t = decltype(std::declval<T>().count(std::declval<K>()));

// When building without exception support, returns whether the container has
// the key, using some heuristics to distinguish vector-likes or map-like
// containers.  Otherwise it just returns true and we use the normal
// exception-based mechanism.
template <typename Whole, typename Key>
bool maybe_has_key(const Whole& whole, const Key& k) noexcept
{
#ifdef LAGER_NO_EXCEPTIONS
    if constexpr (zug::meta::is_detected<has_count_t, Whole, Key>::value) {
        return whole.count(k) > 0;
    } else if constexpr (std::is_convertible<Key, typename Whole::size_type>::
                             value) {
        const auto k_ = static_cast<typename Whole::size_type>(k);
        return k_ >= typename Whole::size_type{} && k_ < whole.size();
    } else {
        static_assert(
            zug::meta::is_detected<has_count_t, Whole, Key>::value ||
                std::is_convertible<Key, typename Whole::size_type>::value,
            "at lense not supported with LAGER_NO_EXCEPTIONS");
        return true;
    }
#else
    return true;
#endif
}

template <typename Whole,
          typename Part,
          typename Key,
          std::enable_if_t<
              !zug::meta::is_detected<set_opt_t, Whole, Key, Part>::value &&
                  !zug::meta::is_detected<insert_opt_t, Whole, Part>::value,
              int> = 0>
std::decay_t<Whole> at_setter_impl(Whole&& whole, Part&& part, Key&& key)
{
    auto r = std::forward<Whole>(whole);
    if (maybe_has_key(r, key) && part.has_value()) {
        LAGER_TRY
        {
            r.at(std::forward<Key>(key)) = std::forward<Part>(part).value();
        }
        LAGER_CATCH(std::out_of_range const&) {}
    }
    return r;
}

template <
    typename Whole,
    typename Part,
    typename Key,
    std::enable_if_t<zug::meta::is_detected<set_opt_t, Whole, Key, Part>::value,
                     int> = 0>
std::decay_t<Whole> at_setter_impl(Whole&& whole, Part&& part, Key&& key)
{
    if (maybe_has_key(whole, key) && part.has_value()) {
        LAGER_TRY
        {
            (void) whole.at(std::forward<Key>(key));
            return std::forward<Whole>(whole).set(
                std::forward<Key>(key), std::forward<Part>(part).value());
        }
        LAGER_CATCH(std::out_of_range const&) {}
    }
    return std::forward<Whole>(whole);
}

template <
    typename Whole,
    typename Part,
    typename Key,
    std::enable_if_t<zug::meta::is_detected<insert_opt_t, Whole, Part>::value,
                     int> = 0>
std::decay_t<Whole> at_setter_impl(Whole&& whole, Part&& part, Key&& key)
{
    if (part.has_value()) {
        LAGER_TRY
        {
            return std::forward<Whole>(whole).insert(
                std::forward<Part>(part).value());
        }
        LAGER_CATCH(std::out_of_range const&) {}
    }
    return std::forward<Whole>(whole);
}

} // namespace detail

//! @defgroup lenses
//! @{

/*!
 * `Key -> Lens<{X}, [X]>`
 */
template <typename Key>
auto at(Key key)
{
    return zug::comp([key](auto&& f) {
        return [f = LAGER_FWD(f), &key](auto&& whole) {
            using Part = std::optional<std::decay_t<decltype(whole.at(key))>>;
            return f([&]() -> Part {
                if (!detail::maybe_has_key(whole, key))
                    return std::nullopt;
                LAGER_TRY { return LAGER_FWD(whole).at(key); }
                LAGER_CATCH(std::out_of_range const&) { return std::nullopt; }
            }())([&](Part part) {
                return detail::at_setter_impl(
                    LAGER_FWD(whole), std::move(part), key);
            });
        };
    });
}

//! @}

} // namespace lenses
} // namespace lager
