#include "runner.h"
#include <iostream>

long long Runner::get_next_bucket()
{
    long long local_next = INF;
    auto it = graph.buckets.upper_bound(current_bucket);
    if (it != graph.buckets.end())
        local_next = it->first;

    long long global_next = INF;
    MPI_Allreduce(&local_next,
                  &global_next,
                  1,
                  MPI_LONG_LONG_INT,
                  MPI_MIN,
                  MPI_COMM_WORLD);

    return (global_next == INF) ? -1 : global_next;
}

void Runner::send_outer_edges(const std::vector<std::pair<int, long long>> &outer_edges, std::unordered_set<int> &active)
{
    std::vector<std::vector<std::pair<int, long long>>> send_buffers(world_size);

    for (const auto &[target, weight] : outer_edges)
    {
        int owner = graph.owner(target, world_size);
        if (owner != rank)
            send_buffers[owner].emplace_back(target, weight);
    }

    std::vector<int> send_counts(world_size, 0);
    for (int i = 0; i < world_size; i++)
        send_counts[i] = send_buffers[i].size() * sizeof(std::pair<int, long long>);

    std::vector<int> recv_counts(world_size, 0);
    MPI_Alltoall(send_counts.data(), 1, MPI_INT,
                 recv_counts.data(), 1, MPI_INT,
                 MPI_COMM_WORLD);

    std::vector<int> send_displs(world_size, 0);
    std::vector<int> recv_displs(world_size, 0);
    for (int i = 1; i < world_size; i++)
    {
        send_displs[i] = send_displs[i - 1] + send_counts[i - 1];
        recv_displs[i] = recv_displs[i - 1] + recv_counts[i - 1];
    }

    int total_send = send_displs[world_size - 1] + send_counts[world_size - 1];
    int total_recv = recv_displs[world_size - 1] + recv_counts[world_size - 1];

    std::vector<std::pair<int, long long>> send_buffer(total_send / sizeof(std::pair<int, long long>));

    std::vector<std::pair<int, long long>> recv_buffer(total_recv / sizeof(std::pair<int, long long>));

    for (int i = 0; i < world_size; i++)
    {
        std::copy(send_buffers[i].begin(), send_buffers[i].end(),
                  send_buffer.begin() + send_displs[i] / sizeof(std::pair<int, long long>));
    }

    MPI_Alltoallv(send_buffer.data(), send_counts.data(), send_displs.data(),
                  MPI_BYTE, recv_buffer.data(), recv_counts.data(), recv_displs.data(),
                  MPI_BYTE, MPI_COMM_WORLD);

    for (int i = 0; i < world_size; i++)
    {
        if (recv_counts[i] > 0)
        {
            for (int j = 0; j < (int)(recv_counts[i] / sizeof(std::pair<int, long long>)); j++)
            {
                int target = recv_buffer[recv_displs[i] / sizeof(std::pair<int, long long>) + j].first;
                long long new_distance = recv_buffer[recv_displs[i] / sizeof(std::pair<int, long long>) + j].second;

                if (target >= graph.start_vertex && target <= graph.end_vertex)
                {
                    int local_target = target - graph.start_vertex;

                    if (new_distance < graph.distances[local_target])
                    {
                        long long old_distance = graph.distances[local_target];
                        graph.distances[local_target] = new_distance;

                        if (bellman_phase)
                        {
                            active.insert(target);
                            continue;
                        }

                        long long new_bucket = graph.update_buckets(local_target, old_distance, new_distance, DELTA);

                        if (new_bucket == current_bucket)
                            active.insert(target);
                    }
                }
            }
        }
    }
}

void Runner::process_bucket()
{
    std::unordered_set<int> active = graph.buckets[current_bucket];
    bool still_processing = false;

    do
    {
        std::unordered_set<int> new_active;
        std::vector<std::pair<int, long long>> outer_updates;

        for (int vertex : active)
        {
            int local_vertex = vertex - graph.start_vertex;
            if (graph.distances[local_vertex] == INF)
                continue;

            for (auto [target, weight] : graph.adj_list[local_vertex])
            {
                long long new_distance = graph.distances[local_vertex] + weight;

                if (EDGE_CLASSIFICATION && (!bellman_phase) && (weight > DELTA || new_distance / DELTA > current_bucket))
                {
                    continue;
                }

                if (target < graph.start_vertex || target > graph.end_vertex)
                {
                    outer_updates.push_back({target, new_distance});
                }
                else
                {
                    int local_target = target - graph.start_vertex;

                    if (new_distance < graph.distances[local_target])
                    {
                        long long old_distance = graph.distances[local_target];
                        graph.distances[local_target] = new_distance;

                        if (bellman_phase)
                        {
                            new_active.insert(target);
                            continue;
                        }

                        long long new_bucket = graph.update_buckets(local_target, old_distance, new_distance, DELTA);

                        if (new_bucket == current_bucket)
                            new_active.insert(target);
                    }
                }
            }
        }

        send_outer_edges(outer_updates, new_active);
        active = new_active;

        bool local_still_processing = !active.empty();
        still_processing = false;
        MPI_Allreduce(&local_still_processing,
                      &still_processing,
                      1,
                      MPI_C_BOOL,
                      MPI_LOR,
                      MPI_COMM_WORLD);

    } while (still_processing);

    active.clear();

    if (EDGE_CLASSIFICATION && !bellman_phase)
    {
        std::vector<std::pair<int, long long>> outer_updates;
        for (int vertex : graph.buckets[current_bucket])
        {
            int local_vertex = vertex - graph.start_vertex;
            if (graph.distances[local_vertex] == INF)
                continue;

            for (auto [target, weight] : graph.adj_list[local_vertex])
            {
                long long new_distance = graph.distances[local_vertex] + weight;

                if (target < graph.start_vertex || target > graph.end_vertex)
                {
                    outer_updates.push_back({target, new_distance});
                }
                else
                {
                    int local_target = target - graph.start_vertex;

                    if (new_distance < graph.distances[local_target])
                    {
                        long long old_distance = graph.distances[local_target];
                        graph.distances[local_target] = new_distance;

                        if (bellman_phase)
                            continue;

                        graph.update_buckets(local_target, old_distance, new_distance, DELTA);
                    }
                }
            }
        }

        send_outer_edges(outer_updates, active);

        MPI_Barrier(MPI_COMM_WORLD);
    }

    if (HYBRIDIZATION)
        settled += graph.buckets[current_bucket].size();

    graph.buckets.erase(current_bucket);
}

void Runner::check_bellman()
{
    if (bellman_phase)
        return;

    int global_settled = 0;
    MPI_Allreduce(&settled, &global_settled, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    if (global_settled >= bellman_threshold)
    {
        std::unordered_set<int> master_bucket;

        for (const auto &[bucket, vertices] : graph.buckets)
            master_bucket.insert(vertices.begin(), vertices.end());

        graph.buckets.clear();

        current_bucket = 0;
        graph.buckets[1] = master_bucket;
        bellman_phase = true;
    }
}

void Runner::run()
{
    if (HYBRIDIZATION)
        bellman_threshold = static_cast<int>(graph.num_vertices * TAU);

    while (true)
    {
        process_bucket();

        if (HYBRIDIZATION)
            check_bellman();

        current_bucket = get_next_bucket();
        if (current_bucket == -1)
            break;
    }
}