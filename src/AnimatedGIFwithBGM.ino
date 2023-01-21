/*
MIT License

Copyright (c) 2021-2023 riraosan.github.io

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <Arduino.h>
#include <WiFi.h>

#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
// If the sample BIT of the DAC is 32 bits,
// you will need to replace the file in the patch folder with the file of the same name from the ESP8266Audio#1.9.5 library.
#include <AudioOutputI2S.h>
#include <AudioFileSourceID3.h>
#include <Ticker.h>
#include <Button2.h>
#include <LGFX_8BIT_CVBS.h>
#include "Video.hpp"

static LGFX_8BIT_CVBS display;

#define TFCARD_CS_PIN 4  // dummy
#define LGFX          LGFX_8BIT_CVBS

#define LGFX_ONLY
#define USE_DISPLAY

#if defined(NON4)
#define SDU_APP_NAME "NON-Chan ep4"
#elif defined(NON5)
#define SDU_APP_NAME "NON-Chan ep5"
#elif defined(KANDENCH)
#define SDU_APP_NAME "KANDENCH flash"
#elif defined(KATAYAMA)
#define SDU_APP_NAME "KATAYAMAgerion"
#endif

#include <M5StackUpdater.h>

AudioGeneratorMP3  *mp3;
AudioFileSourceSD  *file;
AudioOutputI2S     *out;
AudioFileSourceID3 *id3;

Video video(&display);

Ticker audioStart;
Ticker videoStart;

Button2 button;

bool isActive = false;

TaskHandle_t taskHandle;

#define MP3_FILE_4 "/non4.mp3"
#define GIF_FILE_4 "/non4.gif"
#define WAIT_MP3_4 100
#define WAIT_GIF_4 1

#define MP3_FILE_5 "/non5.mp3"
#define GIF_FILE_5 "/non5.gif"
#define WAIT_MP3_5 1
#define WAIT_GIF_5 1000

bool bA = false;
bool bB = false;
bool bC = false;

void handler(Button2 &btn) {
  switch (btn.getType()) {
    case clickType::single_click:
      Serial.print("single ");
      bB = true;
      break;
    case clickType::double_click:
      Serial.print("double ");
      bC = true;
      break;
    case clickType::triple_click:
      Serial.print("triple ");
      break;
    case clickType::long_click:
      Serial.print("long ");
      bA = true;
      break;
    case clickType::empty:
      break;
    default:
      break;
  }

  Serial.print("click");
  Serial.print(" (");
  Serial.print(btn.getNumberOfClicks());
  Serial.println(")");
}

bool buttonAPressed(void) {
  bool temp = bA;
  bA        = false;

  return temp;
}

bool buttonBPressed(void) {
  bool temp = bB;
  bB        = false;

  return temp;
}

bool buttonCPressed(void) {
  bool temp = bC;
  bC        = false;

  return temp;
}

void ButtonUpdate() {
  button.loop();
}

void setupButton(void) {
  // G39 button
  button.setClickHandler(handler);
  button.setDoubleClickHandler(handler);
  button.setTripleClickHandler(handler);
  button.setLongClickHandler(handler);
  button.begin(39);

  SDUCfg.setSDUBtnA(&buttonAPressed);
  SDUCfg.setSDUBtnB(&buttonBPressed);
  SDUCfg.setSDUBtnC(&buttonCPressed);
  SDUCfg.setSDUBtnPoller(&ButtonUpdate);
}

void StatusCallback(void *cbData, int code, const char *string) {
  const char *ptr = reinterpret_cast<const char *>(cbData);

  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1) - 1] = 0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}

void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string) {
  (void)cbData;
  Serial.printf("ID3 callback for: %s = '", type);

  if (isUnicode) {
    string += 2;
  }

  while (*string) {
    char a = *(string++);
    if (isUnicode) {
      string++;
    }
    Serial.printf("%c", a);
  }
  Serial.printf("'\n");
  Serial.flush();
}

void audioTask(void *) {
  for (;;) {
    if (isActive) {
      if (!mp3->loop()) {
        isActive = false;
        mp3->stop();
      }
    }
    delay(1);
  }
}

void startAudio(void) {
  mp3->begin(id3, out);
  isActive = true;
}

void startVideo(void) {
  video.start();
}

void startAV(uint32_t waitMP3, uint32_t waitGIF) {
  // 再生開始タイミングを微調整する
  audioStart.once_ms(waitMP3, startAudio);
  videoStart.once_ms(waitGIF, startVideo);
}

void setupAV(String mp3File, String gifFile) {
  if (taskHandle != nullptr) {
    vTaskDelete(taskHandle);
  }

  if (out != nullptr) {
    delete out;
  }

  if (mp3 != nullptr) {
    delete mp3;
  }

  if (file != nullptr) {
    delete file;
  }

  if (id3 != nullptr) {
    delete id3;
  }

  // Audio
  out = new AudioOutputI2S(I2S_NUM_1);  // CVBSがI2S0を使っている。AUDIOはI2S1を設定
  out->SetPinout(22, 21, 25);
  out->SetGain(0.3);  // 1.0だと音が大きすぎる。0.3ぐらいが適当。後は外部アンプで増幅するのが適切。

  mp3 = new AudioGeneratorMP3();
  mp3->RegisterStatusCB(StatusCallback, (void *)"mp3");

  file = new AudioFileSourceSD(mp3File.c_str());
  id3  = new AudioFileSourceID3(file);
  id3->RegisterMetadataCB(MDCallback, (void *)"ID3TAG");

  // External DAC Audio Task
  xTaskCreatePinnedToCore(audioTask, "audioTask", 4096, nullptr, 2, &taskHandle, PRO_CPU_NUM);

  // Animation
  video.setFilename(gifFile);
  video.openGif();
}

void setup() {
  log_d("Free Heap : %d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  display.setColorDepth(8);
  display.setRotation(0);

  display.begin();
  display.startWrite();

  setupButton();

  setSDUGfx(&display);
  checkSDUpdater(
      SD,            // filesystem (default=SD)
      MENU_BIN,      // path to binary (default=/menu.bin, empty string=rollback only)
      10000,         // wait delay, (default=0, will be forced to 2000 upon ESP.restart() )
      TFCARD_CS_PIN  // (usually default=4 but your mileage ma+-y vary)
  );

  video.begin();

  log_d("Free Heap : %d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

#if defined(NON4)
  episode4();
#elif defined(NON5)
  episode5();
#elif defined(KANDENCH)
  kandench();
#elif defined(KATAYAMA)
  katayama();
#endif
}

void episode4(void) {
  setupAV(MP3_FILE_4, GIF_FILE_4);
  startAV(WAIT_MP3_4, WAIT_GIF_4);
}

void episode5(void) {
  setupAV(MP3_FILE_5, GIF_FILE_5);
  startAV(WAIT_MP3_5, WAIT_GIF_5);
}

void kandench(void) {
  setupAV("/kandenchiflash.mp3", "/kandenchiflash.gif");
  startAV(500, 0);
}

void katayama(void) {
  setupAV("/katayama.mp3", "/katayama.gif");
  out->SetGain(0.5);

  startAV(0, 0);
}

void loop() {
  video.update();  // GIFアニメのウェイト時間毎に１フレームを描画する。
}
