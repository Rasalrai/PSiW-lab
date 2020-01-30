#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include <time.h>
#include <string.h>

/*
 * argv:
 *  [1]: no of barbers      (barbN)
 *  [2]: no of customers    (custN)
 *  [3]: no of seats        (seatN)
 *  [4]: waiting room size  (waitN)
 *  [5]: seed for random generator  (*seed)
 * */


static struct sembuf buf;


void sem_raise(int sem_id, int sem_num) {
    buf.sem_num = sem_num;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    if (semop(sem_id, &buf, 1) == -1) {
        perror("-- Raising semaphore --");
        exit(1);
    }
}


void sem_lower(int sem_id, int sem_num) {
    buf.sem_num = sem_num;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    if (semop(sem_id, &buf, 1) == -1) {
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
        // wait for register access, change and give it (greedy)      # SYNC cust&barb <-> register content
        // LÖÖÖÖP
    }
}

void customer(){
    while(1){
        // make money
        // go to barber's
            // if the waiting room is full, break
        
        // else wait for barber
        // pay              # SYNC_1 cust <-> barb
        // get shaved
        // wait for change to receive
        // LÖÖÖÖP
    }
}

int main(int argc, char* argv[]) {
    unsigned int barbN = 7, custN = 10, waitN = 2, seatN = 5, rand_seed = time(NULL);
    unsigned int *seed = &rand_seed;
    char logging[1024];

    // create log file to follow the execution
    char log_filename[18];
    sprintf(log_filename, "log_%ld.log", time(NULL));
    int log_file = creat(log_filename, 0644);
    printf("Logging to file %s", log_filename);

    // choose seed and print it
    switch (argc) {
        case 6:
            *seed = strtol(argv[5], NULL, 10);
        case 5:
            waitN = strtol(argv[4], NULL, 10);
        case 4:
            seatN = strtol(argv[3], NULL, 10);
        case 3:
            custN = strtol(argv[2], NULL, 10);
        case 2:
            barbN = strtol(argv[1], NULL, 10);
            break;
    }

    sprintf(logging, "--- Starting execution: seed = %d\n%d barbers, %d customers, %d seats, %d in waiting room ---\n\n", *seed, barbN, custN, seatN, waitN);
    write(log_file, logging, strlen(logging));
    // srand with this fancy function for multithreaded

    // init barbers
    for(int i=0; i<barbN; i++)
        if(!fork())
            barber();
    
    // init customers
    for(int i=0; i<custN; i++)
        if(!fork())
            customer();

    wait(NULL);
}

/*
 may get stuck on:
    - everyone waits for change
 
 representations:
    # waiting room: msg Q
        - queue of clients (PIDs?)
        - guarded by a binary semaphore
    # shaving capacity: semafor o maks stopniu F (?)
    
    # cash register: shared memory - no of coins of every value
        - secured with a semaphore: for both paying and giving change
 
 
    - cash register:
        - payment: choose random value (with different probability?) until paid
        - change: greedy (first check if has enough money, then make a fail condition while giving change; remember to free the sem after a fail, so someone else can put money)
 
*/
