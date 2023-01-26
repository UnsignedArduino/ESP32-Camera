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

const uint8_t previewImageSize = OV2640_160x120;
uint8_t captureImageSize = OV2640_1280x1024;

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

const uint8_t UP_BUTTON = 25;
const uint8_t SELECT_BUTTON = 33;
const uint8_t DOWN_BUTTON = 32;
const uint8_t SHUTTER_BUTTON = 26;

Button upButton(UP_BUTTON);
Button selectButton(SELECT_BUTTON);
Button downButton(DOWN_BUTTON);
Button shutterButton(SHUTTER_BUTTON);

const uint8_t BATT_PIN = A0;

ESP32CameraGUI gui;

const char* optionsTitle = "Options";
const uint8_t optionsCount = 4;
const char* optionsMenu[optionsCount] = {"Exit", "View files",
                                         "Change camera settings", "Set clock"};

const char* cameraSettingOptionsTitle = "Camera settings";
const uint8_t cameraSettingOptionsCount = 6;
const char* cameraSettingOptionsMenu[cameraSettingOptionsCount] = {
  "Exit",           "Set light mode", "Set saturation",
  "Set brightness", "Set contrast",   "Set special effect"};

const char* cameraLightModeOptionsTitle = "Set light mode";
const uint8_t cameraLightModeOptionsCount = 6;
const char* cameraLightModeOptionsMenu[cameraLightModeOptionsCount] = {
  "Exit", "Auto", "Sunny", "Cloudy", "Office", "Home"};
const uint8_t cameraLightModeOptionsValues[cameraLightModeOptionsCount] = {
  0xFF, Auto, Sunny, Cloudy, Office, Home};

const char* cameraSaturationOptionsTitle = "Set saturation";
const uint8_t cameraSaturationOptionsCount = 6;
const char* cameraSaturationOptionsMenu[cameraSaturationOptionsCount] = {
  "Exit",         "Saturation+2", "Saturation+1",
  "Saturation+0", "Saturation-1", "Saturation-2"};
const uint8_t cameraSaturationOptionsValues[cameraSaturationOptionsCount] = {
  0xFF, Saturation2, Saturation1, Saturation0, Saturation_1, Saturation_2};

const char* cameraBrightnessOptionsTitle = "Set brightness";
const uint8_t cameraBrightnessOptionsCount = 6;
const char* cameraBrightnessOptionsMenu[cameraBrightnessOptionsCount] = {
  "Exit",         "Brightness+2", "Brightness+1",
  "Brightness+0", "Brightness-1", "Brightness-2"};
const uint8_t cameraBrightnessOptionsValues[cameraBrightnessOptionsCount] = {
  0xFF, Brightness2, Brightness1, Brightness0, Brightness_1, Brightness_2};

const char* cameraContrastOptionsTitle = "Set contrast";
const uint8_t cameraContrastOptionsCount = 6;
const char* cameraContrastOptionsMenu[cameraContrastOptionsCount] = {
  "Exit", "Contrast+2", "Contrast+1", "Contrast+0", "Contrast-1", "Contrast-2"};
const uint8_t cameraContrastOptionsValues[cameraContrastOptionsCount] = {
  0xFF, Contrast2, Contrast1, Contrast0, Contrast_1, Contrast_2};

const char* cameraSpecialEffectOptionsTitle = "Set special effect";
const uint8_t cameraSpecialEffectOptionsCount = 9;
const char* cameraSpecialEffectOptionsMenu[cameraSpecialEffectOptionsCount] = {
  "Exit",    "Antique",    "Bluish",   "Greenish",
  "Reddish", "Blue-white", "Negative", "Blue-white negative",
  "Normal"};
const uint8_t
  cameraSpecialEffectOptionsValues[cameraSpecialEffectOptionsCount] = {
    0xFF, Antique, Bluish, Greenish, Reddish, BW, Negative, BWnegative, Normal};

