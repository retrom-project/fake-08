#!/usr/bin/env bash
set -euo pipefail
make -f retrom/web.mk -j4
cp .retrom-build/fake08-retrom.mjs .retrom-build/fake08-retrom.wasm /output/
node retrom/state-test.mjs /output/fake08-retrom.mjs
node retrom/input-state-test.mjs /output/fake08-retrom.mjs
node retrom/gc-state-test.mjs /output/fake08-retrom.mjs
