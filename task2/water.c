// ZADANIE 2: Producenci wody

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

struct args_t {
    int id;
    unsigned int seed;
};

pthread_cond_t h_wait = PTHREAD_COND_INITIALIZER, o_wait = PTHREAD_COND_INITIALIZER;
pthread_mutex_t create_water = PTHREAD_MUTEX_INITIALIZER;
int h_count = 0, o_count = 0, h_used = 0, o_used = 0;

// ############################################################

void *h_producer(void *arguments) {
    struct args_t args = *((struct args_t*)arguments);
    while(1) {
        // create an atom
        usleep(rand_r(&(args.seed))%1000+100);
        printf("%d:\tH atom created\n", args.id);

        // critical section
        pthread_mutex_lock(&create_water);
        h_count++;
        while(!h_used) {
            if(h_count >=2 && o_count >= 1) {
                // can make water
                h_count -= 2;
                o_count--;
                h_used += 2;
                o_used++;
                printf("%d\t###\tH2O MOLECULE PRODUCED\t###\n", args.id);
                pthread_cond_signal(&h_wait);
                pthread_cond_signal(&o_wait);
            } else {
                // wait for someone else to make water
                pthread_cond_wait(&h_wait, &create_water);
            }
        }
        // every atom used is substracted
        h_used--;

        pthread_mutex_unlock(&create_water);
        // end of critical section
    }
}

// ------------------------------------------------------

void *o_producer(void *arguments) {
    struct args_t args = *((struct args_t*)arguments);
    while(1) {
        // create an atom
        usleep(rand_r(&(args.seed))%1000+100);
        printf("%d:\tO atom created\n", args.id);

        // critical section
        pthread_mutex_lock(&create_water);
        o_count++;
        while(!o_used) {       // if someone has already put a molecule together (and put you there), skip this
            if(h_count >=2 && o_count >= 1) {
                // can make water
                h_count -= 2;
                o_count--;
                h_used += 2;
                o_used++;
                printf("%d\t###\tH2O MOLECULE PRODUCED\t###\n", args.id);
                // wake up producers waiting in the else
                pthread_cond_signal(&h_wait);
                pthread_cond_signal(&h_wait);
            } else {
                // wait for someone else to make water
                pthread_cond_wait(&o_wait, &create_water);
            }
        }
        o_used--;  // this atom is used
        pthread_mutex_unlock(&create_water);
        // end of critical section
    }
}

// ############################################################

int main(int argc, char* argv[]) {
    // init stuff
    unsigned int global_seed = time(NULL);
    int h_prod_n, o_prod_n;

    // make sure that the mutex is unlocked
    pthread_mutex_unlock(&create_water);

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

    
    // thread arguments
    int threads_n = h_prod_n + o_prod_n + 3;
    struct args_t args_tab[threads_n];
    for (int i = 0; i < threads_n; ++i) {
        args_tab[i].id = i;
        args_tab[i].seed = rand_r(&global_seed);
    }
 
    // create threads
    int id = 0;
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
