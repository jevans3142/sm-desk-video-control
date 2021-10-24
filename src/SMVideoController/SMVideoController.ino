#include "SMVideoController.h"
#include "SMVideoController_panels.h"

#include <SoftwareSerial.h>

SoftwareSerial switcherSerial(0, 1); // RX, TX

void setup() {
 setupControlPanels();

 // set the data rate for the SoftwareSerial port attached to the video switcher
 switcherSerial.begin(9600);
}

void loop() {
  
}
