// SM Desk video controller

#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

// Used for commands in queue to send to switcher
struct Queued_Input_Message_Struct {
    uint8_t type; // See below defines

    uint8_t panel; // Used for a routing command (which panel)
    uint8_t panel_button; // Used for a routing command (which button within the panel)
};

// Definitions of message type for input messages 
#define IN_MSG_TYP_ROUTING 0
#define IN_MSG_TYP_IR_TOGGLE 1
#define IN_MSG_TYP_RELAY_STATE 2
#define IN_MSG_TYP_ETHERNET 3

#endif