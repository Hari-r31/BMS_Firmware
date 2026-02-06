#pragma once

void gsmInit();
bool gsmSendSMS(const char* msg);
bool gsmIsReady();
