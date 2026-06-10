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

#define BIT(x) (1 << x)