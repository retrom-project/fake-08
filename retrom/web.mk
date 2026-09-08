CORE_DIR := .
include platform/libretro/Makefile.common
SOURCES_CXX += retrom/bridge.cpp retrom/state.cpp
OBJECTS := $(addprefix .retrom-build/,$(SOURCES_C:.c=.o) $(SOURCES_CXX:.cpp=.o))
FLAGS := -MMD -MP -O2 -D__LIBRETRO__ -DMINIZ_NO_TIME -std=gnu++17 -fno-rtti -fexceptions
all: .retrom-build/fake08-retrom.mjs
.retrom-build/%.o: %.c retrom/web.mk
	@mkdir -p $(dir $@)
	em++ -x c++ $(FLAGS) $(INCFLAGS) -c $< -o $@
.retrom-build/%.o: %.cpp retrom/web.mk
	@mkdir -p $(dir $@)
	em++ $(FLAGS) $(INCFLAGS) -c $< -o $@
.retrom-build/fake08-retrom.mjs: $(OBJECTS)
	em++ -O2 $(OBJECTS) -o $@ -sDISABLE_EXCEPTION_CATCHING=0 -sEXPORT_EXCEPTION_HANDLING_HELPERS=1 -sMODULARIZE=1 -sEXPORT_ES6=1 -sENVIRONMENT=web,node -sALLOW_MEMORY_GROWTH=1 -sMAXIMUM_MEMORY=268435456 -sSTACK_SIZE=2097152 -sEXPORTED_FUNCTIONS='["_malloc","_free"]' -sEXPORTED_RUNTIME_METHODS='["UTF8ToString","FS","HEAPU8","HEAP16"]'

-include $(OBJECTS:.o=.d)
