#include <Arduino.h>
#include <TFT_eSPI.h>
#include "ESP32_Camera_GUI.h"

bool ESP32CameraGUI::begin(TFT_eSPI* tft, SdFs* sd, RTC_DS3231* rtc, Button* upButton,
               Button* downButton, Button* selectButton, Button* shutterButton) {
  this->tft = tft;
  this->sd = sd;
  this->rtc = rtc;
  this->upButton = upButton;
  this->downButton = downButton;
  this->selectButton = selectButton;
  this->shutterButton = shutterButton;
  return true;
}

// https://github.com/adafruit/Adafruit_Arcada/blob/master/Adafruit_Arcada_Alerts.cpp#L200
uint8_t ESP32CameraGUI::menu(const char* title, const char** menu,
                             uint8_t menuCount) {
  const uint8_t charWidth = 6;
  const uint8_t charHeight = 8;
  const uint16_t boxColor = TFT_WHITE;
  const uint16_t textColor = TFT_BLACK;
  const uint8_t topPadding = 8;
  const uint8_t rightPadding = 8;
  const uint8_t bottomPadding = 16;
  const uint8_t leftPadding = 8;

  const uint16_t boxWidth = this->tft->width() - leftPadding - rightPadding;
  const uint16_t boxHeight = this->tft->height() - topPadding - bottomPadding;
  const uint16_t boxX = leftPadding;
  const uint16_t boxY = topPadding;

  const uint8_t maxEntryPerPage = 10;
  const uint16_t timePerChar = 200;
  const uint8_t startEndPauseTicks = 4;
  const uint16_t holdToAccelTime = 500;
  const uint16_t moveThrottleTime = 50;

  uint32_t selectedTime = millis();
  uint32_t lastPressTime = millis();
  uint32_t pressTime = 0;
  uint32_t lastMovedTime = millis();
  const bool showScrollbar = menuCount > maxEntryPerPage;
  const uint8_t maxCharPerRow =
    (boxWidth / charWidth) - (showScrollbar ? 1 : 0);

  const uint8_t fontX = boxX + charWidth / 2;
  uint8_t fontY = boxY + charHeight;

  this->tft->fillRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth + 1,
                           boxColor);
  this->tft->drawRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth - 1,
                           textColor);

  this->tft->setTextColor(textColor, boxColor);
  this->tft->setCursor(fontX, fontY);
  this->tft->print(" ");
  this->tft->print(title);

  fontY += charHeight * 1.5;

  const uint8_t scrollBarX = boxX + boxWidth - charWidth;
  const uint8_t scrollBarY = fontY;
  const uint8_t scrollBarWidth = charWidth / 2;
  const uint8_t scrollBarHeight = maxEntryPerPage * charHeight;

  uint8_t offset = 0;
  uint8_t selected = 0;

  uint8_t offsetPauseTicks = startEndPauseTicks;
  uint8_t selectedCharOffset = 0;
  bool resetAfterPause = false;

  while (true) {
    for (int i = offset;
         i < min((uint16_t)menuCount, (uint16_t)(offset + maxEntryPerPage));
         i++) {
      if (i == selected) {
        this->tft->setTextColor(boxColor, textColor);
      } else {
        this->tft->setTextColor(textColor, boxColor);
      }
      this->tft->setCursor(fontX, fontY + charHeight * (i - offset));
      this->tft->print(" ");
      if (i == selected) {
        for (uint8_t j = 1;
             j < min((size_t)maxCharPerRow - 2, strlen(menu[i]) + 1); j++) {
          this->tft->print(menu[i][j - 1 + selectedCharOffset]);
        }
      } else {
        for (uint8_t j = 1;
             j < min((size_t)maxCharPerRow - 2, strlen(menu[i]) + 1); j++) {
          this->tft->print(menu[i][j - 1]);
        }
      }
      if (strlen(menu[i]) > maxCharPerRow - 3) {
        this->tft->print(" ");
      }
      for (int j = strlen(menu[i]); j < maxCharPerRow - 2; j++) {
        this->tft->print(" ");
      }
    }

    if (showScrollbar) {
      const uint8_t startY =
        map(min((int16_t)offset, (int16_t)(menuCount - maxEntryPerPage)), 0,
            menuCount, scrollBarY, scrollBarY + scrollBarHeight);
      const uint8_t barHeight =
        map(maxEntryPerPage, 0, menuCount, 0, scrollBarHeight);

      this->tft->fillRect(scrollBarX, scrollBarY, scrollBarWidth,
                          scrollBarHeight, boxColor);
      this->tft->fillRect(scrollBarX, startY, scrollBarWidth, barHeight,
                          textColor);
    }

    while (millis() - lastMovedTime < moveThrottleTime) {
      delay(1);
    }

    while (true) {
      if (!this->upButton->read() || !this->downButton->read()) {
        if (lastPressTime > 0) {
          pressTime += millis() - lastPressTime;
        }
        lastPressTime = millis();
      } else {
        pressTime = 0;
        lastPressTime = 0;
      }
      if (this->upButton->pressed() ||
          (!this->upButton->read() && pressTime > holdToAccelTime)) {
        if (selected == 0) {
          selected = menuCount - 1;
        } else {
          selected--;
        }
        selectedCharOffset = 0;
        selectedTime = millis();
        break;
      }
      if (this->downButton->pressed() ||
          (!this->downButton->read() && pressTime > holdToAccelTime)) {
        if (selected == menuCount - 1) {
          selected = 0;
        } else {
          selected++;
        }
        selectedCharOffset = 0;
        selectedTime = millis();
        break;
      }
      if (this->selectButton->pressed()) {
        return selected;
      }
      if (millis() - selectedTime > timePerChar &&
          strlen(menu[selected]) > maxCharPerRow - 3) {
        selectedTime = millis();
        if (offsetPauseTicks > 0) {
          offsetPauseTicks--;
        } else if (resetAfterPause) {
          resetAfterPause = false;
          selectedCharOffset = 0;
          offsetPauseTicks = startEndPauseTicks;
          break;
        } else {
          selectedCharOffset++;
          if (selectedCharOffset + maxCharPerRow >=
              strlen(menu[selected]) + 3) {
            offsetPauseTicks = startEndPauseTicks;
            resetAfterPause = true;
          }
          break;
        }
      }
    }

    if (selected >= offset + maxEntryPerPage) {
      offset += selected - (offset + maxEntryPerPage) + 1;
    } else if (selected < offset) {
      offset = selected;
    }

    lastMovedTime = millis();
  }
}

