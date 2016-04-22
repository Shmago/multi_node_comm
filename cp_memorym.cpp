#include <algorithm>
#include <mpi.h>
#include "memorym.hpp"

void Node::handle_request(request *curent, struct info *inf){ //node receving 
                                                             //asking addresses
    int a=inf->n.mem_local[curent->ref].add;
    address msg_add(curent->ref, curent->id, a);
    //send the ref's address to the central node
    MPI_Send(&msg_add, sizeof(address), MPI_BYTE, CENTRAL_NODE, TAG, 
             MPI_COMM_WORLD);
}
void Node::handle_address(address *curent, struct info *inf){
        MPI_Win win;
        MPI_Group comm_group, group;
        int ranks[2];
        ranks[0] = curent->node;//for forming groups, later
        ranks[1] = curent->node_d;
        MPI_Comm_group(MPI_COMM_WORLD,&comm_group);
    if(curent->rw){//write
        if (inf->rank == curent->node_d){//target node
            MPI_Win_create(&inf->n.mem_local[curent->ref].val,
                           sizeof(int),sizeof(int), 
                           MPI_INFO_NULL,MPI_COMM_WORLD,&win);
        }

        else if(inf->rank == curent->node){
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
                //insert a new list
                Data dat;
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
        }
    }
    else{//read
        if (inf->rank == curent->node_d){//target node
            MPI_Win_create(&inf->n.mem_local[curent->ref].val,
                           sizeof(int),sizeof(int), //the data is 
                                                    //representing by a int
                           MPI_INFO_NULL,MPI_COMM_WORLD,&win);
        }

        else if(inf->rank == curent->node){
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
                //insert a new list
                Data dat;
                inf->n.mem_local.insert(std::make_pair(curent->ref, dat)); 
            }
            MPI_Group_incl(comm_group,1,ranks+1,&group);
            //begin the access epoch
            MPI_Win_start(group,0,win);
            //put into target according to my inf->rank
            MPI_Get(&inf->n.mem_local[curent->ref].val,1,MPI_INT,curent->node_d,
                    0,1, MPI_INT,win);
            //terminate the access epoch
            MPI_Win_complete(win);
        }                                                               }
    }
        //free window and groups
        MPI_Win_free(&win);
        MPI_Group_free(&group);
        MPI_Group_free(&comm_group);
}

void CentralNode::end_task(endTask* et){
    if(et->rw !=0){//write's end
            pending_w[et->ref]=0;//no more write pending
    }
                                       //end_task's req
    auto to_erase= std::find_if(pending[et->ref].begin(), 
                           pending[et->ref].end(), 
                           [&et] (request& v) {
                                return v.id == et->id;
                            });
    pending[et->ref].erase(to_erase);//remove the end_task's req from the list
    if(!pending[et->ref].empty()){//if the list isn't empty
        //launch analysis to check if dependances still remain
        CentralNode::analyse_list(et->ref);
    }
}

void CentralNode::analyse_list(int ref){
    if(pending_w[ref]){//pending write
       //waiting endTask of the pending_w, not necessary the continnue 
       //checking the list
       return;
    }
    else{//no pending write
        for(auto it = pending[ref].begin(); it == pending[ref].end();it++){
            if(it->rw == R){//analyse a read
                if(add_sent_ornot[ref]){//address not already sent
                    address msg_add(ref, it->id, mem_global[ref].ref);
                    //send to the origin node the address it wanted
                    MPI_Send(&msg_add, sizeof(address), MPI_BYTE, 
                             it->node, tag_read, MPI_COMM_WORLD);
                             add_sent_ornot[ref] = 1;
                }
                else{//add already send)
                    continue;
                }
            }
            else{//analyse a write
                if(it == pending[ref].begin()){//if the write is first of the list
                    pending_w[ref] = 1;//this write is pending (blocking)
                    address msg_add(ref, it->id, mem_global[ref].ref);
                    //send to the origin node the address it wanted
                    MPI_Send(&msg_add, sizeof(address), MPI_BYTE, 
                             it->node, tag_read, MPI_COMM_WORLD);
                             add_sent_ornot[ref] = 1;
                    add_sent_ornot[ref] = 1;//useless only usefeull for reads
                }
            }
        }
    }
}

