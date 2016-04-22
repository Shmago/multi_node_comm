#include <algorithm>
#include <mpi.h>
#include "memorym.hpp"
#include <time.h>

extern MPI_Datatype msg_type;

//init memory with a uniq global reference
//size correspond to the total number of ref
void init_memory(int size, struct info *inf){
    if(inf->rank==CENTRAL_NODE){
        inf->cpt_end_simu = 0;//compting msg until next scheduling
        for(int i=0; i<size; i++){
            Data dat = Data(i+1, i+1, 0);//corresponding with ref and add 
                                         //of node's mem_local
            std::pair<int, Data> p = std::make_pair(dat.ref, dat); 
            inf->cn.mem_global.insert(p);
        }
    }
    else{
        init_info(size, inf); 
    }
}

void CentralNode::handle_endChild2(struct msg *endCh){
    
    //the req's end_child potentially doesn't come contigusly
    struct msg curent(0,0,0,0,0,0,0,0,0);
    std::pair<int, int> sendornot = std::make_pair(endCh->id, 1);
    add_sent_ornot.insert(sendornot);//precise that the address that 
                                          //will be recv isn't sent yet
    if(endCh->idp == forest[endCh->ref].back().id){//if child of the last task
                                                   //without parent
        curent = forest[endCh->ref].back();
    }
    else{
        curent = *std::find_if(forest[endCh->ref].begin(), 
                               forest[endCh->ref].end(), 
                               [&endCh] (struct msg & v) {
                                    //find the end_child's parent
                                    return v.id == endCh->idp;
                                });
    }
    // request in pending queu for write dependances
    if(pending.count(curent.ref) == 0){
        //insert a new list
        pending.insert(std::make_pair(curent.ref, 
                                std::list<struct msg >(1, curent)));
    }
    else{
        pending[curent.ref].push_back(curent);
    }
    // send request to des node to obtain address later
    //MPI_Send(&curent, 1, msg_type, 
     //        curent.node_d, TAG, MPI_COMM_WORLD);
}

void send_first_message(struct info *inf, int n, int mode){//mode DIRECT = CentralNode makes 
                                                           //requests and sends them directly to the target
                                                           //mode INDIRECT = CentralNode sends the request first to 
                                                           //the origin node
    srand(time(NULL));
    int ref_alea;
    int Q = n / (inf->nproc-1);//-1 because their is a central node
    int R = n % (inf->nproc-1);
    int nloc_i;
    int ideb_i;
    struct msg req(0,0,0,0,0,0,0,0,0);
    struct msg endCh(0,0,0,0,0,0,0,0,0);
    int ideb, ifin;

    inf->ntot = n;
    for(int i=0; i<inf->nproc; i++){
        if(i!=CENTRAL_NODE){
            if(i<R){
                nloc_i = Q+1;
                ideb_i = i* (Q+1);
                ref_alea = rand()%nloc_i + ideb_i+1;//ref according to the rank CentralNode gonna send 
            }
            else{
                nloc_i = Q;
                ideb_i = R * (Q+1) + (i - R) * Q;
                ref_alea = rand()%nloc_i + ideb_i+1;//ref according to the rank CentralNode gonna send 
            }
            req.type=REQUEST;
            req.node_d=i;
	    /*if (i < R) {
		ideb = i* (Q+1);
		ifin = ideb + nloc;
	    } else{
		ideb = R * (Q+1) + (i- R) * Q;
		ifin = ideb + nloc;
	    }*/
            req.ref=ref_alea;
            req.node = rand()%(inf->nproc);//origin node aleatoire
            while(req.node == CENTRAL_NODE || req.node == req.node_d){
                req.node = rand()%(inf->nproc);
            }
            req.rw=R;
            if(i==inf->nproc/2){
		req.rw=W;	
	    }
            req.id=i;
            req.idp=0;
            endCh.type=END_CHILD;
            endCh.node_d=i;



            endCh.ref=ref_alea;

            endCh.rw=req.rw;
            endCh.id=req.id;
            endCh.idp=req.id;
            endCh.node = req.node;
            if(mode == DIRECT){
                inf->cn.add(&req);
                inf->cn.handle_endChild2(&endCh);
		//MPI_Send() anactive in handle_Child !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                MPI_Send(&req,1, msg_type, i, TAG, MPI_COMM_WORLD);
            }
            /*else{
                inf->cn.handle_endChild2(&req);
                MPI_Send(&req,1, msg_type, req.node, TAG, MPI_COMM_WORLD);
            }*/
        }
    }
}

