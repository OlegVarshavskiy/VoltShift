#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include "battery.h"

void battery_decode(const SYSTEM_POWER_STATUS *raw, BatteryInfo *info) {
    if (!info) return;
    memset(info, 0, sizeof(*info));
    info->ac = info->present = info->percent = info->charging = -1;
    info->low = info->critical = info->saver = -1;
    if (!raw) return;
    info->available = 1;
    if (raw->ACLineStatus <= 1) info->ac = raw->ACLineStatus;
    if (raw->SystemStatusFlag <= 1) info->saver = raw->SystemStatusFlag;
    /* 255 means unknown: test it before interpreting the bitmask. */
    if (raw->BatteryFlag != 255) {
        info->present = (raw->BatteryFlag & 128) ? 0 : 1;
        if (info->present) {
            info->charging = !!(raw->BatteryFlag & 8);
            info->low = !!(raw->BatteryFlag & 2);
            info->critical = !!(raw->BatteryFlag & 4);
        }
    }
    if (info->present != 0) {
        if (raw->BatteryLifePercent <= 100) info->percent = raw->BatteryLifePercent;
        if (info->ac == 0 && raw->BatteryLifeTime != (DWORD)-1) {
            info->remaining_known = 1;
            info->remaining_seconds = raw->BatteryLifeTime;
        }
    }
}

int battery_read(BatteryInfo *info) {
    SYSTEM_POWER_STATUS raw;
    if (!info) return 0;
    battery_decode(NULL, info);
    if (!GetSystemPowerStatus(&raw)) { info->error = GetLastError(); return 0; }
    battery_decode(&raw, info);
    return 1;
}
