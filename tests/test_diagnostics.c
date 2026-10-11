#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include "battery.h"
#include "capabilities.h"

int main(void) {
    SYSTEM_POWER_STATUS raw = {0};
    BatteryInfo battery;
    GPUList gpu = {0};
    PowerPlan plan = {0};
    Capability caps[CAPABILITY_COUNT];
    unsigned flag, percent;
    /* Exhaustively check all byte flag and percentage values. */
    for (flag = 0; flag <= 255; ++flag) {
        for (percent = 0; percent <= 255; ++percent) {
            raw.ACLineStatus = 0;
            raw.BatteryFlag = (BYTE)flag;
            raw.BatteryLifePercent = (BYTE)percent;
            raw.BatteryLifeTime = (DWORD)-1;
            battery_decode(&raw, &battery);
            assert(battery.available);
            assert(!battery.remaining_known);
            if (flag == 255) {
                assert(battery.present == -1 && battery.charging == -1);
                assert(battery.low == -1 && battery.critical == -1);
            } else if (flag & 128) {
                assert(battery.present == 0 && battery.percent == -1);
                assert(battery.charging == -1);
            } else {
                assert(battery.present == 1);
                assert(battery.charging == !!(flag & 8));
                assert(battery.low == !!(flag & 2));
                assert(battery.critical == !!(flag & 4));
            }
            if (battery.present != 0)
                assert(battery.percent == (percent <= 100 ? (int)percent : -1));
        }
    }
    raw.BatteryFlag = 0; raw.BatteryLifePercent = 0;
    raw.ACLineStatus = 0; raw.BatteryLifeTime = 0;
    battery_decode(&raw, &battery);
    assert(battery.percent == 0 && battery.remaining_known && battery.remaining_seconds == 0);
    raw.BatteryLifeTime = 3600;
    battery_decode(&raw, &battery);
    assert(battery.remaining_known && battery.remaining_seconds == 3600);
    raw.ACLineStatus = 1;
    battery_decode(&raw, &battery);
    assert(!battery.remaining_known);
    capabilities_collect(1, &gpu, &battery, &plan, caps);
    assert(caps[6].state == CAP_UNAVAILABLE);
    raw.ACLineStatus = 255; raw.BatteryFlag = 255;
    raw.BatteryLifePercent = 255; raw.SystemStatusFlag = 255;
    battery_decode(&raw, &battery);
    assert(battery.ac == -1 && battery.present == -1 && battery.percent == -1);
    assert(battery.saver == -1 && !battery.remaining_known);
    capabilities_collect(0, &gpu, &battery, &plan, caps);
    assert(caps[0].state == CAP_ERROR && caps[1].state == CAP_ERROR);
    assert(caps[2].state == CAP_UNKNOWN && caps[3].state == CAP_UNKNOWN);
    assert(caps[4].state == CAP_UNKNOWN && caps[5].state == CAP_UNKNOWN);
    assert(caps[7].state == CAP_UNKNOWN && caps[8].state == CAP_ERROR);
    assert(caps[10].state == CAP_NOT_IMPLEMENTED && caps[11].state == CAP_NOT_IMPLEMENTED);
    raw.BatteryFlag = 128;
    battery_decode(&raw, &battery);
    capabilities_collect(1, &gpu, &battery, &plan, caps);
    assert(caps[3].state == CAP_SUPPORTED && caps[4].state == CAP_UNAVAILABLE);
    assert(caps[5].state == CAP_UNAVAILABLE && caps[6].state == CAP_UNAVAILABLE);
    battery_decode(NULL, &battery);
    capabilities_collect(0, &gpu, &battery, &plan, caps);
    assert(caps[2].state == CAP_ERROR && caps[4].state == CAP_ERROR);
    raw.ACLineStatus = 0; raw.BatteryFlag = 8; raw.BatteryLifePercent = 50;
    raw.SystemStatusFlag = 1; raw.BatteryLifeTime = 600;
    battery_decode(&raw, &battery);
    gpu.complete = 1; /* A successful empty list is not an API failure. */
    plan.available = 1; plan.name_available = 0;
    capabilities_collect(1, &gpu, &battery, &plan, caps);
    assert(caps[1].state == CAP_SUPPORTED && caps[4].state == CAP_SUPPORTED);
    assert(caps[5].state == CAP_SUPPORTED && caps[6].state == CAP_SUPPORTED);
    assert(caps[8].state == CAP_SUPPORTED && caps[9].state == CAP_ERROR);
    plan.name_available = 1;
    capabilities_collect(1, &gpu, &battery, &plan, caps);
    assert(caps[9].state == CAP_SUPPORTED);
    assert(!battery_read(NULL) && !gpu_detect(NULL) && !power_plan_read(NULL));
    gpu_free(NULL); battery_decode(NULL, NULL);
    puts("PASS: 65536 flag/percentage combinations, time/AC/absence/unknown boundaries, capability errors and null inputs.");
    return 0;
}
