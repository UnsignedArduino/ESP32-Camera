#include <Arduino.h>
#include <RTClib.h>
#include <SPI.h>  // Needed by TFT_eSPI
#include <SdFat.h>
#include <TFT_eSPI.h>
#include <ArduCAM.h>
#include <memorysaver.h>  // Needed by ArduCAM
#include <SD.h>           // Needed by JPEGDEC because it needs "File"
#include <JPEGDEC.h>

const uint8_t SD_CS = 5;
const int8_t CAM_CS = 14;

#define SPI_CLOCK SD_SCK_MHZ(16)
#define SD_CONFIG SdSpiConfig(SD_CS, SHARED_SPI, SPI_CLOCK)

const uint8_t HSPI_CLK = 0;
const uint8_t HSPI_MOSI = 2;
const uint8_t HSPI_MISO = 15;
SPIClass* hspi = NULL;

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
  Wire.begin();

  hspi = new SPIClass(HSPI);
  hspi->begin(HSPI_CLK, HSPI_MISO, HSPI_MOSI);
  hspi->setFrequency(8000000);
  pinMode(CAM_CS, OUTPUT);

  Serial.print("Testing HSPI...");
  tft.print("Testing HSPI...");
  camera.setSpiBus(hspi);
  camera.write_reg(ARDUCHIP_TEST1, 0x55);
  uint8_t temp = camera.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55) {
    Serial.print("error! (0x55 != 0x");
    Serial.print(temp, HEX);
    Serial.println(")");
    tft.print("error! (0x55 != 0x");
    tft.print(temp, HEX);
    tft.println(")");
    goto cameraBeginError;
  } else {
    Serial.println("ok!");
    tft.println("ok!");
  }

  Serial.print("Checking camera presence...");
  tft.print("Checking camera presence...");

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
    tft.println("missing! (vid: 0x");
    tft.print(vid, HEX);
    tft.print(" pid: 0x");
    tft.print(pid, HEX);
    tft.println(")");
    goto cameraBeginError;
  } else {
    Serial.println("ok!");
    tft.println("ok!");
  }

  camera.set_format(JPEG);
  camera.InitCAM();
  camera.OV2640_set_JPEG_size(OV2640_160x120);
  camera.OV2640_set_Light_Mode(Auto);
  camera.clear_fifo_flag();

  Serial.println("Camera initialization...ok!");
  tft.println("Camera initialization...ok!");

  return true;

cameraBeginError:

  Serial.println("Camera initialization...error!");
  tft.println("Camera initialization...error!");
  return false;
}

size_t cameraCaptureToMemory(uint8_t* dest, size_t destSize) {
  // Serial.println("Starting capture");

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
    // Serial.printf("FIFO size is %lu\n", len);
  }

  camera.CS_LOW();
  camera.set_fifo_burst();

  hspi->transfer(0x00);
  len--;

  size_t i = 0;

  while (len > 0) {
    dest[i] = hspi->transfer(0x00);
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

bool hardwareBegin() {
  tft.begin();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);

  Serial.println("Initiating hardware...");
  tft.println("Initiating hardware...");

  Serial.print("Trying RTC...");
  tft.print("Trying RTC...");
  if (!rtc.begin()) {
    Serial.println("error!");
    tft.println("error!");
    goto hardwareBeginError;
  } else {
    Serial.println("ok!");
    tft.println("ok!");
  }

  Serial.print("Checking for RTC power loss...");
  tft.print("Checking for RTC power loss...");
  if (rtc.lostPower()) {
    Serial.println("error!");
    Serial.println("RTC lost power, setting to compile time");
    tft.println("error!");
    tft.println("RTC lost power, setting to compile time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  } else {
    Serial.println("ok!");
    tft.println("ok!");
  }

  Serial.print("Trying SD card...");
  tft.print("Trying SD card...");

  if (!sd.begin(SD_CONFIG)) {
    Serial.println("error!");
    tft.println("error!");
    if (sd.sdErrorCode()) {
      Serial.print("SD error code: ");
      printSdErrorSymbol(&Serial, sd.sdErrorCode());
      Serial.print("\nSD error data: ");
      Serial.println(sd.sdErrorData());
      tft.print("SD error code: ");
      printSdErrorSymbol(&tft, sd.sdErrorCode());
      tft.print("\nSD error data: ");
      tft.println(sd.sdErrorData());
    }
    goto hardwareBeginError;
  } else {
    Serial.println("ok!");
    tft.println("ok!");
  }

  if (!cameraBegin()) {
    goto hardwareBeginError;
  }

  Serial.println("Hardware initialization...ok!");
  tft.println("Hardware initialization...ok!");

  return true;

hardwareBeginError:

  Serial.println("Hardware initialization...error!");
  tft.println("Hardware initialization...error!");
  return false;
}

void setup() {
  Serial.begin(9600);
  Serial.println("\n\nESP32 camera");

  if (!hardwareBegin()) {
    while (true) {
      ;
    }
  }
}

void loop() {
  memset(previewBuf, 0, PREVIEW_BUF_SIZE);
  const size_t previewSize =
    cameraCaptureToMemory(previewBuf, PREVIEW_BUF_SIZE);
  if (previewSize > 0) {
    if (jpeg.openRAM(previewBuf, previewSize, JPEGDraw)) {
      tft.startWrite();
      jpeg.decode(0, 0, 0);
      tft.endWrite();
      jpeg.close();
    }
  }
}
