#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <wchar.h>
#include "control_win.h"

int main(void) {
    WinControl first, second;
    WCHAR directory[32768], current[32000];
    ControlState state = {0}, loaded;
    HANDLE file;
    DWORD count;
    assert(GetCurrentDirectoryW(32000, current));
    swprintf(directory, 32768, L"%ls\\build\\storage-test-%lu", current, GetCurrentProcessId());
    assert(!control_win_init(&first, directory));
    assert(!control_win_init(&second, directory));
    assert(!control_win_lock(&first));
    assert(control_win_lock(&second) == ERROR_SHARING_VIOLATION);
    assert(first.backend.load(&first, &loaded) == ERROR_FILE_NOT_FOUND);
    state.original.Data1 = 1; state.battery.Data1 = 2;
    assert(!first.backend.save(&first, &state));
    assert(!first.backend.load(&first, &loaded) && !loaded.ready);
    state.ready = 1;
    assert(!first.backend.save(&first, &state));
    assert(!first.backend.load(&first, &loaded) && loaded.ready);
    assert(control_same_guid(&state.original, &loaded.original));
    assert(control_same_guid(&state.battery, &loaded.battery));
    assert(GetFileAttributesW(first.temp_path) == INVALID_FILE_ATTRIBUTES);
    /* Truncated journal must fail closed before any Windows power operation. */
    file = CreateFileW(first.state_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    assert(file != INVALID_HANDLE_VALUE);
    assert(WriteFile(file, "bad", 3, &count, NULL));
    CloseHandle(file);
    assert(first.backend.load(&first, &loaded) == ERROR_INVALID_DATA);
    assert(control_apply(&first.backend, 1) == ERROR_INVALID_DATA);
    assert(control_restore(&first.backend) == ERROR_INVALID_DATA);
    assert(!first.backend.save(&first, &state));
    /* Same length, modified byte: checksum must reject it too. */
    file = CreateFileW(first.state_path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    assert(file != INVALID_HANDLE_VALUE);
    assert(SetFilePointer(file, 8, NULL, FILE_BEGIN) == 8);
    assert(WriteFile(file, "x", 1, &count, NULL));
    CloseHandle(file);
    assert(first.backend.load(&first, &loaded) == ERROR_INVALID_DATA);
    assert(!first.backend.clear(&first));
    control_win_close(&first);
    assert(!control_win_lock(&second));
    control_win_close(&second);
    assert(RemoveDirectoryW(directory));
    puts("PASS: atomic recovery journal, round-trip, exclusive lock, truncated/corrupt state rejection, lock release.");
    return 0;
}
