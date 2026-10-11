#ifndef VOLTSHIFT_CONTROL_WIN_H
#define VOLTSHIFT_CONTROL_WIN_H
#include "controller.h"

typedef struct {
    WCHAR directory[32768];
    WCHAR state_path[32768];
    WCHAR temp_path[32768];
    HANDLE lock;
    ControlBackend backend;
} WinControl;
/* directory=NULL selects %LOCALAPPDATA%/VoltShift; overrides are for tests. */
DWORD control_win_init(WinControl *control, const WCHAR *directory);
DWORD control_win_lock(WinControl *control);
void control_win_close(WinControl *control);
DWORD control_win_preview(WinControl *control);
void control_win_status(WinControl *control);
DWORD control_win_auto(WinControl *control);
#endif
