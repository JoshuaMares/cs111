#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <mraa/gpio.h>
#include <mraa/aio.h>
#include <string.h>
#include <poll.h>
#include <math.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
//#include <errno.h>

//#define B 4275
//#define R0 100000
sig_atomic_t volatile run_flag = 1;
mraa_gpio_context button;
mraa_aio_context temp_sensor;
const float B = 4275;
const float R0 = 100000;
int ofd = 0;
int sockfd = -1;
int tls_ver = 0;
SSL_CTX* context = NULL;
SSL* ssl_client = NULL;
char ssl_buf[256];

int client_connect(char* host_name, unsigned int port){
  struct sockaddr_in serv_addr;
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct hostent* server = gethostbyname(host_name);
  memset(&serv_addr, 0, sizeof(struct sockaddr_in));
  serv_addr.sin_family = AF_INET;
  memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  serv_addr.sin_port = htons(port);
  connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
  return sockfd;
}

SSL_CTX* ssl_init(){
  SSL_CTX* newContext = NULL;
  SSL_library_init();
  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();
  newContext = SSL_CTX_new(TLSv1_client_method());
  return newContext;
}

SSL* attach_ssl_to_socket(int socket){
  SSL* sslClient = SSL_new(context);
  SSL_set_fd(sslClient, socket);
  SSL_connect(sslClient);
  return sslClient;
}

void ssl_clean_client(){
  SSL_shutdown(ssl_client);
  SSL_free(ssl_client);
}

void handle_exit(){
  //close the buttons, polls, and exit with proper status
  //good bad exit?
  ssl_clean_client();
  mraa_gpio_close(button);
  mraa_aio_close(temp_sensor);
  exit(0);
}

void print_current_time(int fd){
  struct timespec ts;
  struct tm *tm;
  clock_gettime(CLOCK_REALTIME, &ts);
  tm = localtime(&(ts.tv_sec));
  dprintf(fd, "%02d:%02d:%02d ", tm->tm_hour, tm->tm_min, tm->tm_sec);
}

void ssl_print_current_time(){
  struct timespec ts;
  struct tm *tm;
  clock_gettime(CLOCK_REALTIME, &ts);
  tm = localtime(&(ts.tv_sec));
  char time_buf[256];
  sprintf(time_buf, "%02d:%02d:%02d ", tm->tm_hour, tm->tm_min, tm->tm_sec);
  SSL_write(ssl_client, ssl_buf, strlen(ssl_buf));
}

float convert_to_scale(char scale, int value){
  float R = 1023.0 / ((float) value)-1.0;
  R *= R0;
  float C = 1.0/(log(R/R0)/B + 1/298.15) - 273.15;
  float F = (C*9)/5 + 32;
  if(scale == 'F'){
    return F;
  }else{
    return C;
  }
}

