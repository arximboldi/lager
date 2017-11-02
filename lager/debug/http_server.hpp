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

#include <lager/context.hpp>
#include <lager/debug/cereal/variant_with_name.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/cereal.hpp>
#include <httpserver.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <sstream>
#include <vector>

#ifndef LAGER_RESOURCES_PREFIX
#define LAGER_RESOURCES_PREFIX "/usr/share/lager"
#endif

namespace lager {

namespace detail {

bool ends_with(const std::string& str, const std::string& ending)
{
    return str.length() >= ending.length()
        && str.compare(str.length() - ending.length(),
                       ending.length(),
                       ending) == 0;

}

} // namespace detail

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
        using context_t   = context<action>;

        void set_context(context_t ctx)
        {
            context_ = std::move(ctx);
        }

        void view(const model& m)
        {
            std::atomic_store(&model_, std::make_shared<const model>(m));
        }

    private:
        friend http_debug_server;

        handle(int argc, const char** argv)
            : program_{join_args_(argc, argv)}
        {}

        std::string program_ = {};
        context_t context_ = {};
        std::shared_ptr<const model> model_ {nullptr};

        static std::string join_args_(int argc, const char** argv)
        {
            assert(argc > 0);
            auto is = std::ostringstream{argv[0]};
            for (int i = 1; i < argc; ++i)
                is << argv[i];
            return is.str();
        }

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
                    a(cereal::make_nvp("program", this->self.program_),
                      cereal::make_nvp("size",    m.history.size()),
                      cereal::make_nvp("cursor",  m.cursor));
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
                this->self.context_.dispatch(
                    typename Debugger::goto_action{cursor});
                return httpserver::http_response_builder("", 200);
            }
        } goto_resource_ = {*this};

        struct : resource_t { using resource_t::resource_t;
            response_t render_POST(const request_t& req) override
            {
                this->self.context_.dispatch(typename Debugger::undo_action{});
                return httpserver::http_response_builder("", 200);
            }
        } undo_resource_ = {*this};

        struct : resource_t { using resource_t::resource_t;
            response_t render_POST(const request_t& req) override
            {
                this->self.context_.dispatch(typename Debugger::redo_action{});
                return httpserver::http_response_builder("", 200);
            }
        } redo_resource_ = {*this};

        struct : resource_t { using resource_t::resource_t;
            response_t render_GET(const request_t& req) override
            {
                auto env_resources_path = std::getenv("LAGER_RESOURCES_PATH");
                auto resources_path = env_resources_path
                    ? env_resources_path
                    : LAGER_RESOURCES_PREFIX;
                auto req_path = req.get_path();
                auto rel_path = req_path == "/"
                    ? "/gui/index.html"
                    : "/gui/" + req_path;
                auto full_path = resources_path + rel_path;
                auto content_type =
                    detail::ends_with(full_path, ".html") ? "text/html" :
                    detail::ends_with(full_path, ".js")   ? "text/javascript" :
                    detail::ends_with(full_path, ".css")  ? "text/css"
                    /* otherwise */                       : "text/plain";
                return httpserver::http_response_builder(full_path, 200, content_type)
                    .file_response();
            }
        } gui_resource_ = {*this};
    };

    http_debug_server(int argc, const char** argv, std::uint16_t port)
        : argc_{argc}
        , argv_{argv}
        , server_{httpserver::create_webserver(port)}
    {}

    template <typename Debugger>
    handle<Debugger>& enable(Debugger)
    {
        assert(!handle_);
        assert(!server_.is_running());
        using handle_t = handle<Debugger>;
        auto hdl_ = std::unique_ptr<handle_t>(new handle_t{argc_, argv_});
        auto& hdl = *hdl_;
        server_.register_resource("/api/step/{cursor}", &hdl.step_resource_);
        server_.register_resource("/api/goto/{cursor}", &hdl.goto_resource_);
        server_.register_resource("/api/undo",          &hdl.undo_resource_);
        server_.register_resource("/api/redo",          &hdl.redo_resource_);
        server_.register_resource("/api/?",             &hdl.root_resource_);
        server_.register_resource("/?.*",               &hdl.gui_resource_);
        handle_ = std::move(hdl_);
        server_.start();
        return hdl;
    }

private:
    int argc_;
    const char** argv_;
    httpserver::webserver server_ = httpserver::create_webserver(8080);
    std::unique_ptr<handle_base> handle_ = nullptr;
};

} // namespace lager
