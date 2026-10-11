# v0.4 validation — 11 October 2026

Source: current local EnergySaver/VoltShift v0.3 (4ac1b80). Changes are in a separate workspace copy. Your original checkout has not been edited, committed or pushed by this task.

Passed:
- Native MSYS2 UCRT64 GCC 16.2.0 compilation with C11, -Wall -Wextra -Wpedantic -Werror -O2.
- All existing 65,536 battery flag/percentage combinations and diagnostic boundary tests.
- Controller tests: save before any power write, repeated apply without clone proliferation, performance/original switching, preparation failure rollback, failed activation/readback, failed journal save, failed restoration/deletion/clear and retry, corrupt state refusal, interrupted preparation recovery, automatic AC/DC/unknown transitions.
- Native storage tests: atomic journal round-trip, temporary-file removal, exclusive lock contention/release, truncation and same-length corruption rejection before control operations.
- CLI status, help and battery --dry-run. Invalid commands/options return 2. Default status returns 0 on this laptop.
- Read-only preview on your laptop: CPU minimum original AC=80 DC=5; maximum AC=100 DC=100; boost AC=2 DC=2; display timeout AC=0 DC=180. All proposed settings were readable.

Live test limitation:
- The opt-in live test was attempted in this agent execution environment. PowerDuplicateScheme returned Windows error 5 (access denied), so clone creation/application could not be verified here.
- Cleanup returned 0. The original Balanced scheme remained active; test recovery storage was cleared.
- Full live apply/restore, physical charger transitions, and Ctrl+C handling in your UCRT64 terminal still need the supplied live-test/manual check.
- The MSYS2 Bash runtime itself cannot start in this restricted agent environment; build.sh is supplied for your terminal. Its equivalent compiler commands and test executables were run natively here without PowerShell.

Not claimed:
- Measured battery-life improvement, vendor GPU/fan control, hardware enforcement of every policy, MSVC/Linux support, or behavior under corporate policies/Modern Standby on other computers.
- Tests simulate controller failures through the backend; they do not simulate an actual power loss during disk writes.
