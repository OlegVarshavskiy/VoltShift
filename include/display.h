#ifndef VOLTSHIFT_DISPLAY_H
#define VOLTSHIFT_DISPLAY_H
#include <windows.h>
#define DISPLAY_SNAPSHOT_LIMIT 4
typedef struct {
    WCHAR source[32];
    WCHAR monitor[128];
    DWORD width, height, bits, orientation, flags;
    DWORD original_hz, battery_hz;
} DisplaySnapshot;
typedef struct { DWORD count; DisplaySnapshot items[DISPLAY_SNAPSHOT_LIMIT]; } DisplayState;
int display_rate_matches(DWORD actual, DWORD requested);
int display_candidate(const DEVMODEW *current, const DEVMODEW *candidate);
DWORD display_capture(DisplayState *state);
DWORD display_apply(const DisplayState *state, int battery);
void display_report(void);
#endif
