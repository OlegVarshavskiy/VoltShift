#include "capabilities.h"
const char *capability_state_name(CapabilityState state) {
    switch (state) {
        case CAP_SUPPORTED: return "supported";
        case CAP_UNAVAILABLE: return "unavailable";
        case CAP_UNKNOWN: return "unknown";
        case CAP_ERROR: return "error";
        case CAP_NOT_IMPLEMENTED: return "not implemented";
        default: return "unknown";
    }
}
static Capability battery_field(const char *name, const BatteryInfo *b, int known) {
    Capability result = { name, CAP_SUPPORTED, "Value returned by Windows" };
    if (!b->available) { result.state = CAP_ERROR; result.reason = "Power status query failed"; }
    else if (b->present == 0) { result.state = CAP_UNAVAILABLE; result.reason = "No system battery"; }
    else if (!known) { result.state = CAP_UNKNOWN; result.reason = "Windows did not report this value"; }
    return result;
}
void capabilities_collect(int cpu_ok, const GPUList *g, const BatteryInfo *b,
                          const PowerPlan *p, Capability out[CAPABILITY_COUNT]) {
    out[0] = (Capability){"CPU identification", cpu_ok ? CAP_SUPPORTED : CAP_ERROR,
                         cpu_ok ? "CPU name read" : "CPU name query failed"};
    out[1] = (Capability){"GPU enumeration", g->complete ? CAP_SUPPORTED : CAP_ERROR,
                         g->complete ? "Present display class devices enumerated" : "Enumeration failed or incomplete"};
    out[2] = (Capability){"Power source", !b->available ? CAP_ERROR : b->ac < 0 ? CAP_UNKNOWN : CAP_SUPPORTED,
                         "GetSystemPowerStatus snapshot"};
    out[3] = battery_field("Battery presence", b, b->present >= 0);
    if (b->available && b->present == 0) out[3].state = CAP_SUPPORTED;
    out[4] = battery_field("Battery level", b, b->percent >= 0);
    out[5] = battery_field("Battery charge flags", b, b->charging >= 0);
    out[6] = battery_field("Battery time estimate", b, b->remaining_known);
    if (b->available && b->present != 0 && b->ac == 1) {
        out[6].state = CAP_UNAVAILABLE;
        out[6].reason = "Windows does not provide remaining battery time on AC";
    }
    out[7] = (Capability){"Battery saver status", !b->available ? CAP_ERROR : b->saver < 0 ? CAP_UNKNOWN : CAP_SUPPORTED,
                         "Windows 10+ system status flag"};
    out[8] = (Capability){"Active power scheme GUID", p->available ? CAP_SUPPORTED : CAP_ERROR,
                         "PowerGetActiveScheme result"};
    out[9] = (Capability){"Power scheme name", !p->available || !p->name_available ? CAP_ERROR : CAP_SUPPORTED,
                         "PowerReadFriendlyName result"};
    out[10] = (Capability){"Battery wear / cycle count", CAP_NOT_IMPLEMENTED,
                          "Not exposed by this diagnostic implementation"};
    out[11] = (Capability){"Windows power profiles", CAP_UNKNOWN,
                          "Preview: battery --dry-run; writes verified during apply"};
    out[12] = (Capability){"GPU / fan control", CAP_NOT_IMPLEMENTED,
                          "No vendor control backend"};
}
