#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "freertos/queue.h" 

#include "main.h"
#include "local_io.h"

QueueHandle_t input_event_queue; 

static const char *TAG = "main";

static void input_logic_task(void)
{
    // Task which responds to button presses on the front panel and relay state changes
    // Uses a queue to respond when required

    uint16_t incoming_msg;

    while(1)
    {   
        incoming_msg = 0;
        if (xQueueReceive(input_event_queue, &incoming_msg, (TickType_t) portMAX_DELAY) == pdTRUE)
        {
            // Message recieved from queue
            ESP_LOGI(TAG,"Processing message in input logic:%i",incoming_msg);

            uint8_t msg_part1 = (uint8_t) incoming_msg;
            uint8_t msg_part2 = (uint8_t) (incoming_msg >> 8);

            switch (msg_part1)
            {
            case IO_MSG_PANEL_1:
                // First button panel
                
                break;
            case IO_MSG_PANEL_2:
                // Second button panel
                
                break;
            case IO_MSG_PANEL_3:
                // Third button panel
                
                break;
            case IO_MSG_PANEL_4:
                // Fourth button panel
                
                break;
            case IO_MSG_IR_BUTTON:
                // IR Button
                
                break;
            default:
                ESP_LOGW(TAG,"Input message unknown:%i",incoming_msg);
                break;
            }

            // Press of router button: send to switcher
            // Press of IR button: send command macro to switcher
            
        }
    }

}

void app_main(void)
{   
    // Set up input event queue - note that 16 bit int used to get two 8-bit parts to message in upper and lower 
    input_event_queue = xQueueCreate (32, sizeof(uint16_t)); 
    if (input_event_queue == NULL)
    {
        ESP_LOGE(TAG,"Unable to create input event queue, rebooting");
        esp_restart();
    }

    //Set up local buttons, LEDs, relay outputs and warning lights
    setup_local_io(&input_event_queue);

    xTaskCreate( (TaskFunction_t) input_logic_task, "input_logic_task", 2048, NULL, 5, NULL);}