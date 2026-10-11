
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include "hardware.h"

int main(void) {
    SYSTEM_POWER_STATUS power;
    CPUInfo cpu;

    puts("=== VoltShift v0.2 ===");

    if (GetSystemPowerStatus(&power)) {
        printf("Power source: %s\n",
            power.ACLineStatus == 1 ? "AC" :
            power.ACLineStatus == 0 ? "Battery" :
            "Unknown");

        if (power.BatteryLifePercent != 255)
            printf("Battery level: %u%%\n",
                   (unsigned)power.BatteryLifePercent);
        else
            puts("Battery level: Unknown");
    } else {
        fprintf(stderr, "Battery diagnostics failed.\n");
    }

    if (hardware_get_cpu(&cpu)) {
        printf("CPU: %s\n", cpu.name);
        printf("Logical processors: %u\n",
               cpu.logical_processors);
    } else {
        puts("CPU identification unavailable.");
    }

    puts("Read-only mode. No settings changed.");
    return 0;
}

