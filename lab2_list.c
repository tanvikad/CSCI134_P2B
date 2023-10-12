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
#include <math.h>

#include "SortedList.h"

int num_threads = 1;
int num_iterations = 1;
int opt_yield = 0;
int got_sync = 0;
int got_pthread_mutex = 0;
int got_spin_lock = 0;
int num_lists = 1;

pthread_mutex_t* mutex_lock;
int* test_and_set_lock;

SortedList_t* list;
SortedListElement_t* elements; 

int hash_string(const char* str) {
    if(str == NULL) {
        fprintf(stderr, "String passed is Null \n");
    }
    int length = strlen(str);
    int P = 2;
    long long  hash = 0;
    for(int i = 0; i < length; i++) {
        hash += (((long long)(str[i]) * (long long )(pow(P, i))));
        // printf("the has is %d and list is %d \n", hash, num_lists);
        hash %= num_lists;
    }


    return hash;
}

double lock(int list_num) {
    struct timespec start, stop;
    if( clock_gettime( CLOCK_MONOTONIC, &start) == -1 ) {
        fprintf(stderr, "Clock Get Time Fails %s\n", strerror(errno));
        exit(1);
    }


    if(got_pthread_mutex) {
        pthread_mutex_lock(&mutex_lock[list_num]);
    } else if(got_spin_lock) {
        while(__sync_lock_test_and_set(&test_and_set_lock[list_num], 1) == 1) 
            ;
    
    }

    if( clock_gettime( CLOCK_MONOTONIC, &stop) == -1 ) {
        fprintf(stderr, "Clock Get Time Fails %s\n", strerror(errno));
        exit(1);
    }

    double accum = (stop.tv_sec - start.tv_sec) * 1000000000 + (stop.tv_nsec - start.tv_nsec);
    return accum;

}

void unlock(int list_num) {
    if(got_pthread_mutex){
        pthread_mutex_unlock(&mutex_lock[list_num]);
    } else if(got_spin_lock) {
        // test_and_set_lock = 0;
        __sync_lock_release(&test_and_set_lock[list_num]);
    }
}

void handle_sigint(int sig)
{
    fprintf(stderr, "Caught a segmentation fault with code %d", sig);
    exit(2);
}


int get_length_of_list(int* length) {
    *length = 0;
    int total_time = 0;
    if(length == NULL) {
        fprintf(stderr, "The length of the list was given a nullptr \n");
    }
    for (int i = 0; i < num_lists; i++) {
        int time = lock(i);
        total_time += time;
        int sublist_length = SortedList_length(&list[i]);
        if(sublist_length == -1) {
            fprintf(stderr, "List %d: The list is corrupted when calling length \n", i);
            exit(2);
        }
        *length += sublist_length;
        unlock(i);
    }   
    
    return total_time;
}

void* thread_action(void* i) {
    double *amount_time= malloc(sizeof(double));
    *amount_time = 0;
    double time = 0;

    long tid = (long)i;
    long start = tid*num_iterations;

    for(int i = start; i < start+num_iterations; i++){
        int list_num = hash_string( elements[i].key);
        time = lock(list_num);
        *amount_time += time; 
        SortedList_insert(&list[list_num], &elements[i]);
        unlock(list_num);
    }

    int length = 0;
    int time_of_length = get_length_of_list(&length);
    // printf("Thread %ld: the length of the list is %d \n", tid, length);
    *amount_time += time_of_length;

    for(int i =start; i < start+num_iterations; i++) {
        const char* string_to_lookup = elements[i].key;
        int list_num = hash_string(string_to_lookup);
        int return_delete = 0;
        time = lock(list_num);
        *amount_time += time; 
        SortedListElement_t* element = SortedList_lookup(&list[list_num], string_to_lookup);
        if(element == NULL) {
            printf("Thread %ld: The element for key %s is null due to corruption\n", tid, string_to_lookup);
            unlock(list_num);
            exit(2);
        } else{
            return_delete = SortedList_delete(element);
        }
        unlock(list_num);

        if(return_delete == 1) {
            fprintf(stderr, "Thread %ld: Deletion failed on key due to corruption %s \n", tid, string_to_lookup);
            exit(2);
        }
    }
    // pthread_exit(NULL);
    return (void*) amount_time;
}