void handle_command(char* buf, char* scale, int* log_status, int* period){
  /*
    SCALE=F
    SCALE=C
    PERIOD=seconds
    STOP
    START
    LOG line of text
    OFF
  */
  char* pos = strstr(buf, "PERIOD=");
  char* pos2 = strstr(buf, "LOG ");
  if(pos2){
    /*if(tls_ver){
      sprintf(buf, "%s", pos2);
      SSL_write(ssl_client, buf, strlen(buf));
    }else{
      dprintf(sockfd, "%s", pos2);//may need newline
    }*/
    if(ofd){
      dprintf(ofd, "%s", pos2);//may need newline
    }
  }else if(pos){//a return val of null means no string found
    *period = atoi(pos+7);//starts reading right after the =
    /*if(tls_ver){
      sprintf(buf, "PERIOD=%i\n", *period);
      SSL_write(ssl_client, buf, strlen(buf));
    }else{
      dprintf(sockfd, "PERIOD=%i\n", *period);
    }*/
    if(ofd){
      dprintf(ofd, "PERIOD=%i\n", *period);
    }
  }else if(strstr(buf, "SCALE=F")){
    /*if(tls_ver){
      SSL_write(ssl_client, "SCALE=F\n", strlen("SCALE=F\n"));
    }else{
      dprintf(sockfd, "SCALE=F\n");
    }*/
    if(ofd){
      dprintf(ofd, "SCALE=F\n");
    }
    *scale = 'F';
  }else if(strstr(buf, "SCALE=C")){
    /*if(tls_ver){
      SSL_write(ssl_client, "SCALE=C\n", strlen("SCALE=C\n"));
    }else{
      dprintf(sockfd, "SCALE=C\n");
    }*/
    if(ofd){
      dprintf(ofd, "SCALE=C\n");
    }
    *scale = 'C';
  }else if(strstr(buf, "STOP")){
    /*if(tls_ver){
      SSL_write(ssl_client, "STOP\n", strlen("STOP\n"));
    }else{
      dprintf(sockfd, "STOP\n");
    }*/
    if(ofd){
      dprintf(ofd, "STOP\n");
    }
    *log_status = 0;
  }else if(strstr(buf, "START")){
    /*if(tls_ver){
      SSL_write(ssl_client, "START\n", strlen("START\n"));
    }else{
      dprintf(sockfd, "START\n");
    }*/
    if(ofd){
      dprintf(ofd, "START\n");
    }
    *log_status = 1;
  }else if(strstr(buf, "OFF")){
    if(tls_ver){
      //SSL_write(ssl_client, "OFF\n", strlen("OFF\n"));
      ssl_print_current_time();
      SSL_write(ssl_client, "SHUTDOWN\n", strlen("SHUTDOWN\n"));
    }else{
      //dprintf(sockfd, "OFF\n");
      print_current_time(sockfd);
      dprintf(sockfd, "SHUTDOWN\n");
    }
    if(ofd){
      dprintf(ofd, "OFF\n");
      print_current_time(ofd);
      dprintf(ofd, "SHUTDOWN\n");
    }
    handle_exit();
  }else{
    /*
    if(tls_ver){
      SSL_write(ssl_client, buf, strlen(buf));
    }else{
      dprintf(sockfd, "%s\n", buf);
    }
    if(ofd){
      dprintf(ofd, "%s\n", buf);
    }
    */
  }
  return;
}

void handle_input(char* buf, char* scale, int* log_status, int* period){
  /*
  add each char to command buffer
  when we reach a newline or eof indicator handle command
  reset command length
  */
  char command_buf[256];
  int clength = 0;
  int i;
  for(i = 0; i < 256; i++){
    command_buf[clength] = buf[i];
    clength++;
    if(buf[i] == '\n'){
      command_buf[clength] = '\0';
      handle_command(command_buf, scale, log_status, period);
      clength = 0;
    }
    if(buf[i] == '\0'){
      command_buf[clength] = '\0';
      handle_command(command_buf, scale, log_status, period);
      break;
    }
  }
  return;
  //handle_command(buf, scale, log_status, period);
  //return;
}

/*
void do_when_interrupted(){
  if(sig == SIGINT){
    run_flag = 0;
  }
}
*/

void button_press(){
  //run_flag = 0;
  if(tls_ver){
    ssl_print_current_time();
    SSL_write(ssl_client, "SHUTDOWN\n", strlen("SHUTDOWN\n"));
  }else{
    print_current_time(sockfd);
    dprintf(sockfd, "SHUTDOWN\n");
  }

  if(ofd){
    print_current_time(ofd);
    dprintf(ofd, "SHUTDOWN\n");
  }
  ssl_clean_client();
  mraa_gpio_close(button);
  mraa_aio_close(temp_sensor);
  exit(0);
}

