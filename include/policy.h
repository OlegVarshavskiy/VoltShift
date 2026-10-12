#ifndef VOLTSHIFT_POLICY_H
#define VOLTSHIFT_POLICY_H
#include <windows.h>
#include <stddef.h>
typedef struct {
    const char *name;
    const GUID *group, *setting;
    DWORD value;
    int required, lower_only, dc_only;
} Policy;
const Policy *policy_list(size_t *count);
DWORD policy_target(const Policy *policy, DWORD original);
typedef struct {
    void *context;
    DWORD (*read)(void *, const GUID *, const Policy *, int, DWORD *);
    DWORD (*write)(void *, const GUID *, const Policy *, int, DWORD);
    int verbose;
} PolicyBackend;
DWORD policy_configure(const GUID *scheme, const PolicyBackend *backend);
#endif
