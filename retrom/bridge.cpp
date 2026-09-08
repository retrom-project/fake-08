// SPDX-License-Identifier: MIT
#include "../platform/libretro/libretro.h"
#include "../source/vm.h"
#include <emscripten.h>
#include <cstdarg>
#include <cstring>
#include <vector>
#include <sys/stat.h>
extern Vm* _vm;
extern size_t frame;
extern uint8_t kHeld, kDown;
static bool running;
static uint32_t input;
static uint32_t pixels[128*128];
static int16_t samples[4096];
static int sampleCount;
static std::vector<unsigned char> snapshot;
static std::string errorText;
static retro_frame_time_callback timing;
static void logger(enum retro_log_level, const char*, ...) {}
static bool environment(unsigned command, void* data) {
    switch(command) {
    case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        static_cast<retro_log_callback*>(data)->log=logger; return true;
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
        *static_cast<const char**>(data)="/save"; return true;
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        return *static_cast<retro_pixel_format*>(data)==RETRO_PIXEL_FORMAT_RGB565;
    case RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK:
        timing=*static_cast<retro_frame_time_callback*>(data); return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        *static_cast<bool*>(data)=false; return true;
    case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
        *static_cast<unsigned*>(data)=2; return true;
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME: return true;
    default: return false;
    }
}
static void video(const void* data,unsigned w,unsigned h,size_t pitch) {
    if(!data || w!=128 || h!=128) return;
    for(unsigned y=0;y<h;y++) for(unsigned x=0;x<w;x++) {
        uint16_t color; memcpy(&color,static_cast<const char*>(data)+y*pitch+x*2,2);
        uint32_t red=((color>>11)&31)*255/31, green=((color>>5)&63)*255/63, blue=(color&31)*255/31;
        pixels[y*128+x]=red|(green<<8)|(blue<<16)|0xff000000;
    }
}
static size_t audio(const int16_t* data,size_t frames) {
    if(frames*2>4096) return 0;
    memcpy(samples,data,frames*2*sizeof(int16_t));sampleCount=static_cast<int>(frames*2);return frames;
}
static void poll() {}
static int16_t inputState(unsigned port,unsigned device,unsigned,unsigned id) {
    return port==0 && device==RETRO_DEVICE_JOYPAD && id<16 && (input&(1u<<id)) ? 1 : 0;
}
#define API extern "C" EMSCRIPTEN_KEEPALIVE
API int retrom_abi() {return 1;}
API void retrom_stop() {
    if(running) retro_deinit();
    running=false;input=0;snapshot.clear();
}
API int retrom_load(const void* data,int size) {
    retrom_stop();
    if(!data || size<1 || size>4*1024*1024) return 0;
    mkdir("/save",0700);mkdir("/save/cdata",0700);
    retro_set_environment(environment);retro_set_video_refresh(video);
    retro_set_audio_sample_batch(audio);retro_set_input_poll(poll);retro_set_input_state(inputState);
    retro_init();running=true;frame=0;kHeld=0;kDown=0;errorText.clear();
    if(!_vm->LoadCart(static_cast<const unsigned char*>(data),size,false)) {
        errorText=_vm->GetBiosError();retrom_stop();return 0;
    }
    _vm->vm_run();
    return 1;
}
API int retrom_step(uint32_t buttons) {
    if(!running) return 0;
    input=buttons;sampleCount=0;
    if(timing.callback) timing.callback(16667);
    retro_run();
    errorText=_vm->GetBiosError();
    return errorText.empty() ? 1 : 0;
}
API const void* retrom_pixels(){return pixels;}
API const void* retrom_audio(){return samples;}
API int retrom_audio_count(){return sampleCount;}
API const char* retrom_error(){return errorText.c_str();}
API int retrom_state_size(){return static_cast<int>(snapshot.size());}
API const void* retrom_state(){
    if(!running)return nullptr;
    snapshot=_vm->retromSave();return snapshot.empty()?nullptr:snapshot.data();
}
API int retrom_restore(const void* data,int size){
    if(!running || size<1 || size>4*1024*1024)return 0;
    bool valid=_vm->retromRestore(static_cast<const unsigned char*>(data),static_cast<size_t>(size));
    if(valid){input=0;}
    return valid?1:0;
}

API int retrom_ready() { return running && !_vm->IsPaused(); }
