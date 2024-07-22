#pragma once

/*******************************************************************************
 * MJPEG related functions
 *
 * Dependent libraries:
 * JPEGDEC: https://github.com/bitbank2/JPEGDEC.git
 ******************************************************************************/
#define READ_BUFFER_SIZE 1024
// #define EXCLUSIVE_DISPLAY_INTERFACE

#include <JPEGDEC.h>

/* variables */
int mjpeg_total_frames;
unsigned long mjpeg_total_read_video;
unsigned long mjpeg_total_decode_video;
unsigned long mjpeg_total_show_video;

bool mjpeg_use_big_endian;
uint8_t *mjpeg_read_buf;
uint8_t *mjpeg_buf;
FILE *mjpeg_file;

JPEGDEC jpegdec;

int32_t mjpeg_file_idx;
int32_t mjpeg_buf_offset;
int32_t mjpeg_buf_read;

bool mjpeg_init(size_t mjpegBufSize, bool useBigEndian)
{
  mjpeg_read_buf = (uint8_t *)malloc(READ_BUFFER_SIZE);
  if (!mjpeg_read_buf)
  {
    Serial.println("mjpeg_read_buf malloc failed!\n");
    return false;
  }

  mjpeg_buf = (uint8_t *)malloc(mjpegBufSize);
  if (!mjpeg_buf)
  {
    Serial.println("mjpeg_buf malloc failed!\n");
    return false;
  }

  mjpeg_use_big_endian = useBigEndian;

  return true;
}

bool mjpeg_open(char *filename)
{
  mjpeg_file = fopen(filename, "r");

  if (!mjpeg_file)
  {
    Serial.printf("ERROR: File %s open Failed!", mjpeg_filename);
    gfx->printf("ERROR: File %s open Failed!", mjpeg_filename);

    return false;
  }

  mjpeg_total_frames = 0;
  mjpeg_total_read_video = 0;
  mjpeg_total_decode_video = 0;
  mjpeg_total_show_video = 0;

  mjpeg_file_idx = 0;

  return true;
}

bool mjpeg_read()
{
  // Serial.println("mjpeg_read()");
  unsigned long ms = millis();

  if (mjpeg_file_idx == 0)
  {
    mjpeg_buf_read = fread(mjpeg_read_buf, 1, READ_BUFFER_SIZE, mjpeg_file);
    // Serial.printf("mjpeg_buf_read: %d\n", mjpeg_buf_read);
    mjpeg_file_idx += mjpeg_buf_read;
  }
  mjpeg_buf_offset = 0;
  int i = 0;
  bool found_FFD8 = false;
  while ((mjpeg_buf_read > 0) && (!found_FFD8))
  {
    i = 0;
    while ((i < mjpeg_buf_read) && (!found_FFD8))
    {
      if ((mjpeg_read_buf[i] == 0xFF) && (mjpeg_read_buf[i + 1] == 0xD8)) // JPEG header
      {
        // Serial.printf("Found FFD8 at: %d.\n", i);
        found_FFD8 = true;
      }
      ++i;
    }
    if (found_FFD8)
    {
      --i;
    }
    else
    {
      mjpeg_buf_read = fread(mjpeg_read_buf, 1, READ_BUFFER_SIZE, mjpeg_file);
    }
  }
  uint8_t *_p = mjpeg_read_buf + i;
  mjpeg_buf_read -= i;
  bool found_FFD9 = false;
  if (mjpeg_buf_read > 0)
  {
    i = 3;
    while ((mjpeg_buf_read > 0) && (!found_FFD9))
    {
      if ((mjpeg_buf_offset > 0) && (mjpeg_buf[mjpeg_buf_offset - 1] == 0xFF) && (_p[0] == 0xD9)) // JPEG trailer
      {
        // Serial.printf("Found FFD9 at: %d.\n", i);
        found_FFD9 = true;
      }
      else
      {
        while ((i < mjpeg_buf_read) && (!found_FFD9))
        {
          if ((_p[i] == 0xFF) && (_p[i + 1] == 0xD9)) // JPEG trailer
          {
            found_FFD9 = true;
            ++i;
          }
          ++i;
        }
      }

      // Serial.printf("i: %d\n", i);
      memcpy(mjpeg_buf + mjpeg_buf_offset, _p, i);
      mjpeg_buf_offset += i;
      size_t o = mjpeg_buf_read - i;
      if (o > 0)
      {
        // Serial.printf("o: %d\n", o);
        memcpy(mjpeg_read_buf, _p + i, o);
        mjpeg_buf_read = fread(mjpeg_read_buf + o, 1, READ_BUFFER_SIZE - o, mjpeg_file);
        _p = mjpeg_read_buf;
        mjpeg_file_idx += mjpeg_buf_read;
        mjpeg_buf_read += o;
        // Serial.printf("mjpeg_buf_read: %d\n", mjpeg_buf_read);
      }
      else
      {
        mjpeg_buf_read = fread(mjpeg_read_buf, 1, READ_BUFFER_SIZE, mjpeg_file);
        _p = mjpeg_read_buf;
        mjpeg_file_idx += mjpeg_buf_read;
      }
      i = 0;
    }
    if (found_FFD9)
    {
      ++mjpeg_total_frames;
      mjpeg_total_read_video += millis() - ms;
      return true;
    }
  }

  mjpeg_total_read_video += millis() - ms;
  return false;
}

// pixel drawing callback
int jpegDrawCallback(JPEGDRAW *pDraw)
{
  // Serial.printf("Draw pos = %d,%d. size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  unsigned long ms = millis();
#ifdef EXCLUSIVE_DISPLAY_INTERFACE
  gfx->writeAddrWindow(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  if (mjpeg_use_big_endian)
  {
    gfx->writeBytes((uint8_t *)pDraw->pPixels, pDraw->iWidth * pDraw->iHeight * 2);
  }
  else
  {
    gfx->writePixels((uint16_t *)pDraw->pPixels, pDraw->iWidth * pDraw->iHeight);
  }
#else
  if (mjpeg_use_big_endian)
  {
    gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  }
  else
  {
    gfx->draw16bitRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  }
#endif
  ms = millis() - ms;
  mjpeg_total_decode_video -= ms;
  mjpeg_total_show_video += ms;

  return 1;
}

bool mjpeg_draw(int x, int y)
{
  // Serial.println("mjpeg_draw()");
  unsigned long ms = millis();

  jpegdec.openRAM(mjpeg_buf, mjpeg_buf_offset, jpegDrawCallback);
  if (mjpeg_use_big_endian)
  {
    jpegdec.setPixelType(RGB565_BIG_ENDIAN);
  }
#ifdef EXCLUSIVE_DISPLAY_INTERFACE
  gfx->startWrite();
#endif
  jpegdec.decode(x, y, 0);
#ifdef EXCLUSIVE_DISPLAY_INTERFACE
  gfx->endWrite();
#endif
#ifdef CANVAS
  gfx->flush();
#endif
  jpegdec.close();

  mjpeg_total_decode_video += millis() - ms;
  return true;
}

void mjpeg_close()
{
  fclose(mjpeg_file);
}