void init_info(int n, struct info *inf) {

    int Q, R;

    Q = n / (inf->nproc-1);//-1 because their is a central node
    R = n % (inf->nproc-1);

    inf->ntot = n;

    if (inf->rank < R) {

	inf->nloc = Q+1;
	inf->ideb = inf->rank * (Q+1);
	inf->ifin = inf->ideb + inf->nloc;

    } else{

	inf->nloc = Q;
	inf->ideb = R * (Q+1) + (inf->rank - R) * Q;
	inf->ifin = inf->ideb + inf->nloc;
    }
    
    //init memory with a uniq global reference
    for(int i=0; i< inf->nloc; i++){
        Data dat = Data(inf->ideb+i+1, inf->ideb+i+1, i);//rank=node
        std::pair<int, Data> p = std::make_pair(dat.ref, dat); 
        inf->n.mem_local.insert(p);
    }

    printf("nproc : %d rank : %d ntot : %d  nloc : %d ideb : %d ifin : %d\n", 
            inf->nproc, inf->rank, inf->ntot, 
            inf->nloc, inf->ideb, inf->ifin);
}

void handle_request_node(struct msg *curent, struct info *inf){ //node receving 
                                                             //asking addresses
    int a = inf->n.mem_local[curent->ref].add;
    struct msg msg_add = *curent;
    msg_add.type=ADD;
    msg_add.add=a;
    //send the ref's address to the central node
    MPI_Send(&msg_add, 1,msg_type, CENTRAL_NODE, TAG, 
             MPI_COMM_WORLD);
}
void handle_address_node(struct msg *curent, struct info *inf){
    
    MPI_Win win;
    MPI_Group comm_group, group;
    int ranks[2];
    ranks[0] = curent->node;//for forming groups, later
    ranks[1] = curent->node_d;
    MPI_Comm_group(MPI_COMM_WORLD,&comm_group);
    /*if(curent->rw==W){//write
        if (inf->rank == curent->node_d){//target node
            MPI_Win_create(&inf->n.mem_local[curent->ref].val,
                           sizeof(int),sizeof(int), 
                           MPI_INFO_NULL,MPI_COMM_WORLD,&win);
        }

        else //if(inf->rank == curent->node){
            MPI_Win_create(NULL,0,sizeof(int),
                           MPI_INFO_NULL,MPI_COMM_WORLD,&win);
        }
        if (inf->rank == curent->node_d) {//target node
            MPI_Group_incl(comm_group,1,ranks,&group);
            //begin the exposure epoch
            MPI_Win_post(group,0,win);
            //wait for epoch to end
            MPI_Win_wait(win);
        }
        else if(inf->rank == curent->node) {//origin node
            //add the data to the mem local if isn't reference yet
            if(inf->n.mem_local.count(curent->ref) == 0){
                //insert a new pair
                Data dat = Data();
                dat.ref = curent->ref;
                dat.add = curent->add;
                inf->n.mem_local.insert(std::make_pair(curent->ref, dat)); 
            }
            MPI_Group_incl(comm_group,1,ranks+1,&group);
            //begin the access epoch
            MPI_Win_start(group,0,win);
            //put into target according to my inf->rank
            MPI_Put(&inf->n.mem_local[curent->ref].val,1,MPI_INT,curent->node_d,
                    0,1, MPI_INT,win);
            //terminate the access epoch
            MPI_Win_complete(win);
            struct msg end_t(0,0,0,0,0,0,0,0,0);
            end_t.type=END_TASK;
            end_t.rw=curent->rw;
            end_t.ref=curent->ref;
            end_t.id=curent->id;
            //send the endTask msg
            MPI_Send(&end_t, 1, msg_type, 
                     CENTRAL_NODE, TAG, MPI_COMM_WORLD);
        }
    }
    //else//{//read
        if (inf->rank == curent->node_d){//target node
            MPI_Win_create(&inf->n.mem_local[curent->ref].val,
                           sizeof(int),sizeof(int), //the data is 
                                                    //representing by a int
                           MPI_INFO_NULL,MPI_COMM_WORLD,&win);
        }

        else //if(inf->rank == curent->nodea){
            MPI_Win_create(NULL,0,sizeof(int),
                           MPI_INFO_NULL,MPI_COMM_WORLD,&win);
        }
        if (inf->rank == curent->node_d) {//target node
            MPI_Group_incl(comm_group,1,ranks,&group);
            //begin the exposure epoch
            MPI_Win_post(group,0,win);
            //wait for epoch to end
            MPI_Win_wait(win);
        }
        else if(inf->rank == curent->node){//origin node
            //add the data to the mem local if isn't reference yet
            if(inf->n.mem_local.count(curent->ref) == 0){
                //insert a new pair 
                Data dat = Data();
                dat.ref=curent->ref;
                dat.add=curent->add;
                inf->n.mem_local.insert(std::make_pair(curent->ref, dat)); 
            }
            MPI_Group_incl(comm_group,1,ranks+1,&group);
            //begin the access epoch
            MPI_Win_start(group,0,win);
            //put into target according to my inf->rank
            MPI_Get(&inf->n.mem_local[curent->ref].val,1,MPI_INT,curent->node_d,
                    0, 1, MPI_INT,win);
            //terminate the access epoch
            MPI_Win_complete(win);
            struct msg end_t(0,0,0,0,0,0,0,0,0);
            end_t.type=END_TASK;
            end_t.rw=curent->rw;
            end_t.ref=curent->ref;
            end_t.id=curent->id;
            //send the endTask msg
            MPI_Send(&end_t, 1, msg_type, 
                     CENTRAL_NODE, TAG, MPI_COMM_WORLD);
        }                                                               
    }
        //free window and groups
        MPI_Win_free(&win);
        MPI_Group_free(&group);
        MPI_Group_free(&comm_group);*/
        struct msg end_t = *curent;
        end_t.type = END_TASK;
        MPI_Send(&end_t, 1, msg_type, 
                     CENTRAL_NODE, TAG, MPI_COMM_WORLD);
        printf("ACTION RANK %d\n\n", inf->rank);
}

