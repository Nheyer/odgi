#pragma once

#include <unordered_map>
#include <stack>
#include <map>

#include <handlegraph/mutable_path_deletable_handle_graph.hpp>
#include <handlegraph/util.hpp>
#include "weakly_connected_components.hpp"

namespace odgi {
    namespace algorithms {

        using namespace handlegraph;

        /* This implementation has been took, and slightly modified, by: https://github.com/jltsiren/gbwtgraph */

        constexpr size_t PATH_COVER_DEFAULT_N = 16;
        constexpr size_t PATH_COVER_DEFAULT_K = 3;

        /*
          Determine whether the given component is acyclic in a nice way. In such graphs,
          when we start from nodes with indegree 0 in forward orientation, we reach each node
          in a single orientation and find no cycles. Return the head nodes when the component
          passes the tests or an empty vector otherwise.
          Ignores node ids that are not present in the graph.
        */
        ska::flat_hash_set<handlegraph::nid_t>
        is_nice_and_acyclic(const HandleGraph &graph, const ska::flat_hash_set<handlegraph::nid_t> &component);

        /*
          Find a path cover of the graph with n paths per component, adding the generated paths in the graph.
          The path cover is built greedily. Each time we extend a path, we choose the extension,
          where the coverage of the k >= 2 node window is the lowest. Note that this is a maximum
          coverage algorithm that tries to maximize the number of windows covered by a fixed number
          of paths.

          This algorithm has been inspired by:
            Ghaffaari and Marschall: Fully-sensitive seed finding in sequence graphs using a
            hybrid index. Bioinformatics, 2019.

          Because the graph here may have cycles and orientation flips, some changes were necessary:
            - If the component is not a DAG, we start from an arbitrary node with minimal
              coverage and extend in both directions.
            - In a DAG, we start from the head node with the lowest coverage so far.
            - We stop when the length of the path reaches the size of the component.
            - The length of the window is in nodes instead of base pairs. We expect a sparse graph,
              where the nodes between variants are long.
            - If the component is not a DAG and the path is shorter than k - 1 nodes, we consider
              the coverage of individual nodes instead of windows.
            - When determining window coverage, we consider the window equivalent to its reverse
              complement.
        */
        void path_cover(handlegraph::MutablePathDeletableHandleGraph &graph,
                        size_t n, size_t k,
                        bool show_progress = false);
    }
}