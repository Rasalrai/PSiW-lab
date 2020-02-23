// ZADANIE 2: Producenci wody

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

void h_producer(unsigned int seed) {


}

void o_producer(unsigned int seed) {

}

void water_assembler() {
    // wait for H mutex and O mutex
}

// ############################################################



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
