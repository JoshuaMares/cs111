#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <poll.h>
#include <sys/wait.h>

struct termios old_mode;

void catch_fun(int signal_message){
  if(signal_message == SIGPIPE){
    fprintf(stderr, "Broken Pipe\n");
    tcsetattr(0, TCSANOW, &old_mode);
    exit(0);
  }
}

int main(int argc, char **argv){
  //getting args
  int shell_arg = 0;
  int debug_arg = 0;
  int option_index = 0;
  int option_short;
  static struct option long_options[] = {
    {"shell",   no_argument,    0,    's'},
    {"debug",   no_argument,    0,    'd'},
    {0,         0,              0,     0 }
  };

  while((option_short = getopt_long(argc, argv, "s", long_options, &option_index)) != -1){
    switch(option_short){
      case 's':
        shell_arg = 1;
        break;
      case 'd':
        debug_arg = 1;
        break;
      case '?':
        printf("Unrecognized option.\nValid option(s) include:\n\t --shell\n\t --debug\n");
        exit(1);
        break;
    }
  }
  if(debug_arg){
    printf("Debug argument chosen.\n");
  }
  if(debug_arg && shell_arg){
    printf("Shell argument chosen\n");
  }
  /*Save mode and then non-canonical input no echo mode*/
  //creates the struct
  struct termios new_mode;
  if(tcgetattr(0, &old_mode) == -1){//copies curr fd (which should be terminal) settings
    fprintf(stderr, "Could not get terminal attributes, \n %s\n", strerror(errno));
    exit(1);
  }
  new_mode = old_mode;
  new_mode.c_iflag = ISTRIP;//declares new settings
  new_mode.c_oflag = 0;
  new_mode.c_lflag = 0;
  if(tcsetattr(0, TCSANOW, &new_mode) == -1){//implements any changes
    fprintf(stderr, "Error, could not set terminal attributes:\n %s \n", strerror(errno));
  }

  if(shell_arg){
    //do forking stuff
    //if parent
    //  read input, echo it, and send to child process
    //if child open bash in process
    //  input to bash will come piped in from parent process (terminal)
    //  stdin and stderr are dupes of a pipe to parent process
    //special cases:
    //  \r  \n
    //    parent prints \r\n
    //    child receives only \n
    //    although child receives only \n
    //      it can send back \r\n
    signal(SIGPIPE, catch_fun);
    int to_shell[2];
    int from_shell[2];
    if(pipe(to_shell) == -1 || pipe(from_shell) == -1){
      fprintf(stderr, "Error, could not pipe:\n %s \n", strerror(errno));
      exit(1);
    }

    int ret = fork();
    if(ret == 0){//child process
      close(to_shell[1]);//only parent writes to toshell(1)
      close(0);//close previous input
      dup(to_shell[0]);//new 0 which =invalue toshell(0)
      close(to_shell[0]);//closes toshell(0)
      //previous code essentially points stdin to read end of pipe
      close(from_shell[0]);
      close(1);//close previous output
      dup(from_shell[1]);
      close(from_shell[1]);
      //pipes
      execlp("/bin/bash", "/bin/bash", NULL);
    }else{//parent process
      close(to_shell[0]);
      close(from_shell[1]);
      //read and write loop:
      struct pollfd pollfds[2];
      pollfds[0].fd = 0;
      pollfds[0].events = POLLIN + POLLHUP + POLLERR;
      pollfds[1].fd = from_shell[0];
      pollfds[1].events = POLLIN + POLLHUP + POLLERR;

      int end_flag = 0;
      while(end_flag != 1){//while we havent exited
        if(poll(pollfds, 2, -1) == -1){
          fprintf(stderr, "Encountered a polling error:\n%s\n", strerror(errno));
          exit(1);
        }
        if(pollfds[0].revents & POLLIN){
          char buf[256];
          int rw_status = read(0, &buf, 256);
          for(int i = 0; i < rw_status; i++){
            if(buf[i] == 0x4){
              if(debug_arg){
                printf("^D\n");
              }
              end_flag = 1;//end has been signaled or reached
            }else if(buf[i] == 0x3){
                if(debug_arg){
                  printf("^C\n");
                }
              kill(ret, SIGINT);
            }else if(buf[i] == '\r' || buf[i] == '\n'){
              write(1, "\r\n", sizeof("\r\n"));
              write(to_shell[1], "\n", 1);
            }else{
              write(1, &buf[i], 1);
              write(to_shell[1], &buf[i], 1);
            }
          }
        }
        if(pollfds[1].revents & POLLIN){
          char buf[256];
          int rw_status = read(from_shell[0], &buf, 256);
          for(int i = 0; i < rw_status; i++){
            if(buf[i] == '\r' || buf[i] == '\n'){
              write(1, "\r\n", sizeof("\r\n"));
            }else{
              write(1, &buf[i], 1);
            }
          }
        }
        if(pollfds[0].revents&(POLLHUP | POLLERR)){
          printf("Encountered a polling error\n");
          tcsetattr(0, TCSANOW, &old_mode);
          exit(1);
        }
        if(pollfds[1].revents&(POLLHUP | POLLERR)){//shell proccess ends via exit or some other method
          end_flag = 1;
        }
      }
      close(from_shell[0]);
      close(to_shell[1]);
      int wait_status;
      waitpid(ret, &wait_status, 0);
      printf("SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(wait_status), WEXITSTATUS(wait_status));
      tcsetattr(0, TCSANOW, &old_mode);
      exit(0);
    }
  }else{
    //read and write stuff
    //!!!without shell argument just turns the terminal into noncanonicalnoechomode
    char buf;
    ssize_t status = read(0, &buf, sizeof(buf));
    while(status > 0){
      if(buf == 0x4){
        if(debug_arg){
          status = write(1, "^D", sizeof("^D"));     //print eof char (optional)
        }
        if(tcsetattr(0, TCSANOW, &old_mode) == -1){//restore terminal
          fprintf(stderr, "Error, could not set terminal attributes:\n %s \n", strerror(errno));
        }
        exit(0);                          //exit with proper code
      }else if(buf == 0x3){
        if(debug_arg){
          status = write(1, "^C", sizeof("^C"));//no need to exit here
        }
      }else if(buf == '\r' || buf == '\n'){//cr lf thing?
        status = write(1, "\r\n", sizeof("\r\n"));
      }else{
        status = write(1, &buf, sizeof(buf));
      }
      status = read(0, &buf, sizeof(buf));
    }
    if(status == -1){
      fprintf(stderr, "Error, could not read/write input:\n %s \n", strerror(errno));
      exit(1);
    }
  }

exit(0);
}

/*
functions:
telnet like client and server
can be seperated into 2 parts:
  character at a time duplex terminal
  polled io and passing input and output between two processes

Arguments:
  --shell
  --debug
    error handling should occur without debug necesarry

Functions:
  termios(3), tcgetattr(3), tcsetattr(3), etc. ... for manipulating terminal attributes
  fork(2) ... for creating new processes
  waitpid(2) ... to allow one process to monitor another process's state, and react to changes in state
  exec(3) ... a family of calls for loading a new program into a running process
  pipe(2) ... for inter-process communication
  kill(3) ... for sending signals to processes by PID
  strerror(3) ... descriptions associated with system call errors
  poll(2) ... to wait for the first of multiple input sources

Goals:
  --put the keyboard (file descriptor 0) into character at a time no echo mode (non-canonical input mode with no echo)
    make the changes with the TCSANOW option.  Set up modes before anything else and reset before exiting
  --read input from keyboard into a buffer.  Can be one at a time or read multiple but make sure each individual character
  --map received <cr> or <lf> into <cr><lf> (see below for a note on carriage return, linefeed, and EOF).
  --write the received characters back out to the display, as they are processed.
  --upon detecting a pre-defined escape sequence (^D), restore normal terminal modes and exit (hint:
    do this by saving the normal terminal settings when you start up, and restoring them on exit).

Exit codes:
  0 normal execution shutdown on ^D
  1 unrecognized argument or system call failure

  tips:
    stty sane
    ctrl+j sane

    c_iflag = ISTRIP; //only lower 7 bits
	  c_oflag = 0;      //no processing
	   c_lflag = 0;		  //no processing
*/
