#include "telegram.h"
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>

/* ================= Configuration ================= */

// Telegram Bot API URL base
static const char* TELEGRAM_API_BASE = "https://api.telegram.org/bot";

/* ================= State ================= */

static bool initialized = false;
static unsigned long lastTelegramTime = 0;

// Prevent spamming
#define TELEGRAM_COOLDOWN_MS 30000  // 30 seconds

/* ================= Public Functions ================= */

void telegramInit() {
  if (initialized) return;

  Serial.println("[TELEGRAM] Initializing Telegram module...");

  if (strlen(TELEGRAM_BOT_TOKEN) == 0 || strlen(TELEGRAM_CHAT_ID) == 0) {
    Serial.println("[TELEGRAM] ❌ Bot token or chat ID missing");
    return;
  }

  initialized = true;
  Serial.println("[TELEGRAM] ✓ Telegram ready");
}

bool sendTelegramAlert(const String& message) {
  if (!initialized) telegramInit();

  // Rate limit
  if (millis() - lastTelegramTime < TELEGRAM_COOLDOWN_MS) {
    Serial.println("[TELEGRAM] Cooldown active, message skipped");
    return false;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[TELEGRAM] WiFi not connected");
    return false;
  }

  HTTPClient http;
  http.setTimeout(8000);

  // Build URL
  String url = String(TELEGRAM_API_BASE) +
               TELEGRAM_BOT_TOKEN +
               "/sendMessage";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  // JSON payload
  String payload =
    "{"
    "\"chat_id\":\"" + String(TELEGRAM_CHAT_ID) + "\","
    "\"text\":\"" + message + "\","
    "\"parse_mode\":\"Markdown\""
    "}";

  int httpCode = http.POST(payload);

  if (httpCode >= 200 && httpCode < 300) {
    Serial.println("[TELEGRAM] ✓ Alert sent");
    lastTelegramTime = millis();
    http.end();
    return true;
  } else {
    Serial.print("[TELEGRAM] ❌ Failed, code: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }
}
