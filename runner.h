#ifndef RUNNER_H
#define RUNNER_H

#include "graph.h"
#include <mpi.h>

#ifndef DELTA
#define DELTA 10
#endif

#ifndef EDGE_CLASSIFICATION
#define EDGE_CLASSIFICATION true
#endif

#ifndef HYBRIDIZATION
#define HYBRIDIZATION true
#endif

#ifndef TAU
#define TAU 0.8
#endif

class Runner
{
public:
    Runner(Graph &graph, int rank, int world_size)
        : graph(graph), rank(rank), world_size(world_size) {};

    void run();

private:
    Graph &graph;
    int rank;
    int world_size;
    long long current_bucket = 0;
    int settled = 0;
    int bellman_threshold = 0;
    bool bellman_phase = false;

    long long get_next_bucket();
    void send_outer_edges(const std::vector<std::pair<int, long long>> &outer_edges, std::unordered_set<int> &active);
    void process_bucket();
    void check_bellman();
};

#endif // RUNNER_H