#include "model.hpp"

#include <lager/util.hpp>

#include <boost/config.hpp>
#include <boost/variant.hpp>

#include <algorithm>

namespace sn {

namespace {

template <typename T>
T wrap(T val, T min, T max)
{
    if (val < min)
        return max - (min - val) + 1;
    if (val > max)
        return min + (val - max) - 1;
    return val;
}

direction left(direction initial)
{
    if (initial == direction::up || initial == direction::down)
        return direction::left;
    return initial;
}

direction right(direction initial)
{
    if (initial == direction::up || initial == direction::down)
        return direction::right;
    return initial;
}

direction up(direction initial)
{
    if (initial == direction::left || initial == direction::right)
        return direction::up;
    return initial;
}

direction down(direction initial)
{
    if (initial == direction::left || initial == direction::right)
        return direction::down;
    return initial;
}

point_t move_forward(point_t pos, direction dir)
{
    switch (dir) {
    case direction::left:
        return {x(pos) - 1, y(pos)};
    case direction::up:
        return {x(pos), y(pos) - 1};
    case direction::right:
        return {x(pos) + 1, y(pos)};
    case direction::down:
        return {x(pos), y(pos) + 1};
    }

    BOOST_UNREACHABLE_RETURN(pos);
}

snake_model::body_t move_forward(snake_model::body_t body, direction dir)
{
    const auto head = move_forward(body.front(), dir);
    std::rotate(body.begin(), body.end() - 1, body.end());
    body[0] = head;
    return body;
}

bool in_bounds(point_t p)
{
    return x(p) >= 0 && x(p) < game_model::width && y(p) >= 0 &&
           y(p) < game_model::height;
}

template <typename Func>
game_model make_game(Func&& random)
{
    const point_t snake_head{game_model::width / 2, game_model::height / 2};
    snake_model::body_t body{snake_head,
                             {x(snake_head) - 1, y(snake_head)},
                             {x(snake_head) - 2, y(snake_head)}};

    return {snake_model{body, direction::right},
            random_apple_pos(std::forward<Func>(random)),
            false};
}

} // namespace

app_model make_initial(int seed)
{
    std::mt19937 gen{unsigned(seed)};
    std::uniform_int_distribution<> dist{0, game_model::width - 1};

    return {gen, dist, make_game([&] { return dist(gen); })};
}

app_model update(app_model m, action_t action)
{
    return boost::apply_visitor(
        lager::visitor{
            [&](go_left) {
                m.game.snake.dir = left(m.game.snake.dir);
                return m;
            },
            [&](go_right) {
                m.game.snake.dir = right(m.game.snake.dir);
                return m;
            },
            [&](go_up) {
                m.game.snake.dir = up(m.game.snake.dir);
                return m;
            },
            [&](go_down) {
                m.game.snake.dir = down(m.game.snake.dir);
                return m;
            },
            [&](tick) {
                const auto prev_back = m.game.snake.body.back();
                auto body = move_forward(m.game.snake.body, m.game.snake.dir);
                const auto head = body.front();

                if (std::any_of(body.begin() + 1,
                                body.end(),
                                [&](const auto& p) { return p == head; }) ||
                    !in_bounds(head)) {
                    m.game.over = true;
                    return m;
                }

                if (prev_back == m.game.apple_pos) {
                    body.push_back(prev_back);
                    m.game.apple_pos =
                        random_apple_pos([&] { return m.dist(m.gen); });
                }
                m.game.snake.body = body;

                return m;
            },
            [&](reset) {
                m.game = make_game([&] { return m.dist(m.gen); });
                return m;
            }},
        action);
}

} // namespace sn