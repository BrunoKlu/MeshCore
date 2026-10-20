#include <Arduino.h>
#include <Wire.h>

#include "GAT56230SMeshKitBoard.h"

// RESETREAS as it was at power-up/reset, captured before SystemInit() and the
// Arduino core touch the register (same trick as NRF52_POWER_MANAGEMENT).
uint32_t gat562_reset_reason = 0;
// GPREGRET2 breadcrumb: the bootloader leaves this register alone and it
// survives any reset except a power-on. RUNNING is written at boot, CLEAN
// before a deliberate reboot/power-off, so the next boot can tell a crash
// (or external reset) from a clean restart or a power cycle (0).
uint8_t gat562_prev_run = 0;
static void __attribute__((constructor(101))) gat562_early_reset_capture() {
  gat562_reset_reason = NRF_POWER->RESETREAS;
  gat562_prev_run = NRF_POWER->GPREGRET2;
  NRF_POWER->GPREGRET2 = GAT562_RUN_RUNNING;
}

void GAT56230SMeshKitBoard::markCleanStop() {
  uint8_t sd_enabled = 0;
  sd_softdevice_is_enabled(&sd_enabled);
  if (sd_enabled) {
    sd_power_gpregret_clr(1, 0xFF);
    sd_power_gpregret_set(1, GAT562_RUN_CLEAN);
  } else {
    NRF_POWER->GPREGRET2 = GAT562_RUN_CLEAN;
  }
}


#ifdef NRF52_POWER_MANAGEMENT
// Static configuration for power management
// Values set in variant.h defines
const PowerMgtConfig power_config = {
  .lpcomp_ain_channel = PWRMGT_LPCOMP_AIN,
  .lpcomp_refsel = PWRMGT_LPCOMP_REFSEL,
  .voltage_bootlock = PWRMGT_VOLTAGE_BOOTLOCK
};


void GAT56230SMeshKitBoard::initiateShutdown(uint8_t reason) {
  // Disable LoRa module power before shutdown
  digitalWrite(SX126X_POWER_EN, LOW);

  if (reason == SHUTDOWN_REASON_LOW_VOLTAGE ||
      reason == SHUTDOWN_REASON_BOOT_PROTECT) {
    configureVoltageWake(power_config.lpcomp_ain_channel, power_config.lpcomp_refsel);
  }

  enterSystemOff(reason);
}
#endif // NRF52_POWER_MANAGEMENT


void GAT56230SMeshKitBoard::begin() {
  NRF52BoardDCDC::begin();
  pinMode(PIN_VBAT_READ, INPUT);

  // Set all button pins to INPUT_PULLUP
  pinMode(PIN_BUTTON1, INPUT_PULLUP);
  pinMode(PIN_BUTTON2, INPUT_PULLUP);
  pinMode(PIN_BUTTON3, INPUT_PULLUP);
  pinMode(PIN_BUTTON4, INPUT_PULLUP);
  pinMode(PIN_BUTTON5, INPUT_PULLUP);
  pinMode(PIN_BUTTON6, INPUT_PULLUP);

#if defined(PIN_BOARD_SDA) && defined(PIN_BOARD_SCL)
  Wire.setPins(PIN_BOARD_SDA, PIN_BOARD_SCL);
#endif

  Wire.begin();

  pinMode(SX126X_POWER_EN, OUTPUT);
#ifdef NRF52_POWER_MANAGEMENT
  // Boot voltage protection check (may not return if voltage too low)
  // We need to call this after we configure SX126X_POWER_EN as output but before we pull high
  checkBootVoltage(&power_config);
#endif
  digitalWrite(SX126X_POWER_EN, HIGH);
  delay(10);   // give sx1262 some time to power up

#ifdef HAS_RGB_STATUS_LED
  rgb_led.begin();
#endif
}