#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <termios.h>

void catch_eof(int signal_message){
  if(signal_message == SIGINT){//enter actual signal message
    //reset original settings?
    exit(1);
  }
}

int main(int argc, char **argv){
  //

  /*Save mode and then non-canonical input no echo mode*/
  struct termios old_mode;//creates the struct
  struct termios new_mode;
  if(tcgetattr(0, &old_mode) < 0){//copies curr fd (which should be terminal) settings
    fprintf(stderr, "Could not get attributes\n", strerror(errno));
    exit(1);
  }
  new_mode = old_mode;
  new_mode.c_iflag = ISTRIP;//declares new settings
  new_mode.c_oflag = 0;
  new_mode.c_lflag = 0;
  tcsetattr(0, TCSANOW, &new_mode);//implements any changes

  //getting args
  int print_err = 0;
  static struct option long_options[] = {
    {"shell",   no_argument,    0,    's'},
    {"debug",   no_argument,    0,    'd'},
    {0,         0,              0,     0 }
  };

  while((option_short = getopt_long(argc, argv, "s", long_options, &option_index)) != -1){
    switch(option_short){
      case 's':
        //do the thing
        break;
      case '?':
        printf("Unrecognized option.\nValid option(s) include:\n\t --shell");
        break;
    }

  }

}

/*
functions:
telnet like client and server
can be seperated into 2 parts:
  character at a time duplex terminal
  polled io and passing input and output between two processes

Arguments:
  --shell

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
