# VoltShift v0.5 — Bash quick start

## Update your existing project

1. Stop auto with Ctrl+C if it is running. In your existing UCRT64 Bash project terminal, restore with the old executable before replacing files:

   ~~~bash
   ./build/voltshift.exe restore
   ~~~

2. Extract the package. Merge the contents of its EnergySaver folder into your existing EnergySaver folder, replacing matching files. Keep your existing .git directory. The archive contains sources/docs only, so your executable is updated by the next build.
3. From the existing project root, build and run the standard tests:

   ~~~bash
   bash build.sh test
   ./build/voltshift.exe save --dry-run
   ~~~

4. Run the opt-in live integration check before relying on v0.5. It temporarily applies settings and attempts to restore them:

   ~~~bash
   bash build.sh live-test
   ~~~

   Keep its printed recovery directory if restoration fails. It prints the original plan GUID and a manual recovery command. A successful standard test run alone does not prove the hardware applies every policy.

5. Apply the daily-use profile:

   ~~~bash
   voltshift save
   voltshift status
   ~~~

Your existing voltshift shell function should continue working if it already points to this project's build/voltshift.exe. Until it is defined, use ./build/voltshift.exe instead. Do not run an older root-level voltshift.exe.

## Make the command available in other Bash directories

If your existing function works, no change is needed. Otherwise add this to ~/.bashrc using a text editor:

~~~bash
voltshift() {
    '/d/MY/MiNi/2025&2026/Programming/C++/Project/EnergySaver/build/voltshift.exe' "$@"
}
~~~

Load it in the current terminal:

~~~bash
source ~/.bashrc
~~~

If a login Git Bash terminal does not load ~/.bashrc, add this line once to ~/.bash_profile:

~~~bash
[[ -f ~/.bashrc ]] && source ~/.bashrc
~~~

This makes the command available in Bash sessions, including from other directories. Building requires the UCRT64 Windows GCC toolchain; running the compiled program does not require PowerShell.

## Daily commands

~~~bash
voltshift save          # apply and exit; profile stays active
voltshift restore       # return to original plan and recorded refresh
voltshift auto          # AC/battery switching while terminal stays open
~~~

Do not run auto and another modifying command together. Check the actual Saver status and refresh in status. Unsupported optional controls are skipped visibly. No command changes fan settings or disables thermal protection.

For a later runtime comparison, use the same workload, charge range, screen conditions and connectivity, allow the machine to settle, and repeat each configuration. Record elapsed time and charge loss rather than judging Windows' remaining-time estimate. Preserve each configuration's intended brightness/refresh when comparing the full experience, and record those values so the difference is understandable.
