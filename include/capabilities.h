#ifndef VOLTSHIFT_CAPABILITIES_H
#define VOLTSHIFT_CAPABILITIES_H
#include "battery.h"
#include "gpu.h"
#include "power_plan.h"
typedef enum { CAP_SUPPORTED, CAP_UNAVAILABLE, CAP_UNKNOWN,
               CAP_ERROR, CAP_NOT_IMPLEMENTED } CapabilityState;
typedef struct { const char *name; CapabilityState state; const char *reason; } Capability;
#define CAPABILITY_COUNT 12
void capabilities_collect(int cpu_ok, const GPUList *gpu, const BatteryInfo *battery,
                          const PowerPlan *plan, Capability out[CAPABILITY_COUNT]);
const char *capability_state_name(CapabilityState state);
#endif
