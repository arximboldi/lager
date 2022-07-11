#pragma once

#include <lager/config.hpp>
#include <lager/event_loop/queue.hpp>

#include <QQuickItem>
#include <QTimer>

#include <functional>
#include <stdexcept>
#include <thread>
#include <utility>

namespace lager {

class event_loop_quick_item : public QQuickItem
{
    lager::queue_event_loop queue_;
    std::thread::id this_id_ = std::this_thread::get_id();
    bool polished_           = false;
    QTimer timer_{this};

public:
    event_loop_quick_item(QQuickItem* parent = nullptr)
        : QQuickItem{parent}
    {
        connect(&timer_, &QTimer::timeout, this, &event_loop_quick_item::step);
    }

    virtual void step() { queue_.step(); }

    void updatePolish() override
    {
        QQuickItem::updatePolish();
        if (!polished_) {
            polished_ = true;
            // Ideally the polish is working at 60Hz, so this reset just keeps
            // the timer always waiting.  However, when the window owning this
            // item is not visible, the polish may actually never be called.
            // This ensures at least a 30Hz update rate in that scenario.
            timer_.start(1000 / 30);
            step();
            update();
        }
        QCoreApplication::postEvent(this, new QEvent{QEvent::PolishRequest});
    }

    QSGNode* updatePaintNode(QSGNode*, UpdatePaintNodeData*) override
    {
        polished_ = false;
        return nullptr;
    }

    bool event(QEvent* event) override
    {
        switch (event->type()) {
        case QEvent::PolishRequest:
            polish();
            return true;
        default:
            return QQuickItem::event(event);
        }
    }

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

    void finish() { LAGER_THROW(std::logic_error{"not implemented!"}); }
    void pause() { LAGER_THROW(std::logic_error{"not implemented!"}); }
    void resume() { LAGER_THROW(std::logic_error{"not implemented!"}); }
    template <typename Fn>
    void async(Fn&& fn)
    {
        LAGER_THROW(std::logic_error{"not implemented!"});
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
