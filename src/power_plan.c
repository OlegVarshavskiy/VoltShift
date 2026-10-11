#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <powrprof.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "power_plan.h"

int power_plan_read(PowerPlan *plan) {
    GUID *guid = NULL;
    DWORD size = 0, result;
    WCHAR *name;
    if (!plan) return 0;
    memset(plan, 0, sizeof(*plan));
    result = PowerGetActiveScheme(NULL, &guid);
    if (result != ERROR_SUCCESS) { plan->error = result; return 0; }
    snprintf(plan->guid, sizeof(plan->guid),
        "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        (unsigned long)guid->Data1, (unsigned)guid->Data2, (unsigned)guid->Data3,
        (unsigned)guid->Data4[0], (unsigned)guid->Data4[1],
        (unsigned)guid->Data4[2], (unsigned)guid->Data4[3],
        (unsigned)guid->Data4[4], (unsigned)guid->Data4[5],
        (unsigned)guid->Data4[6], (unsigned)guid->Data4[7]);
    plan->available = 1;
    result = PowerReadFriendlyName(NULL, guid, NULL, NULL, NULL, &size);
    if ((result == ERROR_SUCCESS || result == ERROR_MORE_DATA) && size > 0) {
        name = calloc(1, (size_t)size + sizeof(WCHAR));
        if (name) {
            result = PowerReadFriendlyName(NULL, guid, NULL, NULL, (BYTE *)name, &size);
            if (result == ERROR_SUCCESS) {
                plan->name_available = WideCharToMultiByte(CP_UTF8, 0, name, -1,
                    plan->name, sizeof(plan->name), NULL, NULL) != 0;
                if (!plan->name_available) result = GetLastError();
            }
            free(name);
        } else result = ERROR_NOT_ENOUGH_MEMORY;
    } else if (result == ERROR_SUCCESS) result = ERROR_INVALID_DATA;
    plan->name_error = result;
    LocalFree(guid);
    return 1;
}
