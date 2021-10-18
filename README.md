<img width="280" align="right" src="https://raw.githubusercontent.com/CHTJonas/roombooking/master/public/logo-long-black.svg?sanitize=true">

# ADC Theatre - SM Desk Video Switcher

This repository hosts the hardware (in the form of KiCad schematics and PCB layouts) and 
firmware (in the form of Arduino code) of the [ADC Theatre's](https://www.adctheatre.com) Stage 
Manager's Desk Video Switcher. This is a control panel within the SM desk on stage which 
provides remote control of a Blackmagic Design Videohub SDI switcher via Ethernet.

## Compilation
The microcontroller used is a PJRC Teensy 4.1. As with most Teensy brand microcontrollers, code is compiled and uploaded via their modified Arduino IDE 'Teensyduino'. The version currently used is 1.8.16. Settings used for the board are the default as follows: 
* Board: Teensy 4.1
* USB Type: Serial
* CPU Speed: 600MHz
* Optimize: Faster
* Keyboard Layout: US English  

## Copyright
Copyright (c) 2021 John Evans and contributors.
The ADC SM Desk Video Switcher hardware and firmware is released under Version 3 of the GNU General Public License.
See the LICENSE file for full details.
