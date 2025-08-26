
#include "state/EngineCommands.h"
#include "state/EventBus.h"
#include <cstdio>
namespace core {
void RegisterEngineCommandHandler(SimulationState&){ core::bus::registerHandler([](const core::bus::Command& c){
#ifdef MI_BINDINGS_USE_DEMO
    std::printf("[BUS] %d idA=%d idB=%d s1=%s s2=%s v1=%.2f flag=%d\n",(int)c.type,c.idA,c.idB,c.s1.c_str(),c.s2.c_str(),c.v1,(int)c.flag);
#else
    (void)c;
#endif
});}
} // namespace core
