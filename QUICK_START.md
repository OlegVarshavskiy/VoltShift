# Copy and run (Bash only)

1. Extract the archive. Copy everything inside EnergySaver into your existing EnergySaver project folder; merge folders and replace matching files. Keep your .git folder.
2. Open your usual UCRT64 Bash terminal in that project folder.
3. Run:

```bash
bash build.sh live-test
./build/voltshift.exe battery --dry-run
./build/voltshift.exe auto
```

The live test temporarily applies and restores the profile. It must end with PASS before treating live control as verified. If it reports an error, share its output.

In auto mode, unplug/reconnect the charger. Use a second terminal to check:

```bash
./build/voltshift.exe status
```

Press Ctrl+C in the auto terminal and wait for the restoration message. Then check status again; the original plan should be active and the VoltShift session should be none.

Manual use:

```bash
./build/voltshift.exe battery
./build/voltshift.exe performance
./build/voltshift.exe restore
```

Run the executable under build, not the stale executable at the project root. Your previous build.ps1 is unnecessary for this version. No PowerShell commands are needed.
