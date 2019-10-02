//
// lager - library for functional interactive c++ programs
// Copyright (C) 2017 Juan Pedro Bolivar Puente
//
// This file is part of lager.
//
// lager is free software: you can redistribute it and/or modify
// it under the terms of the MIT License, as detailed in the LICENSE
// file located at the root of this source code distribution,
// or here: <https://github.com/arximboldi/lager/blob/master/LICENSE>
//

#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_set>

namespace testing {
namespace mocks {

template <typename T>
struct defaulting
{
    template <typename... Args>
    T operator()(Args&&...)
    {
        return T();
    }
};

template <typename T>
struct returning
{
    returning() = default;

    template <typename Fn>
    returning(const Fn& mock)
        : mock_(mock)
    {}

    template <typename... Args>
    T operator()(Args&&...)
    {
        return mock_();
    }

private:
    std::function<T()> mock_;
};

} // namespace mocks

namespace detail {

class spy_base
{
public:
    std::size_t count() const { return *count_; }

    void called() { ++*count_; }

private:
    std::shared_ptr<std::size_t> count_ = std::make_shared<std::size_t>(0);
};

} // namespace detail

/*!
 * Functor that counts the number of times it was called.
 *
 * @todo Support comparing the actual arguments.  Keep generic
 * interface using boost::any
 */
template <typename MockT = mocks::defaulting<void>>
class spy_fn : public detail::spy_base
{
    MockT mock_;

public:
    spy_fn() = default;

    template <typename MockT2>
    spy_fn(MockT2 mock)
        : mock_(std::move(mock))
    {}

    template <typename MockT2>
    spy_fn(MockT2 mock, const spy_base& spy)
        : spy_base(spy)
        , mock_(std::move(mock))
    {}

    template <typename... Args>
    decltype(auto) operator()(Args&&... args)
    {
        called();
        return this->mock_(std::forward<Args>(args)...);
    }
};

/*!
 * Returns a spy object that uses fn as mock implementation.
 */
template <typename Fn>
inline auto spy(const Fn& fn) -> spy_fn<Fn>
{
    return spy_fn<Fn>(fn);
}

/*!
 * Returns a spy object with a no-op mock implementation.
 */
inline auto spy() -> spy_fn<> { return spy_fn<>(); }

namespace detail {

template <typename MockT>
class scoped_intruder
{
    MockT* mock_;
    MockT original_;

public:
    scoped_intruder& operator=(const scoped_intruder&) = delete;
    scoped_intruder(const scoped_intruder&)            = delete;

    scoped_intruder(scoped_intruder&& other) { swap(*this, other); }

    scoped_intruder& operator=(scoped_intruder&& other)
    {
        if (this != &other) {
            swap(*this, other);
        }
    }

    scoped_intruder(MockT& mock, MockT replacement)
        : mock_(&mock)
        , original_(mock)
    {
        *mock_ = replacement;
    }

    ~scoped_intruder()
    {
        if (mock_) {
            *mock_ = original_;
        }
    }

    template <typename... Args>
    auto operator()(Args&&... args)
        -> decltype((*mock_)(std::forward<Args>(args)...))
    {
        assert(mock_ && "must not call intruder after having moved from it");
        return (*mock_)(std::forward<Args>(args)...);
    }

    friend void swap(scoped_intruder& a, scoped_intruder& b)
    {
        using std::swap;
        swap(a.mock_, b.mock_);
        swap(a.original_, b.original_);
    }
};

} // namespace detail

/*!
 * Given a functor object `mock` of a general functor with type erasure
 * (e.g. std::function or boost::function) installs a spy that counts
 * the calls and returns such a spy.
 */
template <typename MockT>
inline auto spy_on(MockT& mock) -> spy_fn<detail::scoped_intruder<MockT>>
{
    auto s = spy(mock);
    return {detail::scoped_intruder<MockT>(mock, s), s};
}

/*!
 * Like @a spy_on(), but it installs the `replacement` function instead
 * of keeping the original one.  The spy is uninstalled on
 * destruction, and it is not copyable.
 */
template <typename MockT, typename FnT>
inline auto spy_on(MockT& mock, const FnT& replacement)
    -> spy_fn<detail::scoped_intruder<MockT>>
{
    auto s = spy(replacement);
    return {detail::scoped_intruder<MockT>(mock, s), s};
}

namespace detail {

struct default_copy_spy_base_t
{};

} // namespace detail

/*!
 * Utility for testing how many times an object is copied.
 */
template <typename BaseT = detail::default_copy_spy_base_t>
struct copy_spy : BaseT
{
    using base_t = BaseT;

    spy_fn<> copied;

    copy_spy(BaseT base = {})
        : BaseT{std::move(base)}
    {}
    copy_spy(copy_spy&&) = default;
    copy_spy& operator=(copy_spy&&) = default;

    copy_spy(const copy_spy& x)
        : base_t(x)
        , copied(x.copied)
    {
        copied();
    }

    copy_spy& operator=(const copy_spy& x)
    {
        base_t::operator=(x);
        copied          = x.copied;
        copied();
        return *this;
    }
};

} // namespace testing
