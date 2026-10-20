#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

/*
 * Status indicator on a single WS2812 (NeoPixel) RGB LED (WS2812_PIN).
 *
 *   - channel message received : white blinking (half brightness) for 3 s
 *   - direct message received  : blue blinking (half brightness) for 3 s
 *   - LoRa transmit in progress: fast red flicker (old HDD activity LED)
 *   - LoRa packet received     : short burst of fast green flicker
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
  void onPacketReceived() { _rx_until = millis() + RX_BURST_MS; }
  void onChannelMessage() { startBlink(128, 128, 128); }
  void onDirectMessage() { startBlink(0, 0, 128); }

private:
  static const uint32_t MSG_BLINK_MS = 3000;    // total duration of a message blink
  static const uint32_t MSG_HALF_MS = 250;      // on/off half period (2 Hz)
  static const uint32_t FLICKER_MS = 30;        // toggle period of the TX/RX flicker
  static const uint32_t RX_BURST_MS = 240;      // length of the flicker burst per received packet
  static const uint8_t ACTIVITY_LEVEL = 40;     // TX/RX flicker brightness (messages use 128)

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
