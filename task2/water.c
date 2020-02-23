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
    // lower H mutex and O mutex
}

// ############################################################



// ############################################################

int main() {
    // init stuff
    unsigned int global_seed = time(NULL), local_seed;
    
    
}


/*
    H Hydrogen producers, O Oxygen producers
    2 processes for couting incoming molecules (of either type)
    2 processes for managing mutexes (?)
    water assembler: locks H and O counters, decreases them and outputs a water molecule
*/
