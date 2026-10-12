#include <stdio.h>
#include "policy.h"
static const GUID processor = {0x54533251,0x82be,0x4824,{0x96,0xc1,0x47,0xb6,0x0b,0x74,0x0d,0x00}};
static const GUID video = {0x7516b95f,0xf776,0x4464,{0x8c,0x53,0x06,0x16,0x7f,0x40,0xcc,0x99}};
static const GUID saver = {0xde830923,0xa562,0x41af,{0xa0,0x86,0xe3,0xa2,0xc6,0xba,0xd2,0xda}};
static const GUID pcie = {0x501a4d13,0x42af,0x4429,{0x9f,0xd1,0xa8,0x21,0x8c,0x26,0x8e,0x20}};
static const GUID usb = {0x2a737441,0x1930,0x4402,{0x8d,0x77,0xb2,0xbe,0xb3,0x08,0xa3,0xa3}};
static const GUID minimum = {0x893dee8e,0x2bef,0x41e0,{0x89,0xc6,0xb5,0x5d,0x09,0x29,0x96,0x4c}};
static const GUID maximum = {0xbc5038f7,0x23e0,0x4960,{0x96,0xda,0x33,0xab,0xaf,0x59,0x35,0xec}};
static const GUID boost = {0xbe337238,0x0d82,0x4146,{0xa9,0x60,0x4f,0x37,0x49,0xd4,0x70,0xc7}};
static const GUID epp = {0x36687f9e,0xe3a5,0x4dbf,{0xb1,0xdc,0x15,0xeb,0x38,0x1c,0x68,0x63}};
static const GUID brightness = {0xaded5e82,0xb909,0x4619,{0x99,0x49,0xf5,0xd7,0x1d,0xac,0x0b,0xcb}};
static const GUID dim = {0xf1fbfde2,0xa960,0x4165,{0x9f,0x88,0x50,0x66,0x79,0x11,0xce,0x96}};
static const GUID display_idle = {0x3c0bc021,0xc8a8,0x4e07,{0xa9,0x73,0x6b,0x14,0xcb,0xcb,0x2b,0x7e}};
static const GUID threshold = {0xe69653ca,0xcf7f,0x4f05,{0xaa,0x73,0xcb,0x83,0x3f,0xa9,0x0a,0xd4}};
static const GUID saver_brightness = {0x13d09884,0xf74e,0x474a,{0xa8,0x52,0xb6,0xbd,0xe8,0xad,0x03,0xa8}};
static const GUID aspm = {0xee12f906,0xd277,0x404b,{0xb6,0xda,0xe5,0xfa,0x1a,0x57,0x6d,0xf5}};
static const GUID suspend = {0x48e6b7a6,0x50f5,0x4782,{0xa5,0xd4,0x53,0xbb,0x8f,0x07,0xe2,0x26}};
static const Policy policies[] = {
    {"CPU minimum state (%)", &processor, &minimum, 5, 1, 1, 0},
    {"CPU maximum state (%)", &processor, &maximum, 60, 1, 1, 0},
    {"CPU boost (0=disabled)", &processor, &boost, 0, 0, 0, 0},
    {"CPU energy preference (0..100)", &processor, &epp, 80, 0, -1, 0},
    {"Display brightness policy (%)", &video, &brightness, 50, 0, 1, 0},
    {"Display dim brightness policy (%)", &video, &dim, 30, 0, 1, 0},
    {"Display idle timeout (seconds)", &video, &display_idle, 120, 0, 0, 0},
    {"Energy Saver battery threshold (%)", &saver, &threshold, 100, 0, 0, 1},
    {"Energy Saver brightness scale (%)", &saver, &saver_brightness, 80, 0, 1, 1},
    {"PCIe link savings (2=maximum)", &pcie, &aspm, 2, 0, 0, 1},
    {"USB selective suspend (1=enabled)", &usb, &suspend, 1, 0, 0, 1}
};
const Policy *policy_list(size_t *count) { *count = sizeof(policies)/sizeof(policies[0]); return policies; }
DWORD policy_target(const Policy *p, DWORD original) {
    if (p->lower_only > 0 && original < p->value) return original;
    if (p->lower_only < 0 && original > p->value) return original;
    return p->value;
}
DWORD policy_configure(const GUID *scheme, const PolicyBackend *b) {
    for (size_t i = 0; i < sizeof(policies)/sizeof(policies[0]); ++i) {
        const Policy *p = &policies[i];
        DWORD old[2], target[2], actual, result;
        result = b->read(b->context, scheme, p, 0, &old[0]);
        if (!result && !p->dc_only) result = b->read(b->context, scheme, p, 1, &old[1]);
        if (result) {
            if (b->verbose) printf("  %s: unavailable (%lu); %s.\n", p->name, result, p->required ? "required" : "unchanged");
            if (p->required) return result;
            continue;
        }
        for (int ac = 0; ac <= !p->dc_only; ++ac) {
            target[ac] = policy_target(p, old[ac]);
            result = b->write(b->context, scheme, p, ac, target[ac]);
            if (!result) result = b->read(b->context, scheme, p, ac, &actual);
            if (!result && actual != target[ac]) result = ERROR_WRITE_FAULT;
            if (result) {
                if (b->verbose) fprintf(stderr, "  %s write/readback failed (%lu); rolling back.\n", p->name, result);
                return result;
            }
        }
        if (b->verbose) {
            if (p->dc_only) printf("  %s: DC=%lu verified (AC inherited).\n", p->name, target[0]);
            else printf("  %s: AC=%lu DC=%lu verified.\n", p->name, target[1], target[0]);
        }
    }
    return 0;
}
