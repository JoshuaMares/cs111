#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <zlib.h>

#define BUF_SIZE 256

char post_buf[BUF_SIZE];

int debug_arg =     0;
z_stream compressor;
z_stream decompressor;

void init_compress_stream(){
  compressor.zalloc =  Z_NULL;
  compressor.zfree =   Z_NULL;
  compressor.opaque =  Z_NULL;
  compressor.avail_in =     0;
  compressor.next_in = Z_NULL;
  if(deflateInit(&compressor, Z_DEFAULT_COMPRESSION) != Z_OK){
    fprintf(stderr, "Error, deflateInit failed\n%s\n", strerror(errno));
    exit(1);
  }

  decompressor.zalloc =  Z_NULL;
  decompressor.zfree =   Z_NULL;
  decompressor.opaque =  Z_NULL;
  decompressor.avail_in =     0;
  decompressor.next_in = Z_NULL;
  if(inflateInit(&decompressor) != Z_OK){
    fprintf(stderr, "Error, inflateInit failed\n%s\n", strerror(errno));
    exit(1);
  }
  return;
}

void end_compress_stream(){
  deflateEnd(&compressor);
  inflateEnd(&decompressor);
}

char* compress_buf(int compress_arg, char cd, char* og_buf, int* buf_length){
    if(!compress_arg){
      return og_buf;
    }

    z_stream temp_stream;
    if(cd == 'c'){
      temp_stream = compressor;
    }else{
      temp_stream = decompressor;
    }

    temp_stream.avail_in = *buf_length;
    temp_stream.next_in = (unsigned char *) og_buf;
    temp_stream.avail_out = sizeof(post_buf);
    temp_stream.next_out = (unsigned char *) post_buf;
    if(cd == 'c'){
      do{
        (void) deflate(&temp_stream, Z_SYNC_FLUSH);
      }while(temp_stream.avail_in > 0);
    }else{
      do{
        (void) inflate(&temp_stream, Z_SYNC_FLUSH);
      }while(temp_stream.avail_in > 0);
    }

    *buf_length = BUF_SIZE - temp_stream.avail_out;
    return post_buf;
}

int server_connect(unsigned int port_num){
  if(debug_arg){
    printf("Entered server_connect function\n");
  }

  int sockfd, new_fd;//listen on sockfd, new connection on newfd
  struct sockaddr_in my_addr;//my addr
  struct sockaddr_in their_addr;//connection addr
  socklen_t sin_size;

  //create socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(debug_arg){
    printf("Socket created\n");
  }

  //set addr info
  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(port_num);//short network byte order
  my_addr.sin_addr.s_addr = INADDR_ANY;//inaddr_any allows clients to connect to any of the hosts ip addrs

  memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));

  //bind the socket to the ip address and port number
  bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr_in));
  if(debug_arg){
    printf("Binded\n");
  }

  if(debug_arg){
    printf("Begin listening\n");
  }
  listen(sockfd, 5);//max 5 pending connections
  if(debug_arg){
    printf("Done listening\n");
  }

  sin_size = sizeof(struct sockaddr_in);
  //wait for clients connection, their addr stores client addrs
  new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
  if(debug_arg){
    printf("Accepted connection\n");
  }
  return new_fd;
}

void catch_fun(int signal_message){
  if(signal_message == SIGPIPE){
    fprintf(stderr, "Broken Pipe\n");
    exit(0);
  }
}

int main(int argc, char **argv){
  /*Overview
    socket
    bind
    listen
    ->accept
    <->read/wrtie
    close
  */
  unsigned int port_arg =      0;
  int compress_arg =           0;
  int option_index =           0;
  int option_short;
  static struct option long_options[] = {
    {"port",      required_argument,    0,    'p'},
    {"compress",  no_argument,          0,    'c'},
    {"debug",     no_argument,          0,    'd'},
    {0,           0,                    0,     0 }
  };

  while((option_short = getopt_long(argc, argv, "p:cd", long_options, &option_index)) != -1){
    switch(option_short){
      case 'p':
        port_arg = atoi(optarg);
        break;
      case 'c':
        compress_arg = 1;
        init_compress_stream();
        break;
      case 'd':
        debug_arg = 1;
        break;
      case '?':
        printf("Unrecognized option.\nValid option(s) include:\n\t --port\n\t --compress\n\tdebug\n");
        exit(1);
        break;
    }
  }

  if(!port_arg){
    printf("--port is a mandatory argument.\n");
    exit(1);
  }

  if(debug_arg){
    printf("Debug argument chosen.\n");
  }
  if(debug_arg && compress_arg){
    printf("Compress argument chosen\n");
  }

  if(debug_arg){
    printf("About to enter server_connect function\n");
  }
  int sockfd = server_connect(port_arg);
  if(debug_arg){
    printf("Connected to client\n");
  }
  //read(sockfd, bufferm, sizof)
  //write to sockfd and into shell program
  //poll twice - client and shell
  /*
  while(1){
    if(sockfd ready to read)
      read from sockfd, send to to_shell[1] //look at slides for pipes
    if(from_shell[0] ready to read)
      read from from_shell[0], send to sockfd
    if(err)
      error handling
  }

  */
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
    pollfds[0].fd = sockfd;
    pollfds[0].events = POLLIN + POLLHUP + POLLERR;
    pollfds[1].fd = from_shell[0];
    pollfds[1].events = POLLIN + POLLHUP + POLLERR;

    int end_flag = 0;
    while(end_flag != 1){//while we havent exited
      if(poll(pollfds, 2, -1) == -1){
        fprintf(stderr, "Encountered a polling error:\n%s\n", strerror(errno));
        exit(1);
      }
      if(pollfds[0].revents & POLLIN){//read from socket
        char buf[256];
        int rw_status = read(sockfd, &buf, 256);
        if(rw_status == -1){
          fprintf(stderr, "Encountered a read error:\n%s\n", strerror(errno));
          exit(1);
        }
        strcpy(buf, compress_buf(compress_arg, 'd', buf, &rw_status));
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
      if(pollfds[1].revents & POLLIN){//read from shell
        char buf[256];
        int rw_status = read(from_shell[0], &buf, 256);
        if(rw_status == -1){
          fprintf(stderr, "Encountered a read error:\n%s\n", strerror(errno));
          exit(1);
        }
        strcpy(buf, compress_buf(compress_arg, 'c', buf, &rw_status));
        for(int i = 0; i < rw_status; i++){//omitting /n/r cause I couldnt figure out how in time
          write(sockfd, &buf[i], 1);
        }
      }
      if(pollfds[0].revents&(POLLHUP | POLLERR)){
        printf("Encountered a polling error\n");
        exit(1);
      }
      if(pollfds[1].revents&(POLLHUP | POLLERR)){//shell proccess ends via exit or some other method
        end_flag = 1;
      }
    }
    close(from_shell[0]);
    close(to_shell[1]);
    end_compress_stream();
    int wait_status;
    waitpid(ret, &wait_status, 0);
    printf("SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(wait_status), WEXITSTATUS(wait_status));
    close(sockfd);
    exit(0);
  }

exit(0);
}
