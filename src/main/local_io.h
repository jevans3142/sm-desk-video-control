// Local IO: Buttons, LEDs, IR relay IO, warning lights 
//-----------------------------------

#ifndef LOCAL_IO_H_INCLUDED
#define LOCAL_IO_H_INCLUDED

// I2C modules
#define I2C_NUM I2C_NUM_0        
#define I2C_FREQ 20000     
#define I2C_TX_BUFFER 0                     
#define I2C_RX_BUFFER 0                          
#define I2C_WRITE_BIT I2C_MASTER_WRITE  
#define I2C_READ_BIT I2C_MASTER_READ
#define I2C_ACK_CHECK_EN I2C_MASTER_ACK
#define I2C_NACK_CHECK_EN I2C_MASTER_NACK         

#define I2C_MODULE_A_ADDRESS 0x20
#define I2C_MODULE_B_ADDRESS 0x21   

#define I2C_MODULE_REGISTER_GP0 0x00
#define I2C_MODULE_REGISTER_GP1 0x01
#define I2C_MODULE_REGISTER_IODIR0 0x06
#define I2C_MODULE_REGISTER_IODIR1 0x07

// Define structures that can be used for state buffers and debouncing of IO
struct Input_Buffer_Struct {
    uint8_t button_panel[4]; // Four panels - 0 is unpressed 1-6 pressed
    uint8_t ir_button; // 0 is unpressed 1 pressed
    uint8_t ir_relay_aux; // 0 is off 1 on - aux contacts on relay
};

struct Output_Buffer_Struct {
    uint8_t led_panel[4]; // Four panels - 0 is unlit, 1-6 lit
    uint8_t ir_button_led; // 0 is off 1 on
    uint8_t ir_relay; // 0 is off 1 on - coil contacts
    uint8_t usb_enable_a; // 0 is disabled 1 enabled
    uint8_t usb_enable_b; // 0 is disabled 1 enabled
    uint8_t ethernet_fail_led; // 0 is off 1 on
    uint8_t relay_fail_led; // 0 is off 1 on
};

// Debounce properties
#define INPUT_DEBOUNCE_LOOP_COUNT 3
#define REFRESH_LOOP_TICKS 10

// How long do we wait when the PSU comes up
#define PSU_ON_WAIT_TICKS 100

void setup_local_io(QueueHandle_t* input_queue);

uint8_t get_button_panel_state(u_int8_t panel);
void set_button_led_state(uint8_t panel, uint8_t value);

uint8_t get_ir_button_state(void);
void set_ir_button_led(uint8_t value);

uint8_t get_ir_relay_state(void);
void set_ir_relay_state(uint8_t value);

void set_ethernet_fail_led(uint8_t value);
void set_relay_fail_led(uint8_t value);

void set_usb_enable_a(uint8_t value);
void set_usb_enable_b(uint8_t value);

#endif  