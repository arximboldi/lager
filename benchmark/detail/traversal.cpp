#include <nonius.h++>

#include "lager/detail/merge_nodes.hpp"
#include "lager/detail/nodes.hpp"
#include "lager/state.hpp"

#include "lager/detail/traversal.hpp"
#include "lager/detail/traversal_dfs.hpp"
#include "lager/detail/traversal_topo.hpp"
#include "lager/detail/traversal_topo_intrusive.hpp"
#include "lager/detail/traversal_topo_naive_mmap.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <vector>
#include <zug/transducer/map.hpp>
#include <zug/util.hpp>

#include <boost/unordered_map.hpp>
#include <chrono>
#include <unordered_map>

using namespace lager;
using namespace lager::detail;

/**
 * Ensures the == test within a node always fails and stores an int so that we
 * can perform a sanity check on value propagation.
 **/
struct unique_value
{
    long unsigned val = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> tag =
        std::chrono::high_resolution_clock::now();
};
bool operator==(const unique_value& x, const unique_value& y)
{
    return x.tag == y.tag;
}

using node_t      = reader_node<unique_value>;
using merge_t     = reader_node<std::tuple<unique_value, unique_value>>;
using node_ptr_t  = std::shared_ptr<node_t>;
using merge_ptr_t = std::shared_ptr<merge_t>;

const unique_value next(const unique_value& x) { return {.val = x.val + 1}; }
const unique_value combine(const std::tuple<unique_value, unique_value>& tuple)
{
    return {.val = std::get<0>(tuple).val + 1};
}

/**
 * A chain is a series of node such that, if the value of the chain root
 * is k, and if the number of "link" in the chain is n, then the value of
 * the chain ought to be k + n.
 **/
struct chain
{
    using state_t = decltype(make_state_node(unique_value{}));

    unsigned long value() const { return last->last().val; }

    state_t root    = make_state_node(unique_value{});
    node_ptr_t last = root;
};

/**
 * Node network of the simplest form:
 *
 * A - B - C - D - E - ....
 *
 * This network is well suited to a DFS. Each one of A, B, C...
 * in the diagram above is a "link" in the chain.
 **/
chain make_simple_chain(long unsigned n)
{
    auto c = chain();

    for (long unsigned i = 0; i < n; ++i) {
        auto p = std::make_tuple(c.last);
        c.last = make_xform_reader_node(zug::map(next), p);
    }

    return c;
}

/**
 * A node network having the following form:
 *
 *     B
 *    * *
 *   A   D * D'
 *    * *
 *     C
 *
 * Where:
 *
 * - A is some reader_node<int>
 * - B,C identity node forwarding A's value
 * - D is a merge node taking in B and C
 * - D' is a transform that simply increment A
 *
 * This network should be better suited to a topological traversal.
 *
 * If the value of A = 1, then the value of D' will be 2. Thus,
 * A,B,C,D,D' taken together form a "link" in the chain.
 **/
chain make_diamond_chain(long unsigned n)
{
    auto c = chain();

    for (long unsigned i = 0; i < n; ++i) {
        auto p      = std::make_tuple(c.last);
        auto xform1 = make_xform_reader_node(identity, p);
        auto xform2 = make_xform_reader_node(identity, p);
        auto merge  = make_merge_reader_node(std::make_tuple(xform1, xform2));
        c.last =
            make_xform_reader_node(zug::map(combine), std::make_tuple(merge));
    }

    return c;
}

NONIUS_PARAM(N, std::size_t{12})

template <typename Traversal, typename ChainFn>
auto traversal_fn(ChainFn&& chain_fn)
{
    return [chain_fn](nonius::chronometer meter) {
        auto n = meter.param<N>();
        auto c = chain_fn(n);
        assert(c.value() == n);
        c.root->send_up(unique_value{1});
        Traversal t{c.root};
        meter.measure([&t, &c] { t.visit(); });
        // sanity to check that values
        // propagate correctly
        assert(c.value() == n + 1);
    };
}

using std_traversal       = topo_traversal<>;
using bmultimap_traversal = topo_traversal<boost::unordered_multimap>;

NONIUS_BENCHMARK("DFS-SC", traversal_fn<dfs_traversal>(make_simple_chain))

NONIUS_BENCHMARK("DFS-DC", traversal_fn<dfs_traversal>(make_diamond_chain))

NONIUS_BENCHMARK("T-CMM-SC",
                 traversal_fn<naive_mmap_topo_traversal>(make_simple_chain))
NONIUS_BENCHMARK("T-CMM-DC",
                 traversal_fn<naive_mmap_topo_traversal>(make_diamond_chain))

NONIUS_BENCHMARK("T-SUMM-SC", traversal_fn<std_traversal>(make_simple_chain))

NONIUS_BENCHMARK("T-SUMM-DC", traversal_fn<std_traversal>(make_diamond_chain))

NONIUS_BENCHMARK("T-BUMM-SC",
                 traversal_fn<bmultimap_traversal>(make_simple_chain))

NONIUS_BENCHMARK("T-BUMM-DC",
                 traversal_fn<bmultimap_traversal>(make_diamond_chain))

NONIUS_BENCHMARK("T-BIMS-SC",
                 traversal_fn<bmultimap_traversal>(make_simple_chain))

NONIUS_BENCHMARK("T-BIMS-DC",
                 traversal_fn<bmultimap_traversal>(make_diamond_chain))
