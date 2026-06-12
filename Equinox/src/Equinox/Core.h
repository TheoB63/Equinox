#pragma once

#ifdef EQN_PLATFORM_WINDOWS
	#ifdef EQN_BUILD_DLL
		#define EQUINOX_API __declspec(dllexport)
	#else
		#define EQUINOX_API __declspec(dllimport)
	#endif
#else
	#error Equinox only supports Windows!
#endif

#ifdef EQN_ENABLE_ASSERTS
	#define EQN_ASSERT(x, ...) { if(!(x)) { EQN_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define EQN_CORE_ASSERT(x, ...) { if(!(x)) { EQN_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define EQN_ASSERT(x, ...)
	#define EQN_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)