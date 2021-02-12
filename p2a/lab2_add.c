#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>


long long ctz = 0;
int opt_yield = 0;
char sync_arg = '\0';
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int lock = 0;

static inline unsigned long tin(struct timespec *spec){
  unsigned long ret = spec->tv_sec;
  ret = (ret*1000000000) + spec->tv_nsec;
  return ret;
}

void add(long long *pointer, long long value) {
  long long sum = 0;
  long long prev = 0;
  switch(sync_arg){
    case 'm'://mutex
      pthread_mutex_lock(&mutex);
      sum = *pointer + value;
      if(opt_yield){
        sched_yield();
      }
      *pointer = sum;
      pthread_mutex_unlock(&mutex);
      break;
    case 's'://spin lock
      while(__sync_lock_test_and_set(&lock, 1));
      sum = *pointer + value;
      if(opt_yield){
        sched_yield();
      }
      *pointer = sum;
      __sync_lock_release(&lock);
      break;
    case 'c'://compare and swap//atomic add
      do{
        prev = *pointer;
        sum = prev + value;
        if(opt_yield){
          sched_yield();
        }
      }while(__sync_bool_compare_and_swap(pointer, prev, sum) == 0);
      break;
    default:
      sum = *pointer + value;
      if(opt_yield){
        sched_yield();
      }
      *pointer = sum;
      break;
  }
}

void * thread_worker(void *arg){
  int iter = *((int*) arg);
  //add 1 for iterations
  for(int i = 0; i < iter; i++){
      add(&ctz, 1);
  }
  //subtract one for iterations
  for(int i = 0; i < iter; i++){
      add(&ctz, -1);
  }
  return NULL;
}

int main(int argc, char **argv){
  int threads_arg = 1;
  int iterations_arg = 1;
  int option_index = 0;//still not why we need this
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
        opt_yield = 1;
        break;
      case 's':
        sync_arg = optarg[0];
        switch(sync_arg){
          case 'm':
          case 's':
          case 'c':
            break;
          default:
            printf("Unrecognized sync option.\nValid options include:\n\tm\n\ts\n\tc\n");
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

  //create pthread objects
  pthread_t *threads = malloc((sizeof(pthread_t)) * threads_arg);
  //timer stuff
  struct timespec start_time, end_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  //run specified function for each thread
  for(int i = 0; i < threads_arg; i++){
    if(pthread_create(&threads[i], NULL, thread_worker, &iterations_arg) != 0){
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

  int total_ops = threads_arg*iterations_arg*2;
  /*
  add-none ... no yield, no synchronization
  add-m ... no yield, mutex synchronization
  add-s ... no yield, spin-lock synchronization
  add-c ... no yield, compare-and-swap synchronization
  add-yield-none ... yield, no synchronization
  add-yield-m ... yield, mutex synchronization
  add-yield-s ... yield, spin-lock synchronization
  add-yield-c ... yield, compare-and-swap synchronization
  */
  char* name;
  if(opt_yield){
    switch(sync_arg){
      case 'm':
        name = "add-yield-m";
        break;
      case 's':
        name = "add-yield-s";
        break;
      case 'c':
        name = "add-yield-c";
        break;
      default:
        name = "add-yield-none";
        break;
    }
  }else{
    switch(sync_arg){
      case 'm':
        name = "add-m";
        break;
      case 's':
        name = "add-s";
        break;
      case 'c':
        name = "add-c";
        break;
      default:
        name = "add-none";
        break;
    }
  }
  printf("%s,%i,%i,%i,%lu,%lu,%lld\n", name, threads_arg, iterations_arg, total_ops, diff, diff/total_ops, ctz);
  exit(0);
}
