#ifndef MEMORYM_HPP
#define MEMORYM_HPP

#include <list>
#include <map>
#include "request.hpp"


class CentralNode
{
private:
public:
    CentralNode(){}
    std::map<int, std::list<struct msg> > forest; //one tree per ref
    std::map<int, Data > mem_global; //ref of all datas
    std::map<int, std::list<struct msg> > pending; //request in treatment
    std::map<int,int> pending_w; //couple of ref, pending_w
    std::map<int,int> add_sent_ornot; //to know if a address for a request 
                                      //has been already sent
    std::vector<std::vector<struct msg> > task;
    // sort request depth first
    void add(struct msg *r);
    void handle_endChild2(struct msg*add_recv);
    void end_task(struct msg* et);
    void analyse_list(int ref);//analyse the pending request for a given ref
    void first_analyse(struct msg *curent);
    void init_msg(std::vector<std::vector<struct msg> > task);
};

class Node
{
private:
public:
    Node(){}
    std::list<struct msg> pending; //request in treatment
    std::map<int, Data > mem_local; //ref of datas knowed by the node
    // sort request depth first
};

struct info{
    int rank;
    int nproc;

    int ntot;
    int nloc;

    int ideb;
    int ifin;

    int cpt_end_simu;

    CentralNode cn;
    Node n;
};


void listening(struct info *inf);
void listening_node(struct info *inf);
void handle_request_node(struct msg* req, struct info *inf); //node receving asking 
void handle_address_node(struct msg* a, struct info *inf);
void init_info(int n, struct info *inf);
void init_memory(int size, struct info *inf);
void send_first_message(struct info *inf, int n, int mode);

#endif
