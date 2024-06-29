// Ethernet IO to video router
//-----------------------------------

#ifndef ETHERNET_H_INCLUDED
#define ETHERNET_H_INCLUDED

// Used for commands in queue to send to switcher
struct Queued_Ethernet_Message_Struct {
    uint8_t type; // See below defines
    uint8_t input; // Used for a routing command
    uint8_t output; // Used for a routing command
};

// Definitions of message type for ethernet messages 
#define ETH_MSG_TYP_ROUTING 0
#define ETH_MSG_TYP_ROUTEDUMP 1

// Length and number of text buffers for TCP input queue
#define ETH_TCP_TEXT_RECV_BUFFER_SIZE 1024
#define ETH_TCP_TEXT_RECV_QUEUE_SIZE 2048
#define ETH_TCP_TEXT_RECV_QUEUE_NUM 16

// TCP socket kepalives
#define ETH_KEEPALIVE_IDLE 1
#define ETH_KEEPALIVE_INTERVAL 1
#define ETH_KEEPALIVE_COUNT 1

// Define state machine for TCP recv task
#define ETH_TCP_RECV_STATE_UNKNOWN 0 
#define ETH_TCP_RECV_STATE_WAIT 1 
#define ETH_TCP_RECV_STATE_IN_UPDATE 2 

void setup_ethernet(uint32_t ip, uint32_t port, QueueHandle_t* input_queue);
void send_video_route(uint8_t input, uint8_t output);
void request_route_dump();

#endif  