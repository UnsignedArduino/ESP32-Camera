#include <Arduino.h>
#include <RTClib.h>
#include <SPI.h>  // Needed by TFT_eSPI
#include <SdFat.h>
#include <TFT_eSPI.h>
#include <ArduCAM.h>
#include <memorysaver.h>  // Needed by ArduCAM
#include <SD.h>           // Needed by JPEGDEC because it needs "File"
#include <JPEGDEC.h>

const uint8_t HSPI_CLK = 5;
const uint8_t HSPI_MISO = 17;
const uint8_t HSPI_MOSI = 16;
const uint8_t SD_CS = 4;
const int8_t CAM_CS = -1;

SoftSpiDriver<HSPI_MISO, HSPI_MOSI, HSPI_CLK> softSpi;
#define SPI_CLOCK SD_SCK_MHZ(27)
#define SD_CONFIG SdSpiConfig(SD_CS, SHARED_SPI, SPI_CLOCK, &softSpi)

RTC_DS3231 rtc;
TFT_eSPI tft = TFT_eSPI();
SdFs sd;
ArduCAM camera(OV2640, CAM_CS);

// 160 * 120 * 12 / 8 = 28800 bytes needed
// https://stackoverflow.com/a/2734699/10291933
const uint32_t PREVIEW_BUF_SIZE = 160 * 120 * 12 / 8;
uint8_t previewBuf[PREVIEW_BUF_SIZE];
JPEGDEC jpeg;

bool cameraBegin() {
  Serial.println("Preparing camera...");

  Wire.begin();

  SPI.begin();
  SPI.setFrequency(8000000);
  pinMode(CAM_CS, OUTPUT);

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
  camera.wrSensorReg8_8(0xFF, 0x01);
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
  camera.OV2640_set_Light_Mode(Auto);
  camera.clear_fifo_flag();

  Serial.println("Camera ok!");

  return true;
}

size_t cameraCaptureToMemory(uint8_t* dest, size_t destSize) {
  Serial.println("Starting capture");

  camera.flush_fifo();
  camera.clear_fifo_flag();
  camera.start_capture();
  while (!camera.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)) {
    ;
  }
  uint32_t len = camera.read_fifo_length();
  if (len >= MAX_FIFO_SIZE) {
    Serial.printf("FIFO oversized (%lu >= %lu)\n", len, MAX_FIFO_SIZE);
    return -1;
  } else if (len == 0) {
    Serial.println("FIFO size is 0");
    return -1;
  } else if (len > destSize) {
    Serial.println("FIFO size bigger than destination buffer");
    return -1;
  } else {
    Serial.printf("FIFO size is %lu\n", len);
  }

  camera.CS_LOW();
  camera.set_fifo_burst();

  size_t i = 0;

  while (len > 0) {
    dest[i] = SPI.transfer(0x00);
    i++;
    len--;
  }

  camera.CS_HIGH();

  return i;
}

int JPEGDraw(JPEGDRAW* pDraw) {
  tft.setAddrWindow(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  tft.setSwapBytes(true);
  tft.pushPixels(pDraw->pPixels, pDraw->iWidth * pDraw->iHeight);
  return 1;
}

void setup() {
  Serial.begin(9600);
  Serial.println("ESP32 camera");

  tft.begin();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);

  tft.println("Hello world!");

  if (!rtc.begin()) {
    Serial.println("Cannot find RTC");
  } else {
    Serial.println("Found RTC");
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting to compile time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  if (!sd.begin(SD_CONFIG)) {
    Serial.println("Cannot find SD card");
    if (sd.sdErrorCode()) {
      Serial.print("SD error code: ");
      printSdErrorSymbol(&Serial, sd.sdErrorCode());
      Serial.print("\nSD error data: ");
      Serial.println(sd.sdErrorData());
    }
  } else {
    Serial.println("Found SD card");
  }

  // if (!cameraBegin()) {
  //   Serial.println("Cannot find camera");
  // } else {
  //   Serial.println("Found camera");
  // }

  // memset(previewBuf, 0, PREVIEW_BUF_SIZE);
  // const size_t previewSize =
  //   cameraCaptureToMemory(previewBuf, PREVIEW_BUF_SIZE);
  // Serial.printf("JPEG size = %d\n", previewSize);

  // if (previewSize > 0) {
  //   FsFile file = sd.open("/image.jpg", O_RDWR | O_CREAT | O_TRUNC);
  //   if (file) {
  //     file.write(previewBuf, previewSize);
  //     file.close();
  //     Serial.println("Wrote image to disk");
  //   } else {
  //     Serial.println("Unable to open file");
  //   }
  // }
}

void loop() { ; }
