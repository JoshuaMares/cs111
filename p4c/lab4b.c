#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <mraa/gpio.h>
#include <mraa/aio.h>
#include <string.h>
#include <poll.h>
//#include <ctype.h>
#include <math.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <errno.h>

//#define B 4275
//#define R0 100000
sig_atomic_t volatile run_flag = 1;
mraa_gpio_context button;
mraa_aio_context temp_sensor;
const float B = 4275;
const float R0 = 100000;
int ofd = 0;

void handle_exit(){
  //close the buttons, polls, and exit with proper status
  //good bad exit?
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
    printf("%s", pos2);//may need newline
    if(ofd){
      dprintf(ofd, "%s", pos2);//may need newline
    }
  }else if(pos){//a return val of null means no string found
    *period = atoi(pos+7);//starts reading right after the =
    printf("PERIOD=%i\n", *period);
    if(ofd){
      dprintf(ofd, "PERIOD=%i\n", *period);
    }
    /*if(*period == 0){//couldnt be converted to int
      printf("Period line is: %s\n", pos);
      handle_exit();
    }*/
  }else if(strstr(buf, "SCALE=F")){
    printf("SCALE=F\n");
    if(ofd){
      dprintf(ofd, "SCALE=F\n");
    }
    *scale = 'F';
  }else if(strstr(buf, "SCALE=C")){
    printf("SCALE=C\n");
    if(ofd){
      dprintf(ofd, "SCALE=C\n");
    }
    *scale = 'C';
  }else if(strstr(buf, "STOP")){
    printf("STOP\n");
    if(ofd){
      dprintf(ofd, "STOP\n");
    }
    *log_status = 0;
  }else if(strstr(buf, "START")){
    printf("START\n");
    if(ofd){
      dprintf(ofd, "START\n");
    }
    *log_status = 1;
  }else if(strstr(buf, "OFF")){
    printf("OFF\n");
    print_current_time(1);
    printf("SHUTDOWN\n");
    if(ofd){
      dprintf(ofd, "OFF\n");
      print_current_time(ofd);
      dprintf(ofd, "SHUTDOWN\n");
    }
    handle_exit();
  }else{
    printf("%s\n", buf);
    if(ofd){
      dprintf(ofd, "%s\n", buf);
    }
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
  print_current_time(1);
  printf("SHUTDOWN\n");
  if(ofd){
    print_current_time(ofd);
    dprintf(ofd, "SHUTDOWN\n");
  }
  mraa_gpio_close(button);
  mraa_aio_close(temp_sensor);
  exit(0);
}


int main(int argc, char **argv){
  int period = 1;//in seconds
  int log_status = 1;//0 stop 1 start
  char scale = 'F';
  char* log_name = NULL;
  int value;//temp value recieved
  int option_index = 0;
  int option_short;

  static struct option long_options[] = {
    {"period",      required_argument,    0,    'p'},
    {"scale",       required_argument,    0,    's'},
    {"log",         required_argument,    0,    'l'},
    {0,             0,                    0,     0}
  };
  while((option_short = getopt_long(argc, argv, "p:s:l:", long_options, &option_index)) != -1){
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
            printf("Unrecognized scale argument\n");
            exit(1);
            break;
        }
        break;
      case 'l':
        log_name = optarg;
        ofd = creat(log_name, 0666);
        if(ofd < 0){
          fprintf(stderr, "Error opening file: %s\n", log_name);
        }
        break;
      case '?':
        printf("Unrecognized option.\n");
        exit(1);
        break;
    }
  }

  //initiate temp_sensor and button
  button = mraa_gpio_init(60);
  temp_sensor = mraa_aio_init(1);
  mraa_gpio_dir(button, MRAA_GPIO_IN);
  mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &button_press, NULL);
  //signal(SIGINT, do_when_interrupted);

  //poll setup
  struct pollfd poll_in = {0, POLLIN, 0};
  char buf[256];

  //time vars
  struct timespec curr_time, prev_time;
  clock_gettime(CLOCK_MONOTONIC, &prev_time);

  //first reading before any input can be proccessed
  value = mraa_aio_read(temp_sensor);
  float temp_value = convert_to_scale(scale, value);
  print_current_time(1);
  printf("%3.1f\n", temp_value);
  if(ofd){
    print_current_time(ofd);
    dprintf(ofd, "%3.1f\n", temp_value);
  }

  while(run_flag){
    value = mraa_aio_read(temp_sensor);
    temp_value = convert_to_scale(scale, value);

    clock_gettime(CLOCK_MONOTONIC, &curr_time);
    int diff = curr_time.tv_sec - prev_time.tv_sec;
    if(diff >= period && log_status){
      prev_time = curr_time;
      print_current_time(1);
      printf("%3.1f\n", temp_value);
      if(ofd){
        print_current_time(ofd);
        dprintf(ofd, "%3.1f\n", temp_value);
      }
    }
    if(poll(&poll_in, 1, 0) > 0){
      int len = read(0, buf, 256);
      buf[len] = '\0';
      //printf("input is:%s\n", buf);
      handle_input(buf, &scale, &log_status, &period);
    }
  }

  mraa_gpio_close(button);
  mraa_aio_close(temp_sensor);

	return 0;
}
