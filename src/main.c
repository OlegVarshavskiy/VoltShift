#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include "hardware.h"
#include "gpu.h"
#include "battery.h"
#include "power_plan.h"
#include "capabilities.h"

static const char *boolean_name(int value) {
    return value < 0 ? "Unknown" : value ? "Yes" : "No";
}
int main(void) {
    CPUInfo cpu;
    GPUList gpu;
    BatteryInfo battery;
    PowerPlan plan;
    Capability caps[CAPABILITY_COUNT];
    size_t i;
    int cpu_ok = hardware_get_cpu(&cpu);
    int gpu_ok = gpu_detect(&gpu);
    int battery_ok = battery_read(&battery);
    int plan_ok = power_plan_read(&plan);
    puts("=== VoltShift v0.3 ===");
    if (cpu_ok) printf("CPU: %s\n", cpu.name);
    else puts("CPU identification unavailable.");
    printf("Logical processors (current processor group): %u\n", cpu.logical_processors);
    printf("Present GPU/display adapters: %zu\n", gpu.count);
    for (i = 0; i < gpu.count; ++i)
        printf("  %zu. %s\n     PnP ID: %s\n", i + 1, gpu.items[i].name, gpu.items[i].instance_id);
    if (!gpu_ok) printf("GPU enumeration incomplete (Windows error %lu).\n", gpu.error);
    if (!battery_ok) printf("Battery diagnostics failed (Windows error %lu).\n", battery.error);
    else {
        printf("Power source: %s\n", battery.ac < 0 ? "Unknown" : battery.ac ? "AC" : "Battery");
        printf("Battery present: %s\n", boolean_name(battery.present));
        if (battery.present != 0) {
            if (battery.percent >= 0) printf("Battery level: %d%%\n", battery.percent);
            else puts("Battery level: Unknown");
            printf("Charging: %s; low: %s; critical: %s\n",
                boolean_name(battery.charging), boolean_name(battery.low), boolean_name(battery.critical));
            if (battery.remaining_known) printf("Estimated remaining time: %lu seconds\n", battery.remaining_seconds);
            else puts("Estimated remaining time: Unknown / unavailable on AC");
        }
        printf("Battery saver enabled: %s\n", boolean_name(battery.saver));
    }
    if (plan_ok) {
        printf("Active power scheme GUID: %s\n", plan.guid);
        if (plan.name_available) printf("Active power scheme name: %s\n", plan.name);
        else printf("Power scheme name unavailable (Windows error %lu).\n", plan.name_error);
    } else printf("Active power scheme unavailable (Windows error %lu).\n", plan.error);
    capabilities_collect(cpu_ok, &gpu, &battery, &plan, caps);
    puts("Capability registry (this runtime snapshot):");
    for (i = 0; i < CAPABILITY_COUNT; ++i)
        printf("  %-28s %-16s %s\n", caps[i].name, capability_state_name(caps[i].state), caps[i].reason);
    gpu_free(&gpu);
    puts("Read-only mode. No settings changed.");
    return cpu_ok && gpu_ok && battery_ok && plan_ok ? 0 : 1;
}
