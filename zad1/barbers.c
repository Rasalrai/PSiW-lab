// ZADANIE 1: Spiacy fryzjerzy-kasjerzy

/*
 * argv:
 *  [1]: no of barbers      (barbN)
 *  [2]: no of customers    (custN)
 *  [3]: no of seats        (seatN)
 *  [4]: waiting room size  (waitN)
 * */

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
#include <sched.h>

#define NEW_CUSTOMER 1

struct msgbuf {
    long mtype;
    int mdata[4];   // coins owned by the customer and their PID
};

struct sembuf sbuf;
int coin_value[3] = {1, 2, 5};       // available coins of value 1, 2 and 5
unsigned int barbN, custN, waitN, seatN, *glob_seed, coinN = 3;
int waiting_room, waiting_door, cash_reg_id, cash_access, styling_chairs, finished_q;

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

// ############################################################

void toss_a_coin(const int* coins, int* cash) {
    /* toss the coins to the cash register */
    // safely open the register
    sem_lower(cash_access, 0);
    printf("%d[B]:\tPut money into cash register: %d*1, %d*2, %d*5\n", getpid(), coins[0], coins[1], coins[2]);

    for (int i = 0; i < 3; i++)
        cash[i] += coins[i];
    //close the register
    sem_raise(cash_access, 0);
}

