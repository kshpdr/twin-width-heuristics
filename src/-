#ifndef BOOSTGRAPH_HPP
#define BOOSTGRAPH_HPP

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/bipartite.hpp>
#include <boost/graph/graph_utility.hpp>
#include <vector>
#include <set>

class BoostGraph {
public:
    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS> Graph;

    BoostGraph() {}

    BoostGraph(int numVertices) : g(numVertices) {}

    void addEdge(int u, int v) {
        add_edge(u, v, g);
    }

    const Graph& getGraph() const {
        return g;
    }

    std::vector<std::set<std::pair<int, int>>> getConnectedComponents() {
        std::vector<int> component(num_vertices(g));
        int num = connected_components(g, &component[0]);

        std::vector<std::set<std::pair<int, int>>> components(num);
        Graph::edge_iterator ei, ei_end;
        for(boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei) {
            int source_vertex = boost::source(*ei, g); // Corrected usage
            int target_vertex = boost::target(*ei, g); // Corrected usage
            components[component[source_vertex]].insert({source_vertex, target_vertex});
        }

        return components;
    }

    std::pair<std::vector<std::set<std::pair<int, int>>>, std::vector<std::vector<int>>> getConnectedComponentAndVertices() {
        std::vector<int> component(num_vertices(g));
        int num = connected_components(g, &component[0]);

        std::vector<std::set<std::pair<int, int>>> components(num);
        std::vector<std::set<int>> vertices_sets(num);
        
        // Process edges and gather vertices that are connected by these edges
        Graph::edge_iterator ei, ei_end;
        for(boost::tie(ei, ei_end) = edges(g); ei != ei_end; ++ei) {
            int source_vertex = boost::source(*ei, g);
            int target_vertex = boost::target(*ei, g);
            components[component[source_vertex]].insert({source_vertex, target_vertex});
            vertices_sets[component[source_vertex]].insert(source_vertex);
            vertices_sets[component[target_vertex]].insert(target_vertex);
        }

        // Also consider all individual vertices 
        for (size_t i = 0; i < num_vertices(g); ++i) {
            vertices_sets[component[i]].insert(i);
        }

        std::vector<std::vector<int>> vertices_in_components(num);
        for (size_t i = 0; i < num; ++i) {
            vertices_in_components[i] = std::vector<int>(vertices_sets[i].begin(), vertices_sets[i].end());
        }

        return {components, vertices_in_components};
    }

    bool isBipartite(std::vector<int>& partition1, std::vector<int>& partition2) {
        std::vector<boost::default_color_type> color_map(num_vertices(g));
        bool is_bipartite = boost::is_bipartite(g, boost::make_iterator_property_map(
            color_map.begin(), get(boost::vertex_index, g)));
        
        if (is_bipartite) {
            for (size_t i = 0; i < color_map.size(); ++i) {
                if (color_map[i] == boost::color_traits<boost::default_color_type>::white()) {
                    partition1.push_back(i);
                } else {
                    partition2.push_back(i);
                }
            }
        }

        return is_bipartite;
    }

private:
    Graph g;
};

#endif // BOOSTGRAPH_HPP
