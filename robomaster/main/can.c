#include "can.h"
#include <driver/twai.h>
#include <hal/twai_types.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/types.h>
#include "checksum.h"
#include "esp_now.h"
#include "connection.h"
#include "esp_mac.h"
#include <esp_wifi.h>
#include <esp_now.h>
#include <string.h>
#include "esp_err.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/semphr.h"


uint8_t can_data_buffer[2048];
int d_rh = 0;  //data read head
int d_wh = 0;  //data write head
SemaphoreHandle_t d_sem = NULL;

uint8_t can_data_length_buffer[2048];
int dl_rh = 0;  //data length read head
int dl_wh = 0;  //data length write head
SemaphoreHandle_t dl_sem = NULL;

void create_semaphore( void * pvParameters )
{
    d_sem = xSemaphoreCreateBinary();
    dl_sem = xSemaphoreCreateBinary();
    xSemaphoreGive(d_sem);
    xSemaphoreGive(dl_sem);
    vTaskDelete(NULL);
}

extern message_data_t movement_data;

void twai_send(void * parameters){
    twai_message_t message;
    message.identifier = 0x201;
    message.extd = 0x00;
    message.rtr = 0x00000000U;
    int can_command_buffer_size = 0;
    for(;;){
        if(dl_sem != NULL && xSemaphoreTake( dl_sem, ( TickType_t ) 10 ) == pdTRUE ){
            while (dl_rh != dl_wh)
            {
                if(can_command_buffer_size <= 0){
                    can_command_buffer_size = can_data_length_buffer[dl_rh];
                }

                if(can_command_buffer_size != 0){
                    if (can_command_buffer_size >= 8){
                        message.data_length_code = 8;
                        can_command_buffer_size -= 8;
                    }
                    else{
                        message.data_length_code = can_command_buffer_size;
                        can_command_buffer_size -= message.data_length_code;
                        dl_rh++;
                        dl_rh %= 2048;
                    }

                    for (int i = 0; i < message.data_length_code; i++){
                        if(i < message.data_length_code){
                        }
                        message.data[i] = can_data_buffer[(d_rh + i) % 2048];
                        // printf("%02X ",message.data[i]);
                    }

                    if(twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK){
                        d_rh += message.data_length_code;
                        d_rh %= 2048;
                        // printf("\n");
                    }
                    else {
                        printf("Failed to queue message for transmission\n");
                    }
                }
            }
            xSemaphoreGive(dl_sem);
        }
    }
}

