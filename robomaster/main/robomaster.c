#include <stdio.h>
#include <stdint.h>
#include "driver/twai.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "hal/twai_types.h"
#include "portmacro.h"
#include "can.h"
#include "timer.h"
#include <inttypes.h>


message_data_t movement_data = {};


void app_main(void)
{

    movement_data.x = 1024;
    movement_data.y = 1024;
    movement_data.z = 1024;
    movement_data.yaw = 0;
    movement_data.roll = 0;

    //Wait for Robomaster to boot
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    xTaskCreate(comms_init, "Init ESP-NOW", 8196, NULL, 1, NULL);
    xTaskCreate(twai_init, "Init CAN BUS", 8196, NULL, 2, NULL);
    xTaskCreate(twai_send, "CAN message send loop", 8196, NULL, 1, NULL);
    xTaskCreate(boot, "Send boot commands", 8196, NULL, 1, NULL);
    xTaskCreate(create_semaphore, "Create semaphore", 8196, NULL, 1, NULL);
    printf("Ready\n");

    timer10ms = xTimerCreate("Timer10ms", 10 / portTICK_PERIOD_MS, pdTRUE, NULL, callback_10ms);
    timer100ms = xTimerCreate("Timer100ms", 100 / portTICK_PERIOD_MS, pdTRUE, NULL, callback_100ms);
    timer1000ms = xTimerCreate("Timer1000ms", 1000 / portTICK_PERIOD_MS, pdTRUE, NULL, callback_1000ms);

    xTimerStart(timer10ms, portMAX_DELAY);
    xTimerStart(timer100ms, portMAX_DELAY);
    xTimerStart(timer1000ms, portMAX_DELAY);

    while(1){}
}
