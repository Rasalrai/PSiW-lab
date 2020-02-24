// ZADANIE 2: Producenci wody

/*  TODO
    error handling on e.g. thread creation
    make sure that the handlers and the master don't miss any signal
    more logging
    make arrays/structs for threads' args
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

/*
struct thread_t {
    pthread_t pth_id;
    void *func;
    int id;
    unsigned int seed;
};
*/

struct args_t {
    int id;
    unsigned int seed;
};

pthread_cond_t *h_ready, *o_ready, *atom, *h_unlock, *o_unlock;
pthread_mutex_t *h_mut, *o_mut, *counter_mut;
pthread_cond_t h_continue = PTHREAD_COND_INITIALIZER, o_continue = PTHREAD_COND_INITIALIZER;
int *h_count, *o_count;

// ############################################################
// printf("%d[]:\t\n", id);

void *h_producer(void *arguments) {
    struct args_t args = *((struct args_t*)arguments);
    while(1) {
        // create an atom
        usleep(rand_r(&(args.seed))%1000+100);
        printf("%d:\tH atom created\n", args.id);
        // atom mutex
        pthread_mutex_lock(h_mut);
        // deliver it
        pthread_cond_signal(h_ready);
        // wait for assembler
        pthread_cond_wait(&h_continue, NULL);
    }
}

void *o_producer(void *arguments) {
    struct args_t args = *((struct args_t*)arguments);
    while(1) {

    }
    // pass
}

void *h_hnd(void *arguments) {
    struct args_t args = *((struct args_t*)arguments);
    while(1) {
        // h mutex locked
        pthread_mutex_lock(counter_mut);
        pthread_cond_wait(h_ready, counter_mut);
        printf("%d:\tH atom \n", args.id);
        (*h_count)++;
        pthread_mutex_unlock(counter_mut);
        pthread_cond_signal(atom);
        pthread_mutex_unlock(h_mut);
    }
    // atom mutex down
    // receive atoms


    // counter mutex down and update counter
    // send info to master
}

void *o_hnd(void *arguments) {
    struct args_t args = *((struct args_t*)arguments);
    while(1) {
        // atom mutex down
        // receive atoms
        // counter mutex down and update counter
        // send info to master
    }
}

// ############################################################

void *water_assembler(void *arguments) {
    struct args_t args = *((struct args_t*)arguments);
    while(1) {
        // wait for info from slaves/handlers
        pthread_mutex_lock(counter_mut);
        pthread_cond_wait(atom, counter_mut);

        if (*h_count >= 2 && *o_count >= 1) {
            *h_count -= 2;
            *o_count -= 1;
            printf("###\t H2O MOLECULE PRODUCED\t###");
            // unlock producers
            pthread_cond_signal(&h_continue);
            pthread_cond_signal(&h_continue);
            pthread_cond_signal(&o_continue);
        }
        pthread_mutex_unlock(counter_mut);
    }
}

// ############################################################

int main(int argc, char* argv[]) {
    // init stuff
    unsigned int global_seed = time(NULL);
    int h_prod_n, o_prod_n;
    
    pthread_cond_t h_ready_v = PTHREAD_COND_INITIALIZER;
    pthread_cond_t o_ready_v = PTHREAD_COND_INITIALIZER;
    pthread_cond_t atom_v = PTHREAD_COND_INITIALIZER;
    pthread_cond_t h_unlock_v = PTHREAD_COND_INITIALIZER;
    pthread_cond_t o_unlock_v = PTHREAD_COND_INITIALIZER;
    h_ready = &h_ready_v;
    o_ready = &o_ready_v;
    atom = &atom_v;
    h_unlock = &h_unlock_v;
    o_unlock = &o_unlock_v;

    pthread_mutex_t h_mut_v = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t o_mut_v = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t counter_mut_v = PTHREAD_MUTEX_INITIALIZER;
    h_mut = &h_mut_v;
    o_mut = &o_mut_v;
    counter_mut = &counter_mut_v;

    // make sure that mutexes are unlocked
    pthread_mutex_unlock(h_mut);
    pthread_mutex_unlock(o_mut);
    pthread_mutex_unlock(counter_mut);

    int h_count_v = 0, o_count_v = 2;       // TODO for testing
    h_count = &h_count_v;
    o_count = &o_count_v;

    // args
    if(argc > 1) {
        h_prod_n = strtol(argv[1], NULL, 10);
        o_prod_n = strtol(argv[2], NULL, 10);

        if(h_prod_n < 2 || o_prod_n < 1) {
            perror("Incorrect argument value(s).");
            exit(1);
        }
    } else {
        h_prod_n = rand_r(&global_seed)%9+2;
        o_prod_n = rand_r(&global_seed)%10+1;
    }
    printf("Starting execution: %d hydrogen producers, %d oxygen producers.\n", h_prod_n, o_prod_n);

    pthread_t h_prod_th[h_prod_n], o_prod_th[o_prod_n];
    pthread_t water_th, h_hnd_th, o_hnd_th;

    // create threads
    // thread arguments
    int threads_n = h_prod_n + o_prod_n + 3;
    struct args_t args_tab[threads_n];
    for (int i = 0; i < threads_n; ++i) {
        args_tab[i].id = i;
        args_tab[i].seed = rand_r(&global_seed);
    }
 
    int id = 0;
    // master - water assembler
    printf("ID %d:\tWater assembler\n", id);
    if(pthread_create(&water_th, NULL, water_assembler, args_tab+id)) {
        perror("Create thread");
        exit(1);
    }
    id++;

    // H and O handlers
    printf("ID %d:\tHydrogen handler\n", id);
    if(pthread_create(&h_hnd_th, NULL, h_hnd, args_tab+id)) {
        perror("Create thread");
        exit(1);
    }
    id++;
    printf("ID %d:\tOxygen handler\n", id);
    if(pthread_create(&o_hnd_th, NULL, o_hnd, args_tab+id)) {
        perror("Create thread");
        exit(1);
    }
    id++;

    // atom producers
    printf("ID %d - %d:\tHydrogen producers\n", id, id+h_prod_n-1);
    for(int i = 0; i < h_prod_n; i++) {
        if(pthread_create(&h_prod_th[i], NULL, h_producer, args_tab+id)) {
            perror("Create thread");
            exit(1);
        }
        id++;
    }
    printf("ID %d - %d:\tOxygen producers\n", id, id+h_prod_n-1);
    for(int i = 0; i < o_prod_n; i++) {
        if(pthread_create(&o_prod_th[i], NULL, o_producer, args_tab+id)) {
            perror("Create thread");
            exit(1);
        }
        id++;
    }

    for(int i = 0; i < h_prod_n; i++)
        pthread_join(h_prod_th[i], NULL);

    for(int i = 0; i < o_prod_n; i++)
        pthread_join(o_prod_th[i], NULL);

    return 0;
}

/*
    H Hydrogen producers, O Oxygen producers
    2 processes for couting incoming molecules (of either type)
    water assembler: locks H and O counters, decreases them and outputs a water molecule
*/
