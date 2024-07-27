/*******************************************************************************
 * Motion JPEG Image Viewer
 * This is a simple Motion JPEG image viewer example
 * Image Source: https://www.pexels.com/video/earth-rotating-video-856356/
 * cropped: x: 598 y: 178 width: 720 height: 720 resized: 352x352
 * ffmpeg -i "Pexels Videos 3931.mp4" -ss 0 -t 20.4s -vf "reverse,setpts=0.5*PTS,fps=10,vflip,hflip,rotate=90,crop=720:720:178:598,scale=352:352:flags=lanczos" -q:v 9 earth352.mjpeg
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
char *mjpeg_filename = (char *)"/root/earth352.mjpeg";
#define IMAGE_DATA_SIZE (352 * 352 * 2)
#define MJPEG_BUFFER_SIZE (IMAGE_DATA_SIZE / 10)

#include "JC3636W518.h"

#include <FFat.h>
#include <LittleFS.h>
#include <SPIFFS.h>
#include <SD.h>
#include <SD_MMC.h>

#include "MjpegFunc.h"

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

  gfx->setTextColor(WHITE, BLACK);
  gfx->setTextBound(60, 60, 240, 240);

  if (!FFat.begin(false, root))
  // if (!LittleFS.begin(false, root))
  // pinMode(SD_CS /* CS */, OUTPUT);
  // digitalWrite(SD_CS /* CS */, HIGH);
  // SD_MMC.setPins(SD_SCK, SD_MOSI /* CMD */, SD_MISO /* D0 */);
  // if (!SD_MMC.begin(root, true /* mode1bit */, false /* format_if_mount_failed */, SDMMC_FREQ_DEFAULT))
  // SD_MMC.setPins(SD_SCK, SD_MOSI /* CMD */, SD_MISO /* D0 */, SD_D1, SD_D2, SD_CS /* D3 */);
  // if (!SD_MMC.begin(root, false /* mode1bit */, false /* format_if_mount_failed */, SDMMC_FREQ_HIGHSPEED))
  {
    Serial.println("ERROR: File System Mount Failed!");
    gfx->println("ERROR: File System Mount Failed!");
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
    Serial.println("MJPEG start");

    unsigned long start_ms = millis();
    while (mjpeg_read())
    {
      mjpeg_draw(4, 4);
    }
    int time_used = millis() - start_ms;
    float fps = 1000.0 * mjpeg_total_frames / time_used;

    mjpeg_close();
    Serial.println("MJPEG end");

    Serial.printf("Total frames: %d\n", mjpeg_total_frames);
    Serial.printf("Time used: %d ms\n", time_used);
    Serial.printf("Average FPS: %0.1f\n", fps);
    Serial.printf("Read MJPEG: %lu ms (%0.1f %%)\n", mjpeg_total_read_video, 100.0 * mjpeg_total_read_video / time_used);
    Serial.printf("Decode video: %lu ms (%0.1f %%)\n", mjpeg_total_decode_video, 100.0 * mjpeg_total_decode_video / time_used);
    Serial.printf("Show video: %lu ms (%0.1f %%)\n", mjpeg_total_show_video, 100.0 * mjpeg_total_show_video / time_used);

    gfx->setCursor(60, 60);
    gfx->printf("Total frames: %d\n", mjpeg_total_frames);
    gfx->printf("Time used: %d ms\n", time_used);
    gfx->printf("Average FPS: %0.1f\n", fps);
    gfx->printf("Read MJPEG: %lu ms (%0.1f %%)\n", mjpeg_total_read_video, 100.0 * mjpeg_total_read_video / time_used);
    gfx->printf("Decode video: %lu ms (%0.1f %%)\n", mjpeg_total_decode_video, 100.0 * mjpeg_total_decode_video / time_used);
    gfx->printf("Show video: %lu ms (%0.1f %%)\n", mjpeg_total_show_video, 100.0 * mjpeg_total_show_video / time_used);
  }

  delay(60 * 1000); // pause 1 minutes
}
