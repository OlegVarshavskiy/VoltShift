#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include "display.h"

/* Treat the enumerated 59/60 Hz family alike; keep higher-rate restoration exact. */
int display_rate_matches(DWORD actual, DWORD requested) {
    return actual == requested ||
           (actual >= 59 && actual <= 60 && requested >= 59 && requested <= 60);
}
int display_candidate(const DEVMODEW *current, const DEVMODEW *candidate) {
    return candidate->dmPelsWidth == current->dmPelsWidth &&
           candidate->dmPelsHeight == current->dmPelsHeight &&
           candidate->dmBitsPerPel == current->dmBitsPerPel &&
           candidate->dmDisplayOrientation == current->dmDisplayOrientation &&
           candidate->dmDisplayFlags == current->dmDisplayFlags &&
           candidate->dmDisplayFrequency >= 59 && candidate->dmDisplayFrequency <= 60 &&
           candidate->dmDisplayFrequency < current->dmDisplayFrequency;
}
typedef struct { DISPLAYCONFIG_PATH_INFO *paths; DISPLAYCONFIG_MODE_INFO *modes; UINT32 count; } Topology;
static LONG topology_read(Topology *topology) {
    LONG result = ERROR_GEN_FAILURE;
    memset(topology, 0, sizeof(*topology));
    for (int attempt = 0; attempt < 3; ++attempt) {
        UINT32 paths, modes;
        result = GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &paths, &modes);
        if (result) return result;
        topology->paths = calloc(paths ? paths : 1, sizeof(*topology->paths));
        topology->modes = calloc(modes ? modes : 1, sizeof(*topology->modes));
        if (!topology->paths || !topology->modes) {
            free(topology->paths); free(topology->modes);
            topology->paths = NULL; topology->modes = NULL;
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        result = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &paths, topology->paths,
                                    &modes, topology->modes, NULL);
        if (!result) { topology->count = paths; return ERROR_SUCCESS; }
        free(topology->paths); free(topology->modes);
        topology->paths = NULL; topology->modes = NULL;
        if (result != ERROR_INSUFFICIENT_BUFFER) return result;
    }
    return result;
}
static int internal_panel(const DISPLAYCONFIG_PATH_INFO *p) {
    return p->targetInfo.outputTechnology == DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL ||
           p->targetInfo.outputTechnology == DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EMBEDDED ||
           p->targetInfo.outputTechnology == DISPLAYCONFIG_OUTPUT_TECHNOLOGY_LVDS;
}
static int source_shared(const Topology *t, UINT32 index) {
    const DISPLAYCONFIG_PATH_SOURCE_INFO *s = &t->paths[index].sourceInfo;
    for (UINT32 i = 0; i < t->count; ++i)
        if (i != index && t->paths[i].sourceInfo.id == s->id &&
            !memcmp(&t->paths[i].sourceInfo.adapterId, &s->adapterId, sizeof(s->adapterId))) return 1;
    return 0;
}
static LONG names(const DISPLAYCONFIG_PATH_INFO *p, WCHAR source[32], WCHAR monitor[128]) {
    DISPLAYCONFIG_SOURCE_DEVICE_NAME s = {0};
    DISPLAYCONFIG_TARGET_DEVICE_NAME m = {0};
    LONG result;
    s.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
    s.header.size = sizeof(s); s.header.adapterId = p->sourceInfo.adapterId; s.header.id = p->sourceInfo.id;
    m.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
    m.header.size = sizeof(m); m.header.adapterId = p->targetInfo.adapterId; m.header.id = p->targetInfo.id;
    result = DisplayConfigGetDeviceInfo(&s.header);
    if (!result) result = DisplayConfigGetDeviceInfo(&m.header);
    if (!result) {
        memcpy(source, s.viewGdiDeviceName, sizeof(s.viewGdiDeviceName)); source[31] = 0;
        memcpy(monitor, m.monitorDevicePath, sizeof(m.monitorDevicePath)); monitor[127] = 0;
        if (!source[0] || !monitor[0]) result = ERROR_NOT_SUPPORTED;
    }
    return result;
}
DWORD display_capture(DisplayState *state) {
    Topology topology;
    LONG result;
    memset(state, 0, sizeof(*state));
    result = topology_read(&topology);
    if (result) { printf("Internal display refresh: unavailable (%ld); unchanged.\n", result); return 0; }
    for (UINT32 i = 0; i < topology.count && state->count < DISPLAY_SNAPSHOT_LIMIT; ++i) {
        DEVMODEW current = {0}, candidate;
        DisplaySnapshot entry = {0};
        DWORD best = 0;
        if (!internal_panel(&topology.paths[i]) || source_shared(&topology, i)) continue;
        if (names(&topology.paths[i], entry.source, entry.monitor)) continue;
        current.dmSize = sizeof(current);
        if (!EnumDisplaySettingsW(entry.source, ENUM_CURRENT_SETTINGS, &current)) continue;
        if (current.dmDisplayFrequency <= 60) {
            printf("Internal display refresh: %lu Hz; already at or below 60 Hz.\n", current.dmDisplayFrequency);
            continue;
        }
        for (DWORD mode = 0; ; ++mode) {
            memset(&candidate, 0, sizeof(candidate)); candidate.dmSize = sizeof(candidate);
            if (!EnumDisplaySettingsW(entry.source, mode, &candidate)) break;
            if (display_candidate(&current, &candidate) && candidate.dmDisplayFrequency > best) best = candidate.dmDisplayFrequency;
        }
        if (!best) { puts("Internal display refresh: no matching 59/60 Hz mode; unchanged."); continue; }
        candidate = current; candidate.dmDriverExtra = 0;
        candidate.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFLAGS |
                             DM_DISPLAYORIENTATION | DM_DISPLAYFREQUENCY;
        candidate.dmDisplayFrequency = best;
        result = ChangeDisplaySettingsExW(entry.source, &candidate, NULL, CDS_TEST, NULL);
        if (result != DISP_CHANGE_SUCCESSFUL) {
            printf("Internal display refresh: test mode rejected (%ld); unchanged.\n", result); continue;
        }
        entry.width = current.dmPelsWidth; entry.height = current.dmPelsHeight;
        entry.bits = current.dmBitsPerPel; entry.orientation = current.dmDisplayOrientation;
        entry.flags = current.dmDisplayFlags; entry.original_hz = current.dmDisplayFrequency; entry.battery_hz = best;
        state->items[state->count++] = entry;
        printf("Internal display refresh: %lu -> %lu Hz, original recorded for restoration.\n", entry.original_hz, best);
    }
    free(topology.paths); free(topology.modes);
    return 0;
}
static int present_same(const Topology *t, const DisplaySnapshot *s) {
    for (UINT32 i = 0; i < t->count; ++i) {
        WCHAR source[32], monitor[128];
        if (!internal_panel(&t->paths[i]) || source_shared(t, i)) continue;
        if (!names(&t->paths[i], source, monitor) && !wcscmp(source, s->source) && !wcscmp(monitor, s->monitor)) return 1;
    }
    return 0;
}
DWORD display_apply(const DisplayState *state, int battery) {
    Topology topology;
    DWORD error = ERROR_SUCCESS;
    LONG result;
    if (!state->count) return 0;
    result = topology_read(&topology);
    if (result) return (DWORD)result;
    for (DWORD i = 0; i < state->count; ++i) {
        const DisplaySnapshot *s = &state->items[i];
        DEVMODEW current = {0}, updated;
        DWORD target = battery ? s->battery_hz : s->original_hz;
        if (!present_same(&topology, s)) { puts("Display identity/topology changed; leaving that display unchanged."); continue; }
        current.dmSize = sizeof(current);
        if (!EnumDisplaySettingsW(s->source, ENUM_CURRENT_SETTINGS, &current)) { error = ERROR_GEN_FAILURE; break; }
        if (current.dmPelsWidth != s->width || current.dmPelsHeight != s->height ||
            current.dmBitsPerPel != s->bits || current.dmDisplayOrientation != s->orientation ||
            current.dmDisplayFlags != s->flags ||
            (!display_rate_matches(current.dmDisplayFrequency, s->original_hz) &&
             !display_rate_matches(current.dmDisplayFrequency, s->battery_hz))) {
            puts("Display mode changed externally; preserving the newer selection."); continue;
        }
        if (display_rate_matches(current.dmDisplayFrequency, target)) continue;
        updated = current; updated.dmDriverExtra = 0;
        updated.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFLAGS |
                           DM_DISPLAYORIENTATION | DM_DISPLAYFREQUENCY;
        updated.dmDisplayFrequency = target;
        result = ChangeDisplaySettingsExW(s->source, &updated, NULL, CDS_TEST, NULL);
        if (result == DISP_CHANGE_SUCCESSFUL) result = ChangeDisplaySettingsExW(s->source, &updated, NULL, 0, NULL);
        if (result != DISP_CHANGE_SUCCESSFUL) {
            fprintf(stderr, "Display refresh change failed (display status %ld).\n", result);
            error = ERROR_GEN_FAILURE; break;
        }
        memset(&current, 0, sizeof(current)); current.dmSize = sizeof(current);
        if (!EnumDisplaySettingsW(s->source, ENUM_CURRENT_SETTINGS, &current) || !display_rate_matches(current.dmDisplayFrequency, target)) {
            error = ERROR_WRITE_FAULT; break;
        }
        printf("Internal display refresh: %lu Hz verified.\n", current.dmDisplayFrequency);
    }
    free(topology.paths); free(topology.modes);
    return error;
}
void display_report(void) {
    Topology topology;
    if (topology_read(&topology)) { puts("Display refresh: unavailable"); return; }
    for (UINT32 i = 0; i < topology.count; ++i) {
        WCHAR source[32], monitor[128];
        DEVMODEW mode = {0};
        if (!internal_panel(&topology.paths[i]) || names(&topology.paths[i], source, monitor)) continue;
        mode.dmSize = sizeof(mode);
        if (EnumDisplaySettingsW(source, ENUM_CURRENT_SETTINGS, &mode))
            printf("Internal display: %lux%lu at %lu Hz%s\n", mode.dmPelsWidth, mode.dmPelsHeight,
                   mode.dmDisplayFrequency, source_shared(&topology, i) ? " (mirrored; refresh control skipped)" : "");
    }
    free(topology.paths); free(topology.modes);
}