void CentralNode::end_task(struct msg* et){
    if(et->rw == W){//write's end
            pending_w[et->ref]=1;//no more write pending
    }
                                       //end_task's req
    auto to_erase= std::find_if(pending[et->ref].begin(), 
                           pending[et->ref].end(), 
                           [&et] (struct msg& v) {
                                return v.id == et->id;
                           });
    pending[et->ref].erase(to_erase);//remove the end_task's req from the list
    auto it_to_erase = add_sent_ornot.find(et->id);
    add_sent_ornot.erase(it_to_erase);

    if(!pending[et->ref].empty()){//if the list isn't empty
        			 //launch analysis to check if dependances still remain
        CentralNode::analyse_list(et->ref);
    }
}

void CentralNode::analyse_list(int ref){
    
    if(!pending_w[ref]){//pending write
       //waiting endTask of the pending_w, not necessary the continnue 
       //checking the list
       return;
    }
    else{//no pending write
        for(auto it = pending[ref].begin(); it != pending[ref].end();it++){
            if(it->rw == R){//analyse a read
                if(add_sent_ornot[it->id]){//address not already sent
                    struct msg msg_add = *it;
                    msg_add.type=ADD;
                    msg_add.ref=ref;
                    msg_add.add=mem_global[ref].ref;
                    //send to the origin node the address it wanted
                    MPI_Send(&msg_add, 1, msg_type, 
                             it->node, TAG, MPI_COMM_WORLD);
                             add_sent_ornot[it->id] = 0;
                }
                else{//add already send)
                    continue;
                }
            }
            else{//analyse a write
                if(it == pending[ref].begin()){//if the write is first of 
                                               //the list
                    if(add_sent_ornot[it->id]){//address not already sent
                        pending_w[ref] = 0;//this write is pending (blocking)
                        struct msg msg_add = *it;
                        msg_add.type=ADD;
                        msg_add.ref=ref;
                        msg_add.add=mem_global[ref].ref;
                        //send to the origin node the address it wanted
                        MPI_Send(&msg_add, 1, msg_type, 
                                 it->node, TAG, MPI_COMM_WORLD);
                        add_sent_ornot[it->id] = 0;//useless only usefeull for     
                                                    //reads
                    }
                    else{//add already send)
                        continue;
                    }
                }
            }
        }
    }
}


