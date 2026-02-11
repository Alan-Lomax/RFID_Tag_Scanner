/*
 * ILI9341_Landscape.h
 * 
 * Custom ILI9341 class for Chinese displays with swapped dimensions
 * Based on working stock ticker display code
 */

#ifndef ILI9341_LANDSCAPE_H
#define ILI9341_LANDSCAPE_H

#include <Adafruit_ILI9341.h>

class ILI9341_Landscape : public Adafruit_ILI9341 {
public:
  ILI9341_Landscape(int8_t cs, int8_t dc, int8_t rst = -1) 
    : Adafruit_ILI9341(cs, dc, rst) {}
  
  // Override setRotation to swap width and height for Chinese display
  void setRotation(uint8_t m) {
    // Call the parent class setRotation first
    Adafruit_ILI9341::setRotation(m);
    
    // Now swap the width and height to compensate for reversed display
    uint16_t temp = _width;
    _width = _height;
    _height = temp;
  }
};

#endif
