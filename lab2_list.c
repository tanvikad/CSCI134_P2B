#include <pthread.h>
#include <stdio.h> //for printing
#include <stdlib.h> 
#include <time.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h> 
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/types.h> 
#include <netinet/in.h>
#include <netdb.h> 
#include <stdlib.h>


#include "SortedList.h"

int num_threads = 1;
int num_iterations = 1;
int opt_yield = 0;
int got_sync = 0;
int got_pthread_mutex = 0;
int got_spin_lock = 0;

pthread_mutex_t mutex_lock;
int test_and_set_lock = 0;

SortedList_t list;
SortedListElement_t* elements; 

void lock() {
    if(got_pthread_mutex) {
        pthread_mutex_lock(&mutex_lock);
    } else if(got_spin_lock) {
        while(__sync_lock_test_and_set(&test_and_set_lock, 1) == 1) 
            ;
    
    } 
}

void unlock() {
    if(got_pthread_mutex){
        pthread_mutex_unlock(&mutex_lock);
    } else if(got_spin_lock) {
        // test_and_set_lock = 0;
        __sync_lock_release(&test_and_set_lock);
    }
}

void handle_sigint(int sig)
{
    fprintf(stderr, "Caught a segmentation fault with code %d", sig);
    exit(2);
}

void* thread_action(void* i) {
    long tid = (long)i;
    long start = tid*num_iterations;

    for(int i = start; i < start+num_iterations; i++){
        lock();
        SortedList_insert(&list, &elements[i]);
        unlock();
    }

    lock();
    int length = SortedList_length(&list);
    unlock();
    if(length == -1) {
        fprintf(stderr, "Thread %ld: The list is corrupted when calling length \n", tid);
        exit(2);
    }

    for(int i =start; i < start+num_iterations; i++) {
        const char* string_to_lookup = elements[i].key;
        int return_delete = 0;
        lock();
        SortedListElement_t* element = SortedList_lookup(&list, string_to_lookup);
        if(element == NULL) {
            printf("Thread %ld: The element for key %s is null due to corruption\n", tid, string_to_lookup);
            unlock();
            exit(2);
        } else{
            return_delete = SortedList_delete(element);
        }
        unlock();

        if(return_delete == 1) {
            fprintf(stderr, "Thread %ld: Deletion failed on key due to corruption %s \n", tid, string_to_lookup);
            exit(2);
        }
    }

    pthread_exit(NULL);
    return NULL;
}


char* get_rand_string() {
    static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";        
    int length = 3;
    char* random_string = malloc(sizeof(char) * (length +1));

    if(random_string) {
        for (int i = 0; i < length; i++) {

            int key = rand() % (int)(sizeof(charset) -1) + i;
            random_string[i] = charset[key];
        }

        random_string[length] = '\0';

    }else {
        fprintf(stderr, "Unable to malloc a random string \n");
        exit(1);
    }

    return random_string;
}

void print_list() {
    SortedListElement_t* curr = list.next;
    while(1) {
        if(curr == NULL) break;
        if(curr->key != NULL) {
            printf("Key is %s, previous is %s, next is %s \n", curr->key, curr->prev->key, curr->next->key);
        }else {
            break;
        }

        curr = curr->next;
    }
}

