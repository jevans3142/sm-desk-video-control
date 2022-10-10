// Local IO: Buttons, LEDs, IR relay IO, warning lights 
//-----------------------------------

#ifndef LOCAL_IO_H_INCLUDED
#define LOCAL_IO_H_INCLUDED

// I2C modules
#define I2C_NUM I2C_NUM_0        
#define I2C_FREQ 100000     
#define I2C_TX_BUFFER 0                     
#define I2C_RX_BUFFER 0                          
#define I2C_WRITE_BIT I2C_MASTER_WRITE  
#define I2C_READ_BIT I2C_MASTER_READ
#define I2C_ACK_CHECK_EN 0x1      

#define I2C_MODULE_A_ADDRESS 0x20
#define I2C_MODULE_B_ADDRESS 0x21   

#define I2C_MODULE_REGISTER_GP0 0x00
#define I2C_MODULE_REGISTER_GP1 0x01
#define I2C_MODULE_REGISTER_IODIR0 0x06
#define I2C_MODULE_REGISTER_IODIR1 0x07

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