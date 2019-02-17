/**
 * LED Matrix control - basic I2C Slave controlling 1x-8x 8x8 MAX7219 LED Matrix displays
 *
 * I2C Slave
 *
 * My test board for the I2CMatrix Slave has as an Arduino Pro Mini connected to 
 * a pair of 8x8 MAX7219 controlled matrixes
 * It depends on the LedControl library to bit-bang the commands out a set of digital pins
 * The board has pins for setting the I2C address; this sketch ignores them and hardcodes it to "1".
 *
 * See http://www.spcoast.com/wiki/index.php/I2C-Codeline-Matrix for the EagleCAD schematic and board
 * 
 * MIT License
 * Copyright (C) 2019 John Plocher
 */

#include <Wire.h>
#include <LedControl.h>
#include <elapsedMillis.h>

#define SLAVE_ADDRESS 0x01
#define PROTOCOL_VERSION 0x02
#define MAX_WIRE_LENGTH 68  


#define DEBUG_PIN     13      // for debugging
#define DEBUG_ON  HIGH
#define DEBUG_OFF  LOW


/**
 * Params :
 * int dataPin    The pin on the Arduino where data gets shifted out
 * int clockPin   The pin for the clock
 * int csPin      The pin for selecting the device when data is to be sent
 * int numDevices The maximum number of devices that can be controlled
 */
LedControl led=LedControl( 4,  2,  3 ,2);    // Port A, pin 2 unused [ 2, 3, 4, 5]


/*
 * I2C Commands
 */
#define ROWMAJOR 0
#define COLMAJOR 1

// this enum copy MUST match that in I2CMatrix.h 
// TODO: include that header, use its enum...

enum {
    RESET         = 0x00,   // R/W  any write causes a reset to defaults
    CONNECTED     = 0x01,   // R/W  How many displays (1-8, integer) are connected
    ENABLE        = 0x02,   // R/W  which displays (1-8, bitmap) are turned on 
    BRIGHTNESS    = 0x03,   // R/W  global brightness bottom 4 bits brightness, top 4 bits UNUSED
    MODE          = 0x04,   // R/W  per display mode:  row=0, column=1
    UNIT          = 0x05,   // W/O  which display unit that the next 8 bytes are addressed to
    BYTE1         = 0x06,   // W/O  DATA BYTE #1
    BYTE2         = 0x07,   // W/O  DATA BYTE #2
    BYTE3         = 0x08,   // W/O  DATA BYTE #3
    BYTE4         = 0x09,   // W/O  DATA BYTE #4
    BYTE5         = 0x0A,   // W/O  DATA BYTE #5
    BYTE6         = 0x0B,   // W/O  DATA BYTE #6
    BYTE7         = 0x0C,   // W/O  DATA BYTE #7
    BYTE8         = 0x0D,   // W/O  DATA BYTE #8
    GETVERSION    = 0x0E,   // R/O  1 byte - protocol version

    NUMCOMMANDS
};

struct Registers {
  byte connectedDevices;
  byte enable;
  byte brightness;
  byte rcmode;
  byte selected;
  byte data[8][8];
};

struct Changes {
  bool needreset;
  bool connectedDevices;
  byte enable;
  byte brightness;
  byte rcmode;
  bool data[8];
};

// elapsedMillis et;
// anything modified by the ISR must be volatile!
volatile byte      receivedCommand;      // sent by master, used to determine how to respond to reads
volatile byte      currentCommand;      
volatile Registers data;
volatile Changes   changes;
volatile byte      rawdata[MAX_WIRE_LENGTH+1];

// debugging print helper - print data as it would be displayed...
void PB(int var) {
  for (unsigned int test = 0x80; test; test >>= 1) {
    Serial.write(var  & test ? '#' : ' ');
  }
  Serial.println();
}

/** 
 *  callback from Wire - the master is writing to us, I2C conversation is in progress...
 *  Possibilities:
 *  1.  The master is setting the address in preparation to read back data from that address
 *  2.  The master is setting the address to write a single byte of data to that address
 *  3.  The master is setting the address to write more than one byte of data to consecutive addresses
 */


