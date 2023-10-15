// Ethernet IO to video router
//-----------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>      
#include <arpa/inet.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "main.h"
#include "ethernet.h"
#include "local_io.h"
#include "pindefs.h"

// Logging tag
static const char *TAG = "ethernet";

// Output message queue - added to from logic in main.c
QueueHandle_t ethernet_message_output_queue; 

// Input message queue - added to here from main TCP loop
QueueHandle_t ethernet_message_input_queue; 

// Input message queue handle pointer - passed in from main module
static QueueHandle_t *input_event_queue_ptr;

// IP and port of router that we're controlling - set in setup function
uint32_t router_ip;
uint32_t router_port;
char router_ip_text[INET_ADDRSTRLEN]; 

// Handle of TCP client task
TaskHandle_t tcp_client_task_handle = NULL;

// Ethernet warning light activate

static void ethernet_warning_on(void)
{
    set_ethernet_fail_led(1);
}

// Ethernet warning light deactivate
static void ethernet_warning_off(void)
{
    set_ethernet_fail_led(0);
}

// Event handler for general Ethernet events 
static void ethernet_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    uint8_t mac_address[6] = {0}; // Use default MAC from efuses on esp32
    esp_eth_handle_t ethernet_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) 
    {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(ethernet_handle, ETH_CMD_G_MAC_ADDR, mac_address);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        if( tcp_client_task_handle != NULL )
        {
            vTaskDelete(tcp_client_task_handle);
            tcp_client_task_handle = NULL;
        }
        ethernet_warning_on();
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        if( tcp_client_task_handle != NULL )
        {
            vTaskDelete(tcp_client_task_handle);
            tcp_client_task_handle = NULL;
        }
        ethernet_warning_on();
        break;
    default:
        break;
    }
}

// Main TCP client loop task 

static void tcp_client_loop(void)
{
    char rx_buffer[ETH_TCP_TEXT_RECV_BUFFER_SIZE];
    int addr_family = 0;
    int ip_protocol = 0;

    struct sockaddr_in dest_addr;
    struct in_addr sin_ip;
    sin_ip.s_addr = htonl(router_ip);
    dest_addr.sin_addr = sin_ip;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(router_port);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;

    int keepAlive = 1;
    int keepIdle = ETH_KEEPALIVE_IDLE;
    int keepInterval = ETH_KEEPALIVE_INTERVAL;
    int keepCount = ETH_KEEPALIVE_COUNT;

    inet_ntop(AF_INET, &(dest_addr.sin_addr), router_ip_text, INET_ADDRSTRLEN);

    while (1) 
    {
        // Outer connection loop - re(connects) to IP 

        int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) 
        {
            ESP_LOGE(TAG, "Unable to create socket: Error number %d", errno);
            ethernet_warning_on();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));

        ESP_LOGI(TAG, "Socket created, connecting to %s:%"PRIu32, router_ip_text, router_port);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) 
        {
            ESP_LOGE(TAG, "Socket unable to connect: Error number %d", errno);
            ethernet_warning_on();
            shutdown(sock, 0);
            close(sock);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "Successfully connected");

        while (1) 
        {
            // Inner event loop - executes in here until something about the connection fails

            // Send any messages if in queue
            uint8_t reset_connection = 0;    
            while ( uxQueueMessagesWaiting(ethernet_message_output_queue) > 0 )
            {
                char buffer[64];
                struct Queued_Ethernet_Message_Struct incoming_message;
                if (xQueueReceive(ethernet_message_output_queue, &incoming_message, 0) != pdTRUE)
                {
                    ESP_LOGW(TAG, "Unable to retrive Ethernet message from queue");
                    break;
                }
                    
                switch (incoming_message.type)
                {
                case ETH_MSG_TYP_ROUTING:
                    snprintf(buffer, sizeof(buffer), "VIDEO OUTPUT ROUTING:\n%d %d\n\n", incoming_message.output, incoming_message.input);
                    break;
                
                case ETH_MSG_TYP_ROUTEDUMP:
                    snprintf(buffer, sizeof(buffer), "VIDEO OUTPUT ROUTING:\n\n");
                    break;

                default:
                    ESP_LOGE(TAG, "Ethernet message type not recognised in queue: %d", incoming_message.type);
                    break;
                }

                if (strlen(buffer) > 0)
                {           
                    int err = send(sock, buffer, strlen(buffer), 0);

                    if (err < 0) 
                    {
                        ESP_LOGE(TAG, "Send failed: Error number %d", errno);
                        ethernet_warning_on();
                        reset_connection = 1;
                        break; // Need to trigger a connection reset (will break out of outer loop below)
                    } else {
                        // Data sent
                        ESP_LOGI(TAG, "Sent %d bytes to %s:", strlen(buffer), router_ip_text);
                        ESP_LOGI(TAG, "%s", buffer);
                        ethernet_warning_off();
                    }
                }
                
            }

            if (reset_connection != 0)
            {
                break; 
            }
  
             // Receive any messages
            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, MSG_DONTWAIT);
            // Did an error occurr during receiving?
            if (len < 0) 
            {
                if (errno == EAGAIN)
                {
                    // Not an error - just no data to recieve
                    ESP_LOGV(TAG, "No data to recive in socket loop");
                } else {
                    ESP_LOGE(TAG, "Recieve failed: Error number %d", errno);
                    ethernet_warning_on();
                    break; // Need to trigger a connection reset
                }
            } else {
                // Data received
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI(TAG, "Received %d bytes:", len);
                ESP_LOGI(TAG, "%s", rx_buffer);
                ethernet_warning_off();
                if (xQueueSend(ethernet_message_input_queue, (void *)&rx_buffer, 1) == pdTRUE)
                {
                    ESP_LOGI(TAG, "Sending message from recv to process logic");
                }
                else
                {
                    ESP_LOGW(TAG, "Sending message from recv failed due to queue full?");
                }
            }

            vTaskDelay(10 / portTICK_PERIOD_MS); // Yield for everything else 

        }

        if (sock != -1) 
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
            ethernet_warning_on();
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS); // Prevents hammering
    }
}

