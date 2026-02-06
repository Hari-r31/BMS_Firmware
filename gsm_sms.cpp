#include <Arduino.h>
#include "gsm_sms.h"
#include "config.h"


#define GSM_RX 16
#define GSM_TX 17

#define GSM_BAUD 9600

static HardwareSerial gsm(2);
static bool gsmReady = false;

static bool sendAT(const char* cmd, const char* expected, uint32_t timeout = 2000) {
  gsm.flush();
  gsm.println(cmd);

  uint32_t t0 = millis();
  String resp = "";

  while (millis() - t0 < timeout) {
    while (gsm.available()) {
      resp += char(gsm.read());
    }
    if (resp.indexOf(expected) >= 0) return true;
  }
  return false;
}

void gsmInit() {
  gsm.begin(GSM_BAUD, SERIAL_8N1, GSM_RX, GSM_TX);
  delay(1000);

  if (!sendAT("AT", "OK")) return;
  if (!sendAT("ATE0", "OK")) return;
  if (!sendAT("AT+CMGF=1", "OK")) return;

  gsmReady = true;
}

bool gsmIsReady() {
  return gsmReady;
}

bool gsmSendSMS(const char* msg) {
  if (!gsmReady) return false;

  gsm.print("AT+CMGS=\"");
  gsm.print(GSM_ALERT_NUMBER);

  gsm.println("\"");
  delay(500);

  gsm.print(msg);
  gsm.write(26);   // CTRL+Z

  return sendAT("", "OK", 5000);
}
