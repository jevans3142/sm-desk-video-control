// Pin definitions for IO
//-----------------------------------

#ifndef PINDEFS_H_INCLUDED
#define PINDEFS_H_INCLUDED

//Pin defs
#define PIN_I2C_SCL 33
#define PIN_I2C_SDA 32 

#define PIN_BUTTONS_INT 34 // Interrupt from I2C expander 

#define PIN_SDCARD_CMD 15
#define PIN_SDCARD_CLK 14
#define PIN_SDCARD_DTA 2

#define PIN_PHY_MDC 23
#define PIN_PHY_MDIO 18
#define PIN_PHY_RST -1
#define	PIN_PHY_POWER 12

#define PIN_AUX_IO_0 4 // Unused currently 
#define PIN_AUX_IO_1 5 // Unused currently
#define PIN_BOARD_LIVE 39 // Gets power state of SM desk

#endif  