// Task that takes incoming TCP message fragments from router and sticks them together
// Uses a state machine to filter to the messages we want and ignore all others
static void tcp_recv_task(void)
{
    uint8_t state = ETH_TCP_RECV_STATE_UNKNOWN; // See header file for state machine definitions

    while(1)
    {   
        char incoming_msg[ETH_TCP_TEXT_RECV_BUFFER_SIZE];
        if (xQueueReceive(ethernet_message_input_queue, &incoming_msg, (TickType_t) portMAX_DELAY) == pdTRUE)
        {
            // Message recieved from queue
            ESP_LOGI(TAG,"Processing incoming text buffer in TCP logic");

            // Go through string to break up lines preserving blank lines

            uint16_t line_count = 0;
            char *msg_ptr = incoming_msg;  

            while (*msg_ptr != '\0') 
            {
                if (*msg_ptr == '\n') 
                {   
                    *msg_ptr = '\0';  // Replace newline with null terminator to break up strings
                    line_count++;
                }
                msg_ptr++;
            }

            line_count++;
            ESP_LOGI(TAG,"Line count - %u",line_count);

            msg_ptr = incoming_msg;

            for (uint16_t i = 0; i < line_count; i++) 
            {
                u_int16_t ptr_length = strlen(msg_ptr); // Need to store the length becase we might insert extra null terminators

                switch (state)
                {
                case ETH_TCP_RECV_STATE_UNKNOWN:
                    // Don't know what message we're currently getting; wait for blank line 
                    if (strlen(msg_ptr) == 0)
                    {
                        // Blank line, now wait for the start of a block
                        state = ETH_TCP_RECV_STATE_WAIT;
                        break;
                    } 
                    break;
                case ETH_TCP_RECV_STATE_WAIT:
                    // Gone past a newline, waiting for the start of a block
                    if (strncmp(msg_ptr, "VIDEO OUTPUT ROUTING:", strlen("VIDEO OUTPUT ROUTING:")) == 0)
                    {
                        state = ETH_TCP_RECV_STATE_IN_UPDATE;
                        break;
                    } 
                    break;
                case ETH_TCP_RECV_STATE_IN_UPDATE:
                    // Gone past the start of a VIDEO OUTPUT ROUTING block
                    if (strlen(msg_ptr) == 0)
                    {
                        // Blank line, now wait for the start of a block
                        state = ETH_TCP_RECV_STATE_WAIT;
                        break;
                    }
                    // If not blank, should be a pair of numbers, so now split by spaces 
                    char *spacesplit;
                    spacesplit = strtok(msg_ptr, " "); 
                    uint8_t output = (uint8_t) atoi(spacesplit);
                    spacesplit = strtok(NULL, " "); 
                    uint8_t input = (uint8_t) atoi(spacesplit);
                    ESP_LOGI(TAG, "Route confirm received! Output: %u Input %u", output, input);

                    struct Queued_Input_Message_Struct new_message;
                    new_message.type = IN_MSG_TYP_ETHERNET;
                    new_message.input = input;
                    new_message.output = output;

                    if (xQueueSend(*input_event_queue_ptr, (void *)&new_message, 0) == pdTRUE)
                    {
                        ESP_LOGI(TAG, "Sending message from route confirm %i,%i,%i", new_message.type, new_message.output, new_message.input);
                    }
                    else
                    {
                        ESP_LOGW(TAG, "Sending message from route confirm failed due to queue full? - %i,%i,%i", new_message.type, new_message.output, new_message.input);
                    }

                    break;
                } 

                msg_ptr = msg_ptr + ptr_length + 1;
            }
        }
    }
}