void send_command(uint8_t command_no)
{
    static int command_counter = 0;
    static const uint8_t can_command_list[16][74] = {
        {/*0*/ //100ms
            0x55,0x0D,0x04,0xFF,0x0A,0xFF,0xFF,0xFF,
            0x40,0x00,0x01,0xFF,0xFF,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {/*1*/ //20ms
            0x55,0x0E,0x04,0xFF,0x09,0x03,0xFF,0xFF,
            0xA0,0x48,0x08,0x01,0xFF,0xFF,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {/*2*/ //100ms
            0x55,0x0F,0x04,0xFF,0xF1,0xC3,0xFF,0xFF,
            0x00,0x0A,0x53,0x32,0x00,0xFF,0xFF
            ,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {/*3*/ //5s
            0x55,0x12,0x04,0xFF,0xF1,0xC3,0xFF,0xFF,
            0x40,0x00,0x58,0x03,0x92,0x06,0x02,0x00,
            0xFF,0xFF,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {/*4*//*GIMBAL*/ //10ms when command is needed, does not work with with movement
            0x55,0x14,0x04,0xFF,0x09,0x04,0xFF,0xFF,
            0x00,0x04,0x69,0x08,0x05,0x00,0x00,0x00,
            0x00,0x6C,0xFF,0xFF,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {/*5*//*MOVE, 23th byte: 00-no movement,04-xy linear, 08-gimbal, 0C-both*/ //10ms
            0x55,0x1B,0x04,0xFF,0x09,0xC3,0xFF,0xFF,
            0x00,0x3F,0x60,0x00,0x04,0x20,0x00,0x01,
            0x08,0x40,0x00,0x02,0x10,0x04,0x0C,0x00,
            0x04,0xFF,0xFF,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {/*6*/ //1s
            0x55,0x49,0x04,0xFF,0x49,0x03,0xFF,0xFF,
            0x00,0x3F,0x70,0xB4,0x0F,0x66,0x03,0x00,
            0x00,0xD3,0x03,0x3D,0x08,0x10,0x00,0x08,
            0x00,0x08,0x00,0x08,0x00,0x08,0x00,0x08,
            0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x00,0x7E,0x0E,0xF3,
            0x0B,0xD9,0x07,0x0E,0x07,0x3D,0x07,0x6A,
            0x08,0x62,0x0A,0x05,0x0B,0xD6,0x0B,0xFF,
            0xFF},
        {/*7*//*BLASTER COMMAND*/
            0x55,0x0E,0x04,0xFF,0x09,0x17,0xFF,0xFF,
            0x00,0x3F,0x51,0x01,0xFF,0xFF,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {/*8*//*BLASTER COMMAND*/
            0x55,0x16,0x04,0xFF,0x09,0x17,0xFF,0xFF,
            0x00,0x3F,0x55,0x73,0x00,0xFF,0x00,0x01,
            0x28,0x00,0x00,0x00,0xFF,0xFF,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {/*9*//*INITIAL COMMAND - FREE MODE, 12th byte: 1-fast, 2-medium, 3-slow, 4-manual*/
            0x55,0x0E,0x04,0xFF,0x09,0xC3,0xFF,0xFF,
            0x40,0x3F,0x3F,0x01,0xFF,0xFF,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {/*10*//*INITIAL COMMAND - CHASSIS ACCELERATOR ON*/
            0x55,0x0E,0x04,0xFF,0x09,0xC3,0xFF,0xFF,
            0x40,0x3F,0x28,0x02,0xFF,0xFF,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {/*11*//*INITIAL COMMAND*/
            0x55,0x15,0x04,0xFF,0xF1,0xC3,0x00,0x00,
            0x00,0x03,0xD7,0x01,0x07,0x00,0x02,0x00,
            0x00,0x00,0x00,0xFF,0xFF,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {/*12*//*INITIAL COMMAND*/
            0x55,0x12,0x04,0xFF,0x09,0x03,0x01,0x00,
            0x40,0x48,0x01,0x09,0x00,0x00,0x00,0x03,
            0xFF,0xFF,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {/*13*//*INITIAL COMMAND*/
            0x55,0x1C,0x04,0xFF,0x09,0x03,0x02,0x00,
            0x40,0x48,0x03,0x09,0x00,0x03,0x00,0x01,
            0xFB,0xDC,0xF5,0xD7,0x03,0x00,0x02,0x00,
            0x01,0x00,0xFF,0xFF,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {/*14*//*INITIAL COMMAND*/
            0x55,0x12,0x04,0xFF,0x09,0x03,0x03,0x00,
            0x40,0x48,0x01,0x09,0x00,0x00,0x00,0x03,
            0xFF,0xFF,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {/*15*//*INITIAL COMMAND*/
            0x55,0x24,0x04,0xFF,0x09,0x03,0x04,0x00,
            0x40,0x48,0x03,0x09,0x01,0x03,0x00,0x02,
            0xA7,0x02,0x29,0x88,0x03,0x00,0x02,0x00,
            0x66,0x3E,0x3E,0x4C,0x03,0x00,0x02,0x00,
            0x32,0x00,0xFF,0xFF,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    };

    uint8_t command_bytes[0xFF];
    uint8_t pos = 0;
    uint8_t command_length = can_command_list[command_no][1];
    // printf("command_length = %i\n",command_length);
    for (int i = 0; i < command_length; i++){
        command_bytes[pos] = can_command_list[command_no][i];
        switch(i){
            case 3:
                setCRC8(command_bytes, i);
                break;
            case 6:
                command_bytes[pos] = command_counter & 0xFF;
                break;
            case 7:
                command_bytes[pos] = (command_counter >> 8) & 0xFF;
                break;
        }
        pos++;
    }
    // printf("\n");

    // if(command_no == 20){
    //     static int fire_counter = 0;
    //     printf("%i\n",fire_counter / 2);
    //     command_bytes[13] = fire_counter / 2;
    //     fire_counter++;
    // }

    //gimbal control
    if(command_no == 4){
        
        command_bytes[13] = movement_data.roll & 0xFF;
        command_bytes[14] = (movement_data.roll >> 8) & 0xFF;
        command_bytes[15] = movement_data.yaw & 0xFF;
        command_bytes[16] = (movement_data.yaw >> 8) & 0xFF;
    }

    if(command_no == 5){
        printf("%i, %i, %i\n",movement_data.x,movement_data.y, movement_data.z);
        command_bytes[11] = movement_data.y & 0xFF;
        command_bytes[12] = ((movement_data.x << 3) | (movement_data.y >> 8)) & 0x07;
        command_bytes[13] = (movement_data.x >> 5) & 0x3F;

        command_bytes[16] = (movement_data.z << 4) | 0x08;
        command_bytes[17] = (movement_data.z >> 4) & 0xFF;

        command_bytes[19] = 0x02 | ((movement_data.z << 2) & 0xFF);
        command_bytes[20] = (movement_data.z >> 6) & 0xFF;
    }

    appendCRC16CheckSum(command_bytes, command_length);

    // write command bytes into buffer
    if(d_sem != NULL){
    if( xSemaphoreTake( d_sem, ( TickType_t ) 10 ) == pdTRUE ) {
        for (int i = 0; i < command_length; i++){
            // printf("%02X ",command_bytes[i]);
            // if((i+1) % 8 == 0){printf("\n");}
            can_data_buffer[d_wh] = command_bytes[i];
            d_wh++;
            d_wh %= 2048;
        }
        command_counter++;
        xSemaphoreGive(d_sem);
    }
    else{
        printf("could not get d_sem\n");
    }
    }

    // write command length into buffer
    if(dl_sem != NULL){
    if( xSemaphoreTake( dl_sem, ( TickType_t ) 10 ) == pdTRUE ) {
        can_data_length_buffer[dl_wh] = command_length;
        dl_wh++;
        dl_wh %= 2048;
        xSemaphoreGive(dl_sem);
    }
    else{
        printf("could not get dl_sem\n");
    } 
    }
}


void twai_init(void * parameters){
    //TWAI (CAN) init task
    for(;;){
        twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_23, GPIO_NUM_22, TWAI_MODE_NO_ACK);
        twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
        twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

        if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
            printf("Failed to install driver\n");
        }
        if (twai_start() != ESP_OK) {
            printf("Failed to start driver\n");
        }
    vTaskDelete(NULL);
    }
}

void boot(void * parameters){
    //Send boot commands task
    for(;;){
        for(int i = 9; i < 16; i++){
            send_command(i);
        }
    vTaskDelete(NULL);
    }
}

void print_mac(uint8_t *address){
    for(int i = 0; i < 5; i++){
        printf("%02X:",address[i]);
    }
    printf("%02X\n",address[5]);
}

extern message_data_t movement_data;

esp_now_recv_info_t recv_data = {};

void OnDataRecv(const esp_now_recv_info_t *recv_data, const uint8_t *incomingData, int len) {
    memcpy(&movement_data, incomingData, sizeof(message_data_t));
    // printf("Recieved data!!! HORAAAAY!\n");
    // printf("x = %" PRIu16 ", y = %" PRIu16 ", z = %" PRIu16 "\n",(&movement_data)->x,(&movement_data)->y,(&movement_data)->z);
}

void comms_init(void * parameters){
    //Send initialize wifi, espnow
    for(;;){
        uint8_t address[6] = {00, 11, 22, 33, 44, 55}; 

        esp_read_mac(address, ESP_MAC_WIFI_STA);
        printf("Robomaster MAC: ");
        print_mac(address);
        printf("Controller MAC: ");
        print_mac(controller_addr);

        esp_now_peer_info_t controller = {};
        memcpy(controller.peer_addr, controller_addr, 6);
        controller.channel = 0;
        controller.encrypt = false;

        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK( nvs_flash_erase() );
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK( ret );

        wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
        if(esp_wifi_init(&wifi_config) != ESP_OK){
            printf("WiFi init failed\n");
            return;
        }
        esp_wifi_set_storage(WIFI_STORAGE_RAM);
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_start();
        esp_wifi_set_channel(0, WIFI_SECOND_CHAN_NONE);
        if(esp_now_init() != ESP_OK){
            printf("ESP-NOW init failed\n");
            return;
        }
        if(esp_now_add_peer(&controller) != ESP_OK){
            printf("Peer add failed\n");
            return;
        }
        esp_now_register_recv_cb(OnDataRecv);
        printf("ESP-NOW initialized\n");

    vTaskDelete(NULL);
    }
}
