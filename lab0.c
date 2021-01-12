#include <stdio.h>
#include <stdlib.h>
#include

int main(int argc, char **argv){
  while(1){
    /*
    see disc slides
    for each iteration
    get argument
      switch to case for option
        in case, assign optarg to appropriate var
    */
    int option_index = 0;
    static struct option long_options[] = {
      {"input",   0,    0,    'i'},
      {"output",  0,    0,    'o'},
      {"segfult", 0,    0,    's'},
      {"catch",   0,    0,    'c'},
      {0,         0,    0,     0}
    };

  }
  //proceed to execute appropriate functions based on variable values




}

/*
functions:
  posix files operations
    open(2), creat(2), close(2), dup(2), read(2), write(2), exit(2), signal(2)
  strerror(3)
  getopt(3)
  getopt_long(3)
  perror()
  fprintf(stderr,....)

programs:
  tar(1)
    -z option in particular
  gdb(1)
    run, bt, list, print, and break

goal
  Write a program that copies standard input into std output by read() from
  descriptor 0 and write() to file descriptor 1.  If no errors (other than eof)
  exit() with return code of 0
  See the site for optional command line arguments
  For printing error messages, give the cause of the error and its nature

Proccess:
  process arguments and store results
  check options and carry out in this order:
    file redirection
    register the signal handler
    cause the segfault
    if no segfault, place stdin to stdout
  errorcodes:
    0-copy successfull
    1-unrecognized arguments
    2-unable to open input file
    3-unable to open output file
    4-caught and received sigsegv

tips:
  use fprintf(stderr,...) over
  for segfault *(char *)0 = 0
*/
