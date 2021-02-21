#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include "SortedList.h"

int iterations_arg = 1;
int lists_arg = 1;
SortedList_t *list = NULL;
char sync_arg = '\0';
static pthread_mutex_t *mutex = NULL;
static int *lock = NULL;
struct timespec *lock_start_time;
struct timespec *lock_end_time;
unsigned long *wait_time;

struct thread_arg{
  SortedListElement_t *earg;
  int thread_number;
};
typedef struct thread_arg thread_arg_t;

void catch_fun(int signal_message){
  if(signal_message == SIGSEGV){
    fprintf(stderr, "Segmentation Fault\n");
    exit(2);
  }
}

static inline unsigned long tin(struct timespec *spec){
  unsigned long ret = spec->tv_sec;
  ret = (ret*1000000000) + spec->tv_nsec;
  return ret;
}

void * thread_worker(void *arg){
  thread_arg_t temp = *((thread_arg_t *) arg);
  SortedListElement_t *element = temp.earg;//pointer to first element designated for this thread
  int list_number = element->key[0] % lists_arg;
  //add to list
  for(int i = 0; i < iterations_arg; i++){
    clock_gettime(CLOCK_MONOTONIC, lock_start_time + temp.thread_number);
    switch(sync_arg){
      case 'm':
        list_number = (element+i)->key[0] % lists_arg;
        pthread_mutex_lock(mutex + list_number);
        break;
      case 's':
        while(__sync_lock_test_and_set(lock + list_number, 1));
        break;
    }
    clock_gettime(CLOCK_MONOTONIC, lock_end_time + temp.thread_number);
    wait_time[temp.thread_number] += tin(lock_end_time + temp.thread_number) - tin(lock_start_time + temp.thread_number);
    SortedList_insert(&list[list_number], element+i);
    switch(sync_arg){
      case 'm':
        pthread_mutex_unlock(mutex + list_number);
        break;
      case 's':
        __sync_lock_release(lock + list_number);
        break;
    }
  }

  //get list length
  clock_gettime(CLOCK_MONOTONIC, lock_start_time + temp.thread_number);
  switch(sync_arg){
    case 'm':
      pthread_mutex_lock(mutex);
      break;
    case 's':
      while(__sync_lock_test_and_set(lock, 1));
      break;
  }
  clock_gettime(CLOCK_MONOTONIC, lock_end_time + temp.thread_number);
  wait_time[temp.thread_number] += tin(lock_end_time + temp.thread_number) - tin(lock_start_time + temp.thread_number);
  if(SortedList_length(list)){
    //no warning
  }
  switch(sync_arg){
    case 'm':
      pthread_mutex_unlock(mutex);
      break;
    case 's':
      __sync_lock_release(lock);
      break;
  }

  //lookup and delete
  clock_gettime(CLOCK_MONOTONIC, lock_start_time + temp.thread_number);
  for(int i = 0; i < iterations_arg; i++){
    switch(sync_arg){
      case 'm':
        list_number = (element+i)->key[0] % lists_arg;
        pthread_mutex_lock(mutex + list_number);
        break;
      case 's':
        while(__sync_lock_test_and_set(lock + list_number, 1));
        break;
    }
    clock_gettime(CLOCK_MONOTONIC, lock_end_time + temp.thread_number);
    wait_time[temp.thread_number] += tin(lock_end_time + temp.thread_number) - tin(lock_start_time + temp.thread_number);
    if(SortedList_lookup(list + list_number, (element+i)->key) == NULL){
      fprintf(stderr, "Corrupted List:lookup. Iteration:%i\n", i);
      exit(2);
    }
    if(SortedList_delete(element+i)){
      fprintf(stderr, "Corrupted List:delete. Iteration:%i\n", i);
      exit(2);
    }
    switch(sync_arg){
      case 'm':
        pthread_mutex_unlock(mutex + list_number);
        break;
      case 's':
        __sync_lock_release(lock + list_number);
        break;
    }
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
    {"yield",       required_argument,    0,    'y'},
    {"lists",       required_argument,    0,    'l'},
    {0,             0,                    0,     0}
  };
  while((option_short = getopt_long(argc, argv, "t:i:s:y:", long_options, &option_index)) != -1){
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
            break;
          case 's':
            break;
          default:
            printf("Unrecognized sync option.\nValid options include:\n\tm\n\ts\n");
            exit(1);
            break;
        }
        break;
      case 'l':
        lists_arg = atoi(optarg);
        break;
      case '?':
        printf("Unrecognized option.\nValid options include:\n\t --threads=#\n\t --iterations=#\n\t --sync=char\n\t --yield={idl}\n\t --lists=#\n");
        exit(1);
        break;
    }
  }
  signal(SIGSEGV, catch_fun);

  //initializing empty list;
  list = malloc((sizeof(SortedList_t)) * lists_arg);
  mutex = malloc(sizeof(pthread_mutex_t) * lists_arg);
  lock = malloc(sizeof(int) * lists_arg);
  for(int i = 0; i < lists_arg; i++){
    list[i].next = list + i;
    list[i].prev = list + i;
    list[i].key  = '\0';
    pthread_mutex_init(mutex+i, NULL);
    lock[i] = 0;
  }

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

  //creating thread arguments
  thread_arg_t *apt = malloc(sizeof(thread_arg_t) * threads_arg);
  for(int i = 0; i < threads_arg; i++){
    apt[i].earg = i*iterations_arg + elements;
    apt[i].thread_number = i;
  }

  //create pthread objects
  pthread_t *threads = malloc((sizeof(pthread_t)) * threads_arg);
  //timer stuff
  lock_start_time = malloc(sizeof(struct timespec) * threads_arg);
  lock_end_time   = malloc(sizeof(struct timespec) * threads_arg);
  wait_time  = malloc(sizeof(unsigned long) * threads_arg);
  struct timespec start_time, end_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  //run specified function for each thread
  for(int i = 0; i < threads_arg; i++){
    if(pthread_create(&threads[i], NULL, thread_worker, apt + i) != 0){
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
  unsigned long sum = 0;
  for(int i = 0; i < threads_arg; i++){
    sum += wait_time[i];
  }
  sum = sum / (threads_arg * iterations_arg * 3);//locked each iteration for each thread for insert, length, and lookup/delete
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
      sum = 0;
      break;
  }
  printf("list%s%s,%i,%i,%i,%i,%lu,%lu,%lu\n", name2, name3, threads_arg, iterations_arg, lists_arg, total_ops, diff, diff/total_ops, sum);
  exit(0);//2 if end list length is not == 0name2 =
}
