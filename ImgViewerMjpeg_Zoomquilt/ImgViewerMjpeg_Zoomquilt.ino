/*******************************************************************************
 * Motion JPEG Image Viewer
 * This is a simple Motion JPEG image viewer example
 * Image Source: https://youtu.be/RpHnKaxt_OQ
 * ffmpeg -y -i "The Zoomquilt - an infinitely zooming collaborative painting.mp4" -ss 0 -t 00:02:00.000 -vf "fps=5,scale=-1:360:flags=lanczos,crop=360:360:(in_w-360)/2:0" -q:v 7 zoomquilt.mjpeg
 *
 * Dependent libraries:
 * JPEGDEC: https://github.com/bitbank2/JPEGDEC.git
 *
 * Setup steps:
 * 1. Change your LCD parameters in Arduino_GFX setting
 * 2. Upload Motion JPEG file
 *   FFat/LittleFS:
 *     upload FFat (FatFS) data with ESP32 Sketch Data Upload:
 *     ESP32: https://github.com/lorol/arduino-esp32fs-plugin
 *   SD:
 *     Copy files to SD card
 ******************************************************************************/
const char *root = "/root";
char *mjpeg_filename = (char *)"/root/zoomquilt.mjpeg";
#define IMAGE_DATA_SIZE (360 * 360 * 2)
#define MJPEG_BUFFER_SIZE (IMAGE_DATA_SIZE / 10)

#include "JC3636W518.h"

#include <FFat.h>
#include <LittleFS.h>
#include <SPIFFS.h>
#include <SD.h>
#include <SD_MMC.h>

#include "MjpegFunc.h"

unsigned long start_ms;

void setup()
{
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("Arduino_GFX Motion JPEG Image Viewer example");

#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  // Init Display
  if (!gfx->begin())
  {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(BLACK);

#ifdef GFX_BL
  ledcSetup(1, 12000, 8);   // 12 kHz PWM, 8-bit resolution
  ledcAttachPin(GFX_BL, 1); // assign RGB led pins to channels
  ledcWrite(1, 63);
#endif

  if (!FFat.begin(false, root))
  // if (!LittleFS.begin(false, root))
  // pinMode(SD_CS /* CS */, OUTPUT);
  // digitalWrite(SD_CS /* CS */, HIGH);
  // SD_MMC.setPins(SD_SCK /* CLK */, SD_MOSI /* CMD/MOSI */, SD_MISO /* D0/MISO */);
  // if (!SD_MMC.begin(root, true /* mode1bit */, false /* format_if_mount_failed */, SDMMC_FREQ_DEFAULT))
  // SD_MMC.setPins(SD_SCK, SD_MOSI /* CMD */, SD_MISO /* D0 */, SD_D1, SD_D2, SD_CS /* D3 */);
  // if (!SD_MMC.begin(root, false /* mode1bit */, false /* format_if_mount_failed */, SDMMC_FREQ_HIGHSPEED))
  {
    Serial.println(F("ERROR: File System Mount Failed!"));
    gfx->println(F("ERROR: File System Mount Failed!"));
  }
  else
  {
    mjpeg_init(MJPEG_BUFFER_SIZE /* mjpegBufSize */, true /* useBigEndian */);
  }
}

void loop()
{
  if (mjpeg_open(mjpeg_filename))
  {
    // Serial.println(F("MJPEG start"));

    start_ms = millis();
    while (mjpeg_read())
    {
      mjpeg_draw(0, 0);
    }
    int time_used = millis() - start_ms;
    float fps = 1000.0 * mjpeg_total_frames / time_used;

    mjpeg_close();
    // Serial.println(F("MJPEG end"));

    // Serial.printf("Total frames: %d\n", mjpeg_total_frames);
    // Serial.printf("Time used: %d ms\n", time_used);
    Serial.printf("Average FPS: %0.1f\n", fps);
    // Serial.printf("Read MJPEG: %lu ms (%0.1f %%)\n", mjpeg_total_read_video, 100.0 * mjpeg_total_read_video / time_used);
    // Serial.printf("Decode video: %lu ms (%0.1f %%)\n", mjpeg_total_decode_video, 100.0 * mjpeg_total_decode_video / time_used);
    // Serial.printf("Show video: %lu ms (%0.1f %%)\n", mjpeg_total_show_video, 100.0 * mjpeg_total_show_video / time_used);
  }
}