char* get_rand_string() {
    static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.-#'?!";    
    int length_of_string = 3;    
    char* random_string = malloc(sizeof(char) * (length_of_string +1));

    if(random_string) {
        for (int i = 0; i < length_of_string; i++) {

            int key = rand() % (int)(sizeof(charset) -1) + i;
            random_string[i] = charset[key];
        }

        random_string[length_of_string] = '\0';

    }else {
        fprintf(stderr, "Unable to malloc a random string \n");
        exit(1);
    }

    return random_string;
}

void print_list(int list_num) {
    SortedListElement_t* curr = list[list_num].next;
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
        { "lists", required_argument, NULL, 'l'},
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
            case 'l':
                num_lists = atoi(optarg);
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
    // list.prev = NULL;
    // list.next = NULL;
    // list.key = NULL;

    list = malloc(sizeof(SortedList_t) * num_lists);
    test_and_set_lock = malloc(sizeof(int) * num_lists);
    mutex_lock = malloc(sizeof(pthread_mutex_t) * num_lists);
    if(mutex_lock == NULL) {
        fprintf(stderr, "Unable to malloc mutex lock");
    }


    for (int i = 0; i < num_lists; i++) {
        list[i].prev = NULL;
        list[i].next = NULL;
        list[i].key = NULL;

        test_and_set_lock[i] = 0;

        int rc = pthread_mutex_init(&mutex_lock[i], NULL);
        if(rc != 0) {
            fprintf(stderr, "Failed to initialize the pthread mutex \n");
            exit(1);
        }

    }


    elements = malloc(sizeof(SortedListElement_t) * num_threads * num_iterations);
    
    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);

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

    double total_time_waiting = 0;
    double* time;
    for (int i = 0; i < num_threads; i++) {

        pthread_join(threads[i], (void**) &time);
        total_time_waiting += *time; 
    }


    int number_lock_operations = (2*num_iterations* num_threads)+(num_lists*num_threads);
    // total_time_waiting /= num_threads * (2 * num_iterations + 1);


    if( clock_gettime( CLOCK_MONOTONIC, &stop) == -1 ) {
        fprintf(stderr, "Clock Get Time Fails %s\n", strerror(errno));
        exit(1);
    }

    int length_at_the_end;
    get_length_of_list(&length_at_the_end);

    double accum = (stop.tv_sec - start.tv_sec) * 1000000000
          + (stop.tv_nsec - start.tv_nsec);

    int num_operations = 3 * num_threads * num_iterations;
    // int num_lists = 1;

    char type_of_str[100] = "list-";
    if(opt_yield&INSERT_YIELD) strcat(type_of_str, "i");
    if(opt_yield&DELETE_YIELD) strcat(type_of_str, "d");
    if(opt_yield&LOOKUP_YIELD) strcat(type_of_str, "l");

    if(opt_yield == 0) strcat(type_of_str, "none");
    strcat(type_of_str, "-");

    if(got_pthread_mutex) strcat(type_of_str, "m");
    else if(got_spin_lock) strcat(type_of_str, "s");
    else strcat(type_of_str, "none");


    int list_csv_fd = open("lab2b_list.csv", O_CREAT | O_WRONLY | O_APPEND, S_IRWXU);
    if(list_csv_fd == -1) {
        fprintf(stderr, "Opening the csv file failed %s \n", strerror(errno));
        exit(1);
    }

    char list_csv_buff[200];
    if(length_at_the_end != 0) {
        fprintf(stderr, "The length at the end is not 0 \n");
        exit(2);
    }
 
    if(!got_pthread_mutex) {
        total_time_waiting = 0;
    }

    // printf("the num lists is %d \n", num_lists);

    snprintf(list_csv_buff, 200, "%s,%d,%d,%d,%d,%ld,%ld,%ld\n", type_of_str, num_threads, num_iterations, num_lists,num_operations, 
    (long)(accum), (long)(accum/num_operations), (long)(total_time_waiting/number_lock_operations));   


    write(list_csv_fd, list_csv_buff, strlen(list_csv_buff));
    write(1, list_csv_buff, strlen(list_csv_buff));


    free(list);
    free(mutex_lock);   
    free(test_and_set_lock);
    free(elements);
    free(threads);
}