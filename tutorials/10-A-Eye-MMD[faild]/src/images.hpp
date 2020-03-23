/**
Copyright (c) 2018 Gombe.

This software is released under the MIT License.
http://opensource.org/licenses/mit-license.php
*/
#ifndef _TEXTURE_IMAGE
#define _TEXTURE_IMAGE
#include <ff.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int filopen(const char *pathname, FIL *f);
extern int filread(FIL *fp, void *buff, size_t byte);

namespace imgs {
namespace bmp {
// common header
typedef struct header {
  uint16_t type;
  uint32_t size;
  uint16_t reserved1;
  uint16_t reserved2;
  uint32_t ofs;
} __attribute__((__packed__)) header;

// read headder size first.

// if 12 byte
typedef struct os2coreheader {
  int16_t width;
  int16_t height;
  uint16_t planes;
  uint16_t bitperpixel;
} __attribute__((__packed__)) os2coreheader;

// 40 byte
typedef struct wincoreheader {
  int32_t width;
  int32_t height;
  uint16_t planes;
  uint16_t bitperpixel;
  uint32_t compression;
  uint32_t imgsize;
  uint32_t xppm;          // pixel per meter for x
  uint32_t yppm;          // pixel per meter for y, ex, 96dpi 3780
  uint32_t clrused;       // Color lookup table used count
  uint32_t clrimportant;  // important color palatte index
} __attribute__((__packed__)) wincoreheader;
}  // namespace bmp

// image loader
class image {
 public:
  uint16_t width;
  uint16_t width_powerof2;  // width=height, width is power of 2 and 4*n
  uint16_t *data;           // RGB565 format, no transparent for now.
  unsigned int ra;

  void newimage(const char *path) {
    bmp::header h;
    FIL f;
    int coreheadersize;

    if (f_open(&f, path, FA_READ) != 0) {
      printf("file open err(%s)\r\n", path);
      // fail();
    }
    // filread(&f, &h, sizeof(bmp::header));
    // filread(&f, &coreheadersize, 4);
    f_read(&f, &h, sizeof(bmp::header), &ra);
    f_read(&f, &coreheadersize, 4, &ra);

    if (coreheadersize == 12) {
      // bmp os/2 format
      bmp::os2coreheader h;
      // filread(&f, &h, sizeof(bmp::os2coreheader));
      f_read(&f, &h, sizeof(bmp::os2coreheader), &ra);
      if (h.width != h.height) {
        printf("%dx%d, not equal\r\n", h.width, h.height);
        // fail();
      }
      if (h.bitperpixel != 16) {
        printf("not RGB565,(%dbps)\r\n", h.bitperpixel);
        // fail();
      }
      width = h.width;
    } else if (coreheadersize == 40) {
      bmp::wincoreheader h;
      // filread(&f, &h, sizeof(bmp::wincoreheader));
      f_read(&f, &h, sizeof(bmp::wincoreheader), &ra);
      if (h.width != h.height) {
        printf("%dx%d, not equal\r\n", h.width, h.height);
        // fail();
      }
      if (h.bitperpixel != 16) {
        printf("not RGB565,(%dbps)\r\n", h.bitperpixel);
        // fail();
      }
      width = h.width;
      if (h.compression != 3) {
        printf("compression %d is not supported\r\n", h.compression);
        // fail();
      }
    } else {
      printf("file format error(%d)\r\n", coreheadersize);
      // fail();
    }
    if ((width & (width - 1)) != 0) {
      printf("width=%d is not power of 2\r\n", width);
      // fail();
    }
    width_powerof2 = -1;
    for (int i = width; i != 0; i >>= 1, width_powerof2++)
      ;
    data = (uint16_t *)malloc(width * width * 2);
    // filread(&f, data, 12);
    f_read(&f, data, 12, &ra);
    if (((uint32_t *)data)[0] != 0x0000F800 ||
        ((uint32_t *)data)[1] != 0x000007E0 ||
        ((uint32_t *)data)[2] != 0x000001F) {
      printf("bit field err, not rgb565 format\r\n");
    }
    f_read(&f, data, width * width * 2, &ra);
    // if (filread(&f, data, width * width * 2) != width * width * 2) {
    if (ra != width * width * 2) {
      printf("file load err\r\n");
    }
  }
};

class images {
 public:
  char **texturenamelist;  // texture name buffer;
  image *imagelist;
  int n;

  void add(const char *path, int idx) {
    imagelist[idx].newimage(path);
    texturenamelist[idx] = (char *)malloc(strlen(path) + 1);
    strcpy(texturenamelist[idx], path);
  }
  void init(int max) {
    imagelist = (image *)malloc(sizeof(image) * max);
    texturenamelist = (char **)malloc(sizeof(char *) * max);
  }

  image get_or_add(const char *path) {
    if (*path == '\0') return imagelist[0];
    if (n == 0) {
      add(path, 0);
    } else {
      for (int i = 0; i < n; i++) {
        if (strcmp(path, texturenamelist[i]) == 0) {
          return imagelist[i];
        }
      }
      add(path, n);
    }

    return imagelist[n++];
  }
};
}  // namespace imgs

#endif
