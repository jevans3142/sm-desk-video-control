<img width="280" align="right" src="https://raw.githubusercontent.com/CHTJonas/roombooking/master/public/logo-long-black.svg?sanitize=true">

# ADC Theatre - SM Desk Video Switcher

This repository hosts the hardware (in the form of KiCad schematics and PCB layouts) and 
firmware (in the form of C code) of the [ADC Theatre's](https://www.adctheatre.com) Stage 
Manager's Desk Video Switcher. This is a control panel within the SM desk on stage which 
provides remote control of a Blackmagic Design Videohub SDI switcher via Ethernet.

## Features
* Controls four screen outputs on the SM Desk, with six sources (which can differ for each of the screens) routable via 24 buttons on the desk
* Provides a special 'Show Relay' source which is switched between the Main and IR camera both on the SM Desk and around the building
* Is the interface for the Main/IR switch button the SM Desk, and also controls the mains contactor to activate the IR floodlights when in IR camera mode

## Configuration

Although some things such as number of buttons etc are fixed in hardware, some aspects can be customised via a text file 
on a microSD card read at boot. This provides the ADC with the ability to reconfigure for example which sources are availible on the SM desk without having to recompile firmware (as long as the physical buttons are relabeled of course!)

Items which can be configured: 
* Which router inputs are used as sources for each of the destinations 
* For the special 'Show Relay' source, which router inputs are the Main and IR camera 
* Which router outputs are used as 'Show Relay' outputs and should be automatically switched between Main and IR cameras in sync with the SM Desk
* IP address of the router


## Compilation
The microcontroller used is an ESP32 on an Olimex ESP32-PoE-ISO board. After standard installation of the esp-idf FreeRTOS toolchain, currently building on v5.1.1 as a stable version with the configuration included in the src folder (ie. when building do not run the idf.py set-target esp32 command as directed in the esp-idf Getting Started instructions to set up the default build config - just go straight to idf.py build)

## Hardware

There are two types of PCB required. Four 'switch-module' PCBs sit behind the four sets of buttons on the SM desk (four 
destinations with six sources each). One 'main-board' PCB is connected to these panel boards via ribbon cables, and also 
to Ethernet and power.

## Copyright
Copyright (c) 2021-2022 John Evans and contributors.
The ADC SM Desk Video Switcher hardware and firmware is released under Version 3 of the GNU General Public License.
See the LICENSE file for full details.
