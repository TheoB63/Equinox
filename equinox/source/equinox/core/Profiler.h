#pragma once

// Enable Tracy in Debug/Release, disable in Dist
#if defined(TRACY_ENABLE)
    #include <tracy/Tracy.hpp>
    
    #define EQN_PROFILE_FRAME(name)          FrameMarkNamed(name)
    #define EQN_PROFILE_FUNCTION()           ZoneScoped
    #define EQN_PROFILE_SCOPE(name)          ZoneScopedN(name)
    #define EQN_PROFILE_SCOPE_DYNAMIC(name)  ZoneScoped; ZoneName(name.c_str(), name.size())
    #define EQN_PROFILE_TAG(key, val)        ZoneText(val, strlen(val))
    #define EQN_PROFILE_ALLOC(ptr, size)     TracyAlloc(ptr, size)
    #define EQN_PROFILE_FREE(ptr)            TracyFree(ptr)
    #define EQN_PROFILE_THREAD(name)         tracy::SetThreadName(name)
    #define EQN_PROFILE_FIBER_ENTER(name)    TracyFiberEnter(name)
    #define EQN_PROFILE_FIBER_LEAVE          TracyFiberLeave
#else
    #define EQN_PROFILE_FRAME(name)
    #define EQN_PROFILE_FUNCTION()
    #define EQN_PROFILE_SCOPE(name)
    #define EQN_PROFILE_SCOPE_DYNAMIC(name)
    #define EQN_PROFILE_TAG(key, val)
    #define EQN_PROFILE_ALLOC(ptr, size)
    #define EQN_PROFILE_FREE(ptr)
    #define EQN_PROFILE_THREAD(name)
    #define EQN_PROFILE_FIBER_ENTER(name)
    #define EQN_PROFILE_FIBER_LEAVE
#endif
