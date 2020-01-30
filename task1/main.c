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

int coin_value[3] = {1, 2, 5};


void sem_lower(int sem_id, int sem_num) {
    buf.sem_num = sem_num;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    if (semop(sem_id, &buf, 1) == -1) {
        perror("-- Lowering semaphore --");
        exit(1);
    }
}

// ############################################################

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

// ############################################################

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

// ############################################################

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
        default:
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

// ############################################################
// helper functions

void toss_a_coin(const int* coins, int* cash_reg) {
    /* customer tosses coins to their barber */
    // TODO WIP
    // safely open the register
    for (int i = 0; i < 3; i++)
        cash_reg[i] += coins[i];
    //close the register
}

int give_change(int paid, int price, int* cash_reg) {       // TODO
    /* available coins of value 1, 2 and 5 */
    // TODO WIP
    int change[3] = {0, 0, 0}, diff = paid - price;
    /*
     * wait for register access
     * calculate and try to give change
     * if succeeded, let the customer go and remember money
     * close the cash register so that others can use it
     * return 1 if succeeded, 0 if not
     * */
    // open safely
    for (int i = 2; i <= 0; i--) {
        while (change[i] <= cash_reg[i] && diff && diff >= coin_value[i]) {     // while there is still cash available
            change[i]++;
            diff -= coin_value[i];
        }
    }
    if (diff < 0) {
        printf("Problem calculating change");
        exit(1);
    }
    if (!diff) {
        // let the customer go
        // take the change
        for (int i = 0; i < 3; i++)
            cash_reg[i] -= change[i];
        // close the register

        return 1;
    }

    // close the register

    return 0;
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
