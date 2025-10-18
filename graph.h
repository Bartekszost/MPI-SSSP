#ifndef __GRAPH_H__
#define __GRAPH_H__

#include <vector>
#include <string>
#include <map>
#include <unordered_set>
#include <climits>

constexpr long long INF = LLONG_MAX;

class Graph
{
public:
    int num_vertices;
    int start_vertex;
    int end_vertex;
    std::vector<std::vector<std::pair<int, int>>> adj_list;
    std::vector<long long> distances;
    std::map<long long, std::unordered_set<int>> buckets;

    Graph();
    void print() const;
    void save_distances(const std::string &filename) const;
    long long update_buckets(int local_vertex, long long old_distance, long long new_distance, long long delta);
    int owner(int vertex, int world_size) const;
    static Graph read_from_file(const std::string &filename);
};

#endif // __GRAPH_H__