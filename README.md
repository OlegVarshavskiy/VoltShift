# VoltShift v0.5

Windows C11 command-line utility for battery-first coding, Bash, browser/cloud AI and music. Use **voltshift save** to apply the profile, **voltshift restore** to return to your saved original plan. GCC/MSYS2 UCRT64 and Bash are sufficient; PowerShell is not needed.

## Install or upgrade

See [QUICK_START.md](QUICK_START.md). This package extends your supplied v0.4 sources. It contains no .git directory or prebuilt executable. Merge it into the existing project, preserving your Git history. Always run build/voltshift.exe; an old executable at the project root is not updated.

## Commands

| Command | Behavior |
| --- | --- |
| voltshift or voltshift status | Read-only diagnostics, current plan policies, actual Saver status and panel refresh |
| voltshift save --dry-run | Read-only preview of the original baseline; display driver mode test only |
| voltshift save | Prepare/reuse a private plan, activate it and apply supported internal-panel refresh savings |
| voltshift battery | Alias of save, retained from v0.4 |
| voltshift performance | Select your saved original plan and restore the recorded panel refresh; retain session for switching |
| voltshift restore | Select original plan, restore recorded refresh, remove private plan and recovery session |
| voltshift auto | Foreground switching every five seconds: battery selects save; AC selects original |
| voltshift help | Usage |

Without a shell function/PATH entry, substitute ./build/voltshift.exe for voltshift.

Manual save persists after the command exits, including on AC. For automatic AC/battery switching, use auto and keep its terminal open. Ctrl+C requests restoration; wait for the completion message. A forced process/terminal close can bypass cleanup: run restore afterward. Stop auto before another modifying command or rebuild. Status and preview remain available. Auto acts on detected power-source transitions, leaving unknown readings alone; it does not continually override changes by other applications. No service or startup task is installed.

## Requested policies

The original power plan is duplicated into a uniquely identified VoltShift Battery plan. The original plan's settings are never edited.

| Policy | Target in private plan | Scope |
| --- | --- | --- |
| CPU minimum state | At most 5% | AC and battery; required |
| CPU maximum state | At most 60% | AC and battery; required |
| CPU boost | Disabled (0) | AC and battery |
| CPU energy preference (EPP) | At least 80/100 toward efficiency | AC and battery |
| Normal brightness policy | At most 50% | AC and battery |
| Dim brightness policy | At most 30% | AC and battery |
| Screen-off idle timeout | 120 seconds | AC and battery |
| Windows Energy Saver threshold | 100% | Battery only |
| Saver brightness scale | At most 80% | Battery only |
| PCIe link power savings | Maximum (2) | Battery only |
| USB selective suspend | Enabled (1) | Battery only |

Lower existing CPU limits and brightness policy values are retained; an already stronger EPP is retained. Battery-only settings inherit the original AC values. Missing optional settings are printed and skipped. Required CPU settings must be available. Every attempted write is read back; any write/readback failure aborts and attempts rollback. Activation is also checked against the active GUID.

These are Windows policy values, not guaranteed physical limits or measured watts. A CPU maximum of 60% is not a promise of 60% frequency, load or heat. EPP depends on processor/firmware support. Brightness values are power-plan policies, not a reading or backup of the live brightness slider; Windows/drivers may apply them differently. Saver brightness scaling depends on actual Saver activation. VoltShift prints the actual Windows Battery/Energy Saver flag: check status rather than assuming a threshold write means On. Feature availability and behavior depend on Windows version, drivers, firmware and power-mode overlays.

On a supported internal panel running above 60 Hz, VoltShift selects an enumerated 59/60 Hz mode at the same resolution, color depth, orientation and scan flags, after Windows approves CDS_TEST. The 59/60 Hz family is accepted on readback; restoration of the original higher rate is checked exactly. The original refresh is journaled before any change. External and mirrored displays are left alone. No custom timing or persistent display registry mode is installed. If Windows rejects the mode, refresh stays unchanged and the reason is printed. Selecting a lower panel refresh is not an application/game FPS limiter. If display identity, topology or mode changes externally, restoration preserves that newer selection instead of forcing an old mode onto it.

Cooling, firmware thermal protection, voltage, CPU cores, GPUs, network adapters and critical-battery/sleep actions remain under their existing management. VoltShift does not disable the NVIDIA GPU or guarantee it sleeps; a browser, external display or another application may keep it awake. Chrome/Spotify application preferences are not edited, and processes are not killed. The requested savings therefore cannot guarantee the absolute lowest possible consumption or a fixed runtime improvement. Compile-heavy work may take longer; compare battery drain and task completion under your real workload.

## Recovery and v0.4 compatibility

A durable, checksummed journal lives at %LOCALAPPDATA%/VoltShift/session-v1.bin. It records the original/private plan IDs, preparation stage, policy revision and display snapshot. The first baseline is retained across repeated commands. The journal is written atomically before cloning or changing display settings, and an exclusive lock prevents concurrent modifying instances for the same Windows user.

v0.5 can read and restore v0.4 journals. Applying save to a completed v0.4 session first restores its original plan and replaces the old clone with the expanded profile. An incomplete or corrupt session stops further application until recovery; corrupt files are not overwritten. Do not run v0.4 against a v0.5 journal: restore with v0.5 before downgrading. Do not delete a recovery file while its session exists.

If restoring the plan/display or deleting the clone fails, the journal remains for retry. Windows error 5 means access was denied by Windows policy, account permissions or the execution environment; a failed command does not establish that the profile became active. If another tool removes the original plan, VoltShift cannot recreate that external deletion.

## Architecture

- hardware, gpu, battery, power_plan: read-only Windows probes.
- capabilities: diagnostic support registry; prepared profile support is based on a valid saved session, not the GPU/CPU model.
- policy: declarative policy registry with injectable read/write callbacks and verified targets.
- display: internal-panel identity, driver mode approval, refresh application/readback and guarded restoration.
- controller: injectable transactions, rollback, migration and automatic source-transition decisions.
- control_win: PowrProf backend, atomic versioned journal, lock, status and console loop.
- main: argument validation and presentation. Default is read-only status.

Exit codes: 0 success, 1 operation/query failure, 2 invalid arguments. See [VALIDATION.md](VALIDATION.md) for tested behavior and remaining live checks.

## Microsoft references

- [Processor maximum state](https://learn.microsoft.com/en-us/windows-hardware/customize/power-settings/options-for-perf-state-engine-maxperformance)
- [Processor boost mode](https://learn.microsoft.com/en-us/windows-hardware/customize/power-settings/options-for-perf-state-engine-perfboostmode)
- [Processor energy preference](https://learn.microsoft.com/en-us/windows-hardware/customize/power-settings/options-for-perf-state-engine-perfenergypreference)
- [Energy Saver battery threshold](https://learn.microsoft.com/en-us/windows-hardware/customize/power-settings/battery-threshold)
- [Energy Saver brightness scaling](https://learn.microsoft.com/en-us/windows-hardware/customize/power-settings/brightness)
- [Display mode change and CDS_TEST](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-changedisplaysettingsexw)
- [USB selective suspend](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-selective-suspend)
