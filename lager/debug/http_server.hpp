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
#include <lager/extra/cereal/variant_with_name.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <atomic>
#include <fstream>
#include <functional>
#include <memory>
#include <regex>
#include <sstream>
#include <thread>
#include <vector>

namespace lager {

namespace beast = boost::beast;
namespace asio  = boost::asio;

namespace detail::http {
class router
{
public:
    using req_t = beast::http::request<beast::http::string_body>;
    using res_t = beast::http::response<beast::http::string_body>;

private:
    struct resource_t
    {
        beast::http::verb method;
        std::regex target;
        std::function<res_t(req_t&&)> handler;
    };

    std::vector<resource_t> resources_;

public:
    void register_resource(beast::http::verb method,
                           const std::string& target,
                           std::function<res_t(req_t&&)> handler)
    {
        resources_.push_back({method, std::regex{target}, std::move(handler)});
    }

    res_t handle_request(req_t&& req)
    {
        auto it = std::find_if(resources_.begin(),
                               resources_.end(),
                               [&](const resource_t& resource) {
                                   return resource.method == req.method() &&
                                          std::regex_match(req.target().begin(),
                                                           req.target().end(),
                                                           resource.target);
                               });

        if (it != resources_.end()) {
            try {
                return it->handler(std::move(req));
            } catch (const std::exception& err) {
                return create_response_(500, "text/html", err.what());
            }
        }

        return create_response_(404, "text/html", "Not found");
    }

private:
    static res_t create_response_(unsigned status_code,
                                  beast::string_view content_type,
                                  std::string body)
    {
        res_t res;
        res.result(status_code);
        res.set(beast::http::field::content_type, content_type);
        res.body() = std::move(body);
        res.prepare_payload();
        return res;
    }
};

class session : public std::enable_shared_from_this<session>
{
    router* router_;
    asio::ip::tcp::socket socket_;
    beast::flat_buffer buffer_;
    beast::http::request<beast::http::string_body> req_;
    beast::http::response<beast::http::string_body> res_;

public:
    session(router* router, asio::ip::tcp::socket&& socket)
        : router_{router}
        , socket_(std::move(socket))
    {
    }

    void run() { do_read_(); }

private:
    void do_read_()
    {
        // Make the request empty before reading
        req_ = {};

        beast::http::async_read(socket_,
                                buffer_,
                                req_,
                                std::bind(&session::on_read_,
                                          shared_from_this(),
                                          std::placeholders::_1,
                                          std::placeholders::_2));
    }

    void on_read_(beast::error_code ec, std::size_t)
    {
        if (ec)
            return do_close_();

        // Keep response alive during the write operation
        res_ = router_->handle_request(std::move(req_));

        beast::http::async_write(socket_,
                                 res_,
                                 std::bind(&session::on_write_,
                                           shared_from_this(),
                                           std::placeholders::_1,
                                           std::placeholders::_2,
                                           res_.need_eof()));
    }

    void on_write_(beast::error_code ec, std::size_t, bool close)
    {
        if (ec || close)
            return do_close_();

        do_read_();
    }

    void do_close_()
    {
        beast::error_code ec;
        socket_.shutdown(asio::ip::tcp::socket::shutdown_send, ec);
    }
};

class listener : public std::enable_shared_from_this<listener>
{
    router* router_;
    asio::ip::tcp::acceptor acceptor_;

public:
    listener(router* router,
             asio::io_context* ioc,
             const asio::ip::tcp::endpoint& endpoint)
        : router_{router}
        , acceptor_{*ioc}
    {
        beast::error_code ec;

        acceptor_.open(endpoint.protocol(), ec);
        if (ec)
            throw std::runtime_error{"acceptor failed on open, err:" +
                                     std::string{ec.message()}};

        acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
        if (ec)
            throw std::runtime_error{"acceptor failed on set_option, err:" +
                                     std::string{ec.message()}};

        acceptor_.bind(endpoint, ec);
        if (ec)
            throw std::runtime_error{"acceptor failed on bind, err:" +
                                     std::string{ec.message()}};

        acceptor_.listen(asio::socket_base::max_listen_connections, ec);
        if (ec)
            throw std::runtime_error{"acceptor failed on listen, err:" +
                                     std::string{ec.message()}};
    }

