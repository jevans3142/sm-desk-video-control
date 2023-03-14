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

// Queue handles input to logic from button panels, messages received on ethernet, relay state changes
// Avoids having to poll inputs from main logic (polling, denbouncing, buffering of buttons etc handled in local_io module)
QueueHandle_t input_event_queue; 

static const char *TAG = "main";

static void input_logic_task(void)
{
    // Task which responds to button presses on the front panel, ethernet messages, and relay state changes
    // Uses a queue to respond when required

    while(1)
    {   
        struct Queued_Input_Message_Struct incoming_msg;
        if (xQueueReceive(input_event_queue, &incoming_msg, (TickType_t) portMAX_DELAY) == pdTRUE)
        {
            // Message recieved from queue
            ESP_LOGI(TAG,"Processing message in input logic, type:%i",incoming_msg.type);

            switch (incoming_msg.type)
            {
            case IN_MSG_TYP_ROUTING:
                // Routing input from button panel - send command to switcher
                
                // TODO

                break;
            case IN_MSG_TYP_IR_TOGGLE:
                // IR Button - send command macro to switcher 

                // TODO

                break;
            case IN_MSG_TYP_RELAY_STATE:
                // IR Flood relay - process state change

                // TODO
                
                break;
            case IN_MSG_TYP_ETHERNET:
                // Incoming text message on ethenet

                // TODO

                break;
            default:
                ESP_LOGW(TAG,"Input message unknown:%i",incoming_msg.type);
                break;
            }
            
        }
    }

}

void app_main(void)
{   
    // Set up input event queue
    input_event_queue = xQueueCreate (32, sizeof(struct Queued_Input_Message_Struct)); 
    if (input_event_queue == NULL)
    {
        ESP_LOGE(TAG,"Unable to create input event queue, rebooting");
        esp_restart();
    }

    //Set up local buttons, LEDs, relay outputs and warning lights
    setup_local_io(&input_event_queue);

    xTaskCreate( (TaskFunction_t) input_logic_task, "input_logic_task", 2048, NULL, 5, NULL);}