#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

/*
 * Status indicator on a single WS2812 (NeoPixel) RGB LED (WS2812_PIN).
 *
 *   - channel message received : strong green blinking for 3 s
 *   - direct message received  : strong white blinking for 3 s
 *   - LoRa transmit in progress: fast amber flicker (old HDD activity LED)
 *   - LoRa packet received     : short blue flash
 *
 * Event methods only record state; all LED writes happen in loop(), so they
 * are safe to call from board hooks (onBeforeTransmit, onPacketReceived).
 * A message blink takes priority over the TX flicker, so the ACK sent right
 * after a direct message does not hide the white blink.
 */
class RgbStatusLed {
public:
  RgbStatusLed(uint8_t pin) : _pixel(1, pin, NEO_GRB + NEO_KHZ800) {}

  void begin();
  void loop();
  void off();

  void onTxStart() { _tx = true; }
  void onTxEnd() { _tx = false; }
  void onPacketReceived() { _rx_until = millis() + RX_FLASH_MS; }
  void onChannelMessage() { startBlink(0, 255, 0); }
  void onDirectMessage() { startBlink(255, 255, 255); }

private:
  static const uint32_t MSG_BLINK_MS = 3000;    // total duration of a message blink
  static const uint32_t MSG_HALF_MS = 125;      // on/off half period (4 Hz)
  static const uint32_t TX_FLICKER_MS = 30;     // toggle period while transmitting
  static const uint32_t RX_FLASH_MS = 120;

  void startBlink(uint8_t r, uint8_t g, uint8_t b);
  void show(uint32_t color);

  Adafruit_NeoPixel _pixel;
  uint32_t _last_color = 0;
  bool _tx = false;
  uint32_t _rx_until = 0;
  uint32_t _blink_start = 0, _blink_until = 0;
  uint32_t _blink_color = 0;
};

#ifdef HAS_RGB_STATUS_LED
extern RgbStatusLed rgb_led;
#endif
