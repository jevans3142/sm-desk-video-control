// Storage module
//-----------------------------------

#ifndef STORAGE_H_INCLUDED
#define STORAGE_H_INCLUDED

struct Settings_Struct {
    uint8_t routing_panel_sources[4][6]; // Sources labeled 1-40 for each panel button
    uint8_t routing_panel_destinations[4]; // Destinations labeled 1-40 for each panel
    uint8_t show_relay_main_source; // Source labelled 1-40
    uint8_t show_relay_ir_source; // Source labelled 1-40
    uint8_t show_relay_switched_destination_flags[40]; // Bool for each destination that requires main/ir switching
    uint32_t router_ip;
    uint32_t router_port;
};

#define MOUNT_POINT "/sdcard"
#define CFG_FILE "/config.txt"
#define MAX_CHAR_SIZE 256

struct Settings_Struct get_settings(void);

#endif  