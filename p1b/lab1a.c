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
Arguments:
  client.c:
    --get rid of shell
    --port=port#
      mandatory argument
      open connection to sever rather than shell
      send input from keyboard to socket, while echoing to the display, and
        input from socket to display
      same noncanonicalnoechomode
        ^D and ^c are passed to server like any other character
    --log=filename
      maintains records of data sent over the socket
      all data written to and read from server should be logged in the format
        SENT # bytes: message \n
        RECEIVED # bytes: message \n
      use ulimit 10000, also check man page

    --compress
    --host=name (optional)
      normal program shell should run on same host
      this allows us to access a shell session from other computers
  server.c
    --port=port#
      connect with client and send them to shell and send the client with outputs from shell
      accept a connection when made
      once a connection is made do the forking and piping stuff like p1a
      input received through the socket should be forwarded to shell
      same handling of p1a with shell arg 
      input received from shell should be forwarded to socket
    --compress

Functions:
  socket(7) ... for creating communication endpoints (networking)
  zlib(3) and the zlib tutorial ... a library for peforming gzip compression and
    decompression (based on a combination of LZW and Huffman coding)

Goals:
  --pass input and output over a tcp socket
  --compress communication between client and server

Exit codes:
  client:
    0-successfull exit (reminder to bring back normal terminal modes)
    1-unrecognized argument, failed syscall (include stderr message)
  server:
    0-eof, sigpipe from shell, eof from client, or read error from client
      (remember to print shell completion status to stderr)
    1-bad arg or failed sys call

Tips:
  client.c ->server.c which is ran on a server and does the piping stuff with shell
  need to start both for both to work properly
  choose random prot number > 1024 for testing
  stderr and stdout to terminal from which it was started
  Shut down order:
    exit(1) is sent to shell and server closes write pipe to shell
    shell exits, sending eof to server terminal
    server collects and reports shells termination status
    server closes socket to client and exits
    client continues to process output from server until i recieves error from socket
    client restores terminal modes and exits


*/
