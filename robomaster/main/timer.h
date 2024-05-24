#ifndef TIMER_H
#define TIMER_H

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "can.h"

static TimerHandle_t timer10ms = NULL;
static TimerHandle_t timer100ms = NULL;
static TimerHandle_t timer1000ms = NULL;
static TimerHandle_t cycle = NULL;

static TimerHandle_t timerfire = NULL;

void timer(void * parameters);
void callback_10ms(TimerHandle_t xTimer);
void callback_100ms(TimerHandle_t xTimer);
void callback_1000ms(TimerHandle_t xTimer);

void callback_fire(TimerHandle_t xTimer);

#endif
