#include <Arduino.h>
#include "ESP32_Camera_GUI.h"

void ESP32CameraGUI::imageViewer(const char* path, JPEGDEC* decoder) {
  Serial.println("Opening image viewer using provided decoder");
  Serial.printf("Width = %d, height = %d\n", decoder->getWidth(),
                decoder->getHeight());
  bool exitImageViewer = false;
  const size_t TEXT_SIZE = 27;
  char text[TEXT_SIZE];
  memset(text, 0, TEXT_SIZE);
  const char* ptr = strstr(path, "/images/");
  if (ptr != nullptr) {
    ptr = &path[9];
  }
  snprintf(text, TEXT_SIZE, "Opened %s", ptr);
  this->setBottomText(text, 3000);
  while (!exitImageViewer) {
    this->tft->startWrite();
    const uint8_t result = decoder->decode(0, 0, JPEG_SCALE_EIGHTH);
    this->tft->endWrite();
    if (result == 1) {
      Serial.println("Decoded successfully");
    } else {
      Serial.println("Failed to decode!");
    }
    while (true) {
      if (this->shutterButton->pressed()) {
        exitImageViewer = true;
        break;
      }
      this->drawBottomToolbar();
    }
  }
}
