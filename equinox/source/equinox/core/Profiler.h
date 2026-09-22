#pragma once

// TODO: Integrate Tracy Profiler here
// Empty macros to prepare the codebase

#if 0 // Set to 1 when Tracy is included
#include <tracy/Tracy.hpp>
#define EQN_PROFILE_FRAME(name)      FrameMarkNamed(name)
#define EQN_PROFILE_FUNCTION()       ZoneScoped
#define EQN_PROFILE_SCOPE(name)      ZoneScopedN(name)
#define EQN_PROFILE_TAG(key, val)    ZoneText(val, strlen(val))
#else
#define EQN_PROFILE_FRAME(name)
#define EQN_PROFILE_FUNCTION()
#define EQN_PROFILE_SCOPE(name)
#define EQN_PROFILE_TAG(key, val)
#endif