void CentralNode::handle_endChild2(endChild *add_recv){
    //the req's end_child potentially doesn't come contigusly
    request curent = request();
    std::pair<int, int> sendornot = std::make_pair(add_recv->ref, 0);
    add_sent_ornot.insert(sendornot);//precise that the address that 
                                          //will be recv isn't sent yet
    if(add_recv->idp == forest[add_recv->ref].back().id){
        curent = forest[add_recv->ref].back();
    }
    else{
        curent = *std::find_if(forest[add_recv->ref].begin(), 
                               forest[add_recv->ref].end(), 
                               [&add_recv] (request& v) {
                                    //find the end_child's parent
                                    return v.id == add_recv->idp;
                                });


    }
    // request in pending queu for write dependances
    if(pending.count(curent.ref) == 0){
        //insert a new list
        pending.insert(std::make_pair(curent.ref, 
                                std::list<request>(1, curent)));
    }
    else{
        pending[curent.ref].push_back(curent);
    }
    //send request to des node to obtain address later
    MPI_Send(&curent, sizeof(request), MPI_BYTE, 
             curent.node_d, tag_read, MPI_COMM_WORLD);
}

void CentralNode::add(request *req){
    // map a new tree if not exist yet
    if(forest.count(req->ref) == 0){
        //insert a new list
        forest.insert(std::make_pair(req->ref, 
                                std::list<request>(1, *req)));
    }
    else{//si tree already exist
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
                                   [&req] (request& v) {
                                        return v.id == req->idp;
                                    });

            // insert just before the next parent id different if 
            // find
            if(forest[req->ref].end() != parent){
                auto next_parent = std::find_if(parent, 
                                       forest[req->ref].end(), 
                                       [&req] (request& v) {
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

void CentralNode::first_analyse(address*curent){
    //update ref's address
    mem_global[curent->ref].add=curent->add;
    if(curent->rw==R){//if a read
        if(pending_w[curent->ref]){//if no pending write
            address msg_add(curent->ref, curent->id);
            // send add with id req to origin node req (the node will trigger
            // the action at reception of this add message
            MPI_Send(&msg_add, sizeof(address), MPI_BYTE, 
                     curent->node, tag_read, MPI_COMM_WORLD);
            add_sent_ornot[curent->ref] = 1;//notice sent
        }
    }
    else{
        if(pending[curent->ref].begin()->id == curent->id){//no dependances
            pending_w[curent->ref]=1;//a write is in process
            address msg_add(curent->ref, curent->id);
            // send add with id req to origin node req (the node will trigger
            // the action at reception of this add message)
            MPI_Send(&msg_add, sizeof(address), MPI_BYTE, 
                     curent->node, tag_read, MPI_COMM_WORLD);
            add_sent_ornot[curent->ref] = 1;//notice sent
        }
    }
}

void CentralNode::listening(){
    MPI_Status *status;
    int *buf=(int *)malloc(MAX_SIZE_MSG*sizeof(int));
    //Recv from random node a msg from random type
    MPI_Recv(buf, sizeof(msg), MPI_BYTE, MPI_ANY_SOURCE, tag_read, 
             MPI_COMM_WORLD, status);
    msg* req_p = (msg*) buf;
    switch(req_p->type){
        case REQUEST :{
            request* req = (request*) req_p;
            add(req);
            }
            break;
        case END_CHILD :{
            endChild * req = (endChild *) req_p;
            handle_endChild2(req);
            }
            break;
        case ADD :{
            address* req = (address*) req_p;
            first_analyse(req);
            }
            break;
        case END_TASK:{
            endTask* et= (endTask*) req_p;
            end_task(et);
            }
            break;
    }
}

void Node::listening(){
    MPI_Status *status;
    int *buf=(int *)malloc(MAX_SIZE_MSG*sizeof(int));
    //Recv from random node a msg from random type
    MPI_Recv(buf, sizeof(msg), MPI_BYTE, MPI_ANY_SOURCE, tag_read, 
             MPI_COMM_WORLD, status);
    msg* req_p = (msg*) buf;
    switch(req_p->type){
        case REQUEST:{
            request* req = (request*) req_p;
            Node::handle_request(req);
            }
            break;
        case ADD :{
            address* req = (address*) req_p;
            Node::handle_address(req);
            }
            break;
    }
    //add msg to the tree if it is a request
    //trigger analyse if it a endChild msg
}
