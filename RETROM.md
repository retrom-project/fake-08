# Retrom Web core

This fork follows upstream master at the exact commit recorded in `retrom-fork.json`. The mirror branch stays upstream-only. Retrom changes live on the maintenance line and feature branches.

Run `.github/rpg-runtime/build-candidate.sh /absolute/empty/directory` with Docker as a non-root user. It builds with pinned Emscripten 4.0.10, runs owned native lifecycle/state regressions, copies `fake08-retrom.mjs`, `fake08-retrom.wasm` and complete license texts, and emits `retrom-core-candidate.json` with source/submodule identities and byte hashes. Cores are built explicitly; the runtime consumes the result without compiling sources.

The ES module default is an Emscripten factory with a per-instance heap. ABI 1 exposes load, step, RGBA pixels, interleaved signed 16-bit stereo samples, readiness, save, restore and stop. Inputs are sampled once per emulated 60 Hz frame. Payloads are limited to 4 MiB. Host envelopes bind state to core and cartridge digest.

FAKE-08 renders 128 × 128 and outputs 22.05 kHz audio through its upstream libretro implementation. `.p8` and `.p8.png` cartridges are supported. This is an emulator and retains upstream compatibility limitations. Multi-cart loading, mouse, keyboard text input and multiplayer are not exposed by this initial single-player adapter.

`fake08-state-v1` is an INSTANT checkpoint: bounded Eris Lua/coroutine data, RAM, audio state, game/frontend frame counters, target frame rate, button edge/repeat state and cartdata keys/files. Restore rejects truncated, mismatched and oversized records. It preserves the first resumed input. Core pause-menu state is explicitly unavailable for saves. Formats are scoped to a fixed core build; incompatible native layouts require a new ABI/format.

Candidate validation does not publish a release. After real Retrom import, review preview, publishing, Launch and fresh-instance restore acceptance, an explicitly authorized release can populate the declared `rpg-runtime-release.json` asset and pin the final core commit/tag in the runtime. Downloaded third-party games are local test inputs and must not enter Git or release assets.

The Eris restore transaction suspends Lua GC until prototypes and upvalues are fully linked, then restores the previous GC mode. `retrom/gc-state-test.mjs` reproduces premature collection with an owned closure graph and checks 180 continuation frames after fresh-instance restore.

Formal release tags are annotated and must point to a tested commit on `retrom/g814991a2571a`. The release workflow reruns native regressions, validates the exact Web ABI and asset allowlist, and publishes the release descriptor consumed by retrom-runtime.
