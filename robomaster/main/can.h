#ifndef CAN_H
#define CAN_H

#include <stdint.h>
#include <stdio.h>
#include "driver/twai.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>

#define TABLE_ROWS 20
#define TABLE_COLUMNS 80

typedef struct{
    int msg_count;
    int length;
    char data[TABLE_COLUMNS];
}message_struct;

typedef struct message_data_t {
    uint16_t x;
    uint16_t y;
    uint16_t z;
    int16_t yaw;
    int16_t roll;
} message_data_t;

void send_command(uint8_t command_no);

void twai_init(void * parameters);
void twai_send(void * parameters);
void boot(void * parameters);
void create_semaphore(void * parameters);
void comms_init(void * parameters);

// void setCRC8(uint8_t *pchMessage, uint32_t msglen);
// void appendCRC16CheckSum(uint8_t *pchMessage, uint32_t msglen);

#endif // !CAN_H
