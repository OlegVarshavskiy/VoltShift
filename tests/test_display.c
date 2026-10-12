#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include "display.h"
int main(void) {
    DEVMODEW current={0}, candidate;
    assert(display_rate_matches(59,60) && display_rate_matches(60,59));
    assert(!display_rate_matches(58,60) && !display_rate_matches(61,60));
    assert(display_rate_matches(144,144) && !display_rate_matches(143,144));
    current.dmPelsWidth=1920; current.dmPelsHeight=1080;
    current.dmBitsPerPel=32; current.dmDisplayFrequency=144;
    candidate=current; candidate.dmDisplayFrequency=60;
    assert(display_candidate(&current,&candidate));
    candidate.dmDisplayFrequency=59; assert(display_candidate(&current,&candidate));
    candidate.dmDisplayFrequency=58; assert(!display_candidate(&current,&candidate));
    candidate.dmDisplayFrequency=61; assert(!display_candidate(&current,&candidate));
    candidate.dmDisplayFrequency=60; candidate.dmPelsWidth=1280; assert(!display_candidate(&current,&candidate));
    candidate=current; candidate.dmDisplayFrequency=60; candidate.dmPelsHeight=720; assert(!display_candidate(&current,&candidate));
    candidate=current; candidate.dmDisplayFrequency=60; candidate.dmBitsPerPel=16; assert(!display_candidate(&current,&candidate));
    candidate=current; candidate.dmDisplayFrequency=60; candidate.dmDisplayOrientation=DMDO_90; assert(!display_candidate(&current,&candidate));
    candidate=current; candidate.dmDisplayFrequency=60; candidate.dmDisplayFlags=DM_INTERLACED; assert(!display_candidate(&current,&candidate));
    candidate=current; candidate.dmDisplayFrequency=60; current.dmDisplayFrequency=60; assert(!display_candidate(&current,&candidate));
    current.dmDisplayFrequency=48; assert(!display_candidate(&current,&candidate));
    puts("PASS: refresh selection preserves resolution, color, orientation, scan mode and never raises refresh or uses modes below 59 Hz.");
    return 0;
}
