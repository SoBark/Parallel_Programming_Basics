#include <mpi.h>
#include <stdio.h>

int main(int argc,char** argv)
{
    int id, nums, nlength;
    char node_name[MPI_MAX_PROCESSOR_NAME];

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&id);
    MPI_Comm_size(MPI_COMM_WORLD,&nums);
    MPI_Get_processor_name(node_name,&nlength);

    printf("Hello, MPI! thread %d of %d on %s\n",
            id, nums, node_name);
    MPI_Finalize();

    return 0;
}
