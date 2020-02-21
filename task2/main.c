// ZADANIE 2: Swiety Mikolaj

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

// ############################################################
// printf("%d[]:\t\n", getpid());

void santa_claus() {
    // define signal handlers
}

void reindeer(unsigned int seed) {
    int sleep_time;
    while(1) {
        // wait
        sleep_time = rand_r(&seed)%300+400;
        printf("%d[R]:\tSleep for %d\n", getpid(), sleep_time);
        usleep(sleep_time);

        // wait for Santa
        // returned from Santa's
    }
}

void elf(unsigned int seed) {
    int sleep_time;
    while(1) {
        // wait
        sleep_time = rand_r(&seed)%300+600;
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

    // create Santa thread

    // create handler threads

    // create reindeers and elves
    for(int i=0;i<reindeersN;i++) {
        local_seed = rand_r(&global_seed);
        if(!fork())
            pthread_create(NULL, NULL, reindeer, local_seed);
    }

    for(int i=0;i<elvesN;i++) {
        local_seed = rand_r(&global_seed);
        if(!fork())
            pthread_create(NULL, NULL, elf, local_seed);
    }
        

}


/*
    9 reindeers
    3/10 elves

    two additional threads for handling elves and reindeers

    the handlers would send signals to Santa Claus, who'd have custom reactions to them

*/