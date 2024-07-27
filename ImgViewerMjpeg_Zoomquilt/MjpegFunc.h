#pragma once

/*******************************************************************************
 * MJPEG related functions
 *
 * Dependent libraries:
 * ESP32_JPEG: https://github.com/esp-arduino-libs/ESP32_JPEG.git
 ******************************************************************************/
#define READ_BUFFER_SIZE 1024

#include <ESP32_JPEG_Library.h>
jpeg_dec_handle_t *jpeg_dec;
jpeg_dec_io_t *jpeg_io;
jpeg_dec_header_info_t *out_info;

/* variables */
int mjpeg_total_frames;
unsigned long mjpeg_total_read_video;
unsigned long mjpeg_total_decode_video;
unsigned long mjpeg_total_show_video;

bool mjpeg_use_big_endian;
uint8_t *mjpeg_read_buf;
uint8_t *mjpeg_buf;
uint16_t *mjpeg_image_buf;
FILE *mjpeg_file;

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

  mjpeg_image_buf = (uint16_t *)aligned_alloc(16, IMAGE_DATA_SIZE);
  if (!mjpeg_image_buf)
  {
    Serial.println("mjpeg_image_buf malloc failed!\n");
    return false;
  }

  mjpeg_use_big_endian = useBigEndian;

  jpeg_dec_config_t config = {
      .output_type = mjpeg_use_big_endian ? JPEG_RAW_TYPE_RGB565_BE : JPEG_RAW_TYPE_RGB565_LE,
      .rotate = JPEG_ROTATE_0D,
  };
  // Create jpeg_dec
  jpeg_dec = jpeg_dec_open(&config);

  // Create io_callback handle
  jpeg_io = (jpeg_dec_io_t *)calloc(1, sizeof(jpeg_dec_io_t));

  // Create out_info handle
  out_info = (jpeg_dec_header_info_t *)calloc(1, sizeof(jpeg_dec_header_info_t));

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
    while ((mjpeg_buf_read > 0) && (!found_FFD9))
    {
      if ((mjpeg_buf_offset > 0) && (mjpeg_buf[mjpeg_buf_offset - 1] == 0xFF) && (_p[0] == 0xD9)) // JPEG trailer
      {
        i = 1;
        // Serial.printf("Found FFD9 at: %d.\n", i);
        found_FFD9 = true;
      }
      else
      {
        i = 3;
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

bool mjpeg_draw(int x, int y)
{
  // Serial.println("mjpeg_draw()");
  unsigned long ms = millis();

  // Set input buffer and buffer len to io_callback
  jpeg_io->inbuf = mjpeg_buf;
  jpeg_io->inbuf_len = mjpeg_buf_offset;

  jpeg_dec_parse_header(jpeg_dec, jpeg_io, out_info);

  // Serial.printf("x: %d, y: %d, w: %d, h: %d\n", x, y, out_info->width, out_info->height);
  if ((out_info->width * out_info->height * 2) > IMAGE_DATA_SIZE)
  {
    return false;
  }

  jpeg_io->outbuf = (unsigned char *)mjpeg_image_buf;

  jpeg_dec_process(jpeg_dec, jpeg_io);

  mjpeg_total_decode_video += millis() - ms;

  ms = millis();

  if (mjpeg_use_big_endian)
  {
    gfx->draw16bitBeRGBBitmap(x, y, mjpeg_image_buf, out_info->width, out_info->height);
  }
  else
  {
    gfx->draw16bitRGBBitmap(x, y, mjpeg_image_buf, out_info->width, out_info->height);
  }
  mjpeg_total_show_video += millis() - ms;

  return true;
}

void mjpeg_close()
{
  fclose(mjpeg_file);
}