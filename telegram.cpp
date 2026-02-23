#include "telegram.h"
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>   // ESP32 core 3.x (replaces WiFiClientSecure)

static bool          initialized      = false;
static unsigned long lastTelegramTime = 0;
static bool          neverSent        = true;  // first message always bypasses cooldown

#define TELEGRAM_COOLDOWN_MS  30000UL   // 30 s minimum between messages

/* ═══════════════════════════════════════════
   INIT
   ═══════════════════════════════════════════ */

void telegramInit() {
  if (initialized) return;

  if (!TELEGRAM_BOT_TOKEN[0] || !TELEGRAM_CHAT_ID[0]) {
    Serial.println("[TELEGRAM] ERROR: Token or Chat ID missing in config.h");
    return;
  }

  /* Set lastTelegramTime far enough in the past so cooldown is already
     expired the moment the first sendTelegramAlert() is called. */
  lastTelegramTime = 0;
  neverSent        = true;

  initialized = true;
  Serial.println("[TELEGRAM] Ready");
}

/* ═══════════════════════════════════════════
   INTERNAL SEND  (shared by both public functions)
   ═══════════════════════════════════════════ */

static bool doSend(const String& message) {
  /* Wait up to 5 s for WiFi (handles messages sent right after boot) */
  unsigned long wifiWait = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiWait < 5000) {
    delay(200);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[TELEGRAM] WiFi not connected – skipped");
    return false;
  }

  String url = String("https://api.telegram.org/bot") +
               TELEGRAM_BOT_TOKEN + "/sendMessage";

  /* Escape characters that break JSON */
  String escaped = message;
  escaped.replace("\\", "\\\\");
  escaped.replace("\"", "\\\"");
  escaped.replace("\n", "\\n");
  escaped.replace("\r", "");

  /* Plain text – NO parse_mode (avoids Markdown/HTML rejection) */
  String payload =
    "{\"chat_id\":\"" + String(TELEGRAM_CHAT_ID) + "\","
    "\"text\":\""    + escaped + "\"}";

  NetworkClientSecure client;
  client.setInsecure();
  client.setTimeout(8);

  HTTPClient http;
  http.setTimeout(8000);

  if (!http.begin(client, url)) {
    Serial.println("[TELEGRAM] http.begin failed");
    return false;
  }

  http.addHeader("Content-Type", "application/json");

  int    code     = http.POST(payload);
  String response = http.getString();
  http.end();

  if (code >= 200 && code < 300) {
    lastTelegramTime = millis();
    neverSent        = false;
    Serial.println("[TELEGRAM] Alert sent OK");
    return true;
  }

  Serial.printf("[TELEGRAM] Failed  HTTP=%d  body=%s\n", code, response.c_str());
  return false;
}

/* ═══════════════════════════════════════════
   PUBLIC – NORMAL SEND  (respects cooldown)
   ═══════════════════════════════════════════ */

bool sendTelegramAlert(const String& message) {
  if (!initialized) telegramInit();
  if (!initialized) return false;

  /* First message ever: always send regardless of cooldown */
  if (neverSent) {
    Serial.println("[TELEGRAM] First message – bypassing cooldown");
    return doSend(message);
  }

  if ((millis() - lastTelegramTime) < TELEGRAM_COOLDOWN_MS) {
    Serial.printf("[TELEGRAM] Cooldown (%lus left) – skipped\n",
                  (TELEGRAM_COOLDOWN_MS - (millis() - lastTelegramTime)) / 1000);
    return false;
  }

  return doSend(message);
}

/* ═══════════════════════════════════════════
   PUBLIC – FORCED SEND  (bypasses cooldown)
   Use for critical one-time events:
     boot alert, fault latch, thermal trip, charging start/stop
   ═══════════════════════════════════════════ */

bool sendTelegramForced(const String& message) {
  if (!initialized) telegramInit();
  if (!initialized) return false;

  Serial.println("[TELEGRAM] Forced send – ignoring cooldown");
  return doSend(message);
}
