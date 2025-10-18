#include <mpi.h>
#include <iostream>
#include "graph.h"
#include "runner.h"

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (argc < 3)
    {
        if (my_rank == 0)
        {
            std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    std::string input_file = argv[1];
    std::string output_file = argv[2];

    Graph graph = Graph::read_from_file(input_file);

    double start_time = MPI_Wtime();

    Runner runner(graph, my_rank, world_size);
    runner.run();

    double end_time = MPI_Wtime();

    graph.save_distances(output_file);

    if (my_rank == 0)
    {
        std::cout << (end_time - start_time) << std::endl;
    }

    MPI_Finalize();

    return 0;
}