#include <cstdio>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>
#include <cassert>
#include <climits>
#include <future>
#include <mutex>
#include <libpmemobj.h>

using namespace std;

#define SIZE 7

class node;
class LIST;

POBJ_LAYOUT_BEGIN(LIST);
POBJ_LAYOUT_ROOT(LIST, LIST);
POBJ_LAYOUT_TOID(LIST, node);
POBJ_LAYOUT_END(LIST);

class node{
    private:
        int value;
        TOID(node) next;

    public:
        void constructor(PMEMobjpool *pop, TOID(node) n){
            value = 0;
            next = n;
            pmemobj_persist(pop, (void *)this, sizeof(node));
        }

        void SetNext(PMEMobjpool *pop, TOID(node) n){
            next = n;
            pmemobj_persist(pop, (void*)&next, sizeof(TOID(node)));
        }

        void insert(PMEMobjpool *pop, int val){
            
            TOID(node) here = this->next;
            if(TOID_IS_NULL(here)){
                TOID(node) new_node;
                POBJ_NEW(pop, &new_node, node, NULL, NULL);
                D_RW(new_node)->constructor(pop, TOID_NULL(node));
                D_RW(new_node)->value = val;

                this->SetNext(pop, new_node);
                pmemobj_persist(pop, (void*)this, sizeof(node));

                return;
            }

            if(D_RO(here)->value > val){
                TOID(node) new_node;
                POBJ_NEW(pop, &new_node, node, NULL, NULL);
                D_RW(new_node)->constructor(pop, this->next);
                D_RW(new_node)->value = val;
                pmemobj_persist(pop, (void*)&new_node, sizeof(node));

                this->SetNext(pop, new_node);
                pmemobj_persist(pop, (void*)this, sizeof(node));

                return;
            }

            while(1){
                TOID(node) nextp = D_RW(here)->next;
                if(TOID_IS_NULL(nextp)){
                    TOID(node) new_node;
                    POBJ_NEW(pop, &new_node, node, NULL, NULL);
                    D_RW(new_node)->constructor(pop, TOID_NULL(node));
                    D_RW(new_node)->value = val;

                    D_RW(here)->SetNext(pop, new_node);
                    pmemobj_persist(pop, (void*)D_RW(here), sizeof(node));

                    return;
                }

                if(D_RO(nextp)->value > val){
                    TOID(node) new_node;
                    POBJ_NEW(pop, &new_node, node, NULL, NULL);
                    D_RW(new_node)->constructor(pop, D_RW(here)->next);
                    D_RW(new_node)->value = val;
                    pmemobj_persist(pop, (void*)&new_node, sizeof(node));

                    D_RW(here)->SetNext(pop, new_node);
                    pmemobj_persist(pop, (void*)D_RW(here), sizeof(node));

                    return;
                }
                
                here = nextp;
            }
        }

        void printAll(){
            TOID(node) here = this->next;
            
            while(1){
                if(TOID_IS_NULL(here))
                    return;
                fprintf(stderr, "%d ", D_RO(here)->value);
                here = D_RW(here)->next;
            }
        }
};

class LIST{
    private:
        TOID(node) list_[SIZE];
        PMEMobjpool *pop_;
    public:
        LIST();
        ~LIST();
        void constructor(PMEMobjpool *pop){
            pop_ = pop;
            for(int i=0;i<SIZE;i++){
                if(!TOID_IS_NULL(list_[i]))
                    continue;
                POBJ_NEW(pop, &list_[i], node, NULL, NULL);
                D_RW(list_[i])->constructor(pop_, TOID_NULL(node));
                pmemobj_persist(pop_, (void *)D_RW(list_[i]), sizeof(node));
            }
        }
        void destructor(){
            pmemobj_close(pop_);
        }

        void insert(int a, int value){
            TOID(node) p = list_[a];
            D_RW(p)->insert(pop_, value);
        }

        void printAll(){
            for(int i=0;i<SIZE;i++){
                fprintf(stderr,"list(%d): ",i);
                TOID(node) p = list_[i];
                D_RW(p)->printAll();
                fprintf(stderr,"\n");
            }
        }
};

int main(){

    char* path = "/dev/shm/pmdk_test";

    PMEMobjpool *pop;
    if(access(path,0) !=0)
        pop = pmemobj_create(path, "PmemTest", 1<<30, 0666);
    else
        pop = pmemobj_open(path, "PmemTest");
    
    if(pop==NULL){
        perror(path);
        return 0;
    }

    TOID(LIST) list2 = POBJ_ROOT(pop, LIST);
    D_RW(list2)->constructor(pop);


    for(int i=0;i<SIZE;i++){
        for(int j=3;j>=0;j--){
            D_RW(list2)->insert(i,j);
        }
    }
    D_RW(list2)->printAll();

    fprintf(stderr,"\n");

    for(int i=0;i<SIZE;i++){
        for(int j=1+SIZE;j>0;j-=3){
            D_RW(list2)->insert(i,j);
        }
    }
    D_RW(list2)->printAll();

    D_RW(list2)->destructor();
    return 0;
}
