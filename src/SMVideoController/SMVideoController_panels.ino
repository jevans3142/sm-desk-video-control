#include "SMVideoController_panels.h"

// This file contains functions to interact with the control panels on the SM desk, including the IR button

// Pin setup
// =========

// LED outputs (4 panels, each 3-bit output ie. 1/2/4)
byte LEDPin[4][3] = {
  {  2,  3,  4},
  {  8,  9, 10},
  { 14, 15, 16},
  { 20, 21, 22}
};

// Button inputs (4 panels, each 3-bit input ie. 1/2/4)
byte buttonPin[4][3] = {
  {  5,  6,  7},
  { 11, 12, 13},
  { 17, 18, 19},
  { 23, 24, 25}
};

// IR button

byte IRButtonPin = 26;
byte IRLEDPin = 27;

// =========


void setupControlPanels() {

  // Called at boot

  // Set up pin modes for both input and outputs on the 4 panels
  for (byte i = 0; i < 4; i++) {
    for (byte j = 0; j < 3; j++) {
      pinMode(buttonPin[i][j], INPUT);
      pinMode(LEDPin[i][j], OUTPUT);
      digitalWrite(LEDPin[i][j], LOW);
    }
  }

  // Set up pin modes for both input and output to the IR camera button
  pinMode(IRButtonPin, INPUT);
  pinMode(IRLEDPin, OUTPUT);
  digitalWrite(IRLEDPin, LOW);

}

void setPanelLEDs(byte panel, byte index) {

  // Called to change which button is iluminated on one of the panels
  // By hardware design, only one can be lit at a time

  // Panel is from 0-3, index from 0-6 (where 0 will cause no button to light, 1-6 then are the buttons) 

  digitalWrite(LEDPin[panel][0], index & 1);
  digitalWrite(LEDPin[panel][1], index & 2);
  digitalWrite(LEDPin[panel][2], index & 4);
  
}

byte getPanelButtonState(byte panel) {

  // Returns which button is currently pressed/held for one of the panels
  // By hardware design, only one can be detected at a time
  // If more than one is pressed the highest numbered button will be returned
  // Returns 0-6 (where 0 is nothing pressed, 1-6 then are the buttons) 
  
  return (1*!digitalRead(buttonPin[panel][0]) + 2*!digitalRead(buttonPin[panel][1]) + 4*!digitalRead(buttonPin[panel][2]));
  
}
