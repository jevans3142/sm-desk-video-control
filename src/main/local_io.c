// Local IO: Buttons, LEDs, IR relay IO, warning lights 
//-----------------------------------

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/task.h"

#include "local_io.h"

// Define internal buffers that hold the 'current' state of the IO expanders
// When reading and writing to these the buffer is used to reduce I2C bus pileups

uint8_t button_panels_state_buffer[] = { 0, 0, 0, 0}; // Four panels - 0 is unpressed 1-6 pressed
uint8_t ir_button_state_buffer[] = 0; // 0 is unpressed 1 pressed 
uint8_t ir_button_led_state_buffer[] = 0; // 0 is off 1 on

uint8_t ir_relay_state_buffer[] = 0; // 0 is off 1 on

uint8_t ethernet_fail_led_state_buffer[] = 0; // 0 is off 1 on
uint8_t relay_fail_led_state_buffer[] = 0; // 0 is off 1 on

uint8_t usb_enable_a_state_buffer[] = 0; // 0 is disabled 1 enabled
uint8_t usb_enable_b_state_buffer[] = 0; // 0 is disabled 1 enabled

// I2C commmunication
//=============================================================================

static void refresh_outputs(void)
{
    // Called when required to update button LEDs, warning lights and IR relay
    // But has ratelimit to not call too often 
    // TODO
}

static void refresh_inputs(void)
{
    // Called every 10ms to get total input state from buttons and relay from I2C expanders
    // TODO
}

void setup_local_io(void)
{
    // Sets up I2C communication with IO expanders, sets default state of outputs
    // TODO 
}

// Button polling
//=============================================================================

// ????? 

// Main button panels
uint8_t get_button_panel_state(uint8_t panel)
{       
    // Returns main button panel state

    // Limit check
    if (panel>3)
    {
        panel = 3; 
    }

    return button_panels_state_buffer[panel]; 
}


// IR Button and relay
//=============================================================================

uint8_t get_ir_button_state(void)
{
    // Returns IR button state from internal buffer last read from I2C bus
    return ir_button_led_state_buffer;
}

void set_ir_button_led(uint8_t value)
{
    // Sets the state of the IR button LED
    // TODO - check buffer before refreshing the outputs
    refresh_outputs();
}

uint8_t get_ir_relay_state(void)
{
    // Returns IR relay state from internal buffer last read from I2C bus
    return ir_relay_state_buffer;
}

void toggle_ir_relay_state(void)
{
    // Starts a timed task to toggle the bistable relay in the patchbay
    // TODO 
}

// Warning lights
//=============================================================================

void set_ethernet_fail_led(uint8_t value)
{
    // Sets the state of the Ethernet sync failure LED
    // TODO - check buffer before refreshing the outputs
    refresh_outputs();
}

void set_relay_fail_led(uint8_t value)
{
    // Sets the state of the IR relay sync failure LED
    // TODO - check buffer before refreshing the outputs 
    refresh_outputs();
}

// USB passthrough enables
//=============================================================================

void set_usb_enable_a(uint8_t value)
{
    // Sets the state the USB passthrough enable A
    // TODO - check buffer before refreshing the outputs
    refresh_outputs();
}

void set_usb_enable_b(uint8_t value)
{
    // Sets the state the USB passthrough enable B
    // TODO - check buffer before refreshing the outputs 
    refresh_outputs();
}