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
const uint8_t cameraSettingOptionsCount = 7;
const char* cameraSettingOptionsMenu[cameraSettingOptionsCount] = {
  "Exit",         "Set light mode",     "Set saturation", "Set brightness",
  "Set contrast", "Set special effect", "Reset settings"};

const char* cameraLightModeOptionsTitle = "Set light mode";
const uint8_t cameraLightModeOptionsCount = 6;
const char* cameraLightModeOptionsMenu[cameraLightModeOptionsCount] = {
  "Exit", "Auto", "Sunny", "Cloudy", "Office", "Home"};
const uint8_t cameraLightModeOptionsValues[cameraLightModeOptionsCount] = {
  0xFF, Auto, Sunny, Cloudy, Office, Home};

const char* cameraSaturationOptionsTitle = "Set saturation";
const uint8_t cameraSaturationOptionsCount = 6;
const char* cameraSaturationOptionsMenu[cameraSaturationOptionsCount] = {
  "Exit", "+2", "+1", "+0", "-1", "-2"};
const uint8_t cameraSaturationOptionsValues[cameraSaturationOptionsCount] = {
  0xFF, Saturation2, Saturation1, Saturation0, Saturation_1, Saturation_2};

const char* cameraBrightnessOptionsTitle = "Set brightness";
const uint8_t cameraBrightnessOptionsCount = 6;
const char* cameraBrightnessOptionsMenu[cameraBrightnessOptionsCount] = {
  "Exit", "+2", "+1", "+0", "-1", "-2"};
const uint8_t cameraBrightnessOptionsValues[cameraBrightnessOptionsCount] = {
  0xFF, Brightness2, Brightness1, Brightness0, Brightness_1, Brightness_2};

const char* cameraContrastOptionsTitle = "Set contrast";
const uint8_t cameraContrastOptionsCount = 6;
const char* cameraContrastOptionsMenu[cameraContrastOptionsCount] = {
  "Exit", "+2", "+1", "+0", "-1", "-2"};
const uint8_t cameraContrastOptionsValues[cameraContrastOptionsCount] = {
  0xFF, Contrast2, Contrast1, Contrast0, Contrast_1, Contrast_2};

const char* cameraSpecialEffectOptionsTitle = "Set special effect";
const uint8_t cameraSpecialEffectOptionsCount = 9;
const char* cameraSpecialEffectOptionsMenu[cameraSpecialEffectOptionsCount] = {
  "Exit",    "Antique",    "Bluish",   "Greenish",
  "Reddish", "Monochrome", "Negative", "Negative monochrome",
  "Normal"};
const uint8_t
  cameraSpecialEffectOptionsValues[cameraSpecialEffectOptionsCount] = {
    0xFF, Antique, Bluish, Greenish, Reddish, BW, Negative, BWnegative, Normal};

FsFile jpegFile;

void* JPEGOpen(const char* filename, int32_t* size) {
  // Serial.printf("Opening JPEG file %s\n", filename);
  jpegFile = sd.open(filename);
  if (jpegFile) {
    // Serial.println("Opened JPEG file successfully!");
  } else {
    Serial.println("Failed to open JPEG file!");
  }
  *size = jpegFile.size();
  // Serial.printf("Size of file is %d\n", size);
  return &jpegFile;
}

void JPEGClose(void* handle) {
  if (jpegFile) {
    // Serial.println("Closing JPEG file");
    jpegFile.close();
  } else {
    Serial.println("Failed to close file, maybe not open?");
  }
}

int32_t JPEGRead(JPEGFILE* handle, uint8_t* buffer, int32_t length) {
  if (!jpegFile) {
    Serial.printf("Attempted reading %ld bytes to %p but file is not open!\n",
                  length, buffer);
    return 0;
  } else {
    // Serial.printf("Reading %ld bytes to %p\n", length, buffer);
    const int16_t read = jpegFile.read(buffer, length);
    // Serial.printf("Read %d bytes from file\n", read);
    return read;
  }
}

int32_t JPEGSeek(JPEGFILE* handle, int32_t position) {
  if (!jpegFile) {
    Serial.printf("Attempted seeking to %ld but file is not open!\n", position);
    return 0;
  } else {
    // Serial.printf("Seeking to %ld\n", position);
    return jpegFile.seek(position);
  }
}

