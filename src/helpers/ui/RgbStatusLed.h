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
 *   - duplicate packet         : the burst turns yellow (already seen, eg. relayed twice)
 *   - invalid packet           : short burst of magenta flicker (CRC error, bad header)
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
  void setEnabled(bool on);               // with a short green/red feedback blink
  bool isEnabled() const { return _enabled; }
  void flash(uint8_t r, uint8_t g, uint8_t b, uint32_t ms) { startBlink(r, g, b, ms); }   // eg. user feedback

  void onTxStart() { _tx = true; }
  void onTxEnd() { _tx = false; }
  // Every received packet first lands here (radio layer) and starts a green burst; the mesh
  // layer may then reclassify it (duplicate / own echo). A green burst of a packet that stayed
  // 'new' runs its full length: later packets do not replace it, so a fresh ACK is always seen.
  void onPacketReceived() {
    if (rxActive() && _rx_is_new) { _rx_protected = true; return; }
    _rx_protected = false; _rx_is_new = true;
    startRx(0, ACTIVITY_LEVEL, 0);                                   // green: new packet
  }
  void onDuplicatePacket() { classifyRx(ACTIVITY_LEVEL, ACTIVITY_LEVEL / 2, 0); }   // yellow/orange: already seen
  void onOwnEcho() { classifyRx(0, ACTIVITY_LEVEL, ACTIVITY_LEVEL); }              // cyan: my own packet relayed back
  void onInvalidPacket() { startRx(ACTIVITY_LEVEL, 0, ACTIVITY_LEVEL); }          // magenta
  void onChannelMessage() { startBlink(128, 128, 128); }
  void onDirectMessage() { startBlink(0, 0, 128); }

private:
  static const uint32_t MSG_BLINK_MS = 3000;    // total duration of a message blink
  static const uint32_t MSG_HALF_MS = 250;      // on/off half period (2 Hz)
  static const uint32_t FLICKER_MS = 30;        // toggle period of the TX/RX flicker
  static const uint32_t RX_BURST_MS = 240;      // length of the flicker burst per received packet
  static const uint8_t ACTIVITY_LEVEL = 40;     // TX/RX flicker brightness (messages use 128)

  void startBlink(uint8_t r, uint8_t g, uint8_t b, uint32_t ms = MSG_BLINK_MS);
  bool rxActive() const { return (int32_t)(_rx_until - millis()) > 0; }
  // reclassification of the packet just received: ignored while a previous 'new' burst is protected
  void classifyRx(uint8_t r, uint8_t g, uint8_t b) {
    if (_rx_protected) return;
    _rx_is_new = false;
    startRx(r, g, b);
  }
  void startRx(uint8_t r, uint8_t g, uint8_t b) {
    _rx_color = Adafruit_NeoPixel::Color(r, g, b);
    _rx_until = millis() + RX_BURST_MS;
  }
  void show(uint32_t color);

  Adafruit_NeoPixel _pixel;
  uint32_t _last_color = 0;
  bool _enabled = true;
  bool _tx = false;
  uint32_t _rx_until = 0;
  uint32_t _rx_color = 0;
  bool _rx_is_new = false;      // the running burst belongs to a packet still considered new
  bool _rx_protected = false;   // the last packet arrived while such a burst was running: its class is not shown
  uint32_t _blink_start = 0, _blink_until = 0;
  uint32_t _blink_color = 0;
};

#ifdef HAS_RGB_STATUS_LED
extern RgbStatusLed rgb_led;
#endif
