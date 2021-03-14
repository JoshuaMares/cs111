#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <mraa/gpio.h>
#include <mraa/aio.h>
#include <string.h>
#include <poll.h>
#include <ctype.h>
#include <math.h>

//#define B 4275
//#define R0 100000
sig_atomic_t volatile run_flag = 1;
mraa_gpio_context button;
mraa_aio_context temp_sensor;
const float B = 4275;
const float R0 = 100000;

void handle_exit(){
  //close the buttons, polls, and exit with proper status
  //good bad exit?
  mraa_gpio_close(button);
  mraa_aio_close(temp_sensor);
  exit(0);
}

void print_current_time(){
  struct timespec ts;
  struct tm *tm;
  clock_gettime(CLOCK_REALTIME, &ts);
  tm = localtime(&(ts.tv_sec));
  printf("%d:%d:%d ", tm->tm_hour, tm->tm_min, tm->tm_sec);
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

void handle_input(char* buf, char* scale, int* log_status, int* period){
  /*
    SCALE=F
    SCALE=C
    PERIOD=seconds
    STOP
    START
    LOG line of text
    OFF
  */
  if(strstr(buf, "SCALE=F")){
    printf("SCALE=F\n");
    *scale = 'F';
  }
  if(strstr(buf, "SCALE=C")){
    printf("SCALE=C\n");
    *scale = 'C';
  }
  char* pos = strstr(buf, "PERIOD=");
  if(pos){//a return val of null means no string found
    *period = atoi(pos+7);//starts reading right after the =
    printf("PERIOD=%i\n", *period);
    /*if(*period == 0){//couldnt be converted to int
      handle_exit();
    }*/
  }
  if(strstr(buf, "STOP")){
    printf("STOP\n");
    *log_status = 0;
  }
  if(strstr(buf, "START")){
    printf("START\n");
    *log_status = 1;
  }
  pos = strstr(buf, "LOG ");
  if(pos){
    int i = 0;
    while(isprint(pos[i])){//keep printing till we hit a newline
      putchar(pos[i]);
    }
    printf("\n");
  }
  if(strstr(buf, "OFF")){
    print_current_time();
    printf("SHUTDOWN\n");
    handle_exit();
  }
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
  print_current_time();
  printf("SHUTDOWN\n");
  mraa_gpio_close(button);
  mraa_aio_close(temp_sensor);
  exit(0);
}


int main(){
  int period = 1;//in seconds
  int log_status = 1;//0 stop 1 start
  char scale = 'F';
  int value;//temp value recieved

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
  while(run_flag){
    value = mraa_aio_read(temp_sensor);
    float temp_value = convert_to_scale(scale, value);

    clock_gettime(CLOCK_MONOTONIC, &curr_time);
    int diff = curr_time.tv_sec - prev_time.tv_sec;
    if(diff >= period && log_status){
      prev_time = curr_time;
      print_current_time();
      printf("%f\n", temp_value);
    }
    if(poll(&poll_in, 1, 0) > 0){
      read(1, buf, 256);
      handle_input(buf, &scale, &log_status, &period);
    }
  }

  mraa_gpio_close(button);
  mraa_aio_close(temp_sensor);

	return 0;
}
