#include "graph.h"
#include <fstream>
#include <sstream>
#include <limits>
#include <stdexcept>
#include <iostream>

Graph::Graph() : num_vertices(0), start_vertex(0), end_vertex(0) {}

Graph Graph::read_from_file(const std::string &filename)
{
    Graph graph;
    std::ifstream file(filename);

    if (!file.is_open())
    {
        throw std::runtime_error("Unable to open file: " + filename);
    }

    file >> graph.num_vertices >> graph.start_vertex >> graph.end_vertex;

    graph.adj_list.resize(graph.num_vertices);
    int num_local_vertices = graph.end_vertex - graph.start_vertex + 1;
    graph.distances.resize(num_local_vertices, INF);

    if (graph.start_vertex == 0)
    {
        graph.distances[0] = 0;
    }

    int u, v, weight;
    while (file >> u >> v >> weight)
    {
        if (u >= graph.start_vertex && u <= graph.end_vertex)
        {
            int local_u = u - graph.start_vertex;

            graph.adj_list[local_u].emplace_back(v, weight);
        }

        if (v >= graph.start_vertex && v <= graph.end_vertex)
        {
            int local_v = v - graph.start_vertex;

            graph.adj_list[local_v].emplace_back(u, weight);
        }
    }

    for (int i = graph.start_vertex; i <= graph.end_vertex; ++i)
    {
        if (graph.distances[i - graph.start_vertex] == 0)
            graph.buckets[0].insert(i);

        else
            graph.buckets[INF].insert(i);
    }

    file.close();
    return graph;
}

void Graph::print() const
{
    std::cout << "Number of vertices: " << num_vertices << std::endl;
    std::cout << "Start vertex: " << start_vertex << std::endl;
    std::cout << "End vertex: " << end_vertex << std::endl;

    for (int i = 0; i < num_vertices; ++i)
    {
        std::cout << "Vertex " << i + start_vertex << ": ";
        for (const auto &edge : adj_list[i])
        {
            std::cout << "(" << edge.first + start_vertex << ", " << edge.second << ") ";
        }
        std::cout << std::endl;
    }
}

void Graph::save_distances(const std::string &filename) const
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Unable to open file: " + filename);
    }

    for (long long distance : distances)
    {
        if (distance == INF)
            file << -1 << std::endl;
        else
            file << distance << std::endl;
    }
}

long long Graph::update_buckets(int local_target, long long old_distance, long long new_distance, long long delta)
{
    long long old_bucket = old_distance / delta;
    if (old_distance == INF)
    {
        old_bucket = INF;
    }
    long long new_bucket = new_distance / delta;

    if (old_bucket != new_bucket)
    {
        buckets[old_bucket].erase(local_target + start_vertex);
        if (buckets[old_bucket].empty())
        {
            buckets.erase(old_bucket);
        }

        buckets[new_bucket].insert(local_target + start_vertex);
    }

    return new_bucket;
}

int Graph::owner(int vertex, int world_size) const
{
    int base_per_process = num_vertices / world_size;
    int remainder = num_vertices % world_size;

    if (vertex < remainder * (base_per_process + 1))
    {
        return vertex / (base_per_process + 1);
    }
    return remainder + (vertex - remainder * (base_per_process + 1)) / base_per_process;
}