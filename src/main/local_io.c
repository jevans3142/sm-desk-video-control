// Local IO: Buttons, LEDs, IR relay IO, warning lights 
//-----------------------------------

#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"

#include "local_io.h"
#include "pindefs.h"

// Logging tag
static const char *TAG = "local_io";

// Define internal buffers that hold the 'current' state of the IO expanders
// When reading and writing to these the buffer is used to reduce I2C bus pileups

uint8_t button_panels_state_buffer[] = { 0, 0, 0, 0}; // Four panels - 0 is unpressed 1-6 pressed
uint8_t ir_button_state_buffer = 0; // 0 is unpressed 1 pressed 
uint8_t ir_button_led_state_buffer = 0; // 0 is off 1 on

uint8_t ir_relay_state_buffer = 0; // 0 is off 1 on

uint8_t ethernet_fail_led_state_buffer = 0; // 0 is off 1 on
uint8_t relay_fail_led_state_buffer = 0; // 0 is off 1 on

uint8_t usb_enable_a_state_buffer = 0; // 0 is disabled 1 enabled
uint8_t usb_enable_b_state_buffer = 0; // 0 is disabled 1 enabled

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

static esp_err_t i2c_init(void)
{
    // Sets up I2C communication
    uint8_t i2c_master_port = I2C_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = PIN_I2C_SDA;
    conf.sda_pullup_en = 0;
    conf.scl_io_num = PIN_I2C_SCL;
    conf.scl_pullup_en = 0;
    conf.master.clk_speed = I2C_FREQ;
    conf.clk_flags = 0;
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_RX_BUFFER, I2C_TX_BUFFER, 0);
}

static void expander_write_command(uint8_t address, uint8_t module_register, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, address << 1 | I2C_WRITE_BIT, I2C_ACK_CHECK_EN); 
    i2c_master_write_byte(cmd, module_register, I2C_ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data, I2C_ACK_CHECK_EN);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
}

static void expander_read_command(uint8_t address, uint8_t module_register, uint8_t* data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, address << 1 | I2C_WRITE_BIT, I2C_ACK_CHECK_EN); 
    i2c_master_write_byte(cmd, module_register, I2C_ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, address << 1 | I2C_READ_BIT, I2C_ACK_CHECK_EN); 
    
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
 
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

   
   
}

void setup_local_io(void)
{
    // Sets up IO expanders, sets default state of outputs
    ESP_ERROR_CHECK(i2c_init());

    // Module A - All outputs (Direction register all 0s)
    expander_write_command(I2C_MODULE_A_ADDRESS, I2C_MODULE_REGISTER_IODIR0, 0x00);
    expander_write_command(I2C_MODULE_A_ADDRESS, I2C_MODULE_REGISTER_IODIR1, 0x00);
    expander_write_command(I2C_MODULE_A_ADDRESS, I2C_MODULE_REGISTER_GP0, 0x00);
    expander_write_command(I2C_MODULE_A_ADDRESS, I2C_MODULE_REGISTER_GP1, 0x00);

    // Module A - 14 inputs, last 2 are outputs (Direction register 14 1s 2 0s)
    expander_write_command(I2C_MODULE_A_ADDRESS, I2C_MODULE_REGISTER_IODIR0, 0x00);
    expander_write_command(I2C_MODULE_A_ADDRESS, I2C_MODULE_REGISTER_IODIR1, 0x3F);
    expander_write_command(I2C_MODULE_A_ADDRESS, I2C_MODULE_REGISTER_GP0, 0x00);
    expander_write_command(I2C_MODULE_A_ADDRESS, I2C_MODULE_REGISTER_GP1, 0x00);
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