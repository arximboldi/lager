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

#include <functional>
#include <vector>

namespace lager {

struct manual_event_loop
{
    template <typename Fn>
    void async(Fn&& fn)
    {
        throw std::logic_error{"manual_event_loop does not support async()"};
    }

    template <typename Fn>
    void post(Fn&& fn)
    {
        auto is_root = queue_.empty();
        queue_.push_back(std::forward<Fn>(fn));
        if (is_root) {
            for (auto i = std::size_t{}; i < queue_.size(); ++i)
                queue_[i]();
            queue_.clear();
        }
    }

    void finish() {}

private:
    using post_fn_t = std::function<void()>;

    std::vector<post_fn_t> queue_;
};

} // namespace lager
