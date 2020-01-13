#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>


static  struct  sembuf  buf;


void sem_raise(int sem_id, int sem_num) {
    buf.sem_num = sem_num;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    if (sem_op(sem_id, &buf, 1) == -1) {
        perror("-- Raising semaphore --");
        exit(1);
    }
}


void sem_lower(int sem_id, int sem_num) {
    buf.sem_num = sem_num;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    if (sem_op(sem_id, &buf, 1) == -1) {
        perror("-- Lowering semaphore --");
        exit(1);
    }
}


// F: No of barbers
// number of customers (as many as you spawn?)

// P: waiting room capacity
// N: shaving capacity
// contents of the shared cash register


void barber(){
    while(1){
        // wait for a customer to arrive
        // wait for a free seat
        // tell the price (alt.: put money in the register)     # SYNC_1 cust <-> barb
        // shave
        // clean the seat
        // wait for change and give it (greedy)      # SYNC cust&barb <-> register content
        // LÖÖÖÖP
    }
}

void customer(){
    while(1){
        // make money
        // go to barber's
            // if the waiting room is full, LOOP
        
        // else wait for barber
        // pay              # SYNC_1 cust <-> barb
        // get shaved
        // wait for change to receive
        // LÖÖÖÖP
    }
}

int main() {
    // choose seed and print it
    // srand with this fancy function for multithreaded
    
    // init barbers
    
    // init customers
}

/*
 may get stuck on:
    - everyone waits for change
 
 representations:
    # waiting room: msg Q
        -- guarded by a binary semaphore
    # shaving capacity: semafor o maks stopniu F (?)
    
    # cash register: shared memory - no of coins of every value
        - secured with a semaphore: for both paying and giving change
 
 
    - cash register:
        - payment: choose random value (with different probability?) until paid
        - change: greedy (first check if has enough money, then make a fail condition while giving change; remember to free the sem after a fail, so someone else can put money)
 
*/