bool ESP32CameraGUI::fileExplorer(char* startDirectory, char* result,
                                  size_t resultSize) {
  uint32_t fileCount;

  FsFile dir;
  FsFile file;

  const size_t MAX_PATH_SIZE = 255;
  char currentDirectory[MAX_PATH_SIZE];
  strncpy(currentDirectory, startDirectory, MAX_PATH_SIZE);
  char selectedPath[MAX_PATH_SIZE];

  const uint8_t charWidth = 6;
  const uint8_t charHeight = 8;
  const uint16_t boxColor = TFT_WHITE;
  const uint16_t textColor = TFT_BLACK;
  const uint8_t topPadding = 8;
  const uint8_t rightPadding = 8;
  const uint8_t bottomPadding = 16;
  const uint8_t leftPadding = 8;

  const uint16_t boxWidth = this->tft->width() - leftPadding - rightPadding;
  const uint16_t boxHeight = this->tft->height() - topPadding - bottomPadding;
  const uint16_t boxX = leftPadding;
  const uint16_t boxY = topPadding;

  const uint8_t maxEntryPerPage = 8;
  const uint16_t timePerChar = 200;
  const uint8_t startEndPauseTicks = 4;
  const uint16_t holdToAccelTime = 500;
  const uint16_t moveThrottleTime = 50;

  const uint8_t extraOptionsCount = 2;
  const char* extraOptions[extraOptionsCount] = {"Exit", "Up a folder"};

  while (true) {
    char menuEntries[maxEntryPerPage][MAX_PATH_SIZE];
    uint32_t menuEntryOffset = -1;

    // dir = sd.open(currentDirectory, O_RDONLY);
    if (!this->getFileCount(currentDirectory, fileCount)) {
      return false;
    }

    Serial.printf("%lu files in %s\n", fileCount, currentDirectory);

    fileCount += extraOptionsCount;

    uint32_t selectedTime = millis();
    uint32_t lastPressTime = millis();
    uint32_t pressTime = 0;
    uint32_t lastMovedTime = millis();
    const bool showScrollbar = fileCount > maxEntryPerPage;
    const uint8_t maxCharPerRow =
      (boxWidth / charWidth) - (showScrollbar ? 1 : 0);
    const uint8_t maxCharInTitle = boxWidth / charWidth - 3;

    const uint8_t fontX = boxX + charWidth / 2;
    uint8_t fontY = boxY + charHeight;

    this->tft->fillRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth + 1,
                             boxColor);
    this->tft->drawRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth - 1,
                             textColor);

    this->tft->setTextColor(textColor, boxColor);
    this->tft->setCursor(fontX, fontY);
    this->tft->print(" File explorer");

    fontY += charHeight * 1.5;

    const uint16_t titleX = fontX;
    const uint16_t titleY = fontY;

    fontY += charHeight * 1.5;

    const uint8_t scrollBarX = boxX + boxWidth - charWidth;
    const uint8_t scrollBarY = fontY;
    const uint8_t scrollBarWidth = charWidth / 2;
    const uint8_t scrollBarHeight = maxEntryPerPage * charHeight;

    uint8_t offset = 0;
    uint8_t selected = 0;

    uint8_t offsetPauseTicks = startEndPauseTicks;
    uint8_t selectedCharOffset = 0;
    bool resetAfterPause = false;

    uint32_t lastTitleShiftTime = millis();
    uint8_t titleOffsetPauseTicks = startEndPauseTicks;
    uint8_t titleSelectedCharOffset = 0;
    bool titleResetAfterPause = false;

    bool needToCompleteRedraw = false;

    while (!needToCompleteRedraw) {
      if (menuEntryOffset != offset) {
        menuEntryOffset = offset;

        for (uint32_t i = offset;
             i < min((uint16_t)fileCount, (uint16_t)(offset + maxEntryPerPage));
             i++) {
          if (i < extraOptionsCount) {
            strncpy(menuEntries[i - offset], extraOptions[i], MAX_PATH_SIZE);
          } else {
            strncpy(menuEntries[i - offset], "Could not read file!",
                    MAX_PATH_SIZE);
            this->getFileNameFromIndex(currentDirectory, i - extraOptionsCount,
                                       menuEntries[i - offset], MAX_PATH_SIZE);
            // Serial.printf("File at index %d (menu entry %d) is \"%s\"\n",
            //               i - extraOptionsCount, i, menuEntries[i - offset]);
          }
        }
      }

      this->tft->setTextColor(textColor, boxColor);
      this->tft->setCursor(titleX, titleY);
      this->tft->print(" ");
      // Serial.printf("Title: %s\n", currentDirectory);
      for (uint8_t i = 1;
           i < min((size_t)maxCharInTitle, strlen(currentDirectory) + 1); i++) {
        this->tft->print(currentDirectory[i - 1 + titleSelectedCharOffset]);
        // Serial.print(currentDirectory[i - 1 + titleSelectedCharOffset]);
      }
      // Serial.println();

      for (uint32_t i = offset;
           i < min((uint16_t)fileCount, (uint16_t)(offset + maxEntryPerPage));
           i++) {
        char* current = menuEntries[i - offset];

        if (i == selected) {
          this->tft->setTextColor(boxColor, textColor);
          strncpy(selectedPath, current, MAX_PATH_SIZE);
          // Serial.printf("Selected file: \"%s\"\n", selectedPath);
        } else {
          this->tft->setTextColor(textColor, boxColor);
        }
        this->tft->setCursor(fontX, fontY + charHeight * (i - offset));
        this->tft->print(" ");
        if (i == selected) {
          for (uint8_t j = 1;
               j < min((size_t)maxCharPerRow - 2, strlen(current) + 1); j++) {
            this->tft->print(current[j - 1 + selectedCharOffset]);
          }
        } else {
          for (uint8_t j = 1;
               j < min((size_t)maxCharPerRow - 2, strlen(current) + 1); j++) {
            this->tft->print(current[j - 1]);
          }
        }
        if (strlen(current) > maxCharPerRow - 3) {
          this->tft->print(" ");
        }
        for (int j = strlen(current) + 1; j < maxCharPerRow - 1; j++) {
          this->tft->print(" ");
        }
      }

      if (showScrollbar) {
        const uint8_t startY =
          map(min((int16_t)offset, (int16_t)(fileCount - maxEntryPerPage)), 0,
              fileCount, scrollBarY, scrollBarY + scrollBarHeight);
        const uint8_t barHeight =
          map(maxEntryPerPage, 0, fileCount, 0, scrollBarHeight);

        this->tft->fillRect(scrollBarX, scrollBarY, scrollBarWidth,
                            scrollBarHeight, boxColor);
        this->tft->fillRect(scrollBarX, startY, scrollBarWidth, barHeight,
                            textColor);
      }

      while (millis() - lastMovedTime < moveThrottleTime) {
        delay(1);
      }

      while (true) {
        if (!this->upButton->read() || !this->downButton->read()) {
          if (lastPressTime > 0) {
            pressTime += millis() - lastPressTime;
          }
          lastPressTime = millis();
        } else {
          pressTime = 0;
          lastPressTime = 0;
        }
        if (this->upButton->pressed() ||
            (!this->upButton->read() && pressTime > holdToAccelTime)) {
          if (selected == 0) {
            selected = fileCount - 1;
          } else {
            selected--;
          }
          selectedCharOffset = 0;
          selectedTime = millis();
          break;
        }
        if (this->downButton->pressed() ||
            (!this->downButton->read() && pressTime > holdToAccelTime)) {
          if (selected == fileCount - 1) {
            selected = 0;
          } else {
            selected++;
          }
          selectedCharOffset = 0;
          selectedTime = millis();
          break;
        }
        if (this->selectButton->pressed()) {
          if (selected < extraOptionsCount) {
            switch (selected) {
              default:
              case 0: {
                return false;
              }
              case 1: {
                char* last = strrchr(currentDirectory, '/');
                if (last != 0) {
                  last[1] = '\0';
                }
                if (currentDirectory[strlen(currentDirectory) - 1] == '/' &&
                    strlen(currentDirectory) > 1) {
                  currentDirectory[strlen(currentDirectory) - 1] = '\0';
                }
                Serial.printf("chdir to .. (%s)\n", currentDirectory);
                needToCompleteRedraw = true;
                break;
              }
            }
            break;
          } else if (selectedPath[strlen(selectedPath) - 1] == '/') {
            if (currentDirectory[strlen(currentDirectory) - 1] == '/') {
              currentDirectory[strlen(currentDirectory) - 1] = '\0';
            }
            if (selectedPath[0] == '/') {
              strncat(currentDirectory, selectedPath, MAX_PATH_SIZE);
            } else {
              strncat(currentDirectory, "/", MAX_PATH_SIZE);
              strncat(currentDirectory, selectedPath, MAX_PATH_SIZE);
            }
            if (currentDirectory[strlen(currentDirectory) - 1] == '/') {
              currentDirectory[strlen(currentDirectory) - 1] = '\0';
            }
            Serial.printf("chdir to %s (%s)\n", selectedPath, currentDirectory);
            needToCompleteRedraw = true;
            break;
          } else {
            if (currentDirectory[strlen(currentDirectory) - 1] == '/') {
              currentDirectory[strlen(currentDirectory) - 1] = '\0';
            }
            if (selectedPath[0] == '/') {
              strncat(currentDirectory, selectedPath, MAX_PATH_SIZE);
            } else {
              strncat(currentDirectory, "/", MAX_PATH_SIZE);
              strncat(currentDirectory, selectedPath, MAX_PATH_SIZE);
            }
            Serial.printf("Selected %s", currentDirectory);
            memset(result, 0, resultSize);
            strncpy(result, currentDirectory, resultSize);
            return true;
          }
        }
        if (millis() - selectedTime > timePerChar &&
            strlen(selectedPath) > maxCharPerRow - 3) {
          selectedTime = millis();
          if (offsetPauseTicks > 0) {
            offsetPauseTicks--;
          } else if (resetAfterPause) {
            resetAfterPause = false;
            selectedCharOffset = 0;
            offsetPauseTicks = startEndPauseTicks;
            break;
          } else {
            selectedCharOffset++;
            if (selectedCharOffset + maxCharPerRow >=
                strlen(selectedPath) + 3) {
              offsetPauseTicks = startEndPauseTicks;
              resetAfterPause = true;
            }
            break;
          }
        }
        if (millis() - lastTitleShiftTime > timePerChar &&
            strlen(currentDirectory) > maxCharInTitle) {
          lastTitleShiftTime = millis();
          if (titleOffsetPauseTicks > 0) {
            titleOffsetPauseTicks--;
          } else if (titleResetAfterPause) {
            titleResetAfterPause = false;
            titleSelectedCharOffset = 0;
            titleOffsetPauseTicks = startEndPauseTicks;
            break;
          } else {
            titleSelectedCharOffset++;
            if (titleSelectedCharOffset + maxCharInTitle >=
                strlen(currentDirectory) + 1) {
              titleOffsetPauseTicks = startEndPauseTicks;
              titleResetAfterPause = true;
            }
            break;
          }
        }
      }
      if (selected >= offset + maxEntryPerPage) {
        offset += selected - (offset + maxEntryPerPage) + 1;
      } else if (selected < offset) {
        offset = selected;
      }

      lastMovedTime = millis();
    }
  }
}

