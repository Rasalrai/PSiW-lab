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

// TODO error handling (e.g. for sending/receiving messages, creating stuff)

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
int coinN = 3, coin_value[3] = {1, 2, 5};       // available coins of value 1, 2 and 5
unsigned int barbN, custN, waitN, seatN, waiting_room, waiting_door, cash_reg_id, cash_access, styling_chairs, finished_q, *glob_seed;

// ############################################################

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

void toss_a_coin(const int* coins, int* cash, int cash_access) {
    /* customer tosses coins to their barber */
    // safely open the register
    sem_lower(cash_access, 0);
    printf("%d[B]:\tPut money into cash register: %d*1, %d*2, %d*5\n", getpid(), coins[0], coins[1], coins[2]);

    for (int i = 0; i < 3; i++)
        cash[i] += coins[i];
    //close the register
    sem_raise(cash_access, 0);
}

int give_change(int price, int* cash_reg, int* wallet, int cash_access) {
    // we assume that the customer can't run away before the transaction is finalized, so we "keep" their money in their wallet all the time, even after adding it to register
    int change[3] = {0, 0, 0}, paid = 0, diff;
    for (int i=0;i<3;i++)
        paid+=wallet[i]*coin_value[i];
    diff = paid - price;
    printf("%d[B]:\tTry to give change: %d\n", getpid(), diff);
    /*
     * wait for register access
     * calculate and try to give change
     * if succeeded, let the customer go and remember money
     * close the cash register so that others can use it
     * return 1 if succeeded, 0 if not
     * */
    // open safely
    sem_lower(cash_access, 0);
    for (int i = 2; i >= 0; i--) {
        while (change[i] <= cash_reg[i] && diff && diff >= coin_value[i]) {     // while cash is available and more needs to be given out
            change[i]++;
            diff -= coin_value[i];
        }
    }
    if (diff < 0) {
        printf("Problem calculating change");
        sem_raise(cash_access, 0);
        exit(1);
    }
    if (!diff) {
        // take change from the register and put it in the wallet
        for (int i = 0; i < 3; i++) {
            cash_reg[i] -= change[i];
            wallet[i] = change[i];
        }
            
        // close the register
        sem_raise(cash_access, 0);
        printf("%d[B]:\tSuccess giving change.\n", getpid());

        return 1;   // successful
    }
    // else: not successful
    printf("%d[B]:\tGiving change not possible (available: %d, %d, %d), need to retry.\n", getpid(), cash_reg[0], cash_reg[1], cash_reg[2]);
    sem_raise(cash_access, 0);
    return 0;   // not finished
}

void payday(int* wallet, int *seed) {
    // the longer the customers wait (which may mean that giving change takes time), the more money (including smaller coins) they have
    // may need to adjust the money
    wallet[0] = rand_r(seed)%3;
    wallet[1] = rand_r(seed)%3;
    wallet[2] = rand_r(seed)%3 + 6;     // assume that the rest of this is spent somewhere else
}


// ############################################################
// printf("%d[]:\t\n", getpid());

void barber(int seed){
    printf("%d:\t\t### Init as barber, seed: %d ###\n", getpid(), seed);
    struct msgbuf buf;
    int cost, *cash_register = (int*)shmat(cash_reg_id, NULL, 0), change_given;
    long cust_id;

    while(1){
        change_given = 0;
        // wait for a customer to arrive - msgrcv
        msgrcv(waiting_room, &buf, 4*sizeof(int), NEW_CUSTOMER, 0);
        cost = rand_r(&seed)%25+4;
        cust_id = buf.mdata[3];
        printf("%d[B]:\tStarting a(n) $%d service for customer %ld\n", getpid(), cost, cust_id);
        // wait for a free chair
        sem_lower(styling_chairs, 0);
        
        // take money (alt.: put money in the register)     # SYNC_1 cust <-> barb
        toss_a_coin(buf.mdata, cash_register, cash_access);
        // todo: send info that more cash is now in register (if someone's been waiting for change, they should check now)

        // shave
        printf("%d[B]:\tWorking.\n", getpid());
        usleep(rand_r(&seed)%100+10);

        // clean the chair
        printf("%d[B]:\tFinished.\n", getpid());
        // usleep(1000);    // for slowing the execution down
        sem_raise(styling_chairs, 0);
        // wait for register access, change and give it (greedy)
        change_given = give_change(cost, cash_register, buf.mdata, cash_access);
        // to prevent active wait, only re-check if cash was added to the register
        while (change_given) {
            // wait
            // check
            change_given = give_change(cost, cash_register, buf.mdata, cash_access);
            // propagate
        }
        // let the customer go
        msgsnd(finished_q, &cust_id, 0, 0);
    }
}