void receiveEvent(int bytesReceived) {
    digitalWrite(DEBUG_PIN, DEBUG_ON);  // light up the pin, since someone is talking to us...
    for (int x = 0; x < bytesReceived; x++) {
        if (x < MAX_WIRE_LENGTH) {
          rawdata[x] = Wire.read();
        } else {
          (void)Wire.read();    // throw extras away
        }
    }
    receivedCommand = rawdata[0];
    if (receivedCommand >= NUMCOMMANDS) receivedCommand = GETVERSION;
    
    if (bytesReceived == 1) {  // One byte means setting an address for a following read
        digitalWrite(DEBUG_PIN, DEBUG_OFF);  // all done ...
        return;
    } 
    // otherwise, we have a stream to handle...
    currentCommand = receivedCommand;
    if (bytesReceived > MAX_WIRE_LENGTH) bytesReceived = MAX_WIRE_LENGTH;
    
    int x = 1;                          // data being processed
    while (bytesReceived > 1) {
      switch (currentCommand) {
        case RESET:       // data is ignored...
                      changes.needreset = true;
                      digitalWrite(DEBUG_PIN, DEBUG_OFF);  // all done ...
                      return;
        case CONNECTED:   // devices are daisy chained, addresses are sequential, thus only need total number
                      data.   connectedDevices = rawdata[x++];
                      changes.connectedDevices = true;

                      if (data.connectedDevices > 8) data.connectedDevices = 8;
                      currentCommand++;
                      bytesReceived--;
                      break;
        case ENABLE:    // bitmask of enabled devices
                      data.   enable = rawdata[x++];
                      changes.enable = true;
                      currentCommand++;
                      bytesReceived--;
                      break;
        case BRIGHTNESS:  // 0-15 
                      data.   brightness = rawdata[x++];
                      changes.brightness = true;
                      if (data.brightness > 15) data.brightness = 15;
                      currentCommand++;
                      bytesReceived--;
                      break;
        case MODE:
                      data.   rcmode = rawdata[x++];
                      changes.rcmode = true;
                      currentCommand++;
                      bytesReceived--;
                      break;
        case UNIT:
                      data.selected = rawdata[x++];
                      // Serial.print("UNIT"); Serial.println(data.selected,          DEC);

                      currentCommand++;
                      bytesReceived--;
                      break;
        case BYTE1:
        case BYTE2:
        case BYTE3:
        case BYTE4:
        case BYTE5:
        case BYTE6:
        case BYTE7:
        case BYTE8:
                      data.   data[data.selected][currentCommand - BYTE1] = rawdata[x++];
                      changes.data[data.selected] = true;

                      //Serial.print("BYTE"); Serial.print(currentCommand - BYTE1, DEC); Serial.print(" = "); 
                      //PB(data.   data[data.selected][currentCommand - BYTE1]);
                      
                      if (currentCommand == BYTE8) {
                          currentCommand = BYTE1;
                          if (data.selected == 0) {  // only daisy chain first two displays due to TwoWire buffer limits
                            data.selected++;
                          } else {
                            digitalWrite(DEBUG_PIN, DEBUG_OFF);  // all done ...
                            return;
                          }
                      } else {
                          currentCommand++;
                      } 

                      bytesReceived--;
                      break;
        case GETVERSION:
                      // ignore writes to a R/O register
                      digitalWrite(DEBUG_PIN, DEBUG_OFF);  // all done ...
                      return;
      }
    }
    digitalWrite(DEBUG_PIN, DEBUG_OFF);  // all done ...
}

void requestEvent(void){
    byte returndata[4];
    returndata[0] = PROTOCOL_VERSION;
    returndata[1] = data.connectedDevices;
    returndata[2] = data.enable;
    returndata[3] = data.rcmode;
    Wire.write(returndata, 4);
}

#define getIntensity(rd)           (((rd)->brightness & 0x0F))
#define getConnected(rd)           (((rd)->connectedDevices))
#define getEnabled(rd, disp)       (((rd)->enable >> (disp)) & 0x01)

#define getMode(rd, disp)    (((rd->rcmode >> (disp)) & 0x01) == 0 ? ROWMAJOR : COLMAJOR)
#define getData(rd, disp, y) (rd->data[disp][y])

void doIntensity(volatile Registers *data) {
    byte intensity = getIntensity(data);
    for (int disp = 0; disp <= getConnected(data); disp++) {
        led.setIntensity(disp, intensity);   // sets brightness (0~15 possible values)
    }
}

