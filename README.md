# VoltShift v0.4

Windows power-profile CLI in C11. Built on the supplied v0.3 source at commit `4ac1b80`.

## Quick start — MSYS2 UCRT64 Bash

Copy the contents of this EnergySaver folder into your existing project, merging folders and replacing matching source files. Keep your existing .git directory. This package contains no Git history and does not require PowerShell.

From the project root:

```bash
bash build.sh
./build/voltshift.exe battery --dry-run
```

The build script compiles the application and runs diagnostic, controller and recovery-storage tests. GCC must be the MinGW-w64/UCRT64 Windows compiler. Set CC to another compiler path if necessary.

For a live check that temporarily activates the battery plan and restores the original:

```bash
bash build.sh live-test
```

This is an explicit opt-in system-writing test. It attempts restoration even after a failed operation. Only the standard tests run when no argument is supplied. `bash build.sh build` builds only the application. Your old root-level voltshift.exe remains v0.3; use the executable in build.

## Commands

| Command | Behavior |
| --- | --- |
| `./build/voltshift.exe` or `status` | Read-only CPU/GPU/battery/plan report and recovery-session state |
| `./build/voltshift.exe battery --dry-run` | Read-only policy preview; no clone, journal, directory or settings created |
| `./build/voltshift.exe battery` | Create/reuse and activate a private battery plan |
| `./build/voltshift.exe performance` | Select the original plan; retain recovery session for subsequent switching |
| `./build/voltshift.exe auto` | Foreground switching every five seconds: battery -> battery plan, AC -> original plan |
| `./build/voltshift.exe restore` | Reactivate original plan, delete the private clone and clear the saved session |
| `./build/voltshift.exe help` | Command reference |

Manual battery mode persists after the command exits, including after reconnecting AC. Use auto for automatic switching. Keep the auto terminal open. Ctrl+C/Ctrl+Break requests restoration and exit; wait for the restoration message. Closing or forcibly killing the terminal/process may bypass cleanup: run restore afterward. One control process per user is allowed; stop auto before running another control command. Status and preview remain available during auto.

Auto does not repeatedly enforce a plan if another application changes it while the power source stays the same. It acts on the next detected source transition. Unknown power source readings cause no profile change. No startup task or background service is installed.

## Battery policy

The current plan is duplicated into a uniquely identified **VoltShift Battery** plan. The original is never edited by VoltShift. Both AC and DC values in the private clone are configured:

| Setting | Requested value | Requirement |
| --- | --- | --- |
| CPU minimum performance state | 5% | Required |
| CPU maximum performance state | 70% | Required |
| Processor boost mode | 0 (disabled) | Optional if not readable |
| Display idle timeout | 120 seconds | Optional if not readable |

The two CPU settings must be readable and writable. Missing optional settings are reported and skipped. Every attempted write is read back; a write/readback error aborts preparation and removes the incomplete clone when cleanup succeeds. Only a fully prepared profile is activated. Successful activation is checked against the active GUID.

Performance means your original configuration, not a forced High Performance scheme or overclocking. This preserves your normal setup and avoids guessing vendor controls.

These are Windows policy requests. Firmware/drivers and Windows power-mode overlays may affect enforcement. A 70% setting is not a guarantee of 70% frequency or any battery-life percentage. Actual benefit depends on workload and must be measured. VoltShift does not toggle Windows Energy/Battery Saver or replace its background-app policies. It does not disable a GPU, change fan profiles, screen brightness, sleep/hibernation or critical-battery actions.

## Recovery

Before cloning or changing the active plan, VoltShift atomically saves the original/private GUIDs and preparation stage to:

`%LOCALAPPDATA%\VoltShift\session-v1.bin`

The first original plan is retained across repeated battery/performance commands. It is not overwritten by later invocations. The directory is per Windows user, so changing executable/project directories does not lose recovery state. Keep the recovery file while a session exists.

Restore is retryable. If activation or clone deletion fails, the journal is retained. Incomplete sessions require restore before another apply. Truncated/corrupt journals cause control commands to stop without changing settings. Status prints the saved GUIDs when the journal is valid. If another tool deletes the original plan, automatic restoration cannot recreate that external deletion.

Windows error 5 means access was denied by permissions, policy, or the execution environment. Do not assume activation occurred: check status and restore any retained session. The application does not request elevation automatically.

## Architecture

- hardware, gpu, battery, power_plan: v0.3 read-only probes.
- capabilities: diagnostic results, with vendor GPU/fan control still explicitly unimplemented. Power-profile write support is checked during apply, never inferred from a CPU/GPU name.
- controller: profile transactions, automatic transition decisions and rollback through an injectable backend.
- control_win: PowrProf operations, policy probing/readback, durable versioned recovery journal, exclusive lock and console auto loop.
- main: argument validation and presentation. No arguments defaults to read-only status.

Exit codes: 0 success; 1 operation/query failure; 2 invalid command/options. No third-party driver or runtime library is installed.

## Tests and limits

See VALIDATION.md. Standard tests perform no Windows power-setting writes. The live test deliberately changes the active plan temporarily and uses a separate recovery directory under build. If its restoration fails, it prints the original GUID and a manual powercfg restoration command; keep its printed recovery directory until recovery is complete.

For functional acceptance, run live-test successfully, then run auto, unplug/reconnect the charger and use status from a second terminal to verify the active profiles. Stop auto with Ctrl+C and verify restoration.

For later comparison with Windows Energy Saver, first restore and use a consistent workload, brightness, starting charge and network conditions. Compare measured discharge across repeated runs rather than Windows' initial remaining-time estimate.

## Microsoft API references

- [PowerDuplicateScheme](https://learn.microsoft.com/en-us/windows/win32/api/powrprof/nf-powrprof-powerduplicatescheme)
- [PowerWriteDCValueIndex](https://learn.microsoft.com/en-us/windows/win32/api/powersetting/nf-powersetting-powerwritedcvalueindex)
- [PowerSetActiveScheme](https://learn.microsoft.com/en-us/windows/win32/api/powersetting/nf-powersetting-powersetactivescheme)
- [Maximum processor state](https://learn.microsoft.com/en-us/windows-hardware/customize/power-settings/options-for-perf-state-engine-maxperformance)
- [Processor boost mode](https://learn.microsoft.com/en-us/windows-hardware/customize/power-settings/options-for-perf-state-engine-perfboostmode)
