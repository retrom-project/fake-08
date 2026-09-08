// SPDX-License-Identifier: MIT
#include "../source/vm.h"
#include <cstring>
#include <array>
#include <algorithm>

extern size_t frame;
extern uint8_t kHeld, kDown;

namespace {
constexpr size_t luaLimit = 1024 * 1024;
struct Header {
    uint32_t magic, version, luaSize, ramSize, audioSize, frame, frontendFrame, targetFps, held, down;
};
}

std::vector<unsigned char> Vm::retromSave() {
    if (!_luaState || _pauseMenu || _cartChangeQueued) { return {}; }
    std::vector<unsigned char> lua(luaLimit);
    size_t count = serializeLuaState(reinterpret_cast<char*>(lua.data()), lua.size());
    if (!count) return {};
    Header header{0x31523846, 1, static_cast<uint32_t>(count), sizeof(PicoRam),
        sizeof(audioState_t), static_cast<uint32_t>(_picoFrameCount), static_cast<uint32_t>(::frame),
        static_cast<uint32_t>(_targetFps), kHeld, kDown};
    std::vector<unsigned char> output;
    auto append = [&](const void* data, size_t size) {
        auto bytes = static_cast<const unsigned char*>(data);
        output.insert(output.end(), bytes, bytes + size);
    };
    auto inputState = _input->retromState();
    append(&header, sizeof(header));
    append(inputState.data(), sizeof(inputState));
    append(lua.data(), count);
    append(_memory->data, sizeof(PicoRam));
    append(_audio->getAudioState(), sizeof(audioState_t));
    append(_drawStateCopy, sizeof(_drawStateCopy));
    // Four cartdata keys plus active key. No host paths enter the state.
    std::array<string, 5> keys{_cartdataKeys[0], _cartdataKeys[1], _cartdataKeys[2], _cartdataKeys[3], _currentCartdataKey};
    for (const auto& key : keys) {
        if (key.size() > 64) return {};
        uint32_t length = static_cast<uint32_t>(key.size());
        append(&length, sizeof(length)); append(key.data(), key.size());
    }
    for (int i = 0; i < 4; i++) {
        auto value = keys[i].empty() ? string() : _host->getCartDataFileContents(keys[i]);
        if (value.size() > 1024) return {};
        uint32_t length = static_cast<uint32_t>(value.size());
        append(&length, sizeof(length)); append(value.data(), value.size());
    }
    return output;
}

bool Vm::retromRestore(const unsigned char* bytes, size_t size) {
    constexpr size_t prefixSize = sizeof(Header) + sizeof(std::array<uint16_t, 10>);
    if (!_luaState || !bytes || size < prefixSize) return false;
    Header header;
    memcpy(&header, bytes, sizeof(header));
    if (header.magic != 0x31523846 || header.version != 1 || !header.luaSize ||
        header.luaSize > luaLimit || header.ramSize != sizeof(PicoRam) ||
        header.audioSize != sizeof(audioState_t) || header.frame > INT32_MAX || header.frontendFrame > INT32_MAX ||
        (header.targetFps != 30 && header.targetFps != 60) || header.held > 255 || header.down > 255) return false;
    std::array<uint16_t, 10> inputState;
    memcpy(inputState.data(), bytes + sizeof(header), sizeof(inputState));
    if (inputState[8] > 255 || inputState[9] > 255) return false;
    size_t cursor = prefixSize + header.luaSize + header.ramSize + header.audioSize + sizeof(_drawStateCopy);
    if (cursor > size) return false;
    std::array<string, 5> keys;
    for (auto& key : keys) {
        if (size - cursor < sizeof(uint32_t)) return false;
        uint32_t length; memcpy(&length, bytes + cursor, sizeof(length)); cursor += sizeof(length);
        if (length > 64 || length > size - cursor) return false;
        key.assign(reinterpret_cast<const char*>(bytes + cursor), length); cursor += length;
        if (!std::all_of(key.begin(), key.end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '_' || c == '-';
        })) return false;
    }
    std::array<string, 4> values;
    for (auto& value : values) {
        if (size - cursor < sizeof(uint32_t)) return false;
        uint32_t length; memcpy(&length, bytes + cursor, sizeof(length)); cursor += sizeof(length);
        if (length > 1024 || length > size - cursor) return false;
        value.assign(reinterpret_cast<const char*>(bytes + cursor), length); cursor += length;
        if (value.find_first_not_of("0123456789abcdefABCDEF\n\r") != string::npos) return false;
    }
    if (cursor != size) return false;
    if (!deserializeLuaState(reinterpret_cast<const char*>(bytes + prefixSize), header.luaSize)) return false;
    cursor = prefixSize + header.luaSize;
    memcpy(_memory->data, bytes + cursor, header.ramSize); cursor += header.ramSize;
    memcpy(_audio->getAudioState(), bytes + cursor, header.audioSize); cursor += header.audioSize;
    memcpy(_drawStateCopy, bytes + cursor, sizeof(_drawStateCopy));
    _picoFrameCount = static_cast<int>(header.frame);
    _cartdataKeyCount = 0;
    for (int i = 0; i < 4; i++) { _cartdataKeys[i] = keys[i]; if (!keys[i].empty()) _cartdataKeyCount++; }
    for (int i = 0; i < 4; i++) {
        if (!keys[i].empty()) _host->saveCartData(keys[i], values[i]);
    }
    _currentCartdataKey = keys[4];
    _pauseMenu = false; _clearInputOnResume = false; _cartChangeQueued = false;
    _input->retromRestore(inputState);
    _targetFps = header.targetFps; ::frame = header.frontendFrame;
    kHeld = header.held; kDown = header.down;
    return true;
}
