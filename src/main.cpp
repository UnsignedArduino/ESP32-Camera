#include <Arduino.h>
#include <RTClib.h>
#include <SPI.h>

#include "TFT_eSPI.h"

#define halt()   \
  while (true) { \
    ;            \
  }

TFT_eSPI tft = TFT_eSPI();
RTC_DS3231 rtc;

void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(9600);

  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);

  tft.println();
  tft.println();

  if (!rtc.begin()) {
    tft.println("Couldn't find RTC");
    halt();
  }

  if (rtc.lostPower()) {
    tft.println("RTC lost power, setting to compile time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {
  DateTime now = rtc.now();

  tft.setCursor(0, 0);
  tft.printf("%d/%d/%d %.2d:%.2d:%.2d  ", now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());

  delay(1000);
}
