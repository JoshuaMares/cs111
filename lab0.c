#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

int main(int argc, char **argv){
  char* input;
  char* output;
  int opt;

  while((opt = getopt_long(argc, argv, "i:o:")) != -1){
    /*
    see disc slides
    for each iteration
    get argument
      switch to case for option
        in case, assign optarg to appropriate var
    */
    //nameString, needsArg?, flag, val
    static struct option long_options[] = {
      {"input",   required_argument,    0,    'i'},
      {"output",  required_argument,    0,    'o'},
      {"segfult", no_argument,          0,    's'},
      {"catch",   no_argument,          0,    'c'},
      {0,         0,                    0,     0}
    };
    switch(opt){
      case 'i':
        break;
      case 'o':
        break;
      case 's':
        break;
      case 'c':
        break;
    }

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
