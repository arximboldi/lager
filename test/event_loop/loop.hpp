#include <thread>

#include <lager/util.hpp>
#include <lager/context.hpp>

namespace loop {

struct model
{
    std::thread::id main_id;
    std::thread::id worker_id;
};

struct set_main_id_action
{
    std::thread::id main_id;
};

struct set_worker_id_action
{
    std::thread::id worker_id;
};

struct request_async_work_action
{};

using action = std::variant<set_main_id_action, set_worker_id_action, request_async_work_action>;

using effect = lager::effect<action>;

using context = lager::context<action>;

inline std::pair<model, effect> update(model m, action action)
{
    return std::visit(lager::visitor{[&](set_main_id_action a) {
                                         m.main_id = a.main_id;
                                         return std::make_pair(m, effect{lager::noop});
                                     },
                                     [&](set_worker_id_action a) {
                                         m.worker_id = a.worker_id;
                                         return std::make_pair(m, effect{lager::noop});
                                     },
                                     [&](request_async_work_action) {
                                         effect async_work_effect = [](auto&& ctx) {
                                             ctx.loop().async([ctx]() {
                                                 ctx.dispatch(set_worker_id_action{std::this_thread::get_id()});
                                             });
                                         };
                                         return std::make_pair(m, async_work_effect);
                                     }},
                      action);
}

}  // namespace loop
