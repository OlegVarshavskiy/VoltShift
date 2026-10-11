#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include "hardware.h"

int hardware_get_cpu(CPUInfo *info) {
    if (info == NULL)
        return 0;

    memset(info, 0, sizeof(*info));

    SYSTEM_INFO system;
    GetNativeSystemInfo(&system);

    info->logical_processors =
        (unsigned int)system.dwNumberOfProcessors;

    HKEY key;
    LONG result = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        0,
        KEY_READ,
        &key
    );

    if (result != ERROR_SUCCESS)
        return 0;

    DWORD type = 0;
    DWORD size = sizeof(info->name);

    result = RegQueryValueExA(
        key,
        "ProcessorNameString",
        NULL,
        &type,
        (LPBYTE)info->name,
        &size
    );

    RegCloseKey(key);

    if (result != ERROR_SUCCESS || type != REG_SZ)
        return 0;

    info->name[CPU_NAME_LENGTH - 1] = '\0';
    return 1;
}