#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>
#include "SortedList.h"

int iterations_arg = 1;
SortedList_t *list = NULL;
char sync_arg = '\0';
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int lock = 0;

static inline unsigned long tin(struct timespec *spec){
  unsigned long ret = spec->tv_sec;
  ret = (ret*1000000000) + spec->tv_nsec;
  return ret;
}

void * thread_worker(void *arg){
  SortedListElement_t *element = ((SortedListElement_t *) arg);//pointer to first element deignated for this thread
  switch(sync_arg){
    case 'm':
      pthread_mutex_lock(&mutex);
      printf("mutex set\n");
      break;
    case 's':
      printf("entering spinlock\n");
      while(__sync_lock_test_and_set(&lock, 1));
      break;
  }
  //add to list
  for(int i = 0; i < iterations_arg; i++){
    SortedList_insert(list, element+i);
  }

  //get list length
  if(SortedList_length(list)){
    //no warning
  }

  //lookup and delete
  for(int i = 0; i < iterations_arg; i++){
    if(SortedList_lookup(list, element[i].key) == NULL){
      fprintf(stderr, "Corrupted List:lookup\n");
      exit(2);
    }
    if(SortedList_delete(element+i)){
      fprintf(stderr, "Corrupted List:delete\n");
      exit(2);
    }
  }
  switch(sync_arg){
    case 'm':
      pthread_mutex_unlock(&mutex);
      break;
    case 's':
      __sync_lock_release(&lock);
      break;
  }
  return NULL;
}

int main(int argc, char **argv){
  int threads_arg = 1;
  int option_index = 0;
  int option_short;

  static struct option long_options[] = {
    {"threads",     required_argument,    0,    't'},
    {"iterations",  required_argument,    0,    'i'},
    {"sync",        required_argument,    0,    's'},
    {"yield",       no_argument,          0,    'y'},
    {0,             0,                    0,     0}
  };
  while((option_short = getopt_long(argc, argv, "t:i:s:y", long_options, &option_index)) != -1){
    switch(option_short){
      case 't':
        threads_arg = atoi(optarg);
        break;
      case 'i':
        iterations_arg = atoi(optarg);
        break;
      case 'y':
        for(size_t i = 0; i < strlen(optarg); i++){
          switch(optarg[i]){
            case 'i':
              opt_yield = opt_yield | INSERT_YIELD;
              break;
            case 'd':
              opt_yield = opt_yield | DELETE_YIELD;
              break;
            case 'l':
              opt_yield = opt_yield | LOOKUP_YIELD;
              break;
            default:
              printf("Unrecognized yield option(s).\nValid options include:\n\ti\n\td\n\tl\n");
              exit(1);
              break;
          }
        }
        break;
      case 's':
        sync_arg = optarg[0];
        switch(sync_arg){
          case 'm':
            printf("Sync arg = m\n");
            break;
          case 's':
            printf("Sync arg = s\n");
            break;
          default:
            printf("Unrecognized sync option.\nValid options include:\n\tm\n\ts\n");
            exit(1);
            break;
        }
        break;
      case '?':
        printf("Unrecognized option.\nValid options include:\n\t --threads=#\n\t --iterations=#\n\t --sync=char\n\t --sync\n");
        exit(1);
        break;
    }
  }

  //initializing empty list;
  list = malloc((sizeof(SortedList_t)) * 1);
  list->next = list;
  list->prev = list;
  list->key  = '\0';

  //creates and initiallizes list elements
  SortedListElement_t *elements = malloc((sizeof(SortedListElement_t)) * threads_arg * iterations_arg);
  srand(time(NULL));
  char** keys = malloc((sizeof(char*)) * threads_arg * iterations_arg);
  for(int i = 0; i < threads_arg*iterations_arg; i++){
    keys[i] = malloc((sizeof(char)) * 16);//length 16 strings, 15 with null
    keys[i][15] = '\0';
    for(int j = 0; j < 15; j++){
      keys[i][j] = (char) (48+rand()%75);//0-z ascii
    }
    elements[i].next = NULL;//avoid sus stuff
    elements[i].prev = NULL;
    elements[i].key = keys[i];
  }

  //create pthread objects
  pthread_t *threads = malloc((sizeof(pthread_t)) * threads_arg);
  //timer stuff
  struct timespec start_time, end_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  //run specified function for each thread
  for(int i = 0; i < threads_arg; i++){
    if(pthread_create(&threads[i], NULL, thread_worker, i*iterations_arg + elements) != 0){//2d array: elements[i or threads_arg][iterations_arg]
      fprintf(stderr, "Thread creation error\n%s\n", strerror(errno));
      exit(1);
    }
  }
  //wait for threads to finish
  for(int i = 0; i < threads_arg; i++){
    if(pthread_join(threads[i], NULL) != 0){
      fprintf(stderr, "Thread joining error\n%s\n", strerror(errno));
      exit(1);
    }
  }

  clock_gettime(CLOCK_MONOTONIC, &end_time);
  unsigned long diff = tin(&end_time) - tin(&start_time);

  if(SortedList_length(list) != 0){
    fprintf(stderr, "List not size 0.\n");
    exit(2);
  }
  int total_ops = threads_arg*iterations_arg*3;
  /*
  the name of the test, which is of the form: list-yieldopts-syncopts: where
    yieldopts = {none, i,d,l,id,il,dl,idl}
    syncopts = {none,s,m}
  */

  char* name2 = "";
  char* name3 = "";
  switch(opt_yield){
    case 0:
      name2 = "-none";
      break;
    case 1:
      name2 = "-i";
      break;
    case 2:
      name2 = "-d";
      break;
    case 3: //INSERT_YIELD | DELETE_YIELD
      name2 = "-id";
      break;
    case 4:
      name2 = "-l";
      break;
    case 5:
      name2 = "-il";
      break;
    case 6:
      name2 = "-dl";
      break;
    case 7:
      name2 = "-idl";
      break;
  }
  switch(sync_arg){
    case 'm':
      name3 = "-m";
      break;
    case 's':
      name3 = "-s";
      break;
    default:
      name3 = "-none";
      break;
  }
  printf("list%s%s,%i,%i,%i,%i,%lu,%lu\n", name2, name3, threads_arg, iterations_arg, 1, total_ops, diff, diff/total_ops);
  exit(0);//2 if end list length is not == 0name2 =
}
