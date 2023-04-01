
#pragma once

#include <memory>
#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <LGFX_8BIT_CVBS.h>
#include <AnimatedGIF.h>

class Video {
public:
  Video(LGFX_8BIT_CVBS *display) : _filename(""),
                                   _story(0),
                                   _isActive(false),
                                   _isOpen(false),
                                   _frameCount(0) {
    p_display = display;
    p_sprite.reset(new M5Canvas(p_display));
  }

  void begin(void) {
    p_display->fillScreen(TFT_BLACK);
    p_display->display();

    _width  = p_display->width();
    _height = p_display->height();

    log_i("width, %d, height, %d", _width, _height);

    p_sprite->setColorDepth(8);
    p_sprite->setRotation(0);

    if (!p_sprite->createSprite(180, 147)) {
      log_e("can not allocate sprite buffer.");
    }

    p_sprite->setSwapBytes(true);

    _gif.begin(LITTLE_ENDIAN_PIXELS);
    log_i("start CVBS");
  }

  void showGuide(String first, String second) {
    p_display->setCursor(10, 10);
    p_display->print(first.c_str());
    p_display->setCursor(10, 10 + 9);
    p_display->print(second.c_str());
  }

  void update(void) {
    _lTimeStart = lgfx::v1::millis();

    if (_isActive) {
      if (_frameCount == 0) {
        p_display->fillScreen(TFT_BLACK);
      }

      if (_gif.playFrame(false, &_waitTime)) {
        int actualWait;

#if defined(NON4)
        p_display->setPivot((_width >> 1) + 3, (_height >> 1) + 30);
        p_sprite->pushRotateZoom(0, 1.3, 1.6);
        p_display->display();

        actualWait = _waitTime - (lgfx::v1::millis() - _lTimeStart);
        if (0 <= _frameCount && _frameCount < 200) {
          actualWait -= 0;
        } else if (1336 <= _frameCount && _frameCount < 1536) {
          actualWait -= 0;
        }
#elif defined(NON5)
        p_display->setPivot((_width >> 1) + 3, (_height >> 1) + 30);
        p_sprite->pushRotateZoom(0, 1.3, 1.6);
        p_display->display();

        actualWait = _waitTime - (lgfx::v1::millis() - _lTimeStart);
        if (0 <= _frameCount && _frameCount < 200) {
          actualWait += 5;
        } else if (200 <= _frameCount && _frameCount < 400) {
          actualWait += 12;
        } else if (400 <= _frameCount && _frameCount < 600) {
          actualWait += 27;
        } else if (600 <= _frameCount && _frameCount < 771) {
          actualWait += 16;
        }
#elif defined(KANDENCH)
        p_display->setPivot((_width >> 1) + 3, (_height >> 1));
        p_sprite->pushRotateZoom(0, 1.0, 1.0);
        p_display->display();

        actualWait = _waitTime - (lgfx::v1::millis() - _lTimeStart);
        if (0 <= _frameCount && _frameCount < 292) {
          actualWait -= 3;
        } else if (292 <= _frameCount && _frameCount < 856) {
          actualWait -= 5;
        } else if (856 <= _frameCount && _frameCount < 1778) {
          actualWait -= 4;
        } else if (1778 <= _frameCount && _frameCount < 2560) {
          actualWait -= 5;
        } else if (2561 <= _frameCount && _frameCount < 3482) {
          actualWait -= 5;
        } else if (3482 <= _frameCount && _frameCount < 4985) {
          actualWait -= 4;
        }
#elif defined(KATAYAMA)
        p_display->setPivot((_width >> 1) + 3, (_height >> 1) + 10);
        p_sprite->pushRotateZoom(0, 1.3, 1.6);
        p_display->display();

        actualWait = _waitTime - (lgfx::v1::millis() - _lTimeStart);
        if (0 <= _frameCount && _frameCount < 30) {
          actualWait -= 5;
        } else if (30 <= _frameCount && _frameCount < 1356) {
          actualWait -= 4;
        } else if (1356 <= _frameCount && _frameCount < 2735) {
          actualWait -= 5;
        }
#endif
        if (actualWait >= 0) {
          delay(actualWait);
        } else {
          log_i("[%04d], GIF _waitTime, %04d [ms], delta, %04d [ms]", _frameCount, _waitTime, actualWait);
        }
        _frameCount++;
        return;
      } else {
        closeGif();
        p_display->fillScreen(TFT_BLACK);
        _isActive   = false;
        _frameCount = 0;
      }
    }

    p_display->display();
  }

  void setFilename(String name) {
    _filename = name;
  }

  void openGif(void) {
    if (_gif.open(_filename.c_str(), _GIFOpenFile, _GIFCloseFile, _GIFReadFile, _GIFSeekFile, _GIFDraw)) {
      _isOpen = true;
    } else {
      log_e("Can not open gif file.");
      _isOpen = false;
    }
  }

