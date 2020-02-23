// ZADANIE 2: Swiety Mikolaj

/*  TODO
    error handling on e.g. thread creation
    
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#include <time.h>
#include <string.h>

static int reindeersN = 9, elvesN = 10, elves_min = 3;
pthread_t* santa_t, *elf_ht, *reindeer_ht, *elf_t, *reindeer_t;      // last two will be arrays
pthread_cond_t* elf_cond, *reindeer_cond;         // need to have addresses for communication

// ############################################################
// printf("%d[]:\t\n", getpid());

void reindeer_hnd() {
    while(1) {
        // wait for 9 reindeers and send a signal to Santa
        for(int i = 0; i < reindeersN; i++)
            pthread_cond_wait(reindeer_cond);
        
        // request Santa
    }
}

void elf_hnd() {
    // wait for 3 elves and send a signal to Santa
}

// ############################################################

void santa_claus() {
    // create reindeer/elf handlers for you

    // define signal handlers
    // including behaviour if a signal comes while another one is being handled
}

// ############################################################

void reindeer(unsigned int seed) {
    int sleep_time;
    while(1) {
        // wait
        sleep_time = rand_r(&seed)%1000+400;
        printf("%d[R]:\tSleep for %d\n", getpid(), sleep_time);
        usleep(sleep_time);

        // send signal to handler and wait for Santa
        pthread_cond_signal();

        // returned from Santa's
    }
}

void elf(unsigned int seed) {
    int sleep_time;
    while(1) {
        // wait
        sleep_time = rand_r(&seed)%1300+600;
        printf("%d[E]:\tSleep for %d\n", getpid(), sleep_time);
        usleep(sleep_time);

        // wait for Santa
        // returned from Santa's
    }
}

// ############################################################

int main() {
    // init stuff
    unsigned int global_seed = time(NULL), local_seed;
    *elf_cond = *reindeer_cond = PTHREAD_COND_INITIALIZER;

    // create Santa thread
    local_seed = rand_r(&global_seed);
    pthread_create(santa_t, NULL, santa_claus, local_seed);

    // create reindeers and elves
    for(int i=0;i<reindeersN;i++) {
        local_seed = rand_r(&global_seed);
        pthread_create(reindeer_t + i, NULL, reindeer, local_seed);
    }

    for(int i=0;i<elvesN;i++) {
        local_seed = rand_r(&global_seed);
        pthread_create(elf_t + i, NULL, elf, local_seed);
    }
}


/*
    9 reindeers
    3/10 elves

    two additional threads for handling elves and reindeers

    the handlers would send signals to Santa Claus, who'd have custom reactions to them

*/
