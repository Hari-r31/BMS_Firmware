#include <Arduino.h>
#include "gsm_sms.h"
#include "config.h"

static HardwareSerial gsm(2);   // UART2
static bool gsmReady = false;

static bool sendAT(const char* cmd, const char* expected, uint32_t timeoutMs = 2000) {
  gsm.flush();
  gsm.println(cmd);

  uint32_t t0 = millis();
  String   resp;

  while (millis() - t0 < timeoutMs) {
    while (gsm.available())
      resp += (char)gsm.read();
    if (resp.indexOf(expected) >= 0) return true;
    delay(1);
  }
  return false;
}

void gsmInit() {
  /* GSM_RX, GSM_TX, GSM_BAUD all come from config.h */
  gsm.begin(GSM_BAUD, SERIAL_8N1, GSM_RX, GSM_TX);
  delay(1000);

  if (!sendAT("AT",        "OK")) { Serial.println("[GSM] No response");      return; }
  if (!sendAT("ATE0",      "OK")) { Serial.println("[GSM] Echo-off failed");  return; }
  if (!sendAT("AT+CMGF=1", "OK")) { Serial.println("[GSM] SMS mode failed");  return; }

  gsmReady = true;
  Serial.println("[GSM] Ready");
}

bool gsmIsReady() { return gsmReady; }

bool gsmSendSMS(const char* msg) {
  if (!gsmReady) {
    Serial.println("[GSM] Not ready – SMS skipped");
    return false;
  }

  gsm.print("AT+CMGS=\"");
  gsm.print(GSM_ALERT_NUMBER);
  gsm.println("\"");
  delay(500);

  gsm.print(msg);
  gsm.write(26);   // CTRL+Z → send

  bool ok = sendAT("", "OK", 5000);
  Serial.println(ok ? "[GSM] SMS sent" : "[GSM] SMS failed");
  return ok;
}
