//
// lager - library for functional interactive c++ programs
// Copyright (C) 2017 Juan Pedro Bolivar Puente
//
// This file is part of lager.
//
// lager is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// lager is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with lager.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <cereal/archives/json.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/cereal.hpp>
#include <httpserver.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <sstream>
#include <vector>

namespace lager {

class http_debug_server
{
    struct handle_base
    {
        virtual ~handle_base() = default;
    };

public:
    template <typename Debugger>
    struct handle : handle_base
    {
        using base_action = typename Debugger::base_action;
        using base_model  = typename Debugger::base_model;
        using action      = typename Debugger::action;
        using model       = typename Debugger::model;

        using dispatcher_t = std::function<void(action)>;

        void dispatcher(dispatcher_t ds)
        {
            dispatcher_ = std::move(ds);
        }

        void view(const model& m)
        {
            std::atomic_store(&model_, std::make_shared<const model>(m));
        }

    private:
        friend http_debug_server;

        dispatcher_t dispatcher_ = {};
        std::shared_ptr<const model> model_ {nullptr};

        model get_model_() const
        {
            auto m = std::atomic_load(&model_);
            if (!m) throw std::runtime_error("not initialized yet");
            return *m;
        }

        struct resource_t : httpserver::http_resource
        {
            handle& self;
            resource_t(handle& hdl) : self{hdl} {}
        };

        using response_t = const httpserver::http_response;
        using request_t  = httpserver::http_request;

        struct : resource_t { using resource_t::resource_t;
            response_t render_GET(const request_t& req) override
            {
                auto m = this->self.get_model_();
                auto s = std::ostringstream{};
                {
                    auto a = cereal::JSONOutputArchive{s};
                    a(cereal::make_nvp("size", m.history.size()),
                      cereal::make_nvp("cursor", m.cursor));
                }
                return httpserver::http_response_builder(
                    s.str(), 200, "text/json");
            }
        } root_resource_ = {*this};

        struct : resource_t { using resource_t::resource_t;
            response_t render_GET(const request_t& req) override
            {
                auto m = this->self.get_model_();
                auto s = std::ostringstream{};
                auto cursor = std::stoul(req.get_arg("cursor"));
                if (cursor > m.history.size()) return {};
                {
                    auto a = cereal::JSONOutputArchive{s};
                    if (cursor == 0)
                        a(cereal::make_nvp("model", m.init));
                    else {
                        auto& step = m.history[cursor - 1];
                        a(cereal::make_nvp("action", step.action));
                        a(cereal::make_nvp("model",  step.model));
                    }
                }
                return httpserver::http_response_builder(
                    s.str(), 200, "text/json");
            }
        } step_resource_ = {*this};

        struct : resource_t { using resource_t::resource_t;
            response_t render_POST(const request_t& req) override
            {
                auto cursor = std::stoul(req.get_arg("cursor"));
                this->self.dispatcher_(typename Debugger::goto_action{cursor});
                return httpserver::http_response_builder("", 200);
            }
        } goto_resource_ = {*this};

        struct : resource_t { using resource_t::resource_t;
            response_t render_POST(const request_t& req) override
            {
                this->self.dispatcher_(typename Debugger::undo_action{});
                return httpserver::http_response_builder("", 200);
            }
        } undo_resource_ = {*this};

        struct : resource_t { using resource_t::resource_t;
            response_t render_POST(const request_t& req) override
            {
                this->self.dispatcher_(typename Debugger::redo_action{});
                return httpserver::http_response_builder("", 200);
            }
        } redo_resource_ = {*this};
    };

    template <typename Debugger>
    handle<Debugger>& enable(Debugger)
    {
        assert(!handle_);
        assert(!server_.is_running());
        auto hdl_ = std::make_unique<handle<Debugger>>();
        auto& hdl = *hdl_;
        server_.register_resource("/",     &hdl.root_resource_);
        server_.register_resource("/step", &hdl.step_resource_);
        server_.register_resource("/goto", &hdl.goto_resource_);
        server_.register_resource("/undo", &hdl.undo_resource_);
        server_.register_resource("/redo", &hdl.redo_resource_);
        handle_ = std::move(hdl_);
        server_.start();
        return hdl;
    }

private:
    httpserver::webserver server_ = httpserver::create_webserver(8080);
    std::unique_ptr<handle_base> handle_;
};

} // namespace lager
