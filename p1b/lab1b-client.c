#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>
#include <getopt.h>
//#include <signal.h>
#include <termios.h>
#include <netdb.h>
#include <poll.h>
#include <zlib.h>
#include <ulimit.h>
#include <fcntl.h>
#include <ulimit.h>

#define BUF_SIZE 256

char post_buf[BUF_SIZE];

struct termios old_mode;
z_stream compressor;
z_stream decompressor;

void reset_terminal(int reset){
  if(!reset){
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
      fprintf(stderr, "Error, could not set new terminal attributes:\n %s \n", strerror(errno));
    }
  }

  if(tcsetattr(0, TCSANOW, &old_mode) == -1){//implements any changes
    fprintf(stderr, "Error, could not reset terminal attributes:\n %s \n", strerror(errno));
  }
}

void init_compress_stream(){
  compressor.zalloc =  Z_NULL;
  compressor.zfree =   Z_NULL;
  compressor.opaque =  Z_NULL;
  compressor.avail_in =     0;
  compressor.next_in = Z_NULL;
  if(deflateInit(&compressor, Z_DEFAULT_COMPRESSION) != Z_OK){
    fprintf(stderr, "Error, deflateInit failed\n%s\n", strerror(errno));
    reset_terminal(1);
    exit(1);
  }

  decompressor.zalloc =  Z_NULL;
  decompressor.zfree =   Z_NULL;
  decompressor.opaque =  Z_NULL;
  decompressor.avail_in =     0;
  decompressor.next_in = Z_NULL;
  if(inflateInit(&decompressor) != Z_OK){
    fprintf(stderr, "Error, inflateInit failed\n%s\n", strerror(errno));
    reset_terminal(1);
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

int client_connect(char* hostname, unsigned int port){
  /*
    struct sockaddr_in{
      short sin_family;        //addr family, af_inet
      unsigned short sin_port;//port number
      struct in_addr sin_addr;//internet address
      unsigned char sin_zero[8];//padding zeros
    }
  */
  /*int sockfd;
  struct sockaddr_in serv_addr;
  struct hostent* server;
  //create socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  //fill in socket address info
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  server = gethostbyname(hostname);//convert hostname to ip addr
  memset(&serv_addr, '\0', sizeof(struct sockaddr_in));//fill with zeros
  memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);//copy ip from server to serv_addr
  */
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct hostent *server = gethostbyname(hostname);
  struct sockaddr_in serv_addr;
  bzero((char *) &serv_addr, sizeof(serv_addr));
  bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  //connect socket with corresponding address
  if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1){
    fprintf(stderr, "Error, could not connect as client\n%s\n", strerror(errno));
  }

  return sockfd;
}

int main(int argc, char **argv){
  /*Overview
    socket
    connect->
    read/write<->
    close
  */
  int log_fd =                -1;
  unsigned int port_arg =      0;//1025
  char* log_arg =           NULL;
  int compress_arg =           0;
  char* host_arg =          NULL;
  int debug_arg =              0;
  int option_index =           0;
  int option_short;
  static struct option long_options[] = {
    {"port",      required_argument,    0,    'p'},
    {"log",       required_argument,    0,    'l'},
    {"host",      required_argument,    0,    'h'},
    {"compress",  no_argument,          0,    'c'},
    {"debug",     no_argument,          0,    'd'},
    {0,           0,                    0,     0 }
  };

  while((option_short = getopt_long(argc, argv, "p:l:h:cd", long_options, &option_index)) != -1){
    switch(option_short){
      case 'p':
        port_arg = atoi(optarg);
        break;
      case 'l':
        log_arg = optarg;
        log_fd = creat(log_arg, 0666);
        if(log_fd == -1){
          fprintf(stderr, "Error, could not create writable file: %s\n%s\n", log_arg, strerror(errno));
          exit(1);
        }
        if(ulimit(UL_SETFSIZE, 10000) == -1){
          fprintf(stderr, "Error, could not set new ulimit\n%s\n", strerror(errno));
          exit(1);
        }
        break;
      case 'h':
        host_arg = optarg;
        break;
      case 'c':
        compress_arg = 1;
        init_compress_stream();
        break;
      case 'd':
        debug_arg = 1;
        break;
      case '?':
        printf("Unrecognized option.\nValid option(s) include:\n\t--port\n\t--log\n\t--host\n\t--compress\n\t--debug\n");
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
  if(debug_arg && log_arg){
    printf("Log argument chosen\n");
  }
  if(debug_arg && host_arg){
    printf("Host argument chosen\n");
  }
  if(debug_arg && compress_arg){
    printf("Compress argument chosen\n");
  }

  int sockfd;
  if(host_arg){
    sockfd = client_connect(host_arg, port_arg);
  }else{
    sockfd = client_connect("localhost", port_arg);
  }

  if(debug_arg){
    printf("Connected to server: %i", sockfd);
  }
  reset_terminal(0);
  //read from input
  //write to sockfd (terminal)
  //can also read from sockfd is 2 way
  //use poll
  /*
  while(1){
    poll the sockfd and stdin
    if(sockfd is ready to read)
      read from sockfd and send to stdout/file
    if(stdin is ready to read)
      read from stdin and send to stdout/file/sock
    if(err)
      do error handling
  }
  */

  struct pollfd pollfds[2];
  pollfds[0].fd = 0;//input
  pollfds[0].events = POLLIN + POLLHUP + POLLERR;
  pollfds[1].fd = sockfd;
  pollfds[1].events = POLLIN + POLLHUP + POLLERR;

  int end_flag = 0;
  while(end_flag != 1){//while we havent exited
    if(poll(pollfds, 2, -1) == -1){
      fprintf(stderr, "Encountered a polling error:\n%s\n", strerror(errno));
      reset_terminal(1);
      exit(1);
    }
    if(pollfds[0].revents & POLLIN){//input
      char buf[BUF_SIZE];
      int rw_status = read(0, &buf, BUF_SIZE);
      if(rw_status == -1){
        fprintf(stderr, "Encountered a read error:\n%s\n", strerror(errno));
        reset_terminal(1);
        exit(1);
      }
      strcpy(buf, compress_buf(compress_arg, 'c', buf, &rw_status));
      for(int i = 0; i < rw_status; i++){
        write(1, &buf[i], 1);//write to stdout
        write(sockfd, &buf[i], 1);
      }
      if(log_arg){
        if(dprintf(log_fd, "RECEIVED %i bytes: %s\n", rw_status, buf) == -1){
          fprintf(stderr, "Error, could not write to file: %s\n%s\n", log_arg, strerror(errno));
          reset_terminal(1);
          exit(1);
        }
      }
    }
    if(pollfds[1].revents & POLLIN){//sockfd
      char buf[BUF_SIZE];
      int rw_status = read(sockfd, &buf, BUF_SIZE);
      if(rw_status == -1){
        fprintf(stderr, "Encountered a read error:\n%s\n", strerror(errno));
        reset_terminal(1);
        exit(1);
      }
      if(log_arg){
        if(dprintf(log_fd, "RECEIVED %i bytes: %s\n", rw_status, buf) == -1){
          fprintf(stderr, "Error, could not write to file: %s\n%s\n", log_arg, strerror(errno));
          reset_terminal(1);
          exit(1);
        }
      }
      strcpy(buf, compress_buf(compress_arg, 'd', buf, &rw_status));
      for(int i = 0; i < rw_status; i++){
        write(1, &buf[i], 1);
      }
    }
    if(pollfds[0].revents&(POLLHUP | POLLERR)){//err from stdin
      printf("Encountered a polling error\n");
      reset_terminal(1);
      exit(1);
    }
    if(pollfds[1].revents&(POLLHUP | POLLERR)){//err rom socket
      end_flag = 1;
    }
  }
  if(close(sockfd) == -1){
    fprintf(stderr, "Error, could not close socket:\n %s\n", strerror(errno));
    reset_terminal(1);
    exit(1);
  }
  end_compress_stream();
  reset_terminal(1);
  exit(0);
}
