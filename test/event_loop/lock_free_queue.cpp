#include <catch2/catch.hpp>

#include <lager/event_loop/lock_free_queue.hpp>
#include <lager/store.hpp>

#include "example/counter/counter.hpp"

TEST_CASE("basic")
{
    auto queue = lager::lock_free_queue_event_loop{};
    auto store = lager::make_store<counter::action>(
        counter::model{}, lager::with_lock_free_queue_event_loop{queue});

    store.dispatch(counter::increment_action{});
    CHECK(store->value == 0);

    queue.step();
    CHECK(store->value == 1);
}

TEST_CASE("threads")
{
    auto queue = lager::lock_free_queue_event_loop{};
    auto store = lager::make_store<counter::action>(
        counter::model{}, lager::with_lock_free_queue_event_loop{queue});
    auto threads = std::vector<std::thread>{};
    threads.reserve(100);

    for (auto i = 0; i < 100; ++i) {
        store.dispatch(counter::increment_action{});
        threads.push_back(
            std::thread([&] { store.dispatch(counter::increment_action{}); }));
    }
    CHECK(store->value == 0);

    for (auto&& t : threads)
        t.join();
    queue.step();

    CHECK(store->value == 200);
}

TEST_CASE("exception")
{
    auto called = 0;
    auto loop   = lager::lock_free_queue_event_loop{};

    loop.post([&] { throw std::runtime_error{"noo!"}; });
    loop.post([&] { ++called; });

    CHECK_THROWS(loop.step());
    CHECK(called == 0);

    loop.step();
    CHECK(called == 1);
}
