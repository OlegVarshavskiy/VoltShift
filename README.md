# VoltShift v0.3

A modular, read-only Windows 10/11 diagnostics CLI written in C11.
Based on the supplied EnergySaver v0.2 sources (Git baseline `0ba52a1`).

## Features

- Existing CPU name and logical processor count (current processor group).
- Present display-class adapters, UTF-8 names and PnP instance IDs through SetupAPI. Includes adapters without attached monitors; may include virtual adapters. Presence does not establish rendering activity, integrated/discrete type, or control support.
- Battery presence, AC source, charge percentage, charging/low/critical flags, battery saver, and estimated remaining seconds when Windows supplies them on battery power.
- Active power scheme GUID and friendly name through PowrProf.
- An in-memory capability registry built from these actual query results, with a reason for every state: `supported`, `unavailable`, `unknown`, `error`, `not implemented`. This is not a Windows Registry modification.

No battery wear, design/full capacity, cycle count, GPU telemetry, vendor control, charge limit or fan control backend is implemented. These are not claimed as supported. Battery diagnostics describe the aggregate system battery status, not individual batteries. Unknown values (255 / DWORD -1) are never treated as real measurements. AC does not imply charging.

## Build and run (PowerShell, MinGW-w64 GCC)

From this project's root:

```powershell
.\build.ps1 -Test
.\build\voltshift.exe
```

The build script finds GCC on PATH or at `C:\msys64\ucrt64\bin\gcc.exe`. You can select another MinGW-w64 installation:

```powershell
.\build.ps1 -Compiler 'C:\path\to\gcc.exe' -Test
```

Git Bash alone does not supply a compiler. The compiler must target Windows and provide the Windows SDK-compatible headers and import libraries. Run PowerShell scripts according to your local execution policy.

Equivalent command in a MinGW-w64 shell (create `build` first):

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -Iinclude src/main.c src/hardware.c src/gpu.c src/battery.c src/power_plan.c src/capabilities.c -o build/voltshift.exe -ladvapi32 -lsetupapi -lpowrprof
./build/voltshift.exe
```

Exit status is 0 if the CPU, GPU enumeration, system battery status and scheme GUID queries succeed; otherwise 1, with available diagnostics still printed. Missing optional GPU labels/IDs and scheme name do not discard the other data. A successful empty adapter list is distinct from enumeration failure. Absence of a battery is not an API failure. GPU names/IDs and scheme names are emitted as UTF-8; use a UTF-8 terminal for localized text. CPU naming retains v0.2's ANSI registry query.

## Architecture

- `hardware`: existing CPU discovery.
- `gpu`: present device enumeration and owned dynamic list; `gpu_free` releases it.
- `battery`: Windows snapshot and separately testable interpretation of sentinel values.
- `power_plan`: scheme GUID, optional name, and API-specific errors.
- `capabilities`: interpretation of collected results; no extra system probes or writes.
- `main`: diagnostic report and exit status.

All read functions are one-shot snapshots. Settings and devices may change between probes. Do not use results as permanent hardware guarantees. `supported` means a diagnostic succeeded now, not permission to change a setting. Returned Win32 error codes are printed for failed main queries.

## Safety

Only queries are performed. The existing CPU registry handle requests `KEY_READ`. SetupAPI enumerates present display devices; PowrProf reads the active scheme. There are no setters, device enable/disable calls, external commands, driver changes, network requests or elevation requests in the application. Building writes only local project build artifacts.

## Verification

See `VALIDATION.md` for the performed build, runtime comparisons and test limits. See `COMMIT_GUIDE_RU.md` for transfer and commit instructions. Build output is ignored by Git.

## API references

- [SetupDiGetClassDevsW](https://learn.microsoft.com/en-us/windows/win32/api/setupapi/nf-setupapi-setupdigetclassdevsw)
- [SYSTEM_POWER_STATUS](https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-system_power_status)
- [PowerGetActiveScheme](https://learn.microsoft.com/en-us/windows/win32/api/powersetting/nf-powersetting-powergetactivescheme)
- [PowerReadFriendlyName](https://learn.microsoft.com/en-us/windows/win32/api/powrprof/nf-powrprof-powerreadfriendlyname)
