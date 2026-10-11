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

static const GUID processor = {0x54533251,0x82be,0x4824,{0x96,0xc1,0x47,0xb6,0x0b,0x74,0x0d,0x00}};
static const GUID video = {0x7516b95f,0xf776,0x4464,{0x8c,0x53,0x06,0x16,0x7f,0x40,0xcc,0x99}};
static const GUID minimum = {0x893dee8e,0x2bef,0x41e0,{0x89,0xc6,0xb5,0x5d,0x09,0x29,0x96,0x4c}};
static const GUID maximum = {0xbc5038f7,0x23e0,0x4960,{0x96,0xda,0x33,0xab,0xaf,0x59,0x35,0xec}};
static const GUID boost = {0xbe337238,0x0d82,0x4146,{0xa9,0x60,0x4f,0x37,0x49,0xd4,0x70,0xc7}};
static const GUID display_idle = {0x3c0bc021,0xc8a8,0x4e07,{0xa9,0x73,0x6b,0x14,0xcb,0xcb,0x2b,0x7e}};
typedef struct { const char *name; const GUID *group, *setting; DWORD value; int required; } Policy;
static const Policy policies[] = {
    {"CPU minimum state (%)", &processor, &minimum, 5, 1},
    {"CPU maximum state (%)", &processor, &maximum, 70, 1},
    {"CPU boost mode (0=disabled)", &processor, &boost, 0, 0},
    {"Display idle timeout (seconds)", &video, &display_idle, 120, 0}
};

/* Fixed, versioned binary journal; checksum detects truncation/corruption. */
typedef struct { char magic[8]; ControlState state; uint32_t checksum; } Journal;
static uint32_t journal_checksum(const Journal *j) {
    const unsigned char *bytes = (const unsigned char *)j;
    uint32_t hash = UINT32_C(2166136261);
    size_t i;
    for (i = 0; i < offsetof(Journal, checksum); ++i) hash = (hash ^ bytes[i]) * UINT32_C(16777619);
    return hash;
}
static DWORD win_load(void *context, ControlState *state) {
    WinControl *c = context;
    Journal journal;
    DWORD size, count, result = ERROR_SUCCESS;
    GUID zero = {0};
    HANDLE file = CreateFileW(c->state_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return GetLastError();
    size = GetFileSize(file, NULL);
    if (size != sizeof(journal)) result = ERROR_INVALID_DATA;
    else if (!ReadFile(file, &journal, sizeof(journal), &count, NULL)) result = GetLastError();
    else if (count != sizeof(journal)) result = ERROR_INVALID_DATA;
    CloseHandle(file);
    if (result) return result;
    if (memcmp(journal.magic, "VOLTSH4", 8) || journal.checksum != journal_checksum(&journal) ||
        journal.state.ready > 1 || control_same_guid(&journal.state.original, &zero) ||
        control_same_guid(&journal.state.battery, &zero) ||
        control_same_guid(&journal.state.original, &journal.state.battery)) return ERROR_INVALID_DATA;
    *state = journal.state;
    return ERROR_SUCCESS;
}
static DWORD win_save(void *context, const ControlState *state) {
    WinControl *c = context;
    Journal journal;
    DWORD count, result = ERROR_SUCCESS;
    HANDLE file;
    memset(&journal, 0, sizeof(journal));
    memcpy(journal.magic, "VOLTSH4", 8);
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
static DWORD read_index(const GUID *scheme, const Policy *policy, int ac, DWORD *value) {
    return ac ? PowerReadACValueIndex(NULL, scheme, policy->group, policy->setting, value)
              : PowerReadDCValueIndex(NULL, scheme, policy->group, policy->setting, value);
}
static DWORD write_index(const GUID *scheme, const Policy *policy, int ac, DWORD value) {
    return ac ? PowerWriteACValueIndex(NULL, scheme, policy->group, policy->setting, value)
              : PowerWriteDCValueIndex(NULL, scheme, policy->group, policy->setting, value);
}
static DWORD win_configure(void *context, const GUID *scheme) {
    const WCHAR name[] = L"VoltShift Battery";
    size_t i;
    DWORD result;
    (void)context;
    result = PowerWriteFriendlyName(NULL, scheme, NULL, NULL, (UCHAR *)name, sizeof(name));
    if (result) return result;
    for (i = 0; i < sizeof(policies)/sizeof(policies[0]); ++i) {
        const Policy *p = &policies[i];
        DWORD old[2], actual;
        int ac;
        /* Probe both sources before writing this policy. */
        result = read_index(scheme, p, 0, &old[0]);
        if (!result) result = read_index(scheme, p, 1, &old[1]);
        if (result) {
            printf("  %s: unavailable (Windows error %lu)\n", p->name, result);
            if (p->required) return result;
            continue;
        }
        for (ac = 0; ac <= 1; ++ac) {
            result = write_index(scheme, p, ac, p->value);
            if (!result) result = read_index(scheme, p, ac, &actual);
            if (!result && actual != p->value) result = ERROR_WRITE_FAULT;
            if (result) {
                /* Fail the transaction rather than leave an unverified partial policy. */
                fprintf(stderr, "  %s write/readback failed (Windows error %lu).\n", p->name, result);
                return result;
            }
        }
        printf("  %s: %lu (AC and battery values verified)\n", p->name, p->value);
    }
    return ERROR_SUCCESS;
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
        win_guid, win_duplicate, win_configure, win_activate, win_delete};
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
    DWORD result = win_load(c, &state);
    DWORD required_error = ERROR_SUCCESS;
    size_t i;
    if (!result) active = state.original;
    else if (result == ERROR_FILE_NOT_FOUND || result == ERROR_PATH_NOT_FOUND) result = win_active(c, &active);
    if (result) { printf("Cannot inspect baseline (Windows error %lu).\n", result); return result; }
    puts("Battery profile preview: separate clone; original plan unchanged.");
    for (i = 0; i < sizeof(policies)/sizeof(policies[0]); ++i) {
        DWORD ac, dc, error = read_index(&active, &policies[i], 1, &ac);
        if (!error) error = read_index(&active, &policies[i], 0, &dc);
        if (error) {
            printf("  %s: unavailable (Windows error %lu)\n", policies[i].name, error);
            if (policies[i].required) required_error = error;
        }
        else printf("  %s: original AC=%lu DC=%lu -> battery profile %lu\n", policies[i].name, ac, dc, policies[i].value);
    }
    puts("Readability is confirmed; write permission is checked only during apply.");
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
