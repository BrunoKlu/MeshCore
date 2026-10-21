#pragma once

#include <bluefruit.h>

/*
 * Nordic legacy DFU service (same UUIDs as Bluefruit's BLEDfu, so nRF Connect
 * and the usual DFU tools can trigger it), with one difference in what happens
 * on "Start DFU":
 *
 *   BLEDfu hands the current peer (and its bond keys, if any) over to the
 *   bootloader and jumps into it without a reset. When the client is a phone
 *   with a resolvable private address, the bootloader then advertises with a
 *   whitelist restricted to that bonded phone, at the same BLE address: the
 *   phone keeps its GATT cache of the application, the transfer never starts,
 *   and nobody else can connect to rescue the device (seen on a GAT562 with
 *   nRF Connect for iOS, 2026-10-21).
 *
 *   BLEDfuOpen wipes the peer data instead and does a clean reset with
 *   DFU_MAGIC_OTA_RESET: the bootloader starts its own SoftDevice, advertises
 *   openly as "AdaDFU" at address+1, and any DFU client (phone, Pi, drone
 *   updater) can finish the job. The trigger itself still requires the
 *   encrypted link set by setPermission().
 */
class BLEDfuOpen : public BLEService {
protected:
  BLECharacteristic _chr_control;

public:
  BLEDfuOpen(void);
  virtual err_t begin(void);
};