bool ESP32CameraGUI::getFileCount(char* start, uint32_t& result) {
  FsFile dir = this->sd->open(start, O_RDONLY);
  if (!dir) {
    return false;
  }

  FsFile file;
  result = 0;

  const size_t MAX_PATH_SIZE = 255;
  char path[MAX_PATH_SIZE];

  dir.rewindDirectory();
  while (file.openNext(&dir, O_RDONLY)) {
    file.getName(path, MAX_PATH_SIZE);
    if (!file.isHidden() && strlen(path) > 0) {
      result++;
    }
    file.close();
  }
  if (dir.getError()) {
    return false;
  } else {
    dir.close();
    return true;
  }
}

bool ESP32CameraGUI::getFileNameFromIndex(char* start, uint32_t index,
                                          char* result, size_t resultSize) {
  FsFile dir = this->sd->open(start, O_RDONLY);
  if (!dir) {
    return false;
  }

  FsFile file;
  dir.rewindDirectory();
  for (uint32_t i = 0; i <= index;) {
    if (!file.openNext(&dir, O_RDONLY)) {
      return false;
    }
    file.getName(result, resultSize);
    if (!file.isHidden() && strlen(result) > 0) {
      i++;
    }
  }

  file.getName(result, resultSize);

  if (file.isDir()) {
    strncat(result, "/", resultSize);
  }

  file.close();
  dir.close();

  return true;
}
