#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>

#include <time.h>
#include <string.h>

#define NEW_CUSTOMER 1

/*
 * argv:
 *  [1]: no of barbers      (barbN)
 *  [2]: no of customers    (custN)
 *  [3]: no of seats        (seatN)
 *  [4]: waiting room size  (waitN)
 *  [5]: seed for random generator  (*seed)
 * */

struct msgbuf {
    long mtype;
    int mdata[4];   // coins owned by the customer and their PID
};

struct sembuf sbuf;
int coinN = 3, coin_value[3] = {1, 2, 5};
unsigned int *seed, barbN, custN, waitN, seatN, waiting_room, waiting_door, cash_register, cash_access, styling_chairs;


// ############################################################

void payday(int* wallet);

void sem_raise(int sem_id, int sem_num) {
    sbuf.sem_num = sem_num;
    sbuf.sem_op = 1;
    sbuf.sem_flg = 0;
    if (semop(sem_id, &sbuf, 1) == -1) {
        perror("-- Raising semaphore --");
        exit(1);
    }
}

void sem_lower(int sem_id, int sem_num) {
    sbuf.sem_num = sem_num;
    sbuf.sem_op = -1;
    sbuf.sem_flg = 0;
    if (semop(sem_id, &sbuf, 1) == -1) {
        perror("-- Lowering semaphore --");
        exit(1);
    }
}

// ############################################################
// printf("%d:\t\n", getpid());

void barber(){
    printf("%d:\t\t### Init as barber ###\n", getpid());
    struct msgbuf buf;

    while(1){
        // wait for a customer to arrive - msgrcv
        msgrcv(waiting_room, &buf, 4*sizeof(int), NEW_CUSTOMER, 0);
        printf("%d[B]:\tStarting with customer no %d\n", getpid(), buf.mdata[3]);
        // TODO
        // wait for a free seat
        sem_lower(styling_chairs, 0);
        // tell the price (alt.: put money in the register)     # SYNC_1 cust <-> barb
        // shave
        // clean the seat
        // wait for register access, change and give it (greedy)      # SYNC cust&barb <-> register content
        // LÖÖÖÖP
    }
}

// ############################################################

void customer(){
    printf("%d:\t\t### Init as customer ###\n", getpid());
    struct msqid_ds q_info;     // info about queue (need its length)
    struct msgbuf wait_msg;
    int tmp;
    wait_msg.mdata[3] = getpid();
    wait_msg.mtype = NEW_CUSTOMER;
    while(1){
        // make money
        tmp = rand_r(seed)%1000 + 50;
        printf("%d[C]:\tWork for %d us\n", getpid(), tmp);
        usleep(tmp);
        // collect your wallet
        payday(wait_msg.mdata);

        // go to barber and see if there's space for you
        sem_lower(waiting_door, 0);
        msgctl(waiting_room, IPC_STAT, &q_info);
        printf("%d:\tGoing to the barber's\n", getpid());

        if(q_info.msg_qnum < waitN) {
            
            // wait for barber     msgsnd
            msgsnd(waiting_room, &wait_msg, 4*sizeof(int), 0);
            sem_raise(waiting_door, 0);     // not sooner, so that noone else can enter before I do
            printf("%d[C]:\tEntered the barber's", getpid());
            
            // wait, pay              # SYNC_1 cust <-> barb
            // get shaved
            // wait for change to receive (msg)
        }
        sem_raise(waiting_door, 0);
    }
}

// ############################################################

int main(int argc, char* argv[]) {
    barbN = 7, custN = 12, waitN = 4, seatN = 5;
    unsigned int rand_seed = time(NULL);
    seed = &rand_seed;
    char logging[1024];

    // TODO consider just writing to stdout instead

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
        case 1:
            break;
    }

    // init all structures used for synchronization
    // waiting room -> msg q
    waiting_room = msgget(IPC_PRIVATE, 0640);
    waiting_door = semget(IPC_PRIVATE, 1, 0640);       // to make sure that 2 customers don't enter at the same time

    // cash register contents and semaphore
    cash_register = shmget(IPC_PRIVATE, coinN*sizeof(int), 0640);
    cash_access = semget(IPC_PRIVATE, 1, 0640);

    // styling chairs: semaphore, because it doesn't matter which chair exactly you use
    styling_chairs = semget(IPC_PRIVATE, seatN, 0640);
    semctl(styling_chairs, 0, SETVAL, seatN);


    printf("--- Starting execution: seed = %d ---\n--- %d barbers, %d customers, %d seats, %d in waiting room ---\n\n", *seed, barbN, custN, seatN, waitN);

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
    sem_raise(cash_reg, 0);
    printf("%d[B]\tput money into cash register: %d*1, %d*2, %d*5\n", getpid(), coins[0], coins[1], coins[2]);
    for (int i = 0; i < 3; i++)
        cash_reg[i] += coins[i];
    //close the register
    sem_lower(cash_reg, 0);
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

        return 1;   // successful
    }

    // close the register

    return 0;   // not finished
}

void payday(int* wallet) {
    // we assume that the customers spend all their money before payday :shrug:
                    // or that they only spend money getting shaved?
    // may need to adjust the money
    wallet[0] = rand_r(seed)%3;
    wallet[1] = rand_r(seed)%3;
    wallet[2] = rand_r(seed)%5 + 5;
}


/*
 may get stuck on:
    - everyone waits for change
 
 representations:
    # waiting room: msg Q
        - clients identified by PIDs?
        - guarded by a binary semaphore
    # shaving capacity: semafor o maks stopniu F (?)
    
    # cash register: shared memory - no of coins of every value
        - secured with a semaphore: for both paying and giving change
 
 
    - cash register:
        - payment: choose random value (with different probability?) until paid
        - change: greedy (first check if has enough money, then make a fail condition while giving change; remember to free the sem after a fail, so someone else can put money)
 
*/
