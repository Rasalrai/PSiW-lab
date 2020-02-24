// ZADANIE 2: Producenci wody

/*  TODO
    error handling on e.g. thread creation
    make sure that the handlers and the master don't miss any signal
*/

/*
    argv
    [1] no of Hydrogen producers (>=2)
    [2] no of Oxygen producers (>=1)
    Incorrect values will cause the program to fail.
    If values are not provided, they will be chosen at random (each max 10)
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
pthread_cond_t *h_ready, *o_ready, *atom, *h_unlock, *o_unlock;
pthread_mutex_t *h_mut, *o_mut, *counter_mut;
int *h_count, *o_count;

// ############################################################
// printf("%d[]:\t\n", id);

void h_producer(int id, unsigned int seed) {
    while(1) {
        // create an atom
        usleep(rand_r(&seed)%1000+100);
        printf("%d:\tH atom created", id);

        // atom mutex
        pthread_mutex_lock(h_mut);
        // deliver it
        pthread_cond_signal(h_ready);
        // wait for assembler
    }
}

void o_producer(int id, unsigned int seed) {
    // pass
}

void h_hnd(int id) {
    while(1) {
        // h mutex locked
        pthread_cond_wait(h_ready, NULL);
        pthread_mutex_lock(counter_mut);
        *h_count++;
        pthread_mutex_unlock(counter_mut);
        pthread_cond_signal(atom);
        pthread_mutex_unlock(h_mut);
    }
    // atom mutex down
    // receive atoms


    // counter mutex down and update counter
    // send info to master
}

void o_hnd(int id) {
    // atom mutex down
    // receive atoms
    // counter mutex down and update counter
    // send info to master
}

// ############################################################

void water_assembler(int id) {
    while(1) {
        // wait for info from slaves/handlers
        pthread_cond_wait(atom, NULL);
        pthread_mutex_lock(counter_mut);
        if ( *h_count >= 2 && o_count >= 1) {
            *h_count -= 2;
            *o_count -= 1;
            printf("###\t H2O MOLECULE PRODUCED\t###");
            // unlock producers
        }
        pthread_mutex_unlock(counter_mut);
    }
}

// ############################################################

int main(int argc, char* argv[]) {
    // init stuff
    unsigned int global_seed = time(NULL), local_seed;
    int h_prod_n, o_prod_n;
    // pthread_cond_t *h_ready, *o_ready, *atom;
    // pthread_mutex_t *h_mut, *o_mut, *counter_mut;
    *h_ready = *o_ready = *atom = *h_unlock = *o_unlock = PTHREAD_COND_INITIALIZER;
    *h_mut = *o_mut = *counter_mut = PTHREAD_MUTEX_INITIALIZER;

    pthread_mutex_unlock(h_ready);
    pthread_mutex_unlock(o_ready);
    pthread_mutex_unlock(atom);

    *h_count = *o_count = 0;

    // args
    if(argv > 1) {
        h_prod_n = strtol(argv[1], NULL, 10);
        o_prod_n = strtol(argv[2], NULL, 10);

        if(h_prod_n < 2 || o_prod_n < 1) {
            perror("Incorrect argument value(s).");
            exit(1);
        }
    } else {
        h_prod_n = rand_r(global_seed)%9+2;
        o_prod_n = rand_r(global_seed)%10+1;
    }

    pthread_t h_prod_th[h_prod_n], o_prod_th[o_prod_n];
    pthread_t water_th, h_hnd_th, o_hnd_th;

    // create threads
    int id = 0;
    // master - water assembler
    if(pthread_create(&water_th, NULL, water_assembler, id)) {
        perror("Create thread");
        exit(1);
    }
    id++;

    // H and O handlers
    if(pthread_create(&h_hnd_th, NULL, h_hnd, id)) {
        perror("Create thread");
        exit(1);
    }
    id++;
    if(pthread_create(&o_hnd_th, NULL, o_hnd, id)) {
        perror("Create thread");
        exit(1);
    }
    id++;

    // atom producers
    for(int i = 0; i < h_prod_n; i++) {
        if(pthread_create(&h_prod_th[i], NULL, h_producer, id)) {
            perror("Create thread");
            exit(1);
        }
        id++;
    }
    for(int i = 0; i < o_prod_n; i++) {
        if(pthread_create(&o_prod_th[i], NULL, o_producer, id)) {
            perror("Create thread");
            exit(1);
        }
        id++;
    }
}


/*
    H Hydrogen producers, O Oxygen producers
    2 processes for couting incoming molecules (of either type)
    2 processes for managing mutexes (?)
    water assembler: locks H and O counters, decreases them and outputs a water molecule
*/
