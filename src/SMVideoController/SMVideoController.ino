#include "SMVideoController.h"
#include "SMVideoController_panels.h"

#include <SoftwareSerial.h>

// Serial connection to VGA switcher
SoftwareSerial switcherSerial(0, 1); // RX, TX

// Keeps track of current source for each output (indexed 0-5)
byte ties[4] = {0, 1, 2, 3};

void setup() {
 setupControlPanels();

 // set the data rate for the SoftwareSerial port attached to the video switcher
 switcherSerial.begin(9600);

 // Setup initial state
 for (byte i = 0; i < 4; i++) {
    setTie(i, ties[i]);
    setPanelLEDs(i,ties[i]++);
 }
}

void loop() {
  // Loop through each destination control panel
  for (byte i = 0; i < 4; i++) {
    byte buttonPressed = getPanelButtonState(i);
    
    if (buttonPressed > 0) {
      // Something is pressed
      if ((buttonPressed-1) != ties[i]) {
        // and it's different from the current tie, so send command to switcher
        setTie(i,(buttonPressed-1));
        ties[i] = (buttonPressed-1);
      }
    }

    //Update LEDs
    setPanelLEDs(i,ties[i]++);
  }

}

void setTie(byte out, byte in) {
  out++;
  in++;
  String s = String(in) + '*' + String(out) + '!';
  switcherSerial.print(s);
}
