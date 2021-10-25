#ifndef _SMVIDEOCONTROLLER_PANELS_H 
#define _SMVIDEOCONTROLLER_PANELS_H 

void setupControlPanels();
void setPanelLEDs(byte panel, byte index);
byte getIRButtonState();
void setIRButtonLED(byte state);
void setRelayFailLED(byte state);
void setEthernetFailLED(byte state);

#endif //_SMVIDEOCONTROLLER_PANELS_H 
