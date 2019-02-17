/**
 * LED Matrix control - basic I2C Slave controlling 1x-8x 8x8 MAX7219 LED Matrix displays
 * I2C Master
 * 
 * MIT License
 * Copyright (C) 2019 John Plocher
 */

#include <Arduino.h>
#include <Wire.h>

class I2CMatrix {
  public:
    /*
     * I2C Commands
     */
    enum {
        ROWMAJOR   = 0,
        COLMAJOR   = 1,
    };
    
    enum {
        RESET         = 0x00,   // R/W  any write causes a reset to defaults
        CONNECTED     = 0x01,   // R/W  which displays (1-8) are connected
        ENABLE        = 0x02,   // R/W  which displays (1-8) are turned on 
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
    
    I2CMatrix(byte address, byte numdisplays, byte brightness, byte mode) :
      _i2c_address(address),
      _num_displays(numdisplays),
      _brightness(brightness),
      _mode(mode),
      _enabled(0x00) {
      I2CMatrix::_init();
    }
    I2CMatrix(byte address, byte numdisplays) :
      _i2c_address(address),
      _num_displays(numdisplays),
      _brightness(0x01),
      _mode(0x00),
      _enabled(0x00) {
      I2CMatrix::_init();
    }

    byte getFWVersion(void);
    byte getNumDisplays(void);
    byte getEnabled(void);
    byte getRCMode(void);

    void writeDisplay(byte disp, byte* content);
    void writeColumn(byte disp, byte colnum, byte content);
    void writeRow(byte disp, byte rownum, byte content);
    void setBrightness(byte level); 

  private:
    void _init(void);
    void _readDeviceInfo(void);
    
    byte _i2c_address;
    byte _num_displays;
    byte _brightness;
    byte _mode;
    byte _enabled;
    byte _device_info[4];
};

