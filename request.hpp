#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <mpi.h>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <vector>
#include <iostream>

#define CENTRAL_NODE 0
#define MAX_SIZE_MSG 50
#define TAG 100
#define TAG_READ 150
#define SIZE_MEM 5

enum operation{R, W, RW};
enum mode_generation{DIRECT, INDIRECT};
enum msg_type{MSG, REQUEST, END_CHILD, ADD, END_TASK, ASKING_END, END_SIMU};


struct msg{
    msg(){};
    msg(int a, int b, int c, int d, int e, int f, int i, int j,int k){
        type=a; 
        ref=b; 
        idp=c; 
        rw=d; 
        add=e; 
        cptc=f; 
        id=i; 
        node=j; 
        node_d=k;}
        int type;
        int ref; // ref data asked
        int idp; // id pere
        int rw; // operation r w rw
        int add; // location data 
        int cptc; //cpt child
        int id; // id request
        int node; // node requesting
        int node_d; // node dest
};
class Data{
    private:
    public:
        Data(){}
        Data(int v, int r, int a) : val(v), ref(r), add(a){}
        int val;
        int ref;
        int add;

};


#endif
