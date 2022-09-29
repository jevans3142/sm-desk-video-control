// Local IO: Buttons, LEDs, IR relay IO, warning lights 
//-----------------------------------

#ifndef LOCAL_IO_H_INCLUDED
#define LOCAL_IO_H_INCLUDED

void setup_local_io(void);

uint8_t get_button_panel_state(u_int8_t panel);

uint8_t get_ir_button_state(void);
void set_ir_button_led(uint8_t value);

uint8_t get_ir_relay_state(void);
void toggle_ir_relay_state(void);

void set_ethernet_fail_led(uint8_t value);
void set_relay_fail_led(uint8_t value);

void set_usb_enable_a(uint8_t value);
void set_usb_enable_b(uint8_t value);

#endif  