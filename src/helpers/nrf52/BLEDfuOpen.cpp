#include "BLEDfuOpen.h"

#define DFU_REV_APPMODE      0x0001
#define DFU_MAGIC_OTA_RESET  0xA8   // bootloader: "entered by soft reset, SD is not inited yet"

// Peer data block read by the bootloader (dfu_ble_svc.c, no-init RAM): 60 bytes + CRC16
#define DFU_PEER_DATA_ADDR   0x20007F80UL
#define DFU_PEER_DATA_LEN    64

/* DFU Service : 00001530-1212-EFDE-1523-785FEABCD123
 * DFU Control : 00001531-1212-EFDE-1523-785FEABCD123
 * DFU Packet  : 00001532-1212-EFDE-1523-785FEABCD123
 * DFU Revision: 00001534-1212-EFDE-1523-785FEABCD123
 */
static const uint8_t UUID128_SVC_DFU_OPEN[16] = {
  0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x30, 0x15, 0x00, 0x00
};
static const uint8_t UUID128_CHR_DFU_OPEN_CONTROL[16] = {
  0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x31, 0x15, 0x00, 0x00
};
static const uint8_t UUID128_CHR_DFU_OPEN_PACKET[16] = {
  0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x32, 0x15, 0x00, 0x00
};
static const uint8_t UUID128_CHR_DFU_OPEN_REVISION[16] = {
  0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x34, 0x15, 0x00, 0x00
};

static void bledfu_open_control_wr_authorize_cb(uint16_t conn_hdl, BLECharacteristic* chr, ble_gatts_evt_write_t* request) {
  if ((request->handle != chr->handles().value_handle) ||
      (request->op == BLE_GATTS_OP_PREP_WRITE_REQ) ||
      (request->op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) ||
      (request->op == BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL)) {
    return;
  }

  ble_gatts_rw_authorize_reply_params_t reply = { .type = BLE_GATTS_AUTHORIZE_TYPE_WRITE };
  if (!chr->notifyEnabled(conn_hdl)) {
    reply.params.write.gatt_status = BLE_GATT_STATUS_ATTERR_CPS_CCCD_CONFIG_ERROR;
    sd_ble_gatts_rw_authorize_reply(conn_hdl, &reply);
    return;
  }
  reply.params.write.gatt_status = BLE_GATT_STATUS_SUCCESS;
  sd_ble_gatts_rw_authorize_reply(conn_hdl, &reply);

  enum { START_DFU = 1 };
  if (request->data[0] != START_DFU) return;

  // No peer hand-over: an invalid CRC makes the bootloader advertise openly.
  memset((void*)DFU_PEER_DATA_ADDR, 0, DFU_PEER_DATA_LEN);

  BLEConnection* conn = Bluefruit.Connection(conn_hdl);
  Bluefruit.Advertising.restartOnDisconnect(false);
  if (conn) conn->disconnect();
  delay(100);   // let the disconnect go out before the reset

  sd_power_gpregret_clr(0, 0xFF);
  sd_power_gpregret_set(0, DFU_MAGIC_OTA_RESET);
  NVIC_SystemReset();
}

BLEDfuOpen::BLEDfuOpen(void)
  : BLEService(UUID128_SVC_DFU_OPEN), _chr_control(UUID128_CHR_DFU_OPEN_CONTROL) {
}

err_t BLEDfuOpen::begin(void) {
  VERIFY_STATUS(BLEService::begin());

  // Same characteristic set as BLEDfu, so DFU clients recognise the service
  BLECharacteristic chr_packet(UUID128_CHR_DFU_OPEN_PACKET);
  chr_packet.setTempMemory();
  chr_packet.setProperties(CHR_PROPS_WRITE_WO_RESP);
  chr_packet.setMaxLen(20);
  VERIFY_STATUS(chr_packet.begin());

  _chr_control.setProperties(CHR_PROPS_WRITE | CHR_PROPS_NOTIFY);
  _chr_control.setMaxLen(23);
  _chr_control.setWriteAuthorizeCallback(bledfu_open_control_wr_authorize_cb);
  VERIFY_STATUS(_chr_control.begin());

  BLECharacteristic chr_revision(UUID128_CHR_DFU_OPEN_REVISION);
  chr_revision.setTempMemory();
  chr_revision.setProperties(CHR_PROPS_READ);
  chr_revision.setFixedLen(2);
  VERIFY_STATUS(chr_revision.begin());
  chr_revision.write16(DFU_REV_APPMODE);

  return ERROR_NONE;
}
