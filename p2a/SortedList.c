#include <stdio.h>
#include <sched.h>
#include <string.h>
#include "SortedList.h"

//assuming never null because circular
//assuming strings can be used for keys
int opt_yield = 0;
void SortedList_insert(SortedList_t *list, SortedListElement_t *element){
  SortedListElement_t* temp = list->next;
  while(temp != list && (strcmp(temp->key, element->key) < 0 )){
    //<0 means str 1 has less value than str 2
    temp = temp->next;
  }

  if(opt_yield & INSERT_YIELD){
    sched_yield();
  }

  element->prev = temp->prev;
  temp->prev = element;
  element->next = temp;
  element->prev->next = element;
  return;
}

int SortedList_delete( SortedListElement_t *element){
  //checking prereqs
  //implies there is a next and prev node
  if((element->next->prev != element) && (element->prev->next != element)){
    return 1;
  }
  //actual delete
  if(opt_yield & DELETE_YIELD){
    sched_yield();
  }
  element->prev->next = element->next;
  element->next->prev = element->prev;
  element = NULL;
  return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key){
  SortedListElement_t* temp = list->next;
  while(temp != list && (strcmp(temp->key, key) != 0)){
    if(opt_yield & LOOKUP_YIELD){
      sched_yield();
    }
    temp = temp->next;
  }
  if(temp == list){
    return NULL;
  }
  return temp;
}

int SortedList_length(SortedList_t *list){
  int i = 0;
  SortedListElement_t* temp = list->next;
  while(temp != list){
    if(opt_yield & LOOKUP_YIELD){
      sched_yield();
    }
    temp = temp->next;
    i++;
  }
  return i;
}