    void run() { do_accept_(); }

private:
    void do_accept_()
    {
        acceptor_.async_accept(std::bind(&listener::on_accept_,
                                         shared_from_this(),
                                         std::placeholders::_1,
                                         std::placeholders::_2));
    }

    void on_accept_(boost::system::error_code ec, asio::ip::tcp::socket socket)
    {
        if (ec)
            return;

        std::make_shared<session>(router_, std::move(socket))->run();

        do_accept_();
    }
};

class server
{
    router router_;
    asio::io_context io_context_;
    std::shared_ptr<listener> listener_;
    std::thread thread_;

public:
    server(uint16_t port, router router)
        : router_{std::move(router)}
        , listener_{std::make_shared<listener>(
              &router_,
              &io_context_,
              asio::ip::tcp::endpoint{asio::ip::tcp::v4(), port})}
    {
        listener_->run();
        thread_ = std::thread{[&] { io_context_.run(); }};
    }

    ~server()
    {
        io_context_.stop();

        if (thread_.joinable())
            thread_.join();
    }
};
} // namespace detail::http

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
        std::string const& resources_path() const { return resources_path_; }

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

        using req_t = detail::http::router::req_t;
        using res_t = detail::http::router::res_t;

        res_t root_resource_(req_t&&) const
        {
            auto m = get_model_();
            auto s = std::ostringstream{};
            {
                auto a = cereal::JSONOutputArchive{s};
                a(cereal::make_nvp("program", program_),
                  cereal::make_nvp("summary", m.summary()),
                  cereal::make_nvp("cursor", m.cursor),
                  cereal::make_nvp("paused", m.paused));
            }
            return create_response_(200, "application/json", s.str());
        }

        res_t step_resource_(req_t&& req) const
        {
            auto m = get_model_();
            auto s = std::ostringstream{};
            {
                auto target = req.target().to_string();
                auto cursor = std::stoul(target.substr(target.rfind('/') + 1));
                auto result = m.lookup(cursor);
                auto a      = cereal::JSONOutputArchive{s};
                if (result.first)
                    a(cereal::make_nvp("action", *result.first));
                a(cereal::make_nvp("model", result.second));
            }
            return create_response_(200, "application/json", s.str());
        }

        res_t goto_resource_(req_t&& req)
        {
            auto target = req.target().to_string();
            auto cursor = std::stoul(target.substr(target.rfind('/') + 1));
            context_.dispatch(typename Debugger::goto_action{cursor});
            return create_response_(200, "text/html", "");
        }

        res_t undo_resource_(req_t&&)
        {
            context_.dispatch(typename Debugger::undo_action{});
            return create_response_(200, "text/html", "");
        }

        res_t redo_resource_(req_t&&)
        {
            context_.dispatch(typename Debugger::redo_action{});
            return create_response_(200, "text/html", "");
        }

        res_t pause_resource_(req_t&&)
        {
            context_.dispatch(typename Debugger::pause_action{});
            return create_response_(200, "text/html", "");
        }

        res_t resume_resource_(req_t&&)
        {
            context_.dispatch(typename Debugger::resume_action{});
            return create_response_(200, "text/html", "");
        }

        res_t gui_resource_(req_t&& req) const
        {
            auto req_path = req.target().to_string();
            auto rel_path =
                req_path == "/" ? "/gui/index.html" : "/gui/" + req_path;
            auto full_path = resources_path() + rel_path;

            std::ifstream ifs{full_path};
            if (!ifs.is_open())
                return create_response_(404, "text/html", "Not found");

            /*
              The whole file content is read synchronously into string, this
              shouldn't be a performance issue because it's used only in gui
              startup and has no blocking effect on the application because
              http::server has its own thread.
            */
            using it_t = std::istreambuf_iterator<char>;
            return create_response_(
                200, mime_type_(full_path), std::string{it_t(ifs), it_t()});
        }

