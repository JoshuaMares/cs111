#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <mraa/gpio.h>
#include <mraa/aio.h>

#define B 4275
#define R0 100000.0

sig_atomic_t volatile run_flag = 1;
mraa_gpio_context button;
mraa_aio_context temp_sensor;

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

void convert_to_scale(char scale, uint16_t* value){
  float R = 1023.0 / ((float) value)-1.0;
  R *= R0;
  float C = 1.0/(log(R/R0)/B + 1/298.15) - 273.15;
  float F = (C*9)/5 + 32;
  if(scale == 'F'){
    *value = F;
  }else{
    *value = C;
  }
  return;
}

void print_message(char* buf, char* scale, int* log_status, int* period){
  /*
    SCALE=F
    SCALE=C
    PERIOD=seconds
    STOP
    START
    LOG line of text
    OFF
  */
  if(!strcmp(buf, "SCALE=F")){
    printf("%s\n", buf);
    *scale = F;
  }else if(!strcmp(buf, "SCALE=C")){
    printf("%s\n", buf);
    *scale = C;
  }else if(!memcmp(buf, "PERIOD=", 7)){
    printf("%s\n", buf);
    *period = atoi(buf+7);
    if(*period == 0){
      //couldnt be converted to int
      handle_exit();
    }
  }else if(!strcmp(buf, "STOP")){
    printf("%s\n", buf);
    log_status = 0;
  }else if(!strcmp(buf, "START")){
    printf("%s\n", buf);
    log_status = 0;
  }else if(!memcmp(buf, "LOG ")){
    printf("%s\n", buf);
  }else if(!strcmp(buf, "OFF")){
    print_current_time();
    printf("SHUTDOWN\N");
    handle_exit();
  }else{
    //some invalid stuff
  }

  print_current_time();
  print
}

/*
void do_when_interrupted(){
  if(sig == SIGINT){
    run_flag = 0;
  }
}
*/

void button_press(){
  run_flag = 0;
}

int main(){
  int period = 1;//in seconds
  int log_status = 1;//0 stop 1 start
  char scale = 'F';

  uint16_t value;//temp value recieved

  //initiate temp_sensor and button
  mraa_gpio_dir(button, MRAA_GPIO_IN);
  mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &button_press, NULL);
  temp_sensor = mraa_aio_init(1);
  //signal(SIGINT, do_when_interrupted);

  while(run_flag){
    value = mraa_aio_read(temp_sensor);
    convert_to_scale(scale, &value);
    if(log_status){
      print_message();
    }
    sleep(period);
  }

  mraa_gpio_close(button);
  mraa_aio_close(temp_sensor);

	return 0;
}
