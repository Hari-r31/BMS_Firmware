#include "nvs_logger.h"
#include <Preferences.h>

static Preferences prefs;

/* ================= Init ================= */

void storageInit() {
  prefs.begin("bms", false);
}

/* ================= Fault Count ================= */

void incrementFaultCount() {
  unsigned long c = prefs.getULong("faults", 0);
  prefs.putULong("faults", c + 1);
}

unsigned long getFaultCount() {
  return prefs.getULong("faults", 0);
}

/* ================= Cycle Count ================= */

void incrementCycleCount() {
  unsigned long cycles = prefs.getULong("cycle_cnt", 0);
  cycles++;
  prefs.putULong("cycle_cnt", cycles);

  Serial.print("[NVS] Cycle count = ");
  Serial.println(cycles);
}

unsigned long getCycleCount() {
  return prefs.getULong("cycle_cnt", 0);
}
