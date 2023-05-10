#pragma once

#include <Arduino.h>
#include <SdFat.h>
#include <TFT_eSPI.h>

// #define File FsFile
typedef FsFile File;

#include <Button.h>
#include <JPEGDEC.h>
#include <RTClib.h>

const uint32_t BOTTOM_TOOLBAR_DRAW_THROTTLE = 1000;
const uint32_t UNLIMITED_BOTTOM_TEXT_TIME = 0xFFFFFFFF;

class ESP32CameraGUI {
  public:
    bool begin(TFT_eSPI* tft, SdFs* sd, RTC_DS3231* rtc, Button* upButton,
               Button* downButton, Button* selectButton, Button* shutterButton,
               uint8_t battPin);

    void dialog(const char* title, const char* text);
    void dialog(const char* title, char* text) {
      this->dialog(title, (const char*)text);
    }
    uint8_t menu(const char* title, const char** menu, uint8_t menuCount,
                 uint8_t startingSelected = 0xFF);
    bool fileExplorer(const char* startDirectory, char* result,
                      size_t resultSize, int32_t startingFileIndex = 0,
                      int32_t startingOffset = 0, char* endingDirectory = NULL,
                      int32_t endingDirectorySize = -1,
                      int32_t* endingFileIndex = NULL,
                      int32_t* endingOffset = NULL);
    bool changeRTCTime();
    void imageViewer(const char* path, JPEGDEC* decoder);

    void setBottomText(const char* text, uint32_t expireTime);
    void setBottomText(char* text, uint32_t expireTime) {
      this->setBottomText((const char*)text, expireTime);
    }
    void drawBottomToolbar(bool forceDraw = false);

    bool getFileCount(const char* start, uint32_t& result);
    bool getFileFromIndex(const char* start, uint32_t index, FsFile* result);
    bool getFileNameFromIndex(const char* start, uint32_t index, char* result,
                              size_t resultSize);
    bool getIndexFromFileName(const char* start, const char* filename,
                              uint32_t* result);

    uint8_t getBattPercent();

  protected:
    bool began = false;

    uint8_t battPin;

    uint32_t lastBottomToolbarDraw = 0;
    bool needToRedrawBottom = true;
    bool hasCustomBottomText = false;
    uint32_t customBottomTextExpire = 0;
    static const size_t maxBottomTextSize = 26;
    char customBottomText[maxBottomTextSize + 1];

    TFT_eSPI* tft;
    SdFs* sd;
    RTC_DS3231* rtc;
    Button* upButton;
    Button* downButton;
    Button* selectButton;
    Button* shutterButton;
};
