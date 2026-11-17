#pragma once

#include "equinox/core/EquinoxTypes.h"
#include "equinox/memory/LinearAllocator.h"
#include "equinox/memory/TaggedPageAllocator.h"

namespace Equinox::Memory
{
	constexpr u32 KB = 1024;
	constexpr u32 MB = 1024 * KB;
	constexpr u32 GB = 1024 * MB;

	inline void* AlignForward(void* address, u8 alignment)
	{
		return (void*)((reinterpret_cast<u64>(address) + static_cast<u64>(alignment - 1)) & static_cast<u64>(~(alignment - 1)));
	}
}