#include <Arduino.h>
#include "ESP32_Camera_GUI.h"

uint8_t ESP32CameraGUI::getBattPercent() {
  const uint16_t milliVolts = map(analogRead(this->battPin), 0, 4096, 0, 3300);
  const uint16_t battVolt = milliVolts * 2;

  // https://blog.ampow.com/lipo-voltage-chart/
  if (battVolt > 4200) {
    return 100;
  } else if (battVolt > 4150) {
    return 95;
  } else if (battVolt > 4110) {
    return 90;
  } else if (battVolt > 4080) {
    return 85;
  } else if (battVolt > 4020) {
    return 80;
  } else if (battVolt > 3980) {
    return 75;
  } else if (battVolt > 3950) {
    return 70;
  } else if (battVolt > 3910) {
    return 65;
  } else if (battVolt > 3870) {
    return 60;
  } else if (battVolt > 3850) {
    return 55;
  } else if (battVolt > 3840) {
    return 50;
  } else if (battVolt > 3820) {
    return 45;
  } else if (battVolt > 3800) {
    return 40;
  } else if (battVolt > 3790) {
    return 35;
  } else if (battVolt > 3770) {
    return 30;
  } else if (battVolt > 3750) {
    return 25;
  } else if (battVolt > 3730) {
    return 20;
  } else if (battVolt > 3710) {
    return 15;
  } else if (battVolt > 3690) {
    return 10;
  } else if (battVolt > 3061) {
    return 5;
  } else {
    return 0;
  }
}
