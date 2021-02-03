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

static inline unsigned long tin(struct timespec *spec){
  unsigned long ret = spec->tv_sec;
  ret = (ret*1000000000) + spec->tv_nsec;
  return ret;
}

void add(long long *pointer, long long value) {
  long long sum = *pointer + value;
  if(opt_yield){
    sched_yield();
  }
  *pointer = sum;
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
  long long ctz = 0;
  int option_index = 0;//still not why we need this
  int option_short;
  char* name = "add-none";

  static struct option long_options[] = {
    {"threads",     required_argument,    0,    't'},
    {"iterations",  required_argument,    0,    'i'},
    {"yield",       no_argument,          0,    'y'},
    {0,             0,                    0,     0}
  };
  while((option_short = getopt_long(argc, argv, "i:o:sc", long_options, &option_index)) != -1){
    switch(option_short){
      case 't':
        threads_arg = atoi(optarg);
        break;
      case 'i':
        iterations_arg = atoi(optarg);
        break;
      case 'y':
        opt_yield = 1;
        name = "add-yield";
        break;
      case '?':
        printf("Unrecognized option.\nValid options include:\n\t --input filename\n\t --output filename\n\t --segfault\n\t --catch\n");
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
      exit(2);
    }
  }
  //wait for threads to finish
  for(int i = 0; i < threads_arg; i++){
    if(pthread_join(threads[i], NULL) != 0){
      fprintf(stderr, "Thread joining error\n%s\n", strerror(errno));
      exit(2);
    }
  }

  clock_gettime(CLOCK_MONOTONIC, &end_time);
  unsigned long diff = tin(&end_time) - tin(&start_time);

  int total_ops = threads_arg*iterations_arg*2;
  printf("%s,%i,%i,%i,%lu,%lu,%lld\n", "add-none", threads_arg, iterations_arg, total_ops, diff, diff/total_ops, ctz);
  exit(0);
}
