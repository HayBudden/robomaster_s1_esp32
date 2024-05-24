#include "timer.h"
#include "can.h"
#include <stdio.h>

void timer(void * parameters){

}

void callback_10ms(TimerHandle_t xTimer){
    // printf("10ms tick\n");
    send_command(5); //drive
    send_command(4); //gimball
}

void callback_100ms(TimerHandle_t xTimer){
    send_command(0); //0x0D
    send_command(1); //0x0E
    send_command(2); //0x0F
}

void callback_1000ms(TimerHandle_t xTimer){
    send_command(3); //0x12
    send_command(6); //0x49
}

void callback_fire(TimerHandle_t xTimer){
    // send_command(7); //bead
    // send_command(8); //laser
}


