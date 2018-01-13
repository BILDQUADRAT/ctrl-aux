#include <SPI.h>
#include <Ethernet.h>

#define aux 3

// Matrix Pins
//unsigned char ledX[] = { 15, 17, 19, 8 };
unsigned char ledX[] = { 18, 16, 14, 8 };
unsigned char ledY[] = { 5, 6 };

//unsigned char btnX[] = { 14, 16, 18, 3 };
unsigned char btnX[] = { 19, 17, 15, 3 };
unsigned char btnY[] = { 2, 7 };

#define numX 4
#define numY 2

boolean ledState[numY][numX] = {
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 }
};

uint16_t multiviewer[8];
boolean multiViewerSet = false;

// MAC address and IP address for this *particular* Arduino / Ethernet Shield!
// The MAC address is printed on a label on the shield or on the back of your device
// The IP address should be an available address you choose on your subnet where the switcher is also present
byte mac[] = {
  0x90, 0xB2, 0xDA, 0x0D, 0x6F, 0xC0 };      // <= SETUP!  MAC address of the Arduino
IPAddress clientIp(10, 0, 1, 241);        // <= SETUP!  IP address of the Arduino
IPAddress switcherIp(10, 0, 1, 30);     // <= SETUP!  IP address of the ATEM Switcher

// ATEM libraries
#include <ATEMbase.h>
#include <ATEMstd.h>
ATEMstd AtemSwitcher;

long time = 0;
long valueTime = 0;
byte auxValue = 0;
byte auxTarget = 0;

void initMatrix() {
  // LED
    // Set up rows
    for(int i = 0; i < numX; i++)
    {
        pinMode(ledX[i], OUTPUT);
        digitalWrite(ledX[i], LOW);
    }
    // Set up cols
    for(int j = 0; j < numY; j++)
    {
        pinMode(ledY[j], OUTPUT);
        digitalWrite(ledY[j], HIGH);
    }
  // Buttons
    // Set up rows
    for(int k = 0; k < numY; k++)
    {
        pinMode(btnY[k], OUTPUT);
        digitalWrite(btnY[k], HIGH);
    }
    // Set up cols
    for(int l = 0; l < numX; l++)
    {
        pinMode(btnX[l], INPUT_PULLUP);
    }
}

void setup() {
  randomSeed(analogRead(7));  // For random port selection

  // Ports konfigurieren
  initMatrix();

  // Start Ethernet
  //if(!Ethernet.begin(mac)) {
    Ethernet.begin(mac,clientIp);
  //}

  // Start ATEM
  AtemSwitcher.begin(switcherIp);
  AtemSwitcher.connect();

  time = millis();
}

void setMatrixOutput(int x, int y, boolean value) {
  ledState[y][x] = value;
}

byte getMatrixInput() {
  byte count = 0;
  for(int i = 0; i < numY; i++) {
    digitalWrite(btnY[i], LOW);

    for(int j = 0; j < numX; j++) {
      count++;
      if(digitalRead(btnX[j]) == LOW) {
        digitalWrite(btnY[i], HIGH);
        return count;
      }
    }

    digitalWrite(btnY[i], HIGH);
  }
  return 0;
}

void matrixLoop() {
  // Iterate rows
  for(int r = 0; r < numY; r++)
  {
      // Switch on a row
      digitalWrite(ledY[r], LOW);

      // Iterate Cols
      for(int c = 0; c < numX; c++)
      {
          digitalWrite(ledX[c], ledState[r][c]);
      }

      // Iterate Cols
      for(int c = 0; c < numX; c++)
      {
          // Switch off cols again
          digitalWrite(ledX[c], LOW);
      }

      // Switch off row again
      digitalWrite(ledY[r], HIGH);
  }
}

void loop() {
  AtemSwitcher.runLoop();

  if(AtemSwitcher.hasInitialized()) {
    if(!multiViewerSet) {
      // Multiviewer Config auslesen, um Belegung der Buttons festzulegen
      for(byte i = 0; i < 8; i++) {
        multiviewer[i] = i+1;
      }
      multiViewerSet = true;
    } else {
      // Button lesen
      byte auxInput = getMatrixInput();
      if(auxInput != auxTarget && auxInput != 0) {
        auxTarget = auxInput;
      }

      // Aux setzen
      time = millis();
      if(time-valueTime > 500) {
        if(auxTarget != 0 && auxValue != multiviewer[auxTarget-1]) {
          auxValue = multiviewer[auxTarget-1];
          AtemSwitcher.setAuxSourceInput(aux, auxValue);
          valueTime = time;
        }
      }

      // Aux-Wert in Matrix setzen
      int tempAuxValue = AtemSwitcher.getAuxSourceInput(aux);
      if(tempAuxValue != auxValue) {
        auxValue = tempAuxValue;
        int count = 0;
        for(int i = 0; i < numY; i++) {
          for(int j = 0; j < numX; j++) {
            count++;
            if(auxValue == multiviewer[count-1]) {
              ledState[i][j] = true;
            } else {
              ledState[i][j] = false;
            }
          }
        }
      }
    }

    matrixLoop();
  }
}
