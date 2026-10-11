#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

static const char *power_source(BYTE status) {
    switch (status) {
        case 0: return "Battery";
        case 1: return "AC power";
        default: return "Unknown";
    }
}

int main(void) {
    SYSTEM_POWER_STATUS power;
    SYSTEM_INFO system;
    OSVERSIONINFOEXW unused; /* no OS version assumptions */
    (void)unused;

    if (!GetSystemPowerStatus(&power)) {
        fprintf(stderr, "GetSystemPowerStatus failed (error %lu)\n", GetLastError());
        return 1;
    }
    GetNativeSystemInfo(&system);

    puts("VoltShift v0.1 - read-only diagnostics");
    printf("Power source: %s\n", power_source(power.ACLineStatus));
    if (power.BatteryLifePercent == 255)
        puts("Battery level: Unknown");
    else
        printf("Battery level: %u%%\n", (unsigned)power.BatteryLifePercent);
    printf("Logical processors: %lu\n", system.dwNumberOfProcessors);
    printf("Processor architecture code: %u\n", (unsigned)system.wProcessorArchitecture);
    puts("No system settings were changed.");
    return 0;
}
