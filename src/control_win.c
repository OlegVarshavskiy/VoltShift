#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <powrprof.h>
#include <objbase.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <wchar.h>
#include "control_win.h"
#include "policy.h"

/* v0.5 journals include display state; v0.4 recovery files remain readable. */
typedef struct { char magic[8]; ControlState state; uint32_t checksum; } Journal;
typedef struct { GUID original, battery; DWORD ready; } LegacyState;
typedef struct { char magic[8]; LegacyState state; uint32_t checksum; } LegacyJournal;
static uint32_t bytes_checksum(const void *data, size_t size) {
    const unsigned char *bytes = data;
    uint32_t hash = UINT32_C(2166136261);
    for (size_t i = 0; i < size; ++i) hash = (hash ^ bytes[i]) * UINT32_C(16777619);
    return hash;
}
static uint32_t journal_checksum(const Journal *j) { return bytes_checksum(j, offsetof(Journal, checksum)); }
static DWORD win_load(void *context, ControlState *state) {
    WinControl *c = context;
    DWORD size, count, result = ERROR_SUCCESS;
    GUID zero = {0};
    HANDLE file = CreateFileW(c->state_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return GetLastError();
    size = GetFileSize(file, NULL);
    memset(state, 0, sizeof(*state));
    if (size == sizeof(LegacyJournal)) {
        LegacyJournal legacy;
        if (!ReadFile(file, &legacy, sizeof(legacy), &count, NULL)) result = GetLastError();
        else if (count != sizeof(legacy) || memcmp(legacy.magic,"VOLTSH4",8) ||
                 legacy.checksum != bytes_checksum(&legacy,offsetof(LegacyJournal,checksum))) result = ERROR_INVALID_DATA;
        else {
            state->original = legacy.state.original; state->battery = legacy.state.battery;
            state->ready = legacy.state.ready; state->revision = 4;
        }
    } else if (size == sizeof(Journal)) {
        Journal journal;
        if (!ReadFile(file, &journal, sizeof(journal), &count, NULL)) result = GetLastError();
        else if (count != sizeof(journal) || memcmp(journal.magic,"VOLTSH5",8) ||
                 journal.checksum != journal_checksum(&journal)) result = ERROR_INVALID_DATA;
        else *state = journal.state;
    } else result = ERROR_INVALID_DATA;
    CloseHandle(file);
    if (result) return result;
    if (state->ready > 1 || state->revision < 4 || state->revision > 5 ||
        state->displays.count > DISPLAY_SNAPSHOT_LIMIT ||
        control_same_guid(&state->original, &zero) || control_same_guid(&state->battery, &zero) ||
        control_same_guid(&state->original, &state->battery)) return ERROR_INVALID_DATA;
    for (DWORD i = 0; i < state->displays.count; ++i) {
        DisplaySnapshot *s = &state->displays.items[i];
        if (!s->source[0] || s->source[31] || !s->monitor[0] || s->monitor[127] ||
            s->original_hz <= 60 || s->battery_hz < 59 || s->battery_hz > 60 || !s->width || !s->height)
            return ERROR_INVALID_DATA;
    }
    return 0;
}
static DWORD win_save(void *context, const ControlState *state) {
    WinControl *c = context;
    Journal journal;
    DWORD count, result = ERROR_SUCCESS;
    HANDLE file;
    memset(&journal, 0, sizeof(journal));
    memcpy(journal.magic, "VOLTSH5", 8);
    journal.state = *state;
    journal.checksum = journal_checksum(&journal);
    file = CreateFileW(c->temp_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
    if (file == INVALID_HANDLE_VALUE) return GetLastError();
    if (!WriteFile(file, &journal, sizeof(journal), &count, NULL)) result = GetLastError();
    else if (count != sizeof(journal)) result = ERROR_WRITE_FAULT;
    if (!result && !FlushFileBuffers(file)) result = GetLastError();
    CloseHandle(file);
    if (!result && !MoveFileExW(c->temp_path, c->state_path,
                              MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) result = GetLastError();
    if (result) DeleteFileW(c->temp_path);
    return result;
}
static DWORD win_clear(void *context) {
    WinControl *c = context;
    if (DeleteFileW(c->state_path)) return ERROR_SUCCESS;
    return GetLastError();
}
static DWORD win_active(void *context, GUID *guid) {
    GUID *allocated = NULL;
    DWORD result;
    (void)context;
    result = PowerGetActiveScheme(NULL, &allocated);
    if (!result) { *guid = *allocated; LocalFree(allocated); }
    return result;
}
static DWORD win_guid(void *context, GUID *guid) {
    (void)context;
    return SUCCEEDED(CoCreateGuid(guid)) ? ERROR_SUCCESS : ERROR_GEN_FAILURE;
}
static DWORD win_duplicate(void *context, const GUID *source, const GUID *dest) {
    GUID storage = *dest;
    GUID *requested = &storage;
    DWORD result;
    (void)context;
    result = PowerDuplicateScheme(NULL, source, &requested);
    if (result) fprintf(stderr, "Cannot create the private battery plan (Windows error %lu). Original plan unchanged.\n", result);
    return result;
}
static DWORD read_index(void *context, const GUID *scheme, const Policy *policy, int ac, DWORD *value) {
    (void)context;
    return ac ? PowerReadACValueIndex(NULL, scheme, policy->group, policy->setting, value)
              : PowerReadDCValueIndex(NULL, scheme, policy->group, policy->setting, value);
}
static DWORD write_index(void *context, const GUID *scheme, const Policy *policy, int ac, DWORD value) {
    (void)context;
    return ac ? PowerWriteACValueIndex(NULL, scheme, policy->group, policy->setting, value)
              : PowerWriteDCValueIndex(NULL, scheme, policy->group, policy->setting, value);
}
static DWORD win_configure(void *context, const GUID *scheme) {
    const WCHAR name[] = L"VoltShift Battery";
    DWORD result = PowerWriteFriendlyName(NULL, scheme, NULL, NULL, (UCHAR *)name, sizeof(name));
    PolicyBackend backend = {context, read_index, write_index, 1};
    return result ? result : policy_configure(scheme, &backend);
}
static DWORD win_capture(void *context, ControlState *state) {
    (void)context;
    return display_capture(&state->displays);
}
static DWORD win_effects(void *context, const ControlState *state, int battery) {
    (void)context;
    return display_apply(&state->displays, battery);
}
static DWORD win_activate(void *context, const GUID *guid) {
    DWORD result;
    (void)context;
    result = PowerSetActiveScheme(NULL, guid);
    if (result) fprintf(stderr, "Cannot activate the requested plan (Windows error %lu).\n", result);
    return result;
}
static DWORD win_delete(void *context, const GUID *guid) {
    (void)context;
    return PowerDeleteScheme(NULL, guid);
}

DWORD control_win_init(WinControl *c, const WCHAR *directory) {
    DWORD count;
    memset(c, 0, sizeof(*c));
    c->lock = INVALID_HANDLE_VALUE;
    if (directory) {
        if (wcslen(directory) > 32000) return ERROR_FILENAME_EXCED_RANGE;
        wcscpy(c->directory, directory);
    } else {
        count = GetEnvironmentVariableW(L"LOCALAPPDATA", c->directory, 32000);
        if (!count || count >= 32000) return ERROR_PATH_NOT_FOUND;
        wcscat(c->directory, L"\\VoltShift");
    }
    swprintf(c->state_path, 32768, L"%ls\\session-v1.bin", c->directory);
    swprintf(c->temp_path, 32768, L"%ls\\session-v1.tmp", c->directory);
    c->backend = (ControlBackend){c, win_load, win_save, win_clear, win_active,
        win_guid, win_duplicate, win_configure, win_activate, win_delete, win_capture, win_effects};
    return ERROR_SUCCESS;
}
DWORD control_win_lock(WinControl *c) {
    WCHAR lock_path[32768];
    if (!CreateDirectoryW(c->directory, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        return GetLastError();
    swprintf(lock_path, 32768, L"%ls\\session.lock", c->directory);
    c->lock = CreateFileW(lock_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    return c->lock == INVALID_HANDLE_VALUE ? GetLastError() : ERROR_SUCCESS;
}
void control_win_close(WinControl *c) {
    if (c->lock != INVALID_HANDLE_VALUE) CloseHandle(c->lock);
    c->lock = INVALID_HANDLE_VALUE;
}
DWORD control_win_preview(WinControl *c) {
    GUID active;
    ControlState state;
    DisplayState displays;
    DWORD result = win_load(c, &state), required_error = ERROR_SUCCESS;
    size_t count;
    const Policy *policies = policy_list(&count);
    if (!result) active = state.original;
    else if (result == ERROR_FILE_NOT_FOUND || result == ERROR_PATH_NOT_FOUND) result = win_active(c, &active);
    if (result) { printf("Cannot inspect baseline (%lu).\n", result); return result; }
    puts("Save profile preview: clone only; original plan and cooling policy unchanged.");
    for (size_t i = 0; i < count; ++i) {
        DWORD ac = 0, dc, error = read_index(c, &active, &policies[i], 0, &dc);
        if (!error && !policies[i].dc_only) error = read_index(c, &active, &policies[i], 1, &ac);
        if (error) {
            printf("  %s: unavailable (%lu); %s.\n", policies[i].name, error, policies[i].required ? "required" : "skipped");
            if (policies[i].required) required_error = error;
        } else if (policies[i].dc_only)
            printf("  %s: DC %lu -> %lu (AC inherited).\n", policies[i].name, dc, policy_target(&policies[i],dc));
        else printf("  %s: AC %lu -> %lu, DC %lu -> %lu.\n", policies[i].name, ac, policy_target(&policies[i],ac),dc,policy_target(&policies[i],dc));
    }
    display_capture(&displays);
    puts("This is a read-only preview. Actual writes and runtime state are verified during apply/status.");
    return required_error;
}
void control_win_status(WinControl *c) {
    ControlState state;
    DWORD error = win_load(c, &state);
    GUID active;
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) puts("VoltShift session: none");
    else if (error) printf("VoltShift recovery state unreadable (Windows error %lu); no changes made.\n", error);
    else if (!state.ready) puts("VoltShift session: incomplete; run restore before applying again.");
    else if (win_active(c, &active)) puts("VoltShift session saved; active profile unknown.");
    else printf("VoltShift session: %s\n", control_same_guid(&active, &state.battery) ? "battery profile" :
                control_same_guid(&active, &state.original) ? "original/performance profile" : "another plan selected externally");
    if (!error) {
        WCHAR guid[40];
        if (StringFromGUID2(&state.original, guid, 40)) printf("Saved original scheme GUID: %ls\n", guid);
        if (StringFromGUID2(&state.battery, guid, 40)) printf("Private battery scheme GUID: %ls\n", guid);
        puts("Recovery journal: %LOCALAPPDATA%\\VoltShift\\session-v1.bin");
    }
    if (!win_active(c, &active)) {
        size_t count;
        const Policy *policies = policy_list(&count);
        puts("Current active plan policy values (requests; not physical sensor readings):");
        for (size_t i = 0; i < count; ++i) {
            DWORD value, read_error = read_index(c, &active, &policies[i], 0, &value);
            if (!read_error) printf("  %s: DC=%lu\n",policies[i].name,value);
            else printf("  %s: unavailable (%lu)\n",policies[i].name,read_error);
        }
    }
    display_report();
    puts("Cooling/thermal protection: unchanged by VoltShift.");
}

static volatile LONG stop_requested;
static BOOL WINAPI stop_handler(DWORD event) {
    if (event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT) {
        InterlockedExchange(&stop_requested, 1);
        return TRUE;
    }
    return FALSE; /* Forced closure relies on the persistent recovery journal. */
}
DWORD control_win_auto(WinControl *c) {
    DWORD result = ERROR_SUCCESS, restored;
    int previous = -1;
    stop_requested = 0;
    if (!SetConsoleCtrlHandler(stop_handler, TRUE)) return GetLastError();
    puts("Auto mode running. AC -> original performance; battery -> VoltShift Battery.");
    puts("Press Ctrl+C to stop and restore. Keep this terminal open.");
    while (!InterlockedCompareExchange(&stop_requested, 0, 0)) {
        SYSTEM_POWER_STATUS raw;
        int target;
        if (!GetSystemPowerStatus(&raw)) { result = GetLastError(); break; }
        target = control_auto_target(raw.ACLineStatus);
        if (target != -1 && target != previous) {
            result = control_auto_step(&c->backend, raw.ACLineStatus, &previous);
            if (result) break;
            puts(target == 0 ? "Auto: battery profile selected." : "Auto: original performance selected.");
            fflush(stdout);
        }
        /* Short interruptible polling without waking a busy diagnostics loop. */
        for (int i = 0; i < 20 && !InterlockedCompareExchange(&stop_requested, 0, 0); ++i) Sleep(250);
    }
    restored = control_restore(&c->backend);
    SetConsoleCtrlHandler(stop_handler, FALSE);
    if (restored) { fprintf(stderr, "Auto restore failed (%lu). Run voltshift restore.\n", restored); return restored; }
    puts("Auto stopped; original plan restored.");
    return result;
}
