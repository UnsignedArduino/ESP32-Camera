#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <ArduCAM.h>

const uint8_t HSPI_CLK = 0;
const uint8_t HSPI_MOSI = 2;
const uint8_t HSPI_MISO = 15;
const uint8_t CAM_CS = 14;

class ArduCamera {
  public:
    bool begin();
    bool end();

    bool isConnected();

    size_t captureToMemory(uint8_t* dest, size_t destSize);

  protected:
    bool began = false;

    SPIClass* hspi = NULL;
    ArduCAM* camera = NULL;
};
