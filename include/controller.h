#ifndef VOLTSHIFT_CONTROLLER_H
#define VOLTSHIFT_CONTROLLER_H
#include <windows.h>

typedef struct {
    GUID original;
    GUID battery;
    DWORD ready;
} ControlState;

/* The transaction logic is independent of WinAPI/file operations for fault tests. */
typedef struct {
    void *context;
    DWORD (*load)(void *, ControlState *);
    DWORD (*save)(void *, const ControlState *);
    DWORD (*clear)(void *);
    DWORD (*active)(void *, GUID *);
    DWORD (*new_guid)(void *, GUID *);
    DWORD (*duplicate)(void *, const GUID *, const GUID *);
    DWORD (*configure)(void *, const GUID *);
    DWORD (*activate)(void *, const GUID *);
    DWORD (*delete_scheme)(void *, const GUID *);
} ControlBackend;

int control_same_guid(const GUID *a, const GUID *b);
int control_auto_target(int ac); /* 0=battery, 1=performance, -1=leave unchanged */
DWORD control_auto_step(ControlBackend *backend, int ac, int *previous);
DWORD control_apply(ControlBackend *backend, int battery);
DWORD control_restore(ControlBackend *backend);
#endif
