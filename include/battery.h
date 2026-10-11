#ifndef VOLTSHIFT_BATTERY_H
#define VOLTSHIFT_BATTERY_H
#include <windows.h>
typedef struct {
    int available;
    unsigned long error;
    int ac, present, percent, charging, low, critical, saver;
    int remaining_known;
    unsigned long remaining_seconds;
} BatteryInfo;
/* Unknown boolean/percentage values are -1. */
void battery_decode(const SYSTEM_POWER_STATUS *raw, BatteryInfo *info);
int battery_read(BatteryInfo *info);
#endif