void CentralNode::add(struct msg*req){
    // map a new tree if not exist yet
    if(forest.count(req->ref) == 0){
        //insert a new list
        forest.insert(std::make_pair(req->ref, 
                                std::list<struct msg> (1, *req)));
        pending_w.insert(std::make_pair(req->ref, 1));//initiate at "no write 
                                                     //pending"
    }
    else{//if tree already exist
        // no father, other tree
        if(req->idp == 0){
            //no father we insert at the end
            forest[req->ref].push_back(*req);
        }
        // if req has a father
        else{
            // find the parent
            auto parent = std::find_if(forest[req->ref].begin(), 
                                   forest[req->ref].end(), 
                                   [&req] (struct msg & v) {
                                        return v.id == req->idp;
                                    });

            // insert just before the next parent id different if 
            // find
            if(forest[req->ref].end() != parent){
                auto next_parent = std::find_if(parent, 
                                       forest[req->ref].end(), 
                                       [&req] (struct msg & v) {
                                       return v.idp != req->idp;
                                        });
                if(parent == forest[req->ref].end()){
                    //insert at the end
                    forest[req->ref].push_back(*req);
                }
                else
                    //insert just before the next req with diff idp
                    forest[req->ref].insert(next_parent, *req);
            }
            else{
                //if parent is at the end, push_back
                forest[req->ref].push_back(*req);
            }
        }
    }
}

void CentralNode::first_analyse(struct msg *curent){
    
    //update ref's address
    mem_global[curent->ref].add=curent->add;
    if(curent->rw==R){//if a read
        if(pending_w[curent->ref]){//if no pending write
            struct msg msg_add = *curent;
            msg_add.type=ADD;
            // send add with id req to origin node req (the node will trigger
            // the action at reception of this add message
            MPI_Send(&msg_add, 1, msg_type, 
                     curent->node, TAG, MPI_COMM_WORLD);
            add_sent_ornot[curent->id] = 0;//notice sent
        }
    }
    else{
        if(pending[curent->ref].begin()->id == curent->id){//no dependances
            pending_w[curent->ref]=0;//a write is in process
            struct msg msg_add = *curent;
            msg_add.type=ADD;
            // send add with id req to origin node req (the node will trigger
            // the action at reception of this add message)
            MPI_Send(&msg_add, 1, msg_type, 
                     curent->node, TAG, MPI_COMM_WORLD);
            add_sent_ornot[curent->id] = 0;//notice sent
        }
    }
}

void listening(struct info *inf){
    
    MPI_Status status;
    printf("listening central_node!!!!!!!\n");
    struct msg buf(0,0,0,0,0,0,0,0,0);        ;
    while(inf->cpt_end_simu != inf->nproc-1){
        //Recv from random node a msg from random type
        MPI_Recv(&buf, 1, msg_type, MPI_ANY_SOURCE, TAG, 
                 MPI_COMM_WORLD, &status);
        switch(buf.type){
            case REQUEST :{
                printf("CENTRAL_NODE RECV REQ FROM RANK %d\t\n\n",
                        status.MPI_SOURCE);
                inf->cn.add(&buf);
            }
                break;
            case END_CHILD :{
                printf("CENTRAL_NODE RECV END CHILD FROM RANK %d\t\n\n",
                        status.MPI_SOURCE);
                inf->cn.handle_endChild2(&buf);
            }
                break;
            case ADD :{
                printf("CENTRAL_NODE RECV ADD FROM RANK %d\t\n\n",
                        status.MPI_SOURCE);
                inf->cn.first_analyse(&buf);
            }
                break;
            case END_TASK:{
                printf("CENTRAL_NODE RECV END TASK FROM RANK %d\t\n\n",
                        status.MPI_SOURCE);
                inf->cn.end_task(&buf);
                inf->cpt_end_simu++;
            }
                break;
        }
        if(inf->cpt_end_simu==inf->nproc-1){
           break; 
        }
    }
}

void listening_node(struct info *inf){
    MPI_Status status;
    
    struct msg buf(0,0,0,0,0,0,0,0,0); 
    //Recv from random node a msg from random type
    while(1){
        MPI_Recv(&buf, 1, msg_type, MPI_ANY_SOURCE, TAG, 
                 MPI_COMM_WORLD, &status);
        switch(buf.type){
            case REQUEST:{
                printf("RANK %d RECV REQ FROM RANK %d\t\n\n",inf->rank,
                        status.MPI_SOURCE);
                handle_request_node(&buf, inf);
            }
            break;
            case ADD :{
                printf("RANK % dRECV ADD FROM RANK %d\t\n\n",inf->rank,
                        status.MPI_SOURCE);
                handle_address_node(&buf, inf);
            }
            break;
        }
	if(buf.type==END_SIMU)
		break;
    }
    //add msg to the tree if it is a request
    //trigger analyse if it a endChild msg
}

/*void init_msg(std::vector<std::vector<request> > task){
   for(int i=0;i<5;i++){
       for(int j=0; j<0; j++){
           request r = request();
           r.node = //nombre alea entre 0et nproc-1
           task[i][j] = request(); 
       }
   }
}*/
