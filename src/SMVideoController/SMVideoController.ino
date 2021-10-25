#include "SMVideoController.h"
#include "SMVideoController_panels.h"

#include <NativeEthernet.h>

// CONFIG====== 
IPAddress arduinoIp(192, 168, 1, 100); // Teensy 4.1 in SM desk
byte arduinoMac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress videohubIp(192, 168, 1, 200); // Switcher in patchbay
EthernetClient client;
//=============

void setup() {
 setupControlPanels();

  // start the Ethernet connection:
  Ethernet.begin(arduinoMac, arduinoIp);

  // give the Ethernet PHY a second to initialize; seems to improve reliability
  delay(1000);

  // TODO: Read in config from SD card
  
}

void loop() {

  // Keep our DHCP lease up to date
  Ethernet.maintain();
  
  if (! client.connected()) {
    Serial.println("Currently disconnected.");
    client.stop();

    delay(2500); // Give videohub time to boot/think

    Serial.println("connecting...");

    if (client.connect(videohubIp, 9990)) {
      Serial.println("connected");
    }
    else {
      // if we didn't get a connection to the server:
      Serial.println("connection failed");
    }
  }
  else {
    // do stuff here

    // Check if any buttons are pressed, and if different to local state 
    
  }
}


void setTie(byte out, byte in) {
  //Ethernet output for Blackmagic
  String s1 = "VIDEO OUTPUT ROUTING:\n"+ String(out) + " " + String(in) + "\n\n";
  client.print(s1);
}
