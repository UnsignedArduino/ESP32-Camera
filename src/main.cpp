#include <Arduino.h>
#include <RTClib.h>
#include <SPI.h>

#include "SdFat.h"
#include "TFT_eSPI.h"

const uint8_t SD_CS = 16;

TFT_eSPI tft = TFT_eSPI();
RTC_DS3231 rtc;
SdFs sd;

void printDirectory(FsFile dir, uint16_t numTabs) {
  while (true) {
    FsFile entry = dir.openNextFile();
    if (!entry) {
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      tft.print("  ");
    }
    char name[255] = {};
    entry.getName(name, 255);
    tft.print(name);
    if (entry.isDirectory()) {
      tft.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      tft.printf(" (%lu)\n", entry.size());
    }
    entry.close();
  }
}

void setup(void) {
  Serial.begin(9600);

  tft.begin();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);

  tft.println();
  tft.println();

  if (!rtc.begin()) {
    tft.println("Couldn't find RTC");
  }

  if (rtc.lostPower()) {
    tft.println("RTC lost power, setting to compile time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  if (!sd.begin(SD_CS, SD_SCK_MHZ(16))) {
    tft.println("Couldn't find SD card");
    if (sd.sdErrorCode()) {
      tft.print("SD error code: ");
      printSdErrorSymbol(&tft, sd.sdErrorCode());
      tft.print("\nSD error data: ");
      tft.println(sd.sdErrorData());
    }
  }

  tft.println("SD card contents: ");
  FsFile root = sd.open("/");
  printDirectory(root, 0);
  root.close();
}

void loop() {
  DateTime now = rtc.now();

  tft.setCursor(0, 0);
  tft.printf("%d/%d/%d %.2d:%.2d:%.2d  ", now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());

  delay(1000);
}