int give_change(int price, int* cash_reg, int* wallet) {
    // we assume that the customer can't run away before the transaction is finalized, so we "keep" their money in their wallet all the time, even after adding it to register
    int change[3] = {0, 0, 0}, paid = 0, diff;
    for (int i=0;i<3;i++)
        paid+=wallet[i]*coin_value[i];      // paid is value of wallet contents that was previously put into chash register
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
        while (change[i] < cash_reg[i] && diff && diff >= coin_value[i]) {     // while cash is available and more needs to be given out
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

void payday(int* wallet, unsigned int *seed) {
    // after payday, the customer decides how much money they should take to pay the barber with
    // may need to adjust the values
    wallet[0] = rand_r(seed)%4;
    wallet[1] = rand_r(seed)%4;
    wallet[2] = rand_r(seed)%3 + 6;
}

// ############################################################

void barber(unsigned int seed){
    printf("%d:\t\t### Init as barber, seed: %d ###\n", getpid(), seed);
    struct msgbuf buf;
    int cost, *cash_register = (int*)shmat(cash_reg_id, NULL, 0);
    if(cash_register == NULL) {
        perror("-- Attaching shared memory for cash register --");
        exit(1);
    }
    long cust_id;

    while(1){
        // wait for a customer to arrive - msgrcv
        msgrcv(waiting_room, &buf, 4*sizeof(int), NEW_CUSTOMER, 0);
        cost = rand_r(&seed)%25+5;
        cust_id = buf.mdata[3];

        printf("%d[B]:\tStarting a(n) $%d service for customer %ld\n", getpid(), cost, cust_id);
        // wait for a free chair
        sem_lower(styling_chairs, 0);

        // put money in the register
        toss_a_coin(buf.mdata, cash_register);

        // shave
        printf("%d[B]:\tWorking.\n", getpid());
        usleep(rand_r(&seed)%100+10);

        // "clean" the chair
        printf("%d[B]:\tCleaning the chair.\n", getpid());
        sem_raise(styling_chairs, 0);
        // wait for register access, change and give it (greedy)
        while (!give_change(cost, cash_register, buf.mdata))
            sched_yield();      // 
        // let the customer go
        buf.mtype = (long)cust_id;
        msgsnd(finished_q, &buf, 3*sizeof(int), 0);
        printf("%d[B]:\t### Finished service and giving change. ###\n", getpid());
    }
}

// ############################################################

void customer(unsigned int seed){
    printf("%d:\t\t### Init as customer, seed: %d ###\n", getpid(), seed);
    struct msqid_ds q_info;     // info about queue (need its length)
    struct msgbuf wait_msg;
    int worktime;
    // wallet: wait_msg.mdata[0:3]
    for(int i=0;i<3;i++) wait_msg.mdata[i] = 0;
    wait_msg.mdata[3] = getpid();
    wait_msg.mtype = NEW_CUSTOMER;
    while(1){
        // make money
        worktime = rand_r(&seed)%1000 + 50;
        usleep(worktime);
        // collect your wallet
        payday(wait_msg.mdata, &seed);
        printf("%d[C]:\tWorked for %d us; wallet: %d, %d, %d\n", getpid(), worktime, wait_msg.mdata[0], wait_msg.mdata[1], wait_msg.mdata[2]);

        // go to barber and see if there's space for you
        sem_lower(waiting_door, 0);
        msgctl(waiting_room, IPC_STAT, &q_info);
        printf("%d[C]:\tGoing to the barber's\n", getpid());

        if(q_info.msg_qnum < waitN) {

            // wait for a barber
            msgsnd(waiting_room, &wait_msg, 4*sizeof(int), 0);  // waiting for a barber with your wallet ready
            sem_raise(waiting_door, 0);     // not sooner, so that noone else can enter before I do
            printf("%d[C]:\tEntered the barber's\n", getpid());

            // wait for service and change
            msgrcv(finished_q, NULL, 3*sizeof(int), (long)getpid(), 0);
            printf("%d[C]:\t### Leaving the barber's ###\n", getpid());
        }
        else sem_raise(waiting_door, 0);
        printf("%d[C]:\tCan't enter, back to work\n", getpid());
    }
}

// ############################################################

int main(int argc, char* argv[]) {
    barbN = 7, custN = 12, waitN = 4, seatN = 5;
    // global random, used for seeding all processes' random generators
    unsigned int rand_seed = time(NULL), process_seed;  // to get randomized seeds for all processes, created in the main process
    glob_seed = &rand_seed;

    switch (argc) {
        default:
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
    if(waiting_room == -1) {
        perror("-- Creating waiting room message queue --");
        exit(1);
    }

    // to make sure that 2 customers don't enter at the same time
    waiting_door = semget(IPC_PRIVATE, 1, 0640);
    if(waiting_door == -1) {
        perror("-- Creating a waiting room door-semaphore --");
        exit(1);
    }
    if(semctl(waiting_door, 0, SETVAL, 1) == -1) {
        perror("-- Setting value of waiting room door-semaphore --");
        exit(1);
    }

    // cash register contents and semaphore
    cash_reg_id = shmget(IPC_PRIVATE, coinN*sizeof(int), 0640);
    if(cash_reg_id == -1) {
        perror("-- Creating shared memory for cash register --");
        exit(1);
    }

    int *cash_register = (int*)shmat(cash_reg_id, NULL, 0);
    if(cash_register == NULL) {
        perror("-- Attaching shared memory for cash register --");
        exit(1);
    }
    cash_register[0] = cash_register[1] = cash_register[2] = 0;

    // guarding cash register access
    cash_access = semget(IPC_PRIVATE, 1, 0640);
    if(cash_access == -1) {
        perror("-- Creating a semaphore for cash register access --");
        exit(1);
    }
    if(semctl(cash_access, 0, SETVAL, 1) == -1) {
        perror("-- Setting value of semaphore for cash register access --");
        exit(1);
    }

    // styling chairs: semaphore, because it doesn't matter which chair exactly you use
    styling_chairs = semget(IPC_PRIVATE, (int)seatN, 0640);
    if(styling_chairs == -1) {
        perror("-- Creating a semaphore counting styling chairs --");
        exit(1);
    }
    if(semctl(styling_chairs, 0, SETVAL, (int)seatN) == -1) {
        perror("-- Setting value of semaphore counting styling chairs --");
        exit(1);
    }

    // msgq for telling customers that they can leave
    finished_q = msgget(IPC_PRIVATE, 0640);
    if(finished_q == -1) {
        perror("-- Creating queue for 'releasing' customers --");
        exit(1);
    }

    printf("--- Starting execution: seed = %d ---\n--- %d barbers, %d customers, %d seats, %d in waiting room ---\n\n", *glob_seed, barbN, custN, seatN, waitN);

    // init barbers
    for(int i=0; i<barbN; i++) {
        process_seed = rand_r(glob_seed);
        if(!fork())
            barber(process_seed);
    }

    // init customers
    for(int i=0; i<custN; i++) {
        process_seed = rand_r(glob_seed);
        if(!fork())
            customer(process_seed);
    }

    wait(NULL);
}
