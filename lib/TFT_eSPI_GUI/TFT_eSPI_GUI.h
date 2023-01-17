#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SdFat.h>
#include <Button.h>

uint8_t TFT_eSPI_GUI_menu(TFT_eSPI tft, const char* title, const char** menu,
                          uint8_t menuCount, Button upButton, Button downButton,
                          Button selectButton);

bool TFT_eSPI_GUI_file_explorer(TFT_eSPI tft, SdFs& sd, char* startDirectory,
                                Button upButton, Button downButton,
                                Button selectButton, Button shutterButton,
                                char* result, size_t resultSize);
