#include <stdint.h>
#include <stdio.h>
#include "driver/twai.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>
#include "esp_err.h"
#include "esp_mac.h"
#include "nvs.h"
#include "esp_netif.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include <inttypes.h>

//Controller device
#include "connection.h"
#include "portmacro.h"

void print_mac(uint8_t *address){
    for(int i = 0; i < 5; i++){
        printf("%02X:",address[i]);
    }
    printf("%02X\n",address[5]);
}

typedef struct message_data_t {
    uint16_t x;
    uint16_t y;
    uint16_t z;
    int16_t yaw;
    int16_t roll;
} message_data_t;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
      // printf(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success\n" : "Delivery Fail\n" );
}

void callback_10ms(TimerHandle_t xTimer){

}

void app_main(void)
{

    message_data_t movement_data;
    movement_data.x = 1024;
    movement_data.y = 1024;
    movement_data.z = 1024;
    movement_data.yaw = 0;
    movement_data.roll = 0;


    static TimerHandle_t timer10ms = NULL;

    void timer(void * parameters);
    void callback_10ms(TimerHandle_t xTimer);

    timer10ms = xTimerCreate("Timer10ms", 40 / portTICK_PERIOD_MS, pdTRUE, NULL, callback_10ms);

    xTimerStart(timer10ms, portMAX_DELAY);

    uint8_t address[6] = {00, 11, 22, 33, 44, 55}; 
    printf("hello world\n");

    esp_read_mac(address, ESP_MAC_WIFI_STA);
    print_mac(address);
    print_mac(robot_addr);
    

    esp_now_peer_info_t robot = {};
    memcpy(robot.peer_addr, robot_addr, 6);
    robot.channel = 0;
    robot.encrypt = false;


    // Initialize NVS
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
    if(esp_now_add_peer(&robot) != ESP_OK){
        printf("Peer add failed\n");
        return;
    }
    esp_now_register_send_cb(OnDataSent);
    printf("Ready\n");


    char key = 'x';
    uint16_t speedx = 100;
    uint16_t speedy= 100;
    uint16_t speedz = 50;
    int16_t speedyaw = 100;
    int16_t speedroll = 100;

    while(1){
        key = fgetc(stdin);
        switch(key){
            case 'w':
                movement_data.x = movement_data.x + speedx;
                break;
            case 'q':
                movement_data.y = movement_data.y - speedy;
                break;
            case 's':
                movement_data.x = movement_data.x - speedx;
                break;
            case 'e':
                movement_data.y = movement_data.y + speedy;
                break;
            case 'a':
                movement_data.z = movement_data.z - speedz;
                break;
            case 'd':
                movement_data.z = movement_data.z + speedz;
                break;
            case 'k':
                movement_data.roll = movement_data.roll + speedroll;
                break;
            case 'j':
                movement_data.roll = movement_data.roll - speedroll;
                break;
            case 'h':
                movement_data.yaw = movement_data.yaw - speedyaw;
                break;
            case 'l':
                movement_data.yaw = movement_data.yaw + speedyaw;
                break;
            case ' ':
                movement_data.x = 1024;
                movement_data.y = 1024;
                movement_data.z = 1024;
                movement_data.yaw = 0;
                movement_data.roll = 0;
        }
        // printf("%c",key);
        // int16_t angular_y = (int) -1024 * movement_data.yaw;
        // int16_t angular_z = (int) -1024 * movement_data.roll;
    printf("x = %" PRIu16 ", y = %" PRIu16 ", z = %" PRIu16 ", ",(&movement_data)->x,(&movement_data)->y,(&movement_data)->z);
    printf("%i,%i\n",movement_data.yaw, movement_data.roll);
        if(movement_data.x > (1024 + 6 * speedx)){
            movement_data.x = 1024 + 6 * speedx;
        }
        if(movement_data.x < (1024 - 6 * speedx)){
            movement_data.x = 1024 - 6 * speedx;
        }
        if(movement_data.y > (1024 + 3 * speedy)){
            movement_data.y = 1024 + 3 * speedy;
        }
        if(movement_data.y < (1024 - 3 * speedy)){
            movement_data.y = 1024 - 3 * speedy;
        }
        if(movement_data.z > (1024 + 9 * speedz)){
            movement_data.z = 1024 + 9 * speedz;
        }
        if(movement_data.z < (1024 - 9 * speedz)){
            movement_data.z = 1024 - 9 * speedz;
        }
        
        esp_err_t result = esp_now_send(robot_addr, (uint8_t *) &movement_data, sizeof(movement_data));
        switch(result){
            case ESP_OK:
                // printf("Message sent succesfully, ");
                break;
            case ESP_ERR_ESPNOW_ARG:            
                printf("Error - invalid argument\n");
                break;
            case ESP_ERR_ESPNOW_INTERNAL:            
                printf("Error - internal error\n");
                break;
            case ESP_ERR_ESPNOW_NOT_FOUND:            
                printf("Error - peer not found\n");
                break;
            case ESP_ERR_ESPNOW_IF:          
                printf("Error - current WiFi interface doesn't match that of peer\n");
                break;
            default:
                printf("Error - unknown\n");
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

}
