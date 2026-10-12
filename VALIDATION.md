# VoltShift v0.5 validation

## Completed in the development environment

The application and all six test executables, including the opt-in live-test executable, compile as C11 with GCC/UCRT64 using -Wall -Wextra -Wpedantic -Werror -O2.

Five standard test suites pass:

- Diagnostics: 65,536 battery flag/percentage combinations, AC/time/unknown/absence boundaries and capability error handling.
- Controller: journal-before-write ordering, repeated apply, rollback, restoration retry, simulated crash recovery, failed display capture/effects, legacy v0.4 migration and unknown power-source behavior.
- Storage: atomic journal replacement, display snapshot round-trip, exclusive locking, corrupt/truncated journal rejection, legacy v0.4 loading and lock release. Tests use isolated directories under build; they do not alter the real recovery file.
- Policy: 11 targets, preservation of stronger existing limits, battery-only writes, optional/required settings, rejected writes and readback mismatch through a fake backend.
- Display selection: same-resolution/depth/orientation/scan-mode requirements and 59/60 Hz boundaries. This tests selection logic, not physical display switching.

Read-only preview was exercised against this laptop's original baseline through its existing v0.4 journal. The expanded CPU, brightness, Saver and PCIe policies were readable. USB selective suspend was unavailable and correctly skipped. The internal panel reported 1920x1080 at 144 Hz; Windows rejected the non-mutating 60 Hz mode test in this execution environment, so refresh was left unchanged.

## Remaining live acceptance

v0.4 was previously live-tested successfully by the user. That does not establish v0.5's new settings or display behavior. This development environment restricts Windows system writes; the v0.5 live integration test has been compiled but has not been run here. No claim of measured battery-life improvement or successful physical refresh/brightness change is made.

In the normal UCRT64 terminal, after restoring any earlier profile:

~~~bash
bash build.sh live-test
./build/voltshift.exe save
./build/voltshift.exe status
./build/voltshift.exe restore
./build/voltshift.exe status
~~~

The opt-in live test temporarily changes the power plan and, only if approved, panel refresh. It checks activation, repeated apply, original-plan selection and refresh readback/restoration for recorded displays. It always attempts rollback. Run it without unplugging displays or changing their modes mid-test. If a display mode is unsupported, it is skipped; a passing test does not mean that skipped feature was applied.

Confirm the original plan returns, and if a refresh change was accepted, that its original refresh returns. Check actual Windows Saver On/Off separately: a stored threshold of 100 is not proof of activation. Plan brightness readback is not a physical brightness measurement. Verify brightness and normal app responsiveness by use.

For auto, inspect status in a second terminal while unplugging/reconnecting AC. Ctrl+C should restore the original plan and recorded refresh. A forced termination requires explicit restore. Keep recovery state until restoration succeeds.

## Performance and energy claims

CPU limits, EPP, display refresh and driver idle policies request savings. They do not measure CPU temperature, fan speed, battery watts or actual GPU idle state. No vendor fan/GPU backend is implemented. Cooling and thermal limits are untouched by the code.

Runtime gains remain unmeasured. Use repeated comparable discharge runs and record workload, elapsed time, start/end charge, brightness, refresh and actual Saver status. Compare useful work completed as well as drain: a longer compilation can offset some instantaneous power savings.
