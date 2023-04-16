// SM Desk video controller

#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

// Used for commands in queue to input logic
struct Queued_Input_Message_Struct {
    uint8_t type; // See below defines

    uint8_t panel; // Used for a routing command (which panel)
    uint8_t panel_button; // Used for a routing command (which button within the panel)

    uint8_t input; // Used for an incoming routing confirm
    uint8_t output; // Used for an incoming routing confirm 
};

// Definitions of message type for input messages 
#define IN_MSG_TYP_ROUTING 0
#define IN_MSG_TYP_IR_TOGGLE 1
#define IN_MSG_TYP_RELAY_STATE 2
#define IN_MSG_TYP_ETHERNET 3


// Definitions of Main/IR state for show relay
#define RELAY_STATE_MAIN 0
#define RELAY_STATE_IR 1

// How long to leave between each IR changeover in ticks
#define RELAY_CHANGEOVER_RIPPLE_INTERVAL 3

#endif