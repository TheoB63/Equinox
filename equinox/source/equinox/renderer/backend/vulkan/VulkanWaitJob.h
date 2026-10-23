#pragma once

#include "equinox/core/EquinoxTypes.h"

namespace Equinox
{
    class TimelineSemaphore;
}

namespace Equinox::JobSystem
{
    struct Counter;
}

namespace Equinox
{
    struct VulkanWaitJob
    {
        // Dispatches a job that polls the timeline semaphore.
        // When the semaphore reaches the target value, the counter is decremented.
        // This allows the CPU to do other work (via JobSystem) while waiting for the GPU.
        static void Dispatch(TimelineSemaphore& semaphore, u64 targetValue, JobSystem::Counter* counter);
    };
}
