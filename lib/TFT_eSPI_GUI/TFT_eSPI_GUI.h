#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Button.h>

uint8_t TFT_eSPI_GUI_menu(TFT_eSPI tft, const char* title, const char** menu,
                          uint8_t menuCount, Button upButton, Button downButton,
                          Button selectButton);
