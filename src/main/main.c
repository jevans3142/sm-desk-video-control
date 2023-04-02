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
#include "ethernet.h"
#include "storage.h"

// Queue handles input to logic from button panels, messages received on ethernet, relay state changes
// Avoids having to poll inputs from main logic (polling, denbouncing, buffering of buttons etc handled in local_io module)
QueueHandle_t input_event_queue; 

// Holds the various settings
// Note that route info is held in 'physical' 1-40 numbering not the 0-39 form - decrements are applied below when commands are sent
struct Settings_Struct settings;

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
                ESP_LOGI(TAG,"Sending video routing message");
                uint8_t input = settings.routing_panel_sources[incoming_msg.panel][incoming_msg.panel_button];
                uint8_t output = settings.routing_panel_destinations[incoming_msg.panel];

                // TODO: need to capture any '99 - show relay' messages here


                // Decrement in/outs by 1 to go from physical 1-40 numbering to zero index 
                send_video_route(input - 1, output - 1);

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
                ESP_LOGI(TAG,"Processing routing confirm message");
                // Work out if the incoming routing confirm applies to any of our screens
                u_int8_t found_panel = 255; // Hacky hack hack - use 255 as a 'not found' flag
                u_int8_t found_button = 0;
                // Increment in/outs by 1 to go from zero index to physical 1-40 numbering 
                for (uint8_t panel = 0; panel<4; panel++)
                {
                    if ((incoming_msg.output + 1) == settings.routing_panel_destinations[panel])
                    {
                        // So it is one of our screens
                        found_panel = panel;
                        for (uint8_t button = 0; button<6; button++)
                        {
                            if ((incoming_msg.input + 1) == settings.routing_panel_sources[panel][button])
                            {
                                // and it's one of our outputs 
                                found_button = button + 1; // Got to convert back from zero index to physical button, because 0 = no LED lit
                                break;
                            }
                        }
                        break;
                    }
                }

                if (found_panel != 255)
                {
                    set_button_led_state(found_panel,found_button);
                }
                
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

    // Retrive settings from SD card 
    settings = get_settings();

    //Set up local buttons, LEDs, relay outputs and warning lights
    setup_local_io(&input_event_queue);
    // Set up ethernet stack and communication with video router
    setup_ethernet(settings.router_ip, settings.router_port, &input_event_queue);

    xTaskCreate( (TaskFunction_t) input_logic_task, "input_logic_task", 2048, NULL, 5, NULL);
}