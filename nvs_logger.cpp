#include "nvs_logger.h"
#include <Preferences.h>

static Preferences prefs;

/* ================= Init ================= */

void storageInit() {
  /* Open and immediately close to verify the namespace is accessible.
     Each getter / setter opens its own transaction so there is no
     persistent handle that would conflict with soh.cpp using the
     same namespace via its own Preferences object. */
  prefs.begin("bms_nvs", false);
  prefs.end();
  Serial.println("[NVS] Storage initialized");
}

/* ================= Fault Count ================= */

void incrementFaultCount() {
  prefs.begin("bms_nvs", false);
  unsigned long c = prefs.getULong("faults", 0);
  prefs.putULong("faults", c + 1);
  prefs.end();
}

unsigned long getFaultCount() {
  prefs.begin("bms_nvs", true);
  unsigned long v = prefs.getULong("faults", 0);
  prefs.end();
  return v;
}

/* ================= Cycle Count ================= */

void incrementCycleCount() {
  prefs.begin("bms_nvs", false);
  unsigned long cycles = prefs.getULong("cycle_cnt", 0) + 1;
  prefs.putULong("cycle_cnt", cycles);
  prefs.end();
  Serial.printf("[NVS] Cycle count = %lu\n", cycles);
}

unsigned long getCycleCount() {
  prefs.begin("bms_nvs", true);
  unsigned long v = prefs.getULong("cycle_cnt", 0);
  prefs.end();
  return v;
}
