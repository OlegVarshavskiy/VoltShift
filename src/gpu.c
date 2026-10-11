#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <setupapi.h>
#include <stdlib.h>
#include <string.h>
#include "gpu.h"

/* Display setup class includes adapters without attached monitors.
 * PnP presence does not prove the device is enabled or currently rendering. */
static const GUID display_class = {0x4d36e968, 0xe325, 0x11ce,
    {0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18}};

int gpu_detect(GPUList *list) {
    HDEVINFO devices;
    DWORD index;
    if (!list) return 0;
    memset(list, 0, sizeof(*list));
    devices = SetupDiGetClassDevsW(&display_class, NULL, NULL, DIGCF_PRESENT);
    if (devices == INVALID_HANDLE_VALUE) {
        list->error = GetLastError();
        return 0;
    }
    for (index = 0; ; ++index) {
        SP_DEVINFO_DATA device = {0};
        GPUInfo item = {0};
        GPUInfo *grown;
        WCHAR text[512] = {0};
        DWORD type = 0;
        device.cbSize = sizeof(device);
        if (!SetupDiEnumDeviceInfo(devices, index, &device)) {
            DWORD error = GetLastError();
            list->complete = error == ERROR_NO_MORE_ITEMS;
            if (!list->complete) list->error = error;
            break;
        }
        if (SetupDiGetDeviceRegistryPropertyW(devices, &device, SPDRP_DEVICEDESC,
                &type, (BYTE *)text, sizeof(text), NULL) && type == REG_SZ) {
            text[511] = L'\0';
            if (!WideCharToMultiByte(CP_UTF8, 0, text, -1, item.name,
                                    sizeof(item.name), NULL, NULL))
                strcpy(item.name, "Name unavailable");
        } else strcpy(item.name, "Name unavailable");
        memset(text, 0, sizeof(text));
        if (SetupDiGetDeviceInstanceIdW(devices, &device, text, 512, NULL)) {
            if (!WideCharToMultiByte(CP_UTF8, 0, text, -1, item.instance_id,
                                    sizeof(item.instance_id), NULL, NULL))
                strcpy(item.instance_id, "Unavailable");
        } else strcpy(item.instance_id, "Unavailable");
        if (list->count >= (size_t)-1 / sizeof(*grown) - 1) {
            list->error = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        grown = realloc(list->items, (list->count + 1) * sizeof(*grown));
        if (!grown) { list->error = ERROR_NOT_ENOUGH_MEMORY; break; }
        list->items = grown;
        list->items[list->count++] = item;
    }
    SetupDiDestroyDeviceInfoList(devices);
    return list->complete;
}

void gpu_free(GPUList *list) {
    if (!list) return;
    free(list->items);
    memset(list, 0, sizeof(*list));
}