  void closeGif(void) {
    if (_isOpen) {
      _gif.close();
      _isOpen   = false;
      _isActive = false;
    }
  }

  void resetGif(void) {
    if (_isOpen) {
      _gif.reset();
    }
  }

  void start(void) {
    if (_isOpen)
      _isActive = true;
  }

  void stop(void) {
    if (_isOpen)
      _isActive = false;
  }

  bool state(void) {
    return _isActive;
  }

  void setEpisode(int epi) {
    _story = epi;
  }

private:
  static void *_GIFOpenFile(const char *fname, int32_t *pSize) {
    _gifFile = SD.open(fname);

    if (_gifFile) {
      *pSize = _gifFile.size();
      return (void *)&_gifFile;
    }

    return nullptr;
  }

  static void _GIFCloseFile(void *pHandle) {
    File *f = static_cast<File *>(pHandle);

    if (f != nullptr)
      f->close();
  }

  static int32_t _GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
    int32_t iBytesRead;
    iBytesRead = iLen;
    File *f    = static_cast<File *>(pFile->fHandle);
    // Note: If you read a file all the way to the last byte, seek() stops working
    if ((pFile->iSize - pFile->iPos) < iLen) {
      iBytesRead = pFile->iSize - pFile->iPos - 1;  // <-- ugly work-around
    }

    if (iBytesRead <= 0) {
      return 0;
    }

    iBytesRead  = (int32_t)f->read(pBuf, iBytesRead);
    pFile->iPos = f->position();

    return iBytesRead;
  }

  static int32_t _GIFSeekFile(GIFFILE *pFile, int32_t iPosition) {
    int   i = micros();
    File *f = static_cast<File *>(pFile->fHandle);

    f->seek(iPosition);
    pFile->iPos = (int32_t)f->position();
    i           = micros() - i;
    // log_i("Seek time = %d us\n", i);
    return pFile->iPos;
  }

  static void _GIFDraw(GIFDRAW *pDraw) {
    uint8_t  *s;
    uint16_t *d, *usPalette, usTemp[180];
    int       x, y, iWidth;

    iWidth = pDraw->iWidth;
    if (iWidth > _width)
      iWidth = _width;

    usPalette = pDraw->pPalette;
    y         = pDraw->iY + pDraw->y;  // current line

    s = pDraw->pPixels;
    if (pDraw->ucDisposalMethod == 2)  // restore to background color
    {
      for (x = 0; x < iWidth; x++) {
        if (s[x] == pDraw->ucTransparent)
          s[x] = pDraw->ucBackground;
      }
      pDraw->ucHasTransparency = 0;
    }

    // Apply the new pixels to the main image
    if (pDraw->ucHasTransparency)  // if transparency used
    {
      uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
      int      x, iCount;
      pEnd   = s + iWidth;
      x      = 0;
      iCount = 0;  // count non-transparent pixels
      while (x < iWidth) {
        c = ucTransparent - 1;
        d = usTemp;
        while (c != ucTransparent && s < pEnd) {
          c = *s++;
          if (c == ucTransparent)  // done, stop
          {
            s--;  // back up to treat it like transparent
          } else  // opaque
          {
            *d++ = usPalette[c];
            iCount++;
          }
        }              // while looking for opaque pixels
        if (iCount) {  // any opaque pixels?
          p_sprite->setWindow(pDraw->iX + x, y, iCount, 1);
          p_sprite->pushPixels((uint16_t *)usTemp, iCount, true);
          x += iCount;
          iCount = 0;
        }

        // no, look for a run of transparent pixels
        c = ucTransparent;
        while (c == ucTransparent && s < pEnd) {
          c = *s++;
          if (c == ucTransparent)
            iCount++;
          else
            s--;
        }
        if (iCount) {
          x += iCount;  // skip these
          iCount = 0;
        }
      }
    } else {
      s = pDraw->pPixels;
      // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
      for (x = 0; x < iWidth; x++) {
        usTemp[x] = usPalette[*s++];
      }
      p_sprite->setWindow(pDraw->iX, y, iWidth, 1);
      p_sprite->pushPixels((uint16_t *)usTemp, iWidth, true);
    }
  }

  static File _gifFile;

  static int _width;
  static int _height;

  AnimatedGIF _gif;
  String      _filename;
  int         _story;

  bool _isActive;
  bool _isOpen;

  unsigned long _lTimeStart;
  int32_t       _waitTime;
  int           _frameCount;

  LGFX_8BIT_CVBS                  *p_display;
  static std::unique_ptr<M5Canvas> p_sprite;
};

std::unique_ptr<M5Canvas> Video::p_sprite;
File                      Video::_gifFile;

int Video::_width  = 0;
int Video::_height = 0;
