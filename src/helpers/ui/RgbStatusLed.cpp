#include "RgbStatusLed.h"

#ifdef HAS_RGB_STATUS_LED
RgbStatusLed rgb_led(WS2812_PIN);
#endif

void RgbStatusLed::begin() {
  _pixel.begin();
  _pixel.setBrightness(255);
  _last_color = 1;   // force the first write
  show(0);
}

void RgbStatusLed::off() {
  _tx = false;
  _rx_until = 0;
  _blink_until = 0;
  show(0);
}

void RgbStatusLed::startBlink(uint8_t r, uint8_t g, uint8_t b) {
  _blink_color = Adafruit_NeoPixel::Color(r, g, b);
  _blink_start = millis();
  _blink_until = _blink_start + MSG_BLINK_MS;
}

void RgbStatusLed::show(uint32_t color) {
  if (color == _last_color) return;   // show() blocks briefly: only write on change
  _last_color = color;
  _pixel.setPixelColor(0, color);
  _pixel.show();
}

void RgbStatusLed::loop() {
  uint32_t now = millis();
  if (_blink_until && (int32_t)(_blink_until - now) > 0) {
    bool on = ((now - _blink_start) / MSG_HALF_MS) % 2 == 0;
    show(on ? _blink_color : 0);
  } else if (_tx) {
    bool on = (now / TX_FLICKER_MS) % 2 == 0;
    show(on ? Adafruit_NeoPixel::Color(255, 70, 0) : 0);
  } else if (_rx_until && (int32_t)(_rx_until - now) > 0) {
    show(Adafruit_NeoPixel::Color(0, 0, 255));
  } else {
    show(0);
  }
}