int JPEGDraw(JPEGDRAW* pDraw) {
  tft.setAddrWindow(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  tft.setSwapBytes(true);
  tft.pushPixels(pDraw->pPixels, pDraw->iWidth * pDraw->iHeight);
  return 1;
}

// https://github.com/greiman/SdFat/blob/master/examples/RtcTimestampTest/RtcTimestampTest.ino#L77
void dateTime(uint16_t* date, uint16_t* time, uint8_t* ms10) {
  DateTime now = rtc.now();
  *date = FS_DATE(now.year(), now.month(), now.day());
  *time = FS_TIME(now.hour(), now.minute(), now.second());
  *ms10 = now.second() & 1 ? 100 : 0;
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

  FsDateTime::setCallback(dateTime);

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

  if (!arduCamera.begin(&sd)) {
    goto hardwareBeginError;
  }
  arduCamera.setImageSize(previewImageSize);

  upButton.begin();
  selectButton.begin();
  downButton.begin();
  shutterButton.begin();

  gui.begin(&tft, &sd, &rtc, &upButton, &downButton, &selectButton,
            &shutterButton, BATT_PIN);

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
          }
          break;
        }
        case 2: {
          bool exitCameraSettingOptionsMenu = false;
          while (!exitCameraSettingOptionsMenu) {
            switch (gui.menu(cameraSettingOptionsTitle,
                             cameraSettingOptionsMenu,
                             cameraSettingOptionsCount)) {
              default:
              case 0: {
                exitCameraSettingOptionsMenu = true;
                break;
              }
              case 1: {  // light mode
                bool exitCameraLightModeOptionsMenu = false;
                uint8_t selected = 1;
                for (uint8_t i = 0; i < cameraLightModeOptionsCount; i++) {
                  if (cameraLightModeOptionsValues[i] ==
                      arduCamera.getLightMode()) {
                    selected = i;
                    break;
                  }
                }
                while (!exitCameraLightModeOptionsMenu) {
                  const uint8_t result = gui.menu(
                    cameraLightModeOptionsTitle, cameraLightModeOptionsMenu,
                    cameraLightModeOptionsCount, selected);
                  if (result > 0) {
                    selected = result;
                    arduCamera.setLightMode(
                      cameraLightModeOptionsValues[selected]);
                    const size_t bufSize = 32;
                    char buf[bufSize];
                    char buf2[bufSize];
                    memset(buf, 0, bufSize);
                    memset(buf2, 0, bufSize);
                    strncpy(buf2, cameraLightModeOptionsMenu[result], bufSize);
                    snprintf(buf, bufSize, "Set light mode to %s!",
                             strlwr(buf2));
                    gui.setBottomText(buf, 3000);
                  } else {
                    exitCameraLightModeOptionsMenu = true;
                  }
                }
                break;
              }
              case 2: {  // saturation
                break;
              }
              case 3: {  // brightness
                break;
              }
              case 4: {  // contrast
                break;
              }
              case 5: {  // special effect
                break;
              }
            }
          }
          break;
        }
        case 3: {
          if (gui.changeRTCTime()) {
            gui.setBottomText("Set time successfully!", 3000);
          } else {
            gui.setBottomText("Canceled setting time!", 3000);
          }
          break;
        }
      }
    }
  } else if (shutterButton.pressed()) {
    gui.setBottomText("Taking photo...", UNLIMITED_BOTTOM_TEXT_TIME);
    gui.drawBottomToolbar();
    arduCamera.setImageSize(captureImageSize);
    delay(1000);
    const size_t result = arduCamera.captureToDisk();
    arduCamera.setImageSize(previewImageSize);
    delay(1000);
    if (result > 0) {
      gui.setBottomText("Photo saved!", 3000);
    } else if (result == CAMERA_ERROR) {
      gui.setBottomText("Camera error!", 3000);
    } else if (result == DISK_IO_ERROR) {
      gui.setBottomText("Failed to write to disk!", 3000);
    } else {
      gui.setBottomText("Unknown error!", 3000);
    }
  }

  gui.drawBottomToolbar();
}
