#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <powrprof.h>
#include <objbase.h>
#include <stdio.h>
#include <wchar.h>
#include "control_win.h"

static DWORD check_display_rates(const ControlState *state, int battery) {
    for (DWORD i = 0; i < state->displays.count; ++i) {
        DEVMODEW mode = {0};
        const DisplaySnapshot *s = &state->displays.items[i];
        mode.dmSize = sizeof(mode);
        if (!EnumDisplaySettingsW(s->source, ENUM_CURRENT_SETTINGS, &mode)) return ERROR_GEN_FAILURE;
        if (mode.dmPelsWidth != s->width || mode.dmPelsHeight != s->height ||
            !display_rate_matches(mode.dmDisplayFrequency, battery ? s->battery_hz : s->original_hz)) return ERROR_WRITE_FAULT;
    }
    return 0;
}

/* Opt-in integration test: changes the active plan temporarily, always attempts rollback. */
int main(void) {
    WinControl control;
    WCHAR directory[32768], current[32000];
    GUID *original = NULL, *after = NULL;
    WCHAR original_text[40] = {0};
    ControlState state = {0};
    DWORD result, restored;
    if (!GetCurrentDirectoryW(32000, current)) return 1;
    swprintf(directory, 32768, L"%ls\\build\\live-test-%lu-%llu", current,
             GetCurrentProcessId(), (unsigned long long)GetTickCount64());
    result = control_win_init(&control, directory);
    if (!result) result = control_win_lock(&control);
    if (result) { fprintf(stderr,"Live test storage failed (%lu).\n",result); return 1; }
    result = PowerGetActiveScheme(NULL, &original);
    if (!result) {
        StringFromGUID2(original, original_text, 40);
        printf("Live test original plan: %ls\n", original_text);
        fflush(stdout);
    }
    if (!result) result = control_apply(&control.backend, 1);
    if (!result) {
        result = control.backend.load(&control, &state);
        if (!result) result = PowerGetActiveScheme(NULL, &after);
        if (!result && !control_same_guid(after, &state.battery)) result = ERROR_WRITE_FAULT;
        if (after) { LocalFree(after); after = NULL; }
        if (!result) result = check_display_rates(&state, 1);
    }
    if (!result) result = control_apply(&control.backend, 1); /* idempotent */
    if (!result) result = control_apply(&control.backend, 0);
    if (!result) result = check_display_rates(&state, 0);
    if (!result) result = PowerGetActiveScheme(NULL, &after);
    if (!result && !control_same_guid(original, after)) result = ERROR_WRITE_FAULT;
    if (after) { LocalFree(after); after = NULL; }
    restored = control_restore(&control.backend);
    if (!restored && original) {
        restored = PowerGetActiveScheme(NULL, &after);
        if (!restored && !control_same_guid(original, after)) restored = ERROR_WRITE_FAULT;
    }
    if (!restored) restored = check_display_rates(&state, 0);
    if (original) LocalFree(original);
    if (after) LocalFree(after);
    control_win_close(&control);
    if (!restored) RemoveDirectoryW(directory);
    if (result || restored) {
        fprintf(stderr,"Live test failed: operation=%lu restore=%lu. Recovery directory: %ls\n",result,restored,directory);
        if (restored && original_text[0]) fprintf(stderr,"Manual restoration: powercfg /setactive %ls\n", original_text);
        return 1;
    }
    puts("PASS: live battery apply, activation readback, repeated apply, performance, display-rate readback/restoration when supported, and private clone cleanup.");
    return 0;
}
