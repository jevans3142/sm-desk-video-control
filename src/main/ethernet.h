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

void setup_ethernet(uint32_t ip, uint32_t port);
void send_video_route(uint8_t input, uint8_t output);

#endif  