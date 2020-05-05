#pragma once

#include <lager/event_loop/queue.hpp>

#include <QQuickItem>

namespace lager {

class event_loop_quick_item : public QQuickItem
{
    lager::queue_event_loop queue_;
    std::thread::id this_id_ = std::this_thread::get_id();

public:
    event_loop_quick_item(QQuickItem* parent = nullptr)
        : QQuickItem{parent}
    {}

    void updatePolish() override { queue_.step(); }

    template <typename Fn>
    void post(Fn&& fn)
    {
        auto current_id = std::this_thread::get_id();
        if (this_id_ == current_id) {
            queue_.post(std::forward<Fn>(fn));
            polish();
        } else {
            QMetaObject::invokeMethod(
                this,
                [this, fn = std::forward<Fn>(fn)]() mutable {
                    queue_.post(std::move(fn));
                    polish();
                },
                Qt::QueuedConnection);
        }
    }

    void finish() { throw std::logic_error{"not implemented!"}; }
    void pause() { throw std::logic_error{"not implemented!"}; }
    void resume() { throw std::logic_error{"not implemented!"}; }
    template <typename Fn>
    void async(Fn&& fn)
    {
        throw std::logic_error{"not implemented!"};
    }
};

struct with_qml_event_loop
{
    std::reference_wrapper<event_loop_quick_item> loop;

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
