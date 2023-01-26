#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SdFat.h>
#include <ArduCAM.h>

const uint8_t HSPI_CLK = 13;
const uint8_t HSPI_MOSI = 27;
const uint8_t HSPI_MISO = 14;
const uint8_t CAM_CS = 15;

const int32_t CAMERA_ERROR = -1;
const int32_t DISK_IO_ERROR = -2;

class ArduCamera {
  public:
    bool begin(SdFs* sd);
    bool end();

    bool isConnected();

    size_t captureToMemory(uint8_t* dest, size_t destSize);
    int32_t captureToDisk();

    void setImageSize(uint8_t size);
    void setLightMode(uint8_t mode);
    void setSaturation(uint8_t mode);
    void setBrightness(uint8_t brightness);
    void setContrast(uint8_t contrast);
    void setSpecialEffect(uint8_t effect);

    uint8_t getImageSize();
    uint8_t getLightMode();
    uint8_t getSaturation();
    uint8_t getBrightness();
    uint8_t getContrast();
    uint8_t getSpecialEffect();

    void getNextFilename(char* dest, size_t destSize);

  protected:
    bool began = false;

    uint32_t nextImageNumber = 0;

    uint8_t imageSize;
    uint8_t lightMode;
    uint8_t saturation;
    uint8_t brightness;
    uint8_t contrast;
    uint8_t specialEffect;

    SPIClass* hspi = NULL;
    ArduCAM* camera = NULL;
    SdFs* sd;
};
