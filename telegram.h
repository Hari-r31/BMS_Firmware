#pragma once

#include <Arduino.h>

/**
 * Initialize Telegram module
 * Call once in setup()
 */
void telegramInit();

/**
 * Send Telegram alert message
 * @param message Message text
 * @return true if sent successfully
 */
bool sendTelegramAlert(const String& message);