        static beast::string_view mime_type_(beast::string_view path)
        {
            using beast::iequals;
            auto const ext = [&path] {
                auto const pos = path.rfind(".");
                if (pos == beast::string_view::npos)
                    return beast::string_view{};
                return path.substr(pos);
            }();
            if (iequals(ext, ".htm"))
                return "text/html";
            if (iequals(ext, ".html"))
                return "text/html";
            if (iequals(ext, ".php"))
                return "text/html";
            if (iequals(ext, ".css"))
                return "text/css";
            if (iequals(ext, ".txt"))
                return "text/plain";
            if (iequals(ext, ".js"))
                return "application/javascript";
            if (iequals(ext, ".json"))
                return "application/json";
            if (iequals(ext, ".xml"))
                return "application/xml";
            if (iequals(ext, ".swf"))
                return "application/x-shockwave-flash";
            if (iequals(ext, ".flv"))
                return "video/x-flv";
            if (iequals(ext, ".png"))
                return "image/png";
            if (iequals(ext, ".jpe"))
                return "image/jpeg";
            if (iequals(ext, ".jpeg"))
                return "image/jpeg";
            if (iequals(ext, ".jpg"))
                return "image/jpeg";
            if (iequals(ext, ".gif"))
                return "image/gif";
            if (iequals(ext, ".bmp"))
                return "image/bmp";
            if (iequals(ext, ".ico"))
                return "image/vnd.microsoft.icon";
            if (iequals(ext, ".tiff"))
                return "image/tiff";
            if (iequals(ext, ".tif"))
                return "image/tiff";
            if (iequals(ext, ".svg"))
                return "image/svg+xml";
            if (iequals(ext, ".svgz"))
                return "image/svg+xml";
            return "application/text";
        }

        static res_t create_response_(unsigned status_code,
                                      beast::string_view content_type,
                                      std::string body)
        {
            res_t res;
            res.result(status_code);
            res.set(beast::http::field::content_type, content_type);
            res.body() = std::move(body);
            res.prepare_payload();
            return res;
        }
    };

    http_debug_server(int argc,
                      const char** argv,
                      std::uint16_t port,
                      std::string resources_path)
        : argc_{argc}
        , argv_{argv}
        , port_{port}
        , resources_path_{std::move(resources_path)}
    {
    }

    template <typename Debugger>
    handle<Debugger>& enable(Debugger)
    {
        assert(!handle_);
        using handle_t = handle<Debugger>;
        auto hdl_      = std::unique_ptr<handle_t>(
            new handle_t{argc_, argv_, resources_path_});
        auto& hdl = *hdl_;

        detail::http::router router;

        namespace ph = std::placeholders;

        router.register_resource(
            beast::http::verb::get,
            "/api/?",
            std::bind(&handle_t::root_resource_, &hdl, ph::_1));

        router.register_resource(
            beast::http::verb::get,
            "/api/step/[0-9]+",
            std::bind(&handle_t::step_resource_, &hdl, ph::_1));

        router.register_resource(
            beast::http::verb::post,
            "/api/goto/[0-9]+",
            std::bind(&handle_t::goto_resource_, &hdl, ph::_1));

        router.register_resource(
            beast::http::verb::post,
            "/api/undo",
            std::bind(&handle_t::undo_resource_, &hdl, ph::_1));

        router.register_resource(
            beast::http::verb::post,
            "/api/redo",
            std::bind(&handle_t::redo_resource_, &hdl, ph::_1));

        router.register_resource(
            beast::http::verb::post,
            "/api/pause",
            std::bind(&handle_t::pause_resource_, &hdl, ph::_1));

        router.register_resource(
            beast::http::verb::post,
            "/api/resume",
            std::bind(&handle_t::resume_resource_, &hdl, ph::_1));

        router.register_resource(
            beast::http::verb::get,
            "/?.*",
            std::bind(&handle_t::gui_resource_, &hdl, ph::_1));

        server_ =
            std::make_unique<detail::http::server>(port_, std::move(router));

        handle_ = std::move(hdl_);
        return hdl;
    }

private:
    int argc_;
    const char** argv_;
    std::uint16_t port_;
    std::string resources_path_;
    std::unique_ptr<detail::http::server> server_ = nullptr;
    std::unique_ptr<handle_base> handle_          = nullptr;
};

} // namespace lager
