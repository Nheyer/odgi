#include "topological_sort.hpp"

namespace odgi {
namespace algorithms {

using namespace handlegraph;

std::vector<handle_t> head_nodes(const HandleGraph* g) {
    std::vector<handle_t> to_return;
    g->for_each_handle([&](const handle_t& found) {
        // For each (locally forward) node
        
        bool no_left_edges = true;
        g->follow_edges(found, true, [&](const handle_t& ignored) {
            // We found a left edge!
            no_left_edges = false;
            // We only need one
            return false;
        });
        
        if (no_left_edges) {
            to_return.push_back(found);
        }
    });
    
    return to_return;
    
}

std::vector<handle_t> tail_nodes(const HandleGraph* g) {
    std::vector<handle_t> to_return;
    g->for_each_handle([&](const handle_t& found) {
        // For each (locally forward) node
        
        bool no_right_edges = true;
        g->follow_edges(found, false, [&](const handle_t& ignored) {
            // We found a right edge!
            no_right_edges = false;
            // We only need one
            return false;
        });
        
        if (no_right_edges) {
            to_return.push_back(found);
        }
    });
    
    return to_return;
    
}

std::vector<handle_t> topological_order(const HandleGraph* g, bool use_heads) {
    
    // Make a vector to hold the ordered and oriented nodes.
    std::vector<handle_t> sorted;
    sorted.reserve(g->node_size());
    
    // Instead of actually removing edges, we add them to this set of masked edges.
    hash_set<pair<handle_t, handle_t> > masked_edges;
    
    // This (s) is our set of oriented nodes.
    // using a map instead of a set ensures a stable sort across different systems
    map<handlegraph::nid_t, handle_t> s;

    // We find the head and tails, if there are any
    //std::vector<handle_t> heads{head_nodes(g)};
    // No need to fetch the tails since we don't use them

    
    // Maps from node ID to first orientation we suggested for it.
    map<handlegraph::nid_t, handle_t> seeds;
    
    // Dump all the heads into the oriented set, rather than having them as
    // seeds. We will only go for cycle-breaking seeds when we run out of
    // heads. This is bad for contiguity/ordering consistency in cyclic
    // graphs and reversing graphs, but makes sure we work out to just
    // topological sort on DAGs. It mimics the effect we used to get when we
    // joined all the head nodes to a new root head node and seeded that. We
    // ignore tails since we only orient right from nodes we pick.
    if (use_heads) {
        for(const handle_t& head : head_nodes(g)) {
            s[g->get_id(head)] = head;
        }
    } else {
        for(const handle_t& tail : tail_nodes(g)) {
            s[g->get_id(tail)] = tail;
        }
    }

    // We will use an ordered map handles by ID for nodes we have not visited
    // yet. This ensures a consistent sort order across systems.
    map<handlegraph::nid_t, handle_t> unvisited;
    g->for_each_handle([&](const handle_t& found) {
        if (!s.count(g->get_id(found))) {
            // Only nodes that aren't yet in s are unvisited.
            // Nodes in s are visited but just need to be added tot he ordering.
            unvisited.emplace(g->get_id(found), found);
        }
    });

    while(!unvisited.empty() || !s.empty()) {

        // Put something in s. First go through seeds until we can find one
        // that's not already oriented.
        while(s.empty() && !seeds.empty()) {
            // Look at the first seed
            auto first_seed = (*seeds.begin()).second;

            if(unvisited.count(g->get_id(first_seed))) {
                // We have an unvisited seed. Use it
#ifdef debug
#pragma omp critical (cerr)
                cerr << "Starting from seed " << g->get_id(first_seed) << " orientation " << g->get_is_reverse(first_seed) << endl;
#endif

                s[g->get_id(first_seed)] = first_seed;
                unvisited.erase(g->get_id(first_seed));
            }
            // Whether we used the seed or not, don't keep it around
            seeds.erase(seeds.begin());
        }

        if(s.empty()) {
            // If we couldn't find a seed, just grab any old node.
            // Since map order is stable across systems, we can take the first node by id and put it locally forward.
#ifdef debug
#pragma omp critical (cerr)
            cerr << "Starting from arbitrary node " << unvisited.begin()->first << " locally forward" << endl;
#endif

            s[unvisited.begin()->first] = unvisited.begin()->second;
            unvisited.erase(unvisited.begin()->first);
        }

        while (!s.empty()) {
            // Grab an oriented node
            auto n = s.begin()->second;
            s.erase(g->get_id(n));
            // Emit it
            sorted.push_back(n);
#ifdef debug
#pragma omp critical (cerr)
            cerr << "Using oriented node " << g->get_id(n) << " orientation " << g->get_is_reverse(n) << endl;
#endif

            // See if it has an edge from its start to the start of some node
            // where both were picked as places to break into cycles. A
            // reversing self loop on a cycle entry point is a special case of
            // this.
            g->follow_edges(n, true, [&](const handle_t& prev_node) {
                if(!unvisited.count(g->get_id(prev_node))) {
                    // Look at the edge
                    auto edge = g->edge_handle(prev_node, n);
                    if (masked_edges.count(edge)) {
                        // We removed this edge, so skip it.
                        return;
                    }
                    
#ifdef debug
#pragma omp critical (cerr)
                    cerr << "\tHas left-side edge to cycle entry point " << g->get_id(prev_node)
                         << " orientation " << g->get_is_reverse(prev_node) << endl;
#endif

                    // Mask the edge
                    masked_edges.insert(edge);
                    
#ifdef debug
#pragma omp critical (cerr)
                    cerr << "\t\tEdge: " << g->get_id(edge.first) << " " << g->get_is_reverse(edge.first)
                        << " -> " << g->get_id(edge.second) << " " << g->get_is_reverse(edge.second) << endl;
#endif
                }
            });

            // All other connections and self loops are handled by looking off the right side.

            // See what all comes next, minus deleted edges.
            g->follow_edges(n, false, [&](const handle_t& next_node) {

                // Look at the edge
                auto edge = g->edge_handle(n, next_node);
                if (masked_edges.count(edge)) {
                    // We removed this edge, so skip it.
                    return;
                }

#ifdef debug
#pragma omp critical (cerr)
                cerr << handle_helper::unpack_number(next_node) << endl;
                cerr << "\tHas edge to " << g->get_id(next_node) << " orientation " << g->get_is_reverse(next_node) << endl;
#endif

                // Mask the edge connecting these nodes in this order and
                // relative orientation, so we can't traverse it again

#ifdef debug
#pragma omp critical (cerr)
                cerr << "\t\tEdge: " << g->get_id(edge.first) << " " << g->get_is_reverse(edge.first)
                    << " -> " << g->get_id(edge.second) << " " << g->get_is_reverse(edge.second) << endl;
#endif

                // Mask the edge
                masked_edges.insert(edge);

                if(unvisited.count(g->get_id(next_node))) {
                    // We haven't already started here as an arbitrary cycle entry point

#ifdef debug
#pragma omp critical (cerr)
                    cerr << "\t\tAnd node hasn't been visited yet" << endl;
#endif

                    bool unmasked_incoming_edge = false;
                    g->follow_edges(next_node, true, [&](const handle_t& prev_node) {
                        // Get a handle for each incoming edge
                        auto prev_edge = g->edge_handle(prev_node, next_node);
                        
                        if (!masked_edges.count(prev_edge)) {
                            // We found such an edghe and can stop looking
                            unmasked_incoming_edge = true;
                            return false;
                        }
                        // Otherwise check all the edges on the left of this handle
                        return true;
                    });

                    if(!unmasked_incoming_edge) {

#ifdef debug
#pragma omp critical (cerr)
                        cerr << "\t\t\tIs last incoming edge" << endl;
#endif
                        // Keep this orientation and put it here
                        s[g->get_id(next_node)] = next_node;
                        // Remember that we've visited and oriented this node, so we
                        // don't need to use it as a seed.
                        unvisited.erase(g->get_id(next_node));

                    } else if(!seeds.count(g->get_id(next_node))) {
                        // We came to this node in this orientation; when we need a
                        // new node and orientation to start from (i.e. an entry
                        // point to the node's cycle), we might as well pick this
                        // one.
                        // Only take it if we don't already know of an orientation for this node.
                        seeds[g->get_id(next_node)] = next_node;

#ifdef debug
#pragma omp critical (cerr)
                        cerr << "\t\t\tSuggests seed " << g->get_id(next_node) << " orientation " << g->get_is_reverse(next_node) << endl;
#endif
                    }
                } else {
#ifdef debug
#pragma omp critical (cerr)
                    cerr << "\t\tAnd node was already visited (to break a cycle)" << endl;
#endif
                }
            });
        }
    }

    // Send away our sorted ordering.
    return sorted;
}

std::vector<handle_t> two_way_topological_order(const HandleGraph* g) {
    // take the average assigned order for each handle
    hash_map<handle_t, uint64_t> avg_order;
    uint64_t i = 0;
    for (auto& handle : topological_order(g, true)) {
        avg_order[handle] = ++i;
    }
    i = 0;
    for (auto& handle : topological_order(g, false)) {
        avg_order[handle] = max(avg_order[handle], (++i));
    }
    std::vector<std::pair<handle_t, uint64_t>> order;
    for (auto& p : avg_order) {
        order.push_back(p);
    }
    std::sort(order.begin(), order.end(), [](const std::pair<handle_t, uint64_t>& a,
                                             const std::pair<handle_t, uint64_t>& b) {
                  return a.second < b.second;
              });
    std::vector<handle_t> result;
    for (auto& p : order) {
        result.push_back(p.first);
    }
    return result;
}

std::vector<handle_t> lazy_topological_order_internal(const HandleGraph* g, bool lazier) {
    
    // map that will contain the orientation and the in degree for each node
    hash_map<handle_t, int64_t> inward_degree;
    inward_degree.reserve(g->node_size());
    
    // stack for the traversal
    std::vector<handle_t> stack;
    
    if (lazier) {
        // take the locally forward orientation as a single stranded orientation
        g->for_each_handle([&](const handle_t& handle) {
            int64_t& degree = inward_degree[handle];
            g->follow_edges(handle, true, [&](const handle_t& ignored) {
                degree++;
            });
            // initialize the stack with head nodes
            if (degree == 0) {
                stack.emplace_back(handle);
            }
        });
    }
    else {
        // get an orientation over which we can consider the graph single stranded
        std::vector<handle_t> orientation = single_stranded_orientation(g);
        
        if (orientation.size() != g->node_size()) {
            cerr << "error:[algorithms] attempting to use lazy topological sort on unorientable graph" << endl;
            assert(false);
        }
        
        // compute the degrees by following the edges backward
        for (auto& handle : orientation) {
            int64_t& degree = inward_degree[handle];
            g->follow_edges(handle, true, [&](const handle_t& ignored) {
                degree++;
            });
            // initialize the stack with head nodes
            if (degree == 0) {
                stack.emplace_back(handle);
            }
        }
    }
    
    // the return value
    std::vector<handle_t> order;
    order.reserve(g->node_size());
    
    while (!stack.empty()) {
        // get a head node off the queue
        handle_t here = stack.back();
        stack.pop_back();
        
        // add it to the topological order
        order.push_back(here);
        
        // remove its outgoing edges
        g->follow_edges(here, false, [&](const handle_t& next) {
            
            auto iter = inward_degree.find(next);
            // we should never be able to reach the opposite orientation of a node
            assert(iter != inward_degree.end());
            // implicitly remove the edge
            iter->second--;
            if (iter->second == 0) {
                // after removing this edge, the node is now a head, add it to the queue
                stack.push_back(next);
            }
        });
    }
    
    if (order.size() != g->node_size()) {
        cerr << "error:[algorithms] lazy topological sort is invalid on non-DAG graph, cannot complete algorithm" << endl;
        assert(false);
    }
    
    return order;
}
    
    
std::vector<handle_t> lazy_topological_order(const HandleGraph* g) {
    return lazy_topological_order_internal(g, false);
}
    
std::vector<handle_t> lazier_topological_order(const HandleGraph* g) {
    return lazy_topological_order_internal(g, true);
}


void topological_sort(MutableHandleGraph& g, bool compact_ids) {
    g.apply_ordering(topological_order(&g), compact_ids);
}

}

}
