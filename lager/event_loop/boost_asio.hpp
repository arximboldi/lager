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

#include <boost/asio/io_service.hpp>
#include <thread>
#include <functional>

namespace lager {

struct boost_asio_event_loop
{
    std::reference_wrapper<boost::asio::io_service> service;

    template <typename Fn>
    void async(Fn&& fn) const
    {
        std::thread([fn=std::move(fn),
                     work=boost::asio::io_service::work(service)] {
            fn();
        }).detach();
    }

    template <typename Fn>
    void post(Fn&& fn) const
    {
        service.get().post(std::forward<Fn>(fn));
    }
};

} // namespace lager
