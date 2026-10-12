#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
mkdir -p build
cc="${CC:-gcc}"
flags=(-std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -Iinclude)
diagnostics=(src/hardware.c src/gpu.c src/battery.c src/power_plan.c src/capabilities.c)
control=(src/controller.c src/control_win.c src/policy.c src/display.c)
libs=(-ladvapi32 -lsetupapi -lpowrprof -lole32 -luser32)
case "${1:-test}" in
    build|test|live-test) ;;
    *) printf 'Usage: bash build.sh [build|test|live-test]\n' >&2; exit 2 ;;
esac
"$cc" "${flags[@]}" src/main.c "${diagnostics[@]}" "${control[@]}" -o build/voltshift.exe "${libs[@]}"
printf 'Built build/voltshift.exe\n'
if [[ "${1:-test}" != build ]]; then
    "$cc" "${flags[@]}" tests/test_diagnostics.c "${diagnostics[@]}" -o build/test_diagnostics.exe "${libs[@]}"
    "$cc" "${flags[@]}" tests/test_controller.c src/controller.c -o build/test_controller.exe
    "$cc" "${flags[@]}" tests/test_storage.c "${control[@]}" -o build/test_storage.exe "${libs[@]}"
    "$cc" "${flags[@]}" tests/test_policy.c src/policy.c -o build/test_policy.exe
    "$cc" "${flags[@]}" tests/test_display.c src/display.c -o build/test_display.exe -luser32
    ./build/test_policy.exe
    ./build/test_display.exe
    ./build/test_diagnostics.exe
    ./build/test_controller.exe
    ./build/test_storage.exe
fi
if [[ "${1:-test}" == live-test ]]; then
    printf 'Live test temporarily applies the battery profile, verifies settings, then restores.\n'
    "$cc" "${flags[@]}" tests/test_live.c "${control[@]}" -o build/test_live.exe "${libs[@]}"
    ./build/test_live.exe
fi