int JPEGDraw(JPEGDRAW* pDraw) {
  tft.setAddrWindow(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  tft.setSwapBytes(true);
  tft.pushPixels(pDraw->pPixels, pDraw->iWidth * pDraw->iHeight);
  return 1;
}

int JPEGDrawContained(JPEGDRAW* pDraw) {
  tft.startWrite();
  tft.setAddrWindow(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  tft.setSwapBytes(true);
  tft.pushPixels(pDraw->pPixels, pDraw->iWidth * pDraw->iHeight);
  tft.endWrite();
  return 1;
}

// https://github.com/greiman/SdFat/blob/master/examples/RtcTimestampTest/RtcTimestampTest.ino#L77
void dateTime(uint16_t* date, uint16_t* time, uint8_t* ms10) {
  DateTime now = rtc.now();
  *date = FS_DATE(now.year(), now.month(), now.day());
  *time = FS_TIME(now.hour(), now.minute(), now.second());
  *ms10 = now.second() & 1 ? 100 : 0;
}

const uint16_t HARDWARE_BEGIN_OK = 0b0000000000000000;
const uint16_t HARDWARE_BEGIN_RTC_FAIL = 0b0000000000000001;
const uint16_t HARDWARE_BEGIN_RTC_RESET = 0b0000000000000010;
const uint16_t HARDWARE_BEGIN_SD_FAIL = 0b0000000000000100;
const uint16_t HARDWARE_BEGIN_CAMERA_FAIL = 0b0000000000001000;
const uint16_t HARDWARE_BEGIN_GUI_FAIL = 0b0000000000010000;
const uint16_t HARDWARE_BEGIN_FAIL = 0b111111111111101;

uint16_t hardwareBegin() {
  uint8_t returnCode = HARDWARE_BEGIN_OK;

  tft.begin();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);

  Serial.println("Initiating hardware...");

  Serial.print("Trying RTC...");
  if (!rtc.begin()) {
    Serial.println("error!");
    Serial.println("Hardware initialization...error!");
    returnCode |= HARDWARE_BEGIN_RTC_FAIL;
  } else {
    Serial.println("ok!");
  }

  Serial.print("Checking for RTC power loss...");
  if (rtc.lostPower()) {
    Serial.println("error!");
    Serial.println("RTC lost power, setting to compile time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    returnCode |= HARDWARE_BEGIN_RTC_RESET;
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
    Serial.println("Hardware initialization...error!");
    returnCode |= HARDWARE_BEGIN_SD_FAIL;
  } else {
    Serial.println("ok!");
  }

  if (!arduCamera.begin(&sd)) {
    Serial.println("Hardware initialization...error!");
    returnCode |= HARDWARE_BEGIN_CAMERA_FAIL;
  }
  arduCamera.setImageSize(previewImageSize);
  arduCamera.loadCameraSettings();

  upButton.begin();
  selectButton.begin();
  downButton.begin();
  shutterButton.begin();

  if (!gui.begin(&tft, &sd, &rtc, &upButton, &downButton, &selectButton,
                 &shutterButton, BATT_PIN)) {
    returnCode |= HARDWARE_BEGIN_GUI_FAIL;
  }

  Serial.print("Hardware initialization returned 0b");
  Serial.println(returnCode, BIN);

  return returnCode;
}

void setup() {
  Serial.begin(9600);
  Serial.println("\n\nESP32 camera");

  const uint8_t hardwareBeginStatus = hardwareBegin();

  if ((hardwareBeginStatus & HARDWARE_BEGIN_FAIL) > 0) {
    const size_t msgSize = 128;
    char msg[msgSize];
    memset(msg, 0, msgSize);
    if ((hardwareBeginStatus & HARDWARE_BEGIN_SD_FAIL) > 0) {
      snprintf(msg, msgSize,
               "No micro SD card\ndetected! Make sure\nyou have a micro "
               "SD\ncard formatted with\nFAT32 or exFAT.",
               hardwareBeginStatus);
    } else {
      snprintf(msg, msgSize, "Failed to initialize\nhardware!\n\nError: 0x%X",
               hardwareBeginStatus);
    }
    gui.dialog("Hardware error", msg);
    while (true) {
      ;
    }
  }
  if (hardwareBeginStatus & HARDWARE_BEGIN_RTC_RESET) {
    gui.setBottomText("Clock lost power!", 3000);
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
          bool exitFileExplorer = false;
          int32_t startingIndex = 0;
          int32_t startingOffset = 0;
          char endingDirectory[MAX_PATH_SIZE];
          memset(endingDirectory, 0, MAX_PATH_SIZE);
          bool alreadyUseExplorer = false;
          while (!exitFileExplorer) {
            if (gui.fileExplorer(alreadyUseExplorer ? endingDirectory : "/",
                                 result, MAX_PATH_SIZE, startingIndex,
                                 startingOffset, endingDirectory, MAX_PATH_SIZE,
                                 &startingIndex, &startingOffset)) {
              if (jpeg.open(result, JPEGOpen, JPEGClose, JPEGRead, JPEGSeek,
                            JPEGDrawContained)) {
                Serial.println(
                  "Decoded headers successfully, opening image viewer");
                gui.imageViewer(&jpeg);
                alreadyUseExplorer = true;
              }
            } else {
              exitFileExplorer = true;
            }
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
                bool exitCameraSaturationOptionsMenu = false;
                uint8_t selected = 1;
                for (uint8_t i = 0; i < cameraSaturationOptionsCount; i++) {
                  if (cameraSaturationOptionsValues[i] ==
                      arduCamera.getSaturation()) {
                    selected = i;
                    break;
                  }
                }
                while (!exitCameraSaturationOptionsMenu) {
                  const uint8_t result = gui.menu(
                    cameraSaturationOptionsTitle, cameraSaturationOptionsMenu,
                    cameraSaturationOptionsCount, selected);
                  if (result > 0) {
                    selected = result;
                    arduCamera.setSaturation(
                      cameraSaturationOptionsValues[selected]);
                    const size_t bufSize = 32;
                    char buf[bufSize];
                    char buf2[bufSize];
                    memset(buf, 0, bufSize);
                    memset(buf2, 0, bufSize);
                    strncpy(buf2, cameraSaturationOptionsMenu[result], bufSize);
                    snprintf(buf, bufSize, "Set saturation to %s!",
                             strlwr(buf2));
                    gui.setBottomText(buf, 3000);
                  } else {
                    exitCameraSaturationOptionsMenu = true;
                  }
                }
                break;
              }
              case 3: {  // brightness
                bool exitCameraBrightnessOptionsMenu = false;
                uint8_t selected = 1;
                for (uint8_t i = 0; i < cameraBrightnessOptionsCount; i++) {
                  if (cameraBrightnessOptionsValues[i] ==
                      arduCamera.getBrightness()) {
                    selected = i;
                    break;
                  }
                }
                while (!exitCameraBrightnessOptionsMenu) {
                  const uint8_t result = gui.menu(
                    cameraBrightnessOptionsTitle, cameraBrightnessOptionsMenu,
                    cameraBrightnessOptionsCount, selected);
                  if (result > 0) {
                    selected = result;
                    arduCamera.setBrightness(
                      cameraBrightnessOptionsValues[selected]);
                    const size_t bufSize = 32;
                    char buf[bufSize];
                    char buf2[bufSize];
                    memset(buf, 0, bufSize);
                    memset(buf2, 0, bufSize);
                    strncpy(buf2, cameraBrightnessOptionsMenu[result], bufSize);
                    snprintf(buf, bufSize, "Set brightness to %s!",
                             strlwr(buf2));
                    gui.setBottomText(buf, 3000);
                  } else {
                    exitCameraBrightnessOptionsMenu = true;
                  }
                }
                break;
              }
              case 4: {  // contrast
                bool exitCameraContrastOptionsMenu = false;
                uint8_t selected = 1;
                for (uint8_t i = 0; i < cameraContrastOptionsCount; i++) {
                  if (cameraContrastOptionsValues[i] ==
                      arduCamera.getContrast()) {
                    selected = i;
                    break;
                  }
                }
                while (!exitCameraContrastOptionsMenu) {
                  const uint8_t result = gui.menu(
                    cameraContrastOptionsTitle, cameraContrastOptionsMenu,
                    cameraContrastOptionsCount, selected);
                  if (result > 0) {
                    selected = result;
                    arduCamera.setContrast(
                      cameraContrastOptionsValues[selected]);
                    const size_t bufSize = 32;
                    char buf[bufSize];
                    char buf2[bufSize];
                    memset(buf, 0, bufSize);
                    memset(buf2, 0, bufSize);
                    strncpy(buf2, cameraSaturationOptionsMenu[result], bufSize);
                    snprintf(buf, bufSize, "Set contrast to %s!", strlwr(buf2));
                    gui.setBottomText(buf, 3000);
                  } else {
                    exitCameraContrastOptionsMenu = true;
                  }
                }
                break;
              }
              case 5: {  // special effect
                bool exitCameraSpecialEffectOptionsMenu = false;
                uint8_t selected = 1;
                for (uint8_t i = 0; i < cameraSpecialEffectOptionsCount; i++) {
                  if (cameraSpecialEffectOptionsValues[i] ==
                      arduCamera.getSpecialEffect()) {
                    selected = i;
                    break;
                  }
                }
                while (!exitCameraSpecialEffectOptionsMenu) {
                  const uint8_t result =
                    gui.menu(cameraSpecialEffectOptionsTitle,
                             cameraSpecialEffectOptionsMenu,
                             cameraSpecialEffectOptionsCount, selected);
                  if (result > 0) {
                    selected = result;
                    arduCamera.setSpecialEffect(
                      cameraSpecialEffectOptionsValues[selected]);
                    const size_t bufSize = 32;
                    char buf[bufSize];
                    char buf2[bufSize];
                    memset(buf, 0, bufSize);
                    memset(buf2, 0, bufSize);
                    strncpy(buf2, cameraSpecialEffectOptionsMenu[result],
                            bufSize);
                    snprintf(buf, bufSize, "Set effect to %s!", strlwr(buf2));
                    gui.setBottomText(buf, 3000);
                  } else {
                    exitCameraSpecialEffectOptionsMenu = true;
                  }
                }
                break;
              }
              case 6: {  // reset
                arduCamera.resetCameraSettings();
                gui.setBottomText("Reset all settings!", 3000);
                exitCameraSettingOptionsMenu = true;
                break;
              }
            }
          }
          arduCamera.saveCameraSettings();
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
