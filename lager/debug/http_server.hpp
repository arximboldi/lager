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

#pragma once

#include <lager/context.hpp>
#include <lager/reader.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/optional.hpp>
#include <lager/debug/cereal/variant_with_name.hpp>

#include <httpserver.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <sstream>
#include <vector>

namespace lager {

namespace detail {

bool ends_with(const std::string& str, const std::string& ending)
{
    return str.length() >= ending.length() &&
           str.compare(
               str.length() - ending.length(), ending.length(), ending) == 0;
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
        using reader_t    = reader<model>;

        void set_context(context_t ctx) { context_ = std::move(ctx); }
        void set_reader(reader_t data) { data_ = std::move(data); }
        std::string const& resources_path() { return resources_path_; }

    private:
        friend http_debug_server;

        handle(int argc, const char** argv, std::string resources_path)
            : program_{join_args_(argc, argv)}
            , resources_path_(std::move(resources_path))
        {
            data_.watch([this](auto&& v) {
                std::atomic_store(&model_,
                                  std::make_shared<const model>(LAGER_FWD(v)));
            });
        }

        std::string program_        = {};
        std::string resources_path_ = {};
        context_t context_          = {};
        reader_t data_              = {};
        std::shared_ptr<const model> model_{nullptr};

        static std::string join_args_(int argc, const char** argv)
        {
            assert(argc > 0);
            auto is = std::ostringstream{};
            is << argv[0];
            for (int i = 1; i < argc; ++i)
                is << " " << argv[i];
            return is.str();
        }

        model get_model_() const
        {
            auto m = std::atomic_load(&model_);
            if (!m)
                throw std::runtime_error("not initialized yet");
            return *m;
        }

        struct resource_t : httpserver::http_resource
        {
            handle& self;
            resource_t(handle& hdl)
                : self{hdl}
            {}
        };

        using response_t = const std::shared_ptr<httpserver::http_response>;
        using request_t  = httpserver::http_request;

        struct : resource_t
        {
            using resource_t::resource_t;
            response_t render_GET(const request_t& req) override
            {
                auto m = this->self.get_model_();
                auto s = std::ostringstream{};
                {
                    auto a = cereal::JSONOutputArchive{s};
                    a(cereal::make_nvp("program", this->self.program_),
                      cereal::make_nvp("summary", m.summary()),
                      cereal::make_nvp("cursor", m.cursor),
                      cereal::make_nvp("paused", m.paused));
                }
                return std::make_shared<httpserver::string_response>(
                    s.str(), 200, "text/json");
            }
        } root_resource_ = {*this};

        struct : resource_t
        {
            using resource_t::resource_t;
            response_t render_GET(const request_t& req) override
            {
                auto m = this->self.get_model_();
                auto s = std::ostringstream{};
                {
                    auto cursor = std::stoul(req.get_arg("cursor"));
                    auto result = m.lookup(cursor);
                    auto a      = cereal::JSONOutputArchive{s};
                    if (result.first)
                        a(cereal::make_nvp("action", *result.first));
                    a(cereal::make_nvp("model", result.second));
                }
                return std::make_shared<httpserver::string_response>(
                    s.str(), 200, "text/json");
            }
        } step_resource_ = {*this};

        struct : resource_t
        {
            using resource_t::resource_t;
            response_t render_POST(const request_t& req) override
            {
                auto cursor = std::stoul(req.get_arg("cursor"));
                this->self.context_.dispatch(
                    typename Debugger::goto_action{cursor});
                return std::make_shared<httpserver::string_response>("", 200);
            }
        } goto_resource_ = {*this};

        struct : resource_t
        {
            using resource_t::resource_t;
            response_t render_POST(const request_t& req) override
            {
                this->self.context_.dispatch(typename Debugger::undo_action{});
                return std::make_shared<httpserver::string_response>("", 200);
            }
        } undo_resource_ = {*this};

        struct : resource_t
        {
            using resource_t::resource_t;
            response_t render_POST(const request_t& req) override
            {
                this->self.context_.dispatch(typename Debugger::redo_action{});
                return std::make_shared<httpserver::string_response>("", 200);
            }
        } redo_resource_ = {*this};

        struct : resource_t
        {
            using resource_t::resource_t;
            response_t render_POST(const request_t& req) override
            {
                this->self.context_.dispatch(typename Debugger::pause_action{});
                return std::make_shared<httpserver::string_response>("", 200);
            }
        } pause_resource_ = {*this};

        struct : resource_t
        {
            using resource_t::resource_t;
            response_t render_POST(const request_t& req) override
            {
                this->self.context_.dispatch(
                    typename Debugger::resume_action{});
                return std::make_shared<httpserver::string_response>("", 200);
            }
        } resume_resource_ = {*this};

        struct : resource_t
        {
            using resource_t::resource_t;
            response_t render_GET(const request_t& req) override
            {
                auto req_path = req.get_path();
                auto rel_path =
                    req_path == "/" ? "/gui/index.html" : "/gui/" + req_path;
                auto full_path = this->self.resources_path() + rel_path;
                auto content_type =
                    detail::ends_with(full_path, ".html")
                        ? "text/html"
                        : detail::ends_with(full_path, ".js")
                              ? "text/javascript"
                              : detail::ends_with(full_path, ".css")
                                    ? "text/css"
                                    /* otherwise */
                                    : "text/plain";
                return std::make_shared<httpserver::file_response>(
                    full_path, 200, content_type);
            }
        } gui_resource_ = {*this};
    };

    http_debug_server(int argc,
                      const char** argv,
                      std::uint16_t port,
                      std::string resources_path)
        : argc_{argc}
        , argv_{argv}
        , resources_path_{std::move(resources_path)}
        , server_{httpserver::create_webserver(port)}
    {}

    template <typename Debugger>
    handle<Debugger>& enable(Debugger)
    {
        assert(!handle_);
        assert(!server_.is_running());
        using handle_t = handle<Debugger>;
        auto hdl_      = std::unique_ptr<handle_t>(
            new handle_t{argc_, argv_, resources_path_});
        auto& hdl = *hdl_;
        server_.register_resource("/api/step/{cursor}", &hdl.step_resource_);
        server_.register_resource("/api/goto/{cursor}", &hdl.goto_resource_);
        server_.register_resource("/api/undo", &hdl.undo_resource_);
        server_.register_resource("/api/redo", &hdl.redo_resource_);
        server_.register_resource("/api/pause", &hdl.pause_resource_);
        server_.register_resource("/api/resume", &hdl.resume_resource_);
        server_.register_resource("/api/?", &hdl.root_resource_);
        server_.register_resource("/?.*", &hdl.gui_resource_);
        handle_ = std::move(hdl_);
        server_.start();
        return hdl;
    }

private:
    int argc_;
    const char** argv_;
    std::string resources_path_;
    httpserver::webserver server_        = httpserver::create_webserver(8080);
    std::unique_ptr<handle_base> handle_ = nullptr;
};

} // namespace lager
