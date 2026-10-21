#!/bin/sh
# Desktop tests: normal build and fault-injection build.  Usage: tools/run_native_tests.sh
set -e
cd "$(dirname "$0")/.."
python3 tools/gen_errors.py >/dev/null
python3 tools/gen_pins.py >/dev/null
SRC="firmware/test/native_tests.cpp firmware/common/log.cpp firmware/common/commands.cpp firmware/wm/wm_machine.cpp firmware/sm/sm_machine.cpp firmware/uv/uv_machine.cpp"
mkdir -p /tmp/mercursense_tests
g++ -std=c++14 -Wall -Wextra -I firmware $SRC -o /tmp/mercursense_tests/normal && /tmp/mercursense_tests/normal
g++ -std=c++14 -Wall -Wextra -DENABLE_FAULT_INJECTION=1 -I firmware $SRC -o /tmp/mercursense_tests/fi && /tmp/mercursense_tests/fi
python3 tools/test_log2csv.py
