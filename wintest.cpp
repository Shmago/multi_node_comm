

#include <mpi.h>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <vector>
#include <iostream>
 
int main(int argc, char **argv){
    MPI_Init(&argc, &argv);
    int rank, nproc;
    
    MPI_Comm_size(MPI_COMM_WORLD, &(nproc));
    MPI_Comm_rank(MPI_COMM_WORLD, &(rank));
    MPI_Win win;
    MPI_Aint remote;
        MPI_Aint local;
    MPI_Win_create_dynamic(MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    
    if(rank==0){
        //int *a = (int*)malloc(sizeof(int)); 
        int a= 4;
        MPI_Win_attach(win, &a, sizeof(int));
        MPI_Get_address(&a,  &local);
        MPI_Send(&local, 1, MPI_AINT, 1, 1, MPI_COMM_WORLD);
    }
    else{
        //MPI_Status reqstat;
        //MPI_Recv(&sdisp_remote, 1, MPI_AINT, 0, 1, MPI_COMM_WORLD, &reqstat );
        int val;
        MPI_Status reqstat;
        MPI_Recv(&remote, 1, MPI_AINT, 0, 1, MPI_COMM_WORLD, &reqstat );
        MPI_Get(&val, 1, MPI_INT, 0, remote, 1, MPI_INT, win);
        
    }
    //MPI_Win_free(&win);
}

