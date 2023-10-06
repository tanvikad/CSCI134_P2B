#include "SortedList.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
    if(list->next == NULL) {
        if(opt_yield & INSERT_YIELD) sched_yield();
        list->next = element;
        list->prev = element;
        element->next = list;
        element->prev = list;
        return;
    }


    SortedListElement_t* current = list->next;
    while(1) {
        if(current->key == NULL) {
            //we should add it right before the element
            SortedListElement_t* previous_element = current->prev;
            if(opt_yield & INSERT_YIELD) sched_yield();
            current->prev = element;
            element->next = current;
            element->prev = previous_element;
            previous_element->next = element;
            break;
        }
        //a b c d f 
        if(strcmp(element->key, current->key) < 0) {
            SortedListElement_t* previous_element = current->prev;
            if(opt_yield & INSERT_YIELD) sched_yield();
            current->prev = element;
            element->next = current;
            element->prev = previous_element;
            previous_element->next = element;
            break;
        } else {
            current = current->next;
            continue;
        }
    }

}


int SortedList_delete( SortedListElement_t *element) {
    if(element == NULL) {
        return 1; 
    }

    if(element->next->prev != element || element->prev->next != element) {
        return 1;
    }

    if(element->next == element->prev) {
        //there is going to be an empty list
        if(element->next->key != NULL) {
            return 1;
        }
        SortedListElement_t* empty_list = element->next;
        if(opt_yield & DELETE_YIELD) sched_yield();
        empty_list->next = NULL;
        empty_list->prev = NULL;
        return 0;
    }

    SortedListElement_t* prev_elem = element->prev;
    SortedListElement_t* next_elem = element->next;

    if(opt_yield & DELETE_YIELD) sched_yield();

    prev_elem->next = next_elem;
    next_elem->prev = prev_elem;

    return 0;
}


SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
    if(list == NULL) return NULL;
    if(key == NULL) return NULL;
    SortedListElement_t* current_element = list->next;
    while(1) {
        if(current_element == NULL) return NULL;
        if(current_element->key == NULL) return NULL;
        if(*current_element->key == *key) return current_element;

        if(opt_yield & LOOKUP_YIELD) sched_yield();

        current_element = current_element->next;
    }

    return NULL;
}


int SortedList_length(SortedList_t *list) {
    if(list == NULL) return -1;
    int length = 0; 
    SortedListElement_t* current_element = list->next;
    if(current_element == NULL) return 0;
    while(1) {
        if(current_element->key == NULL) return length;
        if(current_element->next ==NULL || current_element->prev == NULL) return -1;
        if(current_element->next->prev != current_element || current_element->prev->next != current_element) return -1;
        
        if(opt_yield & LOOKUP_YIELD) sched_yield();

        length += 1;
        current_element = current_element->next;
    }

    return length;
}