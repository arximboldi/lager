#pragma once

#include <lager/lenses.hpp>
#include <memory>
#include <type_traits>
#include <utility>

namespace lager {
namespace detail {

template <typename Whole, typename Part>
struct lens_i
{
    virtual ~lens_i()                                  = default;
    virtual Part view(Whole const&) const              = 0;
    virtual Whole set(Whole const&, Part const&) const = 0;
};

template <typename Lens, typename Whole, typename Part>
struct lens_holder : public lens_i<Whole, Part>
{
    Lens value;

    template <typename T>
    lens_holder(T&& other)
        : value{std::forward<T>(other)}
    {}

    Part view(Whole const& w) const override { return ::lager::view(value, w); }

    Whole set(Whole const& w, Part const& p) const override
    {
        return ::lager::set(value, w, p);
    }
};

} // namespace detail

//! @defgroup lenses-api
//! @{

template <typename Whole, typename Part>
class lens : zug::detail::pipeable
{
    std::shared_ptr<detail::lens_i<Whole, Part> const> holder_;

public:
    template <typename Lens,
              typename std::enable_if<
                  !std::is_same_v<std::decay_t<Lens>, std::decay_t<lens>>,
                  int>::type = 0>
    lens(Lens&& lens)
        : holder_{new detail::lens_holder<std::decay_t<Lens>, Whole, Part>{
              std::forward<Lens>(lens)}}
    {}

    template <typename F>
    auto operator()(F&& f) const
    {
        return [this, f = std::forward<F>(f)](auto&& p) {
            return f(holder_->view(std::forward<decltype(p)>(p)))(
                [&](auto&& x) {
                    return holder_->set(std::forward<decltype(p)>(p),
                                        std::forward<decltype(x)>(x));
                });
        };
    }
};

//! @}

} // namespace lager
