#include <Arduino.h>
#include <RTClib.h>
#include <SPI.h>
#include <SdFat.h>
#include <TFT_eSPI.h>
#include <ArduCAM.h>
#include <memorysaver.h>

const uint8_t SD_CS = 16;
const uint8_t CAM_CS = 5;
const uint8_t CAM_MISO_TRISTATE = 17;

RTC_DS3231 rtc;
TFT_eSPI tft = TFT_eSPI();
SdFs sd;
ArduCAM camera(OV2640, CAM_CS);

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

bool cameraBegin() {
  Serial.println("Preparing camera...");

  Wire.begin();

  SPI.begin();
  SPI.setFrequency(8000000);

  pinMode(CAM_CS, OUTPUT);
  pinMode(CAM_MISO_TRISTATE, OUTPUT);
  digitalWrite(CAM_MISO_TRISTATE, HIGH);

  Serial.print("Testing SPI...");

  camera.write_reg(ARDUCHIP_TEST1, 0x55);
  uint8_t temp = camera.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55) {
    Serial.print("error! (0x55 != 0x");
    Serial.print(temp, HEX);
    Serial.println(")");
    return false;
  } else {
    Serial.println("ok!");
  }

  Serial.print("Checking camera presence...");

  uint8_t vid, pid;
  camera.wrSensorReg8_8(0xff, 0x01);
  camera.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  camera.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
  if ((vid != 0x26) && ((pid != 0x41) || (pid != 0x42))) {
    Serial.println("missing! (vid: 0x");
    Serial.print(vid, HEX);
    Serial.print(" pid: 0x");
    Serial.print(pid, HEX);
    Serial.println(")");
    return false;
  } else {
    Serial.println("found!");
  }

  camera.set_format(JPEG);
  camera.InitCAM();
  camera.OV2640_set_JPEG_size(OV2640_160x120);
  camera.clear_fifo_flag();

  digitalWrite(CAM_MISO_TRISTATE, LOW);

  Serial.println("Camera ok!");

  return true;
}

void cameraCapture() {
  Serial.println("Starting capture");

  digitalWrite(CAM_MISO_TRISTATE, HIGH);

  camera.clear_fifo_flag();
  camera.start_capture();

  while (!camera.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)) {
    ;
  }

  size_t len = camera.read_fifo_length();
  if (len >= 0x07ffff) {
    Serial.println("Image over size");
    return;
  } else if (len == 0) {
    Serial.println("Image size is 0");
    return;
  } else {
    Serial.print("Image size is ");
    Serial.println(len);
  }

  camera.CS_LOW();
  camera.set_fifo_burst();

  SPI.transfer(0xFF);

  static const size_t bufferSize = 4096;
  static uint8_t buffer[bufferSize] = {0xFF};

  while (len > 0) {
    size_t will_copy = (len < bufferSize) ? len : bufferSize;
    SPI.transferBytes(&buffer[0], &buffer[0], will_copy);
    // client.write(&buffer[0], will_copy);
    len -= will_copy;
  }

  camera.CS_HIGH();
  digitalWrite(CAM_MISO_TRISTATE, LOW);

  Serial.println("Camera capture finished");
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

  if (!cameraBegin()) {
    tft.println("Couldn't find camera");
  }

  tft.println("Starting capture");
  cameraCapture();
  tft.println("Capture finished");
}

void loop() {
  DateTime now = rtc.now();

  tft.setCursor(0, 0);
  tft.printf("%d/%d/%d %.2d:%.2d:%.2d  ", now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());

  delay(1000);
}
