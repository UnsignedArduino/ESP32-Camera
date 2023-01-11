#include <Arduino.h>
#include <SPI.h>

#include "TFT_eSPI.h"

TFT_eSPI tft = TFT_eSPI();

void setup(void) {
  Serial.begin(9600);
  
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);

  tft.print("Hello, world!");
}

void loop() {
  ;
}
