#include "memorym.hpp"


MPI_Datatype msg_type;

int main(int argc, char **argv){
    struct info inf;
    MPI_Init(&argc, &argv);
    struct msg calibre(0,0,0,0,0,0,0,0,0);
    MPI_Datatype array_of_types[1];
    array_of_types[0] = MPI_INT;
    
    int array_of_blocklengths[1];
    array_of_blocklengths[0] = 9;
    
    MPI_Aint array_of_displaysments[1];
    MPI_Aint address1, address2;
    MPI_Get_address(&calibre, &address1);
    MPI_Get_address(&calibre.type, &address2);
    array_of_displaysments[0] = address2 - address1;
    
    MPI_Type_create_struct(1, array_of_blocklengths, array_of_displaysments, 
                           array_of_types, &msg_type);

    MPI_Type_commit(&msg_type);

    MPI_Comm_size(MPI_COMM_WORLD, &(inf.nproc));
    MPI_Comm_rank(MPI_COMM_WORLD, &(inf.rank));
    int size = inf.nproc-1;
    init_memory(size, &inf);
    MPI_Barrier(MPI_COMM_WORLD);
    if(inf.rank != CENTRAL_NODE){
        listening_node(&inf);
    }
    else{
        send_first_message(&inf, size, DIRECT);
        listening(&inf);
	struct msg end(END_SIMU,0,0,0,0,0,0,0,0);
	for(int i=0; i<inf.nproc;i++){
		if(i!=CENTRAL_NODE){
			MPI_Send(&end, 1, msg_type, i, TAG, MPI_COMM_WORLD); 
		}
	}
    } 
    MPI_Type_free(&msg_type);
    MPI_Finalize();
return 0;
}