int main(int argc, char *argv[]) {
    srand(time(0));

    signal(SIGSEGV, handle_sigint);

    int curr_option;
    const struct option options[] = {
        { "threads",  required_argument, NULL,  't' },
        { "iterations", required_argument, NULL,  'i' },
        { "yield", required_argument, NULL, 'y'},
        { "sync", required_argument, NULL, 's'},
        { 0, 0, 0, 0}
    };

    char* yield_argument = NULL; 
    while((curr_option = getopt_long(argc, argv, "i:t:y", options, NULL)) != -1)  {
        switch(curr_option) {
            case 't':
                sscanf(optarg, "%d", &num_threads);
                // num_threads = optarg;
                break;
            case 'i':
                sscanf(optarg, "%d", &num_iterations);
                // num_iterations = optarg;
                break;
            case 'y':
                yield_argument = optarg;  
                break;
            case 's':
                if(*optarg == 'm') {
                    got_pthread_mutex = 1;
                }else if (*optarg == 's') {
                    got_spin_lock = 1;
                }else {
                    fprintf(stderr, "Did not give a synchronization method \n ");
                    exit(1);
                }
                break;
            default:
                fprintf(stderr, "Use the options --iterations --threads");
                exit(1);
                break;
        }
    }

    if(got_spin_lock && got_pthread_mutex) {
        fprintf(stderr, "Cannot use both a spin lock and a mutext lock \n");
        exit(1);
    }

    if(yield_argument) {
        for(int i = 0; i < (int)(strlen(yield_argument)); i++){
            if(yield_argument[i] == 'i')  opt_yield |= INSERT_YIELD;
            else if(yield_argument[i] == 'd') opt_yield |= DELETE_YIELD;
            else if (yield_argument[i] == 'l')  opt_yield |= LOOKUP_YIELD;
            else {
                fprintf(stderr, "Gave an invalid yield argument \n");
                exit(1);
            }
        }
    }

    //iinitalized list;
    list.prev = NULL;
    list.next = NULL;
    list.key = NULL;

    elements = malloc(sizeof(SortedListElement_t) * num_threads * num_iterations);
    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);

    if(got_pthread_mutex) {
        int rc = pthread_mutex_init(&mutex_lock, NULL);
        if(rc != 0) {
            fprintf(stderr, "Failed to initialize the pthread mutex \n");
            exit(1);
        }
    }

    //initalize the elements
    for (int i = 0; i < (num_threads*num_iterations); i++) {
        elements[i].key = get_rand_string();
        elements[i].prev = NULL;
        elements[i].next = NULL;
    }

    struct timespec start, stop;
    if( clock_gettime( CLOCK_MONOTONIC, &start) == -1 ) {
        fprintf(stderr, "Clock Get Time Fails %s\n", strerror(errno));
        exit(1);
    }
    long t; 
    for (t = 0; t < num_threads; ++t) {
        // integers[t] = t;
        int rc = pthread_create(&threads[t], NULL, thread_action, (void*)t);
        if(rc != 0) {
            fprintf(stderr, "Failed to initialize the pthread \n");
            exit(1);
        }
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    if( clock_gettime( CLOCK_MONOTONIC, &stop) == -1 ) {
        fprintf(stderr, "Clock Get Time Fails %s\n", strerror(errno));
        exit(1);
    }

    int length_at_the_end = SortedList_length(&list);
    double accum = (stop.tv_sec - start.tv_sec) * 1000000000
          + (stop.tv_nsec - start.tv_nsec);

    int num_operations = 3 * num_threads * num_iterations;
    int num_lists = 1;

    char type_of_str[100] = "list-";
    if(opt_yield&INSERT_YIELD) strcat(type_of_str, "i");
    if(opt_yield&DELETE_YIELD) strcat(type_of_str, "d");
    if(opt_yield&LOOKUP_YIELD) strcat(type_of_str, "l");

    if(opt_yield == 0) strcat(type_of_str, "none");
    strcat(type_of_str, "-");

    if(got_pthread_mutex) strcat(type_of_str, "m");
    else if(got_spin_lock) strcat(type_of_str, "s");
    else strcat(type_of_str, "none");


    int list_csv_fd = open("lab2_list.csv", O_CREAT | O_WRONLY | O_APPEND, S_IRWXU);
    if(list_csv_fd == -1) {
        fprintf(stderr, "Opening the csv file failed %s \n", strerror(errno));
        exit(1);
    }

    char list_csv_buff[200];
    if(length_at_the_end != 0) {
        fprintf(stderr, "The length at the end is not 0 \n");
        exit(2);
    }
    snprintf(list_csv_buff, 200, "%s,%d,%d,%d,%d,%ld,%ld \n", type_of_str, num_threads, num_iterations, num_lists,num_operations,
        (long)(accum), (long)(accum/num_operations));
    write(list_csv_fd, list_csv_buff, strlen(list_csv_buff));
    write(1, list_csv_buff, strlen(list_csv_buff));



    free(elements);
    free(threads);
}