// Event handler for IP_EVENT_ETH_GOT_IP
static void got_ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    ip_event_got_ip_t* event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t* ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask) );
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");

    if( tcp_client_task_handle != NULL )
    {
        vTaskDelete(tcp_client_task_handle);
        tcp_client_task_handle = NULL;
    }

    xTaskCreate( (TaskFunction_t) tcp_client_loop, "tcp_client_loop", 8192, NULL, 5, &tcp_client_task_handle);
}

void setup_ethernet(uint32_t ip, uint32_t port, QueueHandle_t* input_queue)
{
    router_ip = ip;
    router_port = port;

    // Set up output event queue
    ethernet_message_output_queue = xQueueCreate (64, sizeof(struct Queued_Ethernet_Message_Struct)); 
    if (ethernet_message_output_queue == NULL)
    {
        ESP_LOGE(TAG,"Unable to create ethernet output  message queue, rebooting");
        esp_restart();
    }

    // Set up input message queue
    ethernet_message_input_queue = xQueueCreate (ETH_TCP_TEXT_RECV_BUFFER_NUM, ETH_TCP_TEXT_RECV_BUFFER_SIZE * (sizeof(char))); 
    if (ethernet_message_input_queue == NULL)
    {
        ESP_LOGE(TAG,"Unable to create ethernet input message queue, rebooting");
        esp_restart();
    }
    xTaskCreate( (TaskFunction_t) tcp_recv_task, "tcp_recv_task", 4096, NULL, 5, NULL);

    // Set up local pointers to the event queue in the main logic
    input_event_queue_ptr = input_queue;

    // Initialize TCP/IP network interface
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create new default instance of esp-netif for Ethernet
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t* eth_netif = esp_netif_new(&cfg);

    // Init MAC and PHY configs to default
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();

    phy_config.phy_addr = 0;
    phy_config.reset_gpio_num = PIN_PHY_RST;

    eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    esp32_emac_config.smi_mdc_gpio_num = PIN_PHY_MDC;
    esp32_emac_config.smi_mdio_gpio_num = PIN_PHY_MDIO;
    esp32_emac_config.clock_config.rmii.clock_mode = EMAC_CLK_OUT;
    esp32_emac_config.clock_config.rmii.clock_gpio = EMAC_CLK_OUT_180_GPIO;


    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);
    esp_eth_phy_t* phy = esp_eth_phy_new_lan87xx(&phy_config);

    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t ethernet_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&config, &ethernet_handle));
    
    // Attach Ethernet driver to TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(ethernet_handle)));

    // Register event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &ethernet_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    // Start Ethernet driver state machine
    ESP_ERROR_CHECK(esp_eth_start(ethernet_handle));

}

void send_video_route(uint8_t input, uint8_t output)
{
    // Add message to queue for output to switcher
    struct Queued_Ethernet_Message_Struct new_message;
    
    new_message.type = ETH_MSG_TYP_ROUTING;
    new_message.input = input;
    new_message.output = output;

    if (xQueueSend(ethernet_message_output_queue, (void *)&new_message, 0) == pdTRUE)
    {
        ESP_LOGI(TAG, "Putting message into ethernet output queue %i,%i,%i", new_message.type, new_message.input, new_message.output);
        ESP_LOGI(TAG, "%i messages in queue",uxQueueMessagesWaiting(ethernet_message_output_queue));
    }
    else
    {
        ESP_LOGW(TAG, "Putting message into ethernet output queue failed due to queue full? - %i,%i,%i", new_message.type, new_message.input, new_message.output);
        ESP_LOGW(TAG, "%i messages in queue",uxQueueMessagesWaiting(ethernet_message_output_queue));
    }


}

void request_route_dump()
{
    // Request a full dump of all the video routes as a status update
    struct Queued_Ethernet_Message_Struct new_message;
    
    new_message.type = ETH_MSG_TYP_ROUTEDUMP;

    if (xQueueSend(ethernet_message_output_queue, (void *)&new_message, 0) == pdTRUE)
    {
        ESP_LOGI(TAG, "Putting message into ethernet output queue %i", new_message.type);
        ESP_LOGI(TAG, "%i messages in queue",uxQueueMessagesWaiting(ethernet_message_output_queue));
    }
    else
    {
        ESP_LOGW(TAG, "Putting message into ethernet output queue failed due to queue full? - %i", new_message.type);
        ESP_LOGW(TAG, "%i messages in queue",uxQueueMessagesWaiting(ethernet_message_output_queue));
    }


}
