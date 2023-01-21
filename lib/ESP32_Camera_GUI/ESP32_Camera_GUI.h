#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SdFat.h>
#include <RTClib.h>
#include <Button.h>

class ESP32CameraGUI {
  public:
    bool begin(TFT_eSPI* tft, SdFs* sd, RTC_DS3231* rtc, Button* upButton,
               Button* downButton, Button* selectButton, Button* shutterButton);

    uint8_t menu(const char* title, const char** menu, uint8_t menuCount);
    bool fileExplorer(char* startDirectory, char* result, size_t resultSize);
    void drawBottomToolbar();

    bool getFileCount(char* start, uint32_t& result);
    bool getFileNameFromIndex(char* start, uint32_t index, char* result,
                              size_t resultSize);

  protected:
    bool began = false;

    TFT_eSPI* tft;
    SdFs* sd;
    RTC_DS3231* rtc;
    Button* upButton;
    Button* downButton;
    Button* selectButton;
    Button* shutterButton;
};
