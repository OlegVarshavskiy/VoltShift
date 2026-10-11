#include <string.h>
#include "controller.h"

int control_same_guid(const GUID *a, const GUID *b) {
    return memcmp(a, b, sizeof(*a)) == 0;
}
int control_auto_target(int ac) { return ac == 0 || ac == 1 ? ac : -1; }
DWORD control_auto_step(ControlBackend *b, int ac, int *previous) {
    int target = control_auto_target(ac);
    DWORD result;
    if (target == -1 || target == *previous) return ERROR_SUCCESS;
    result = control_apply(b, target == 0);
    if (!result) *previous = target;
    return result;
}

static DWORD activate_checked(ControlBackend *b, const GUID *target) {
    GUID current;
    DWORD result = b->active(b->context, &current);
    if (result) return result;
    if (control_same_guid(&current, target)) return ERROR_SUCCESS;
    result = b->activate(b->context, target);
    if (result) return result;
    result = b->active(b->context, &current);
    if (result) return result;
    return control_same_guid(&current, target) ? ERROR_SUCCESS : ERROR_WRITE_FAULT;
}

DWORD control_restore(ControlBackend *b) {
    ControlState state;
    DWORD result = b->load(b->context, &state);
    if (result == ERROR_FILE_NOT_FOUND) return ERROR_SUCCESS;
    if (result) return result; /* Never overwrite an unreadable recovery file. */
    result = activate_checked(b, &state.original);
    if (result) return result; /* Keep recovery state until restoration succeeds. */
    result = b->delete_scheme(b->context, &state.battery);
    if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) return result;
    return b->clear(b->context);
}

DWORD control_apply(ControlBackend *b, int battery) {
    ControlState state;
    DWORD result = b->load(b->context, &state);
    if (result == ERROR_FILE_NOT_FOUND) {
        if (!battery) return ERROR_SUCCESS; /* Normal performance already active. */
        memset(&state, 0, sizeof(state));
        result = b->active(b->context, &state.original);
        if (result) return result;
        result = b->new_guid(b->context, &state.battery);
        if (result) return result;
        if (control_same_guid(&state.original, &state.battery)) return ERROR_INVALID_DATA;
        /* Journal both GUIDs before creating or changing a scheme. */
        result = b->save(b->context, &state);
        if (result) return result;
        result = b->duplicate(b->context, &state.original, &state.battery);
        if (result) {
            /* A failed duplicate is not owned: do not delete an existing scheme. */
            DWORD cleanup = b->clear(b->context);
            return cleanup ? cleanup : result;
        }
        result = b->configure(b->context, &state.battery);
        if (!result) {
            state.ready = 1;
            result = b->save(b->context, &state);
        }
        if (result) {
            DWORD cleanup = control_restore(b);
            return cleanup ? cleanup : result;
        }
    } else if (result) return result;
    if (!state.ready) return ERROR_RECOVERY_FAILURE;
    result = activate_checked(b, battery ? &state.battery : &state.original);
    if (result) {
        DWORD cleanup = control_restore(b);
        return cleanup ? cleanup : result;
    }
    return ERROR_SUCCESS;
}
