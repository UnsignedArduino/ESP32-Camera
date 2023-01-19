#include <Arduino.h>
#include <TFT_eSPI.h>
#include "TFT_eSPI_GUI.h"

// https://github.com/adafruit/Adafruit_Arcada/blob/master/Adafruit_Arcada_Alerts.cpp#L200
uint8_t TFT_eSPI_GUI_menu(TFT_eSPI tft, const char* title, const char** menu,
                          uint8_t menuCount, Button upButton, Button downButton,
                          Button selectButton) {
  const uint8_t charWidth = 6;
  const uint8_t charHeight = 8;
  const uint16_t boxColor = TFT_WHITE;
  const uint16_t textColor = TFT_BLACK;
  const uint8_t topPadding = 8;
  const uint8_t rightPadding = 8;
  const uint8_t bottomPadding = 16;
  const uint8_t leftPadding = 8;

  const uint16_t boxWidth = tft.width() - leftPadding - rightPadding;
  const uint16_t boxHeight = tft.height() - topPadding - bottomPadding;
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

  tft.fillRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth + 1, boxColor);
  tft.drawRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth - 1, textColor);

  tft.setTextColor(textColor, boxColor);
  tft.setCursor(fontX, fontY);
  tft.print(" ");
  tft.print(title);

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
        tft.setTextColor(boxColor, textColor);
      } else {
        tft.setTextColor(textColor, boxColor);
      }
      tft.setCursor(fontX, fontY + charHeight * (i - offset));
      tft.print(" ");
      if (i == selected) {
        for (uint8_t j = 1;
             j < min((size_t)maxCharPerRow - 2, strlen(menu[i]) + 1); j++) {
          tft.print(menu[i][j - 1 + selectedCharOffset]);
        }
      } else {
        for (uint8_t j = 1;
             j < min((size_t)maxCharPerRow - 2, strlen(menu[i]) + 1); j++) {
          tft.print(menu[i][j - 1]);
        }
      }
      if (strlen(menu[i]) > maxCharPerRow - 3) {
        tft.print(" ");
      }
      for (int j = strlen(menu[i]); j < maxCharPerRow - 2; j++) {
        tft.print(" ");
      }
    }

    if (showScrollbar) {
      const uint8_t startY =
        map(min((int16_t)offset, (int16_t)(menuCount - maxEntryPerPage)), 0,
            menuCount, scrollBarY, scrollBarY + scrollBarHeight);
      const uint8_t barHeight =
        map(maxEntryPerPage, 0, menuCount, 0, scrollBarHeight);

      tft.fillRect(scrollBarX, scrollBarY, scrollBarWidth, scrollBarHeight,
                   boxColor);
      tft.fillRect(scrollBarX, startY, scrollBarWidth, barHeight, textColor);
    }

    while (millis() - lastMovedTime < moveThrottleTime) {
      delay(1);
    }

    while (true) {
      if (!upButton.read() || !downButton.read()) {
        if (lastPressTime > 0) {
          pressTime += millis() - lastPressTime;
        }
        lastPressTime = millis();
      } else {
        pressTime = 0;
        lastPressTime = 0;
      }
      if (upButton.pressed() ||
          (!upButton.read() && pressTime > holdToAccelTime)) {
        if (selected == 0) {
          selected = menuCount - 1;
        } else {
          selected--;
        }
        selectedCharOffset = 0;
        selectedTime = millis();
        break;
      }
      if (downButton.pressed() ||
          (!downButton.read() && pressTime > holdToAccelTime)) {
        if (selected == menuCount - 1) {
          selected = 0;
        } else {
          selected++;
        }
        selectedCharOffset = 0;
        selectedTime = millis();
        break;
      }
      if (selectButton.pressed()) {
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

bool getFileCount(SdFs& sd, char* start, uint32_t& result) {
  FsFile dir = sd.open(start, O_RDONLY);
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

bool getFileNameFromIndex(SdFs& sd, char* start, uint32_t index, char* result,
                          size_t resultSize) {
  FsFile dir = sd.open(start, O_RDONLY);
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

bool TFT_eSPI_GUI_file_explorer(TFT_eSPI tft, SdFs& sd, char* startDirectory,
                                Button upButton, Button downButton,
                                Button selectButton, Button shutterButton,
                                char* result, size_t resultSize) {
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

  const uint16_t boxWidth = tft.width() - leftPadding - rightPadding;
  const uint16_t boxHeight = tft.height() - topPadding - bottomPadding;
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
    if (!getFileCount(sd, currentDirectory, fileCount)) {
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

    tft.fillRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth + 1, boxColor);
    tft.drawRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth - 1,
                      textColor);

    tft.setTextColor(textColor, boxColor);
    tft.setCursor(fontX, fontY);
    tft.print(" File explorer");

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
            getFileNameFromIndex(sd, currentDirectory, i - extraOptionsCount,
                                 menuEntries[i - offset], MAX_PATH_SIZE);
            // Serial.printf("File at index %d (menu entry %d) is \"%s\"\n",
            //               i - extraOptionsCount, i, menuEntries[i - offset]);
          }
        }
      }

      tft.setTextColor(textColor, boxColor);
      tft.setCursor(titleX, titleY);
      tft.print(" ");
      // Serial.printf("Title: %s\n", currentDirectory);
      for (uint8_t i = 1;
           i < min((size_t)maxCharInTitle, strlen(currentDirectory) + 1); i++) {
        tft.print(currentDirectory[i - 1 + titleSelectedCharOffset]);
        // Serial.print(currentDirectory[i - 1 + titleSelectedCharOffset]);
      }
      // Serial.println();

      for (uint32_t i = offset;
           i < min((uint16_t)fileCount, (uint16_t)(offset + maxEntryPerPage));
           i++) {
        char* current = menuEntries[i - offset];

        if (i == selected) {
          tft.setTextColor(boxColor, textColor);
          strncpy(selectedPath, current, MAX_PATH_SIZE);
          // Serial.printf("Selected file: \"%s\"\n", selectedPath);
        } else {
          tft.setTextColor(textColor, boxColor);
        }
        tft.setCursor(fontX, fontY + charHeight * (i - offset));
        tft.print(" ");
        if (i == selected) {
          for (uint8_t j = 1;
               j < min((size_t)maxCharPerRow - 2, strlen(current) + 1); j++) {
            tft.print(current[j - 1 + selectedCharOffset]);
          }
        } else {
          for (uint8_t j = 1;
               j < min((size_t)maxCharPerRow - 2, strlen(current) + 1); j++) {
            tft.print(current[j - 1]);
          }
        }
        if (strlen(current) > maxCharPerRow - 3) {
          tft.print(" ");
        }
        for (int j = strlen(current) + 1; j < maxCharPerRow - 1; j++) {
          tft.print(" ");
        }
      }

      if (showScrollbar) {
        const uint8_t startY =
          map(min((int16_t)offset, (int16_t)(fileCount - maxEntryPerPage)), 0,
              fileCount, scrollBarY, scrollBarY + scrollBarHeight);
        const uint8_t barHeight =
          map(maxEntryPerPage, 0, fileCount, 0, scrollBarHeight);

        tft.fillRect(scrollBarX, scrollBarY, scrollBarWidth, scrollBarHeight,
                     boxColor);
        tft.fillRect(scrollBarX, startY, scrollBarWidth, barHeight, textColor);
      }

      while (millis() - lastMovedTime < moveThrottleTime) {
        delay(1);
      }

      while (true) {
        if (!upButton.read() || !downButton.read()) {
          if (lastPressTime > 0) {
            pressTime += millis() - lastPressTime;
          }
          lastPressTime = millis();
        } else {
          pressTime = 0;
          lastPressTime = 0;
        }
        if (upButton.pressed() ||
            (!upButton.read() && pressTime > holdToAccelTime)) {
          if (selected == 0) {
            selected = fileCount - 1;
          } else {
            selected--;
          }
          selectedCharOffset = 0;
          selectedTime = millis();
          break;
        }
        if (downButton.pressed() ||
            (!downButton.read() && pressTime > holdToAccelTime)) {
          if (selected == fileCount - 1) {
            selected = 0;
          } else {
            selected++;
          }
          selectedCharOffset = 0;
          selectedTime = millis();
          break;
        }
        if (selectButton.pressed()) {
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
