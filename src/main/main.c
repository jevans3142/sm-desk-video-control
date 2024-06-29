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

// Current main/IR state for show relay
// Held here rather than on switcher, because of the complexity of the multiple outputs switched and the possiblity they get out of sync
u_int8_t relay_ir_state = RELAY_STATE_MAIN;
// This holds a 'cache' state of if a button panel is currently viewing show relay and therefore if we need to do a main/IR switch for it
u_int8_t cache_panel_relay_states[4];

// USB enable state
// Cache of actual routing for each panel, used only for USB enables not LEDs
u_int8_t cache_panel_routing[4];
// Cache of state
u_int8_t usb_enable_a_state;
u_int8_t usb_enable_b_state;

static const char *TAG = "main";

static void input_logic_task(void)
{
    // Task which responds to button presses on the front panel, ethernet messages, and relay state changes
    // Uses a queue to respond when required

    for (uint8_t panel = 0; panel<4; panel++)
    {   
        cache_panel_relay_states[panel] = 0;
        cache_panel_routing[panel] = 0;
    }
    usb_enable_a_state = 0;
    usb_enable_b_state = 0;

    set_ir_relay_state(RELAY_STATE_MAIN);

    set_usb_enable_a(usb_enable_a_state);
    set_usb_enable_b(usb_enable_b_state);

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

                if (input == 99 )
                {
                    // Special case to handle main/IR show relay
                    if (relay_ir_state == RELAY_STATE_MAIN)
                    {
                        input = settings.show_relay_main_source;
                    } else {
                        input = settings.show_relay_ir_source;
                    }
                    cache_panel_relay_states[incoming_msg.panel] = 1;
                } else {
                    cache_panel_relay_states[incoming_msg.panel] = 0;
                }

                // Decrement in/outs by 1 to go from physical 1-40 numbering to zero index 
                send_video_route(input - 1, output - 1);

                break;
            case IN_MSG_TYP_IR_TOGGLE:
                // IR Button - send command macro to switcher 
                ESP_LOGI(TAG,"Toggling IR");

                

                uint8_t new_source = 0; 
                if (relay_ir_state == RELAY_STATE_MAIN)
                {
                    // Switch to IR 
                    relay_ir_state = RELAY_STATE_IR;
                    new_source = settings.show_relay_ir_source;
                    set_ir_button_led(RELAY_STATE_IR);  
                    set_ir_relay_state(RELAY_STATE_IR);
                } else {
                    // Switch to Main
                    relay_ir_state = RELAY_STATE_MAIN;
                    new_source = settings.show_relay_main_source;  
                    set_ir_button_led(RELAY_STATE_MAIN);  
                    set_ir_relay_state(RELAY_STATE_MAIN);
                }

                // First identify if we are currently viewing show relay and need to switch 
                for (uint8_t panel = 0; panel<4; panel++)
                {   
                    if( cache_panel_relay_states[panel] == 1)
                    {
                        // Then we are currently watching show relay and need to switch
                        if (relay_ir_state == RELAY_STATE_MAIN)
                        {
                            // Decrement in/outs by 1 to go from physical 1-40 numbering to zero index 
                            send_video_route(settings.show_relay_main_source - 1, settings.routing_panel_destinations[panel] - 1);
                        } else {
                            // Decrement in/outs by 1 to go from physical 1-40 numbering to zero index 
                            send_video_route(settings.show_relay_ir_source - 1, settings.routing_panel_destinations[panel] - 1);
                        }
                    }
                }

                // Also loop through all secondary channels that get main/IR switched
                for (uint8_t dest = 0; dest<40; dest++)
                {
                    if (settings.show_relay_switched_destination_flags[dest] != 0)
                    {
                        // Decrement input by 1 to go from physical 1-40 numbering to zero index 
                        send_video_route(new_source - 1, dest);
                        vTaskDelay(RELAY_CHANGEOVER_RIPPLE_INTERVAL); 
                    }
                }

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
                                cache_panel_relay_states[panel] = 0;
                                cache_panel_routing[panel] = incoming_msg.input + 1;
                                break;
                            } 
                            if ( (settings.routing_panel_sources[panel][button] == 99) && (((incoming_msg.input + 1) == settings.show_relay_main_source) || ((incoming_msg.input + 1) == settings.show_relay_ir_source)) )
                            {
                                // It's one of the show relay sources
                                found_button = button + 1; // Got to convert back from zero index to physical button, because 0 = no LED lit
                                cache_panel_relay_states[panel] = 1;
                                cache_panel_routing[panel] = incoming_msg.input + 1;
                                break;
                            }
                        }
                        break;
                    }
                }

                if (found_panel != 255)
                {
                    // This means we have a confirmed routing change to one of our screens
                    set_button_led_state(found_panel,found_button);
                    ESP_LOGI(TAG, "Cache of routing state: %i,%i,%i,%i", cache_panel_routing[0], cache_panel_routing[1], cache_panel_routing[2], cache_panel_routing[3]);
                    
                    // Now work out what USB enable state should be 
                    uint8_t new_a_state = 0; 
                    uint8_t new_b_state = 0;
                    for (uint8_t panel = 0; panel<4; panel++)
                    {   
                        if (cache_panel_routing[panel] == settings.usb_source_a)
                        {
                            new_a_state = 1;
                        }
                        if (cache_panel_routing[panel] == settings.usb_source_b)
                        {
                            new_b_state = 1;
                        }
                    }
                    if (new_a_state != usb_enable_a_state)
                    {
                        usb_enable_a_state = new_a_state;
                        set_usb_enable_a(usb_enable_a_state);
                        ESP_LOGI(TAG, "USB A enable set to %d",usb_enable_a_state);
                    }
                    if (new_b_state != usb_enable_b_state)
                    {
                        usb_enable_b_state = new_b_state;
                        set_usb_enable_b(usb_enable_b_state);
                        ESP_LOGI(TAG, "USB B enable set to %d",usb_enable_b_state);
                    }

                }

                break;
            default:
                ESP_LOGW(TAG,"Input message unknown:%i",incoming_msg.type);
                break;
            }
            
        }
    }

}

static void local_test_mode(void)
{
    // Vegas mode - LED and button test only
    while (1)
    {
        // Do some dumb polling of the buttons to light any up that are selected
        for (uint8_t panel = 0; panel<4; panel++)
        {
                set_button_led_state(panel,get_button_panel_state(panel));
                set_ir_button_led(get_ir_button_state());
                vTaskDelay(5);
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

    // Set up ethernet stack and communication with video router
    setup_ethernet(settings.router_ip, settings.router_port, &input_event_queue);
    setup_local_io(&input_event_queue);  

    vTaskDelay(100); // Wait to see if buttons are being held down
    //Check to see if we're heading into 'vegas mode' for testing rather than the proper application
    if ((get_button_panel_state(0) == 1) && (get_button_panel_state(2) == 1))
    {
        ESP_LOGW(TAG,"Going into local test mode");
        local_test_mode();
        return; 
    } 

    xTaskCreate( (TaskFunction_t) input_logic_task, "input_logic_task", 2048, NULL, 5, NULL);
}