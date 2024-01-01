#pragma once

#include <lager/config.hpp>

#include <cassert>
#include <functional>
#include <stdexcept>
#include <thread>

#include <moodycamel/concurrentqueue.h>

namespace lager {

struct lock_free_queue_event_loop
{
    using event_fn = std::function<void()>;

    void post(event_fn ev) { shared_queue_.enqueue(ev); }

    void finish() { LAGER_THROW(std::logic_error{"not implemented!"}); }
    void pause() { LAGER_THROW(std::logic_error{"not implemented!"}); }
    void resume() { LAGER_THROW(std::logic_error{"not implemented!"}); }
    template <typename Fn>
    void async(Fn&& fn)
    {
        LAGER_THROW(std::logic_error{"not implemented!"});
    }

    // If there is an exception, the step() function needs to be re-run for the
    // queue to be fully processed.
    void step()
    {
        assert(thread_id_ == std::this_thread::get_id());
        event_fn ev;
        while (shared_queue_.try_dequeue(ev)) {
            std::move(ev)();
        }
    }

    void adopt()
    {
        assert(shared_queue_.size_approx() == 0);
        thread_id_ = std::this_thread::get_id();
    }

private:
    std::thread::id thread_id_ = std::this_thread::get_id();
    moodycamel::ConcurrentQueue<event_fn> shared_queue_;
};

struct with_lock_free_queue_event_loop
{
    std::reference_wrapper<lock_free_queue_event_loop> loop;

    template <typename Fn>
    void async(Fn&& fn)
    {
        loop.get().async(std::forward<Fn>(fn));
    }
    template <typename Fn>
    void post(Fn&& fn)
    {
        loop.get().post(std::forward<Fn>(fn));
    }
    void finish() { loop.get().finish(); }
    void pause() { loop.get().pause(); }
    void resume() { loop.get().resume(); }
};

} // namespace lager