void doClear(volatile Registers *data) {
    for (int disp = 0; disp <= getConnected(data); disp++) {
        changes.data[disp] = true;
        led.clearDisplay(disp);              // clear screen
    }
}

void doShutdown(volatile Registers *data) {
    for (int disp = 0; disp <= getConnected(data); disp++) {
        byte enable = getEnabled(data, disp);
        led.shutdown(disp, (enable == 1) ? false : true);   // turn off power saving, enables display
    }
}

void doDisplayData(volatile Registers *data) {
    for (int disp = 0; disp <= getConnected(data); disp++) {
        byte rcmode = getMode(data, disp);
        for (byte y = 0; y < 8; y++) {
          if (rcmode == ROWMAJOR) {
            led.setRow(disp,y, getData(data, disp, y));
          } else {
            led.setColumn(disp,y, getData(data, disp, y));
          }
        }
        changes.data[disp] = false;  
    }
}

bool isDataChanged(byte disp) {
    return changes.data[disp];    
}
bool isDataChanged(void) {
    for (byte disp = 0; disp <= getConnected(&data); disp++) {
      if (isDataChanged(disp)) return true;
    }
    return false;    
}

void clearDataChanges() {
    for (int x = 0; x < 8; x++) {
        changes.data[x] = false;
    }
}
void doRESET(void) {
      data.connectedDevices = 2;   // number of displays
      data.enable           = B00000011;   // 2x displays on
      data.brightness       = 0x01;        // dim
      data.rcmode           = 0x00;        // all displays row-major...
      data.selected         = 0;

      doShutdown(&data);
      doIntensity(&data);
      doClear(&data);
      
      setup_LEDCLEAR();
      setup_LEDTEST();
      delay(80);
      setup_LEDCLEAR();

      changes.needreset        = false;
      changes.connectedDevices = false;
      changes.enable           = false;
      changes.rcmode           = false;

      clearDataChanges();
}


void setup_LEDTEST(void) {
      data.data[0][0] = 0x00;     // "C"ontrols
      data.data[0][1] = 0x3E;
      data.data[0][2] = 0x02;
      data.data[0][3] = 0x02;
      data.data[0][4] = 0x02;
      data.data[0][5] = 0x02;
      data.data[0][6] = 0x3E;
      data.data[0][7] = 0x00;
      
      data.data[1][0] = 0x00;     // "I"ndications
      data.data[1][1] = 0x3E;
      data.data[1][2] = 0x08;
      data.data[1][3] = 0x08;
      data.data[1][4] = 0x08;
      data.data[1][5] = 0x08;
      data.data[1][6] = 0x3E;
      data.data[1][7] = 0x00;
      doDisplayData(&data);
}

void setup_LEDCLEAR(void) {
      for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            data.data[x][y] = 0x00;
        }
    }
    doDisplayData(&data);
}

void setup() {
    //Initialize serial and wait for port to open:
    Serial.begin(115200);
    while (!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
    }
    Serial.println("I2C Slave - John Plocher");
    Serial.print("Version "); Serial.println(PROTOCOL_VERSION, HEX);
    
    pinMode(DEBUG_PIN, OUTPUT);
    // I2C and LEDMatrix pin initializations handled by their own subsystems
    
    // set defaults
    Wire.begin(SLAVE_ADDRESS);
    Wire.onRequest(requestEvent);
    Wire.onReceive(receiveEvent);
    
    doRESET();
}

void loop() {
  if (changes.needreset) {
      changes.needreset        = false;
      doRESET();
  }

  if (changes.connectedDevices) {
      changes.connectedDevices = false;
      changes.enable           = false;
      changes.rcmode           = false;
      doClear(&data);
      doIntensity(&data);
      clearDataChanges();
  }
  
  if (changes.brightness ) {
      changes.brightness       = false;
      doIntensity(&data);
  }

  if (changes.enable ) {
      changes.enable           = false;
      doShutdown(&data);
      clearDataChanges();
  }
  if (changes.rcmode ) {
      changes.rcmode           = false;
      doClear(&data);
      clearDataChanges();
  }

  if (isDataChanged()) {
      clearDataChanges();
      doDisplayData(&data);
  }
}