// ############################################################

void customer(int seed){
    printf("%d:\t\t### Init as customer, seed: %d ###\n", getpid(), seed);
    struct msqid_ds q_info;     // info about queue (need its length)
    struct msgbuf wait_msg;
    int tmp;
    // wallet: wait_msg.mdata[0:3]
    for(int i=0;i<3;i++) wait_msg.mdata[i] = 0;
    wait_msg.mdata[3] = getpid();
    wait_msg.mtype = NEW_CUSTOMER;
    while(1){
        // make money
        tmp = rand_r(&seed)%1000 + 50;
        usleep(tmp);
        // collect your wallet
        payday(wait_msg.mdata, &seed);
        printf("%d[C]:\tWorked for %d us; wallet: %d, %d, %d\n", getpid(), tmp, wait_msg.mdata[0], wait_msg.mdata[1], wait_msg.mdata[2]);

        // go to barber and see if there's space for you
        sem_lower(waiting_door, 0);
        msgctl(waiting_room, IPC_STAT, &q_info);
        printf("%d[C]:\tGoing to the barber's\n", getpid());

        if(q_info.msg_qnum < waitN) {
            
            // wait for barber     msgsnd
            msgsnd(waiting_room, &wait_msg, 4*sizeof(int), 0);  // waiting for barber "with wallet out"
            sem_raise(waiting_door, 0);     // not sooner, so that noone else can enter before I do
            printf("%d[C]:\tEntered the barber's\n", getpid());            

            // wait for service and change
            // todo: receive updated contents of the wallet
            msgrcv(finished_q, NULL, 0, (long)getpid(), 0);
        }
        else sem_raise(waiting_door, 0);
        printf("%d[C]:\tCan't enter, back to work\n", getpid());
    }
}

// ############################################################

int main(int argc, char* argv[]) {
    barbN = 7, custN = 12, waitN = 4, seatN = 5;
    // global random, used for seeding all processes' random generators
    unsigned int rand_seed = time(NULL);
    glob_seed = &rand_seed;

    switch (argc) {
        default:
        case 6:
            *glob_seed = strtol(argv[5], NULL, 10);
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
    semctl(waiting_door, 0, SETVAL, 1);

    // cash register contents and semaphore
    cash_reg_id = shmget(IPC_PRIVATE, coinN*sizeof(int), 0640);
    cash_access = semget(IPC_PRIVATE, 1, 0640);
    semctl(cash_access, 0, SETVAL, 1);
    // put some money to start with
    int *cash_register = (int*)shmat(cash_reg_id, NULL, 0);
    cash_register[0] = cash_register[1] = cash_register[2] = 0;

    // styling chairs: semaphore, because it doesn't matter which chair exactly you use
    styling_chairs = semget(IPC_PRIVATE, seatN, 0640);
    semctl(styling_chairs, 0, SETVAL, (int)seatN);

    // msgq for telling customers that they can leave
    finished_q = msgget(IPC_PRIVATE, 0640);

    printf("--- Starting execution: seed = %d ---\n--- %d barbers, %d customers, %d seats, %d in waiting room ---\n\n", *glob_seed, barbN, custN, seatN, waitN);

    // init barbers
    for(int i=0; i<barbN; i++) {
        if(!fork())
            barber(rand_r(glob_seed));
    }
    
    // init customers
    for(int i=0; i<custN; i++) {
        if(!fork())
            customer(rand_r(glob_seed));
    }

    wait(NULL);
}

/*
 * may get stuck on:
 *  - everyone waits for change
 * representations:
 * # waiting room: msg Q
 *      - clients identified by PIDs?
 * # chairs: a semaphore with max=chairN 
 * # cash register: shared memory - no of coins of every value
 *      - secured with a semaphore: for both paying and giving change
*/
