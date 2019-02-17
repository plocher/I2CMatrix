/**
 * LED Matrix control - basic I2C Slave controlling 1x-8x 8x8 MAX7219 LED Matrix displays
 * I2C Abstraction Library
 * 
 * MIT License
 * Copyright (C) 2019 John Plocher
 */

#include <I2CMatrix.h>
#include <Wire.h>

byte I2CMatrix::getFWVersion(void)   { return I2CMatrix::_device_info[0]; }
byte I2CMatrix::getNumDisplays(void) { return I2CMatrix::_device_info[1]; }
byte I2CMatrix::getEnabled(void)     { return I2CMatrix::_device_info[2]; }
byte I2CMatrix::getRCMode(void)      { return I2CMatrix::_device_info[3]; }

void I2CMatrix::writeDisplay(byte disp, byte* content) {
    Wire.beginTransmission(I2CMatrix::_i2c_address); 
    Wire.write(I2CMatrix::UNIT);             // Select Display 
    Wire.write(disp);
	for(int idy = 7; idy >= 0; idy--) {
	  Wire.write(content[idy]);  
	}
    Wire.endTransmission();
}

void I2CMatrix::writeColumn(byte disp, byte colnum, byte content) {
  bool isColumn = I2CMatrix::_mode & (0x01 << disp);
  if (!isColumn) {
      I2CMatrix::_mode |= (0x01 << disp);
      Wire.beginTransmission(I2CMatrix::_i2c_address); 
      Wire.write(I2CMatrix::MODE);
      Wire.write(I2CMatrix::_mode);
      Wire.endTransmission();
  }
  Wire.beginTransmission(I2CMatrix::_i2c_address); 
  Wire.write(I2CMatrix::UNIT);             // Select Display 
  Wire.write(disp);
  Wire.endTransmission();

  Wire.beginTransmission(I2CMatrix::_i2c_address); 
  Wire.write(I2CMatrix::BYTE1 + colnum);   // Select Column to update 
  Wire.write(content);  
  Wire.endTransmission();
}

void I2CMatrix::writeRow(byte disp, byte rownum, byte content) {
  bool isColumn = I2CMatrix::_mode & (0x01 << disp);
  if (isColumn) {
      I2CMatrix::_mode &= ~(0x01 << disp);
      Wire.beginTransmission(I2CMatrix::_i2c_address); 
      Wire.write(I2CMatrix::MODE);
      Wire.write(I2CMatrix::_mode);
      Wire.endTransmission();
  }
  Wire.beginTransmission(I2CMatrix::_i2c_address); 
  Wire.write(I2CMatrix::UNIT);             // Select Display 
  Wire.write(disp);
  Wire.endTransmission();

  Wire.beginTransmission(I2CMatrix::_i2c_address); 
  Wire.write(I2CMatrix::BYTE1 + rownum);   // Select Row to update 
  Wire.write(content);  
  Wire.endTransmission();
}

void I2CMatrix::setBrightness(byte level) {
    I2CMatrix::_brightness = level & 0x0F;
    Wire.beginTransmission(I2CMatrix::_i2c_address); 
    Wire.write(I2CMatrix::BRIGHTNESS);
    Wire.write(I2CMatrix::_brightness);
    Wire.endTransmission();
}

 void I2CMatrix::_init(void) {
    Wire.begin();

    Wire.beginTransmission(I2CMatrix::_i2c_address); 
    Wire.write(I2CMatrix::RESET);            // Put device in known state 
    Wire.write(0x00);             // value doesn't matter for RESET...
    Wire.endTransmission();  
    
    delay(500);                   // wait for unit to re-initialize

    I2CMatrix::_enabled = 0;
    for (int x = I2CMatrix::_num_displays; x > 0; x--) {
      I2CMatrix::_enabled |= (1 << (x-1));
    }
    Wire.beginTransmission(I2CMatrix::_i2c_address); 
    Wire.write(I2CMatrix::CONNECTED);        // How many Displays connected?
    Wire.write(I2CMatrix::_num_displays);    // 
    Wire.write(I2CMatrix::_enabled);         // devices 1 and 2 are enabled
    Wire.write(I2CMatrix::_brightness);      // medium brightness
    Wire.write(B00000000);        // all taking row-major data
    Wire.endTransmission();       // stop transmitting
    
    I2CMatrix::_readDeviceInfo();
}

void I2CMatrix::_readDeviceInfo(void) {
    Wire.beginTransmission(I2CMatrix::_i2c_address);
    Wire.write(I2CMatrix::GETVERSION);
    Wire.endTransmission(0);  
    Wire.requestFrom((int)I2CMatrix::_i2c_address, (int)4);
    I2CMatrix::_device_info[0] = Wire.read() & 0x7F;
    I2CMatrix::_device_info[1] = Wire.read() & 0x7F;
    I2CMatrix::_device_info[2] = Wire.read() & 0x7F;
    I2CMatrix::_device_info[3] = Wire.read() & 0x7F;
    // Wire.endTransmission();       // I2C transaction Auto termintes after 4th read()
}


