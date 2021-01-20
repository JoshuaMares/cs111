#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
//#include <sys/stat.h>//o_rdwr

void catch_fun(int signal_message){
  if(signal_message == SIGSEGV){
    fprintf(stderr, "Segmentation Fault");
    exit(4);
  }
}

int main(int argc, char **argv){
  char* input = NULL;
  char* output = NULL;
  int sf = 0;
  int option_index = 0;//still not why we need this
  int option_short;


  //nameString, needsArg?, flag, val
  static struct option long_options[] = {
    {"input",   required_argument,    0,    'i'},
    {"output",  required_argument,    0,    'o'},
    {"segfault", no_argument,          0,    's'},
    {"catch",   no_argument,          0,    'c'},
    {0,         0,                    0,     0}
  };
  while((option_short = getopt_long(argc, argv, "i:o:sc", long_options, &option_index)) != -1){
    /*
    see disc slides
    for each option
      switch to case for option
        in case, assign optarg to appropriate var
          ***optarg is the argument given to option and does not need to be
             declared because it is already declared in unistd and assigned in
             getopt_long
    */
    switch(option_short){
      case 'i':
        //set input
        input = optarg;
        break;
      case 'o':
        //set output
        output = optarg;
        break;
      case 's':
        //set segfault
        sf = 1;
        break;
      case 'c':
        signal(SIGSEGV, catch_fun);
        break;
      case '?':
        //incorrect option
        //print out correct usage
        //help page
        printf("Unrecognized option.\nValid options include:\n\t --input filename\n\t --output filename\n\t --segfault\n\t --catch\n");
        exit(1);
        break;
    }//end of switch
    //default case is covered by '?' (i think)

  }//end of while
  //proceed to execute appropriate functions based on variable values
  if(input){
    int ifd = open(input, O_RDONLY);//-1 if failed
    if(ifd>=0){
      close(0);
      dup(ifd);
      close(ifd);
    }else{
      fprintf(stderr, "--input error\nCould not open input file: %s\n%s\n", input, strerror(errno));
      exit(2);
    }
  }
  if(output){
    //o_rdwr isnt working
    //why does o_rdonly work but not o_rdwr?
    //for 0666 think chmod permissions, first 0 indicates file or directory
    //for some reason 0666 isnt working properly either but it sets enough of permissions to make this work
    /*do we cancel if file exists?
      do we write over file or append to it?*/
    //doesnt throw error when read only?
    int ofd = creat(output, 0666);
    if(ofd>=0){
      close(1);
      dup(ofd);
      close(ofd);
    }else{
      fprintf(stderr, "--output error\nCould not open output file: %s\n%s\n", output, strerror(errno));
      exit(3);
    }
  }
  if(sf){
      char* str = NULL;
      *str = 'b';
  }

  //reading and writing
  char buf;
  ssize_t status = read(0, &buf, 1);
  while(status > 0){
    write(1, &buf, 1);
    status = read(0, &buf, 1);
  }
  if(status == -1){
    //print error code
    fprintf(stderr, "Error, could not write to file: %s\n%s\n", output, strerror(errno));
    exit(3);
  }
  exit(0);
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
