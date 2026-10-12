#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include "hardware.h"
#include "gpu.h"
#include "battery.h"
#include "power_plan.h"
#include "capabilities.h"
#include <string.h>
#include "control_win.h"

static const char *boolean_name(int value) {
    return value < 0 ? "Unknown" : value ? "Yes" : "No";
}
static int report_status(WinControl *control) {
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
    puts("=== VoltShift v0.5 ===");
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
    {
        ControlState saved;
        if (!control->backend.load(control, &saved) && saved.ready) {
            caps[11].state = CAP_SUPPORTED;
            caps[11].reason = "Saved profile was prepared with verified Windows writes";
        }
    }
    puts("Capability registry (this runtime snapshot):");
    for (i = 0; i < CAPABILITY_COUNT; ++i)
        printf("  %-28s %-16s %s\n", caps[i].name, capability_state_name(caps[i].state), caps[i].reason);
    gpu_free(&gpu);
    puts("Read-only mode. No settings changed.");
    return cpu_ok && gpu_ok && battery_ok && plan_ok ? 0 : 1;
}

static void usage(void) {
    puts("VoltShift v0.5 - Windows power profiles");
    puts("Usage: voltshift [save|status|battery|performance|auto|restore|help]");
    puts("       voltshift save --dry-run");
    puts("       voltshift battery --dry-run");
    puts("status: read-only diagnostics and session state (also the default)");
    puts("save/battery: battery-first profile; CPU cap 60%, boost off, efficiency and display/device savings");
    puts("performance: return to your original plan; no overclocking or forced high-performance plan");
    puts("auto: foreground AC/battery switching; Ctrl+C restores the original plan");
    puts("restore: restore original plan and remove VoltShift's private clone/session");
}
int main(int argc, char **argv) {
    WinControl control;
    DWORD result;
    const char *command = argc > 1 ? argv[1] : "status";
    if (!strcmp(command, "save")) command = "battery";
    int dry_run = argc == 3 && strcmp(argv[2], "--dry-run") == 0;
    if (!strcmp(command, "help") || !strcmp(command, "--help") || !strcmp(command, "-h")) {
        usage(); return argc == 2 ? 0 : 2;
    }
    if (argc > 3 || (argc == 3 && (!dry_run || strcmp(command, "battery"))) ||
        (strcmp(command, "status") && strcmp(command, "battery") && strcmp(command, "performance") &&
         strcmp(command, "auto") && strcmp(command, "restore"))) {
        usage(); return 2;
    }
    result = control_win_init(&control, NULL);
    if (result) { fprintf(stderr, "Cannot locate recovery storage (Windows error %lu).\n", result); return 1; }
    if (!strcmp(command, "status")) {
        int status = report_status(&control);
        control_win_status(&control);
        return status;
    }
    if (dry_run) return control_win_preview(&control) ? 1 : 0;
    result = control_win_lock(&control);
    if (result) {
        fprintf(stderr, "Cannot lock VoltShift session (Windows error %lu). Stop auto mode before another control command.\n", result);
        return 1;
    }
    if (!strcmp(command, "restore")) result = control_restore(&control.backend);
    else if (!strcmp(command, "auto")) result = control_win_auto(&control);
    else result = control_apply(&control.backend, !strcmp(command, "battery"));
    control_win_close(&control);
    if (result) {
        fprintf(stderr, "Command failed (Windows error %lu). Run 'voltshift restore' to recover any saved session.\n", result);
        return 1;
    }
    if (!strcmp(command, "battery")) {
        SYSTEM_POWER_STATUS raw;
        puts("Save profile active. Cooling protections are unchanged. Run 'voltshift restore' to undo.");
        if (GetSystemPowerStatus(&raw)) {
            printf("Windows Energy Saver actual status: %s\n", raw.SystemStatusFlag == 1 ? "On" : raw.SystemStatusFlag == 0 ? "Off" : "Unknown");
            if (raw.ACLineStatus == 0 && raw.SystemStatusFlag == 0)
                puts("Energy Saver threshold was requested; Windows has not reported Saver on. Do not assume it is active.");
        }
    }
    else if (!strcmp(command, "performance")) puts("Original performance plan active. Recovery session retained if present.");
    else if (!strcmp(command, "restore")) puts("Original plan restored; VoltShift session cleared (or no session existed).");
    return 0;
}
