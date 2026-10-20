#pragma once

#include <MeshCore.h>
#include <Arduino.h>
#include <helpers/NRF52Board.h>
#ifdef HAS_RGB_STATUS_LED
  #include <helpers/ui/RgbStatusLed.h>
#endif


#define GAT562_RUN_RUNNING 0xC1   // GPREGRET2 breadcrumb values (see the .cpp)
#define GAT562_RUN_CLEAN   0xC2

class GAT56230SMeshKitBoard : public NRF52BoardDCDC {
protected:
#ifdef NRF52_POWER_MANAGEMENT
  void initiateShutdown(uint8_t reason) override;
#endif

public:
  GAT56230SMeshKitBoard() : NRF52Board("GAT562_OTA") {}
  void begin();

  #define BATTERY_SAMPLES 8

  uint16_t getBattMilliVolts() override {
    analogReadResolution(12);

    uint32_t raw = 0;
    for (int i = 0; i < BATTERY_SAMPLES; i++) {
      raw += analogRead(PIN_VBAT_READ);
    }
    raw = raw / BATTERY_SAMPLES;

    return (ADC_MULTIPLIER * raw) / 4096;
  }

  const char* getManufacturerName() const override {
    return "GAT562 30S Mesh Kit";
  }

#if defined(P_LORA_TX_LED)
  void onBeforeTransmit() override {
    digitalWrite(P_LORA_TX_LED, HIGH);   // turn TX LED on
  #ifdef HAS_RGB_STATUS_LED
    rgb_led.onTxStart();
  #endif
  }

  void onAfterTransmit() override {
    digitalWrite(P_LORA_TX_LED, LOW);   // turn TX LED off
  #ifdef HAS_RGB_STATUS_LED
    rgb_led.onTxEnd();
  #endif
  }
#endif

#ifdef HAS_RGB_STATUS_LED
  void onPacketReceived() override {
    rgb_led.onPacketReceived();
  }
#endif

  // CLI "bootreason": raw RESETREAS captured before SystemInit() (see the
  // .cpp), to diagnose unexpected reboots.
  bool handleCommand(const char* command, uint32_t sender_timestamp, char* reply) override {
    if (strcmp(command, "bootreason") == 0) {
      extern uint32_t gat562_reset_reason;
      extern uint8_t gat562_prev_run;
      uint32_t r = gat562_reset_reason;
      const char* prev = gat562_prev_run == GAT562_RUN_RUNNING ? "ABRUPT (crash/reset while running)"
                       : gat562_prev_run == GAT562_RUN_CLEAN ? "clean reboot"
                       : gat562_prev_run == 0 ? "power-on or first boot" : "unknown";
      sprintf(reply, "> prev=0x%02X %s; RESETREAS=0x%lX pin=%d sreq=%d dog=%d lockup=%d sysoff=%d",
              gat562_prev_run, prev, (unsigned long)r,
              (int)!!(r & POWER_RESETREAS_RESETPIN_Msk), (int)!!(r & POWER_RESETREAS_SREQ_Msk),
              (int)!!(r & POWER_RESETREAS_DOG_Msk), (int)!!(r & POWER_RESETREAS_LOCKUP_Msk),
              (int)!!(r & POWER_RESETREAS_OFF_Msk));
      return true;
    }
    return false;
  }

  void markCleanStop();

  void reboot() override {
    markCleanStop();
    NRF52Board::reboot();
  }

  void powerOff() override {
    markCleanStop();
    uint32_t button_pin = PIN_BUTTON1;
    nrf_gpio_cfg_input(button_pin, NRF_GPIO_PIN_PULLUP);
    nrf_gpio_cfg_sense_set(button_pin, NRF_GPIO_PIN_SENSE_LOW);
    NRF52Board::powerOff();
  }

};