int main(int argc, char **argv){
  //command line variables
  int period = 1;//in seconds
  char scale = 'F';
  char* log_name = NULL;
  char* id_string = NULL;
  char* host_name = NULL;
  unsigned int port_number = 0;

  int log_status = 1;//0 stop 1 start
  int value;//temp value recieved

  //arg parsing
  int option_index = 0;
  int option_short;

  static struct option long_options[] = {
    {"period",      required_argument,    0,    'p'},
    {"scale",       required_argument,    0,    's'},
    {"log",         required_argument,    0,    'l'},
    {"id",          required_argument,    0,    'i'},
    {"host",        required_argument,    0,    'h'},
    {0,             0,                    0,     0}
  };
  while((option_short = getopt_long(argc, argv, "p:s:l:i:h:", long_options, &option_index)) != -1){
    switch(option_short){
      case 'p':
        period = atoi(optarg);
        break;
      case 's':
        scale = optarg[0];
        switch(scale){
          case 'F':
            break;
          case 'C':
            break;
          default:
            fprintf(stderr, "Unrecognized scale argument\n");
            exit(1);
            break;
        }
        break;
      case 'l':
        log_name = optarg;
        ofd = creat(log_name, 0666);
        if(ofd < 0){
          fprintf(stderr, "Error opening file: %s\n", log_name);
          exit(1);
        }
        break;
      case 'i':
        id_string = optarg;
        //if string length isnt correct exit
        if(strlen(id_string) != 9){
          fprintf(stderr, "id argument must be 9 digits long\n");
          exit(1);
        }
        break;
      case 'h':
        host_name = optarg;
        if(strcmp(host_name, "lever.cs.ucla.edu")){
          fprintf(stderr, "Host name not lever.cs.ucla.edu\n");
          exit(1);
        }
        break;
      case '?':
        fprintf(stderr, "Unrecognized option.\n");
        exit(1);
        break;
    }
  }
  port_number = (unsigned int) atoi(argv[argc-1]);//last arg should be port number
  if(port_number == 0){
    fprintf(stderr, "Error reading port number\n");
  }
  //printf("Exec name is: %s\n", argv[0]);
  if(!strcmp(argv[0], "./lab4c_tls")){
    //printf("TLS option chosen\n");
    tls_ver = 1;
  }


  //connection initialization
  sockfd = client_connect(host_name, port_number);
  if(tls_ver){
    context = ssl_init();
    ssl_client = attach_ssl_to_socket(sockfd);
  }
  if(tls_ver){
    sprintf(ssl_buf, "ID=%s\n", id_string);
    SSL_write(ssl_client, ssl_buf, strlen(ssl_buf));
  }else{
    dprintf(sockfd, "ID=%s\n", id_string);
  }

  //initiate temp_sensor and button
  button = mraa_gpio_init(60);
  temp_sensor = mraa_aio_init(1);
  mraa_gpio_dir(button, MRAA_GPIO_IN);
  mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &button_press, NULL);
  //signal(SIGINT, do_when_interrupted);

  //poll setup
  struct pollfd poll_in = {sockfd, POLLIN, 0};//reading from sockfd
  char buf[256];

  //time vars
  struct timespec curr_time, prev_time;
  clock_gettime(CLOCK_MONOTONIC, &prev_time);

  //first reading before any input can be proccessed
  value = mraa_aio_read(temp_sensor);
  float temp_value = convert_to_scale(scale, value);
  if(tls_ver){
    ssl_print_current_time();
    sprintf(ssl_buf, "%3.1f\n", temp_value);
    SSL_write(ssl_client, ssl_buf, strlen(ssl_buf));
  }else{
    print_current_time(sockfd);
    dprintf(sockfd, "%3.1f\n", temp_value);
  }
  if(ofd){
    print_current_time(ofd);
    dprintf(ofd, "%3.1f\n", temp_value);
  }

  //logic loop
  while(run_flag){
    value = mraa_aio_read(temp_sensor);
    temp_value = convert_to_scale(scale, value);

    clock_gettime(CLOCK_MONOTONIC, &curr_time);
    int diff = curr_time.tv_sec - prev_time.tv_sec;
    if(diff >= period && log_status){
      prev_time = curr_time;
      if(tls_ver){
        ssl_print_current_time();
        sprintf(ssl_buf, "%3.1f\n", temp_value);
        SSL_write(ssl_client, ssl_buf, strlen(ssl_buf));
      }else{
        print_current_time(sockfd);
        dprintf(sockfd, "%3.1f\n", temp_value);
      }
      if(ofd){
        print_current_time(ofd);
        dprintf(ofd, "%3.1f\n", temp_value);
      }
    }
    if(poll(&poll_in, 1, 0) > 0){
      if(tls_ver){
        int len = SSL_read(ssl_client, buf, 256);
        buf[len] = '\0';
      }else{
        int len = read(sockfd, buf, 256);
        buf[len] = '\0';
      }
      //printf("input is:%s\n", buf);
      handle_input(buf, &scale, &log_status, &period);
    }
  }

  ssl_clean_client();
  mraa_gpio_close(button);
  mraa_aio_close(temp_sensor);

	return 0;
}
