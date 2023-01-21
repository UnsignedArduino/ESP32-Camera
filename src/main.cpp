#include <Arduino.h>
#include <RTClib.h>
#include <SPI.h>  // Needed by TFT_eSPI
#include <SdFat.h>
#include <TFT_eSPI.h>
#include <ESP32_Camera_GUI.h>
#include <ArduCAM.h>
#include <memorysaver.h>  // Needed by ArduCAM
#include <SD.h>           // Needed by JPEGDEC because it needs "File"
#include <JPEGDEC.h>
#include <ArduCamera.h>
#include <Button.h>

ArduCamera arduCamera;

const uint8_t SD_CS = 5;
#define SPI_CLOCK SD_SCK_MHZ(24)
#define SD_CONFIG SdSpiConfig(SD_CS, SHARED_SPI, SPI_CLOCK)

RTC_DS3231 rtc;
TFT_eSPI tft = TFT_eSPI();
SdFs sd;

// 160 * 120 * 12 / 8 = 28800 bytes needed
// https://stackoverflow.com/a/2734699/10291933
const uint32_t PREVIEW_BUF_SIZE = 160 * 120 * 12 / 8;
uint8_t previewBuf[PREVIEW_BUF_SIZE];
JPEGDEC jpeg;

const uint8_t UP_BUTTON = 26;
const uint8_t SELECT_BUTTON = 25;
const uint8_t DOWN_BUTTON = 33;
const uint8_t SHUTTER_BUTTON = 32;

Button upButton(UP_BUTTON);
Button selectButton(SELECT_BUTTON);
Button downButton(DOWN_BUTTON);
Button shutterButton(SHUTTER_BUTTON);

ESP32CameraGUI gui;

const char* optionsTitle = "Options";
const uint8_t optionsCount = 2;
const char* optionsMenu[optionsCount] = {"Cancel", "View files"};

int JPEGDraw(JPEGDRAW* pDraw) {
  tft.setAddrWindow(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  tft.setSwapBytes(true);
  tft.pushPixels(pDraw->pPixels, pDraw->iWidth * pDraw->iHeight);
  return 1;
}

bool hardwareBegin() {
  tft.begin();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);

  Serial.println("Initiating hardware...");

  Serial.print("Trying RTC...");
  if (!rtc.begin()) {
    Serial.println("error!");
    goto hardwareBeginError;
  } else {
    Serial.println("ok!");
  }

  Serial.print("Checking for RTC power loss...");
  if (rtc.lostPower()) {
    Serial.println("error!");
    Serial.println("RTC lost power, setting to compile time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  } else {
    Serial.println("ok!");
  }

  Serial.print("Trying SD card...");

  if (!sd.begin(SD_CONFIG)) {
    Serial.println("error!");
    if (sd.sdErrorCode()) {
      Serial.print("SD error code: ");
      printSdErrorSymbol(&Serial, sd.sdErrorCode());
      Serial.print("\nSD error data: ");
      Serial.println(sd.sdErrorData());
    }
    goto hardwareBeginError;
  } else {
    Serial.println("ok!");
  }

  if (!arduCamera.begin()) {
    goto hardwareBeginError;
  }

  upButton.begin();
  selectButton.begin();
  downButton.begin();
  shutterButton.begin();

  gui.begin(&tft, &sd, &rtc, &upButton, &downButton, &selectButton, &shutterButton);

  Serial.println("Hardware initialization...ok!");

  return true;

hardwareBeginError:

  Serial.println("Hardware initialization...error!");
  return false;
}

void setup() {
  Serial.begin(9600);
  Serial.println("\n\nESP32 camera");

  if (!hardwareBegin()) {
    while (true) {
      ;
    }
  }
}

void loop() {
  memset(previewBuf, 0, PREVIEW_BUF_SIZE);
  const size_t previewSize =
    arduCamera.captureToMemory(previewBuf, PREVIEW_BUF_SIZE);
  if (previewSize > 0) {
    if (jpeg.openRAM(previewBuf, previewSize, JPEGDraw)) {
      tft.startWrite();
      jpeg.decode(0, 0, 0);
      tft.endWrite();
      jpeg.close();
    }
  }

  if (selectButton.pressed()) {
    bool exitOptionsMenu = false;
    while (!exitOptionsMenu) {
      switch (gui.menu(optionsTitle, optionsMenu, optionsCount)) {
        default:
        case 0: {
          exitOptionsMenu = true;
          break;
        }
        case 1: {
          const size_t MAX_PATH_SIZE = 255;
          char result[MAX_PATH_SIZE];
          if (gui.fileExplorer("/", result, MAX_PATH_SIZE)) {
            exitOptionsMenu = true;
          }
          break;
        }
      }
    }
  